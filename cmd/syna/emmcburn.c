/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Shaojun Feng <Shaojun.feng@synaptics.com>
 */
#include <common.h>
#include <command.h>
#include <mmc.h>
#include <memalign.h>
#include <part.h>
#include <part_efi.h>
#include <uuid.h>
#include <gzip.h>
#include <image-sparse.h>
#include <make_ext4fs.h>
#include <getopt.h>

#define EMMC_PART_USER			0
#define EMMC_PART_BOOT1			1
#define EMMC_PART_BOOT2			2
#define EMMC_PART_RPMB			3
#define GPT_LBA_NUM				34
#define MAX_GPT_PT_NUM			128

static int curr_device = -1;
static struct mmc *mmc;
static int curr_part = -1;

static int is_gpt_ready = 0;
static gpt_header g_gpt_h;
static gpt_entry g_gpt_e[MAX_GPT_PT_NUM];

struct pt_info {
	__le64 part;
	__le64 start_lba;
	__le64 cnt;
	char partition_name[PARTNAME_SZ + 1];
};

enum {
	BURN_IMAGE = 0,
	ERASE_PT,
	FORMAT_PT,
};

struct pt_operation {
	unsigned int op;
	char pt_name[8];
	char imgname[32]; //doesn't include path
};

char * srcstr[] = {"tftp", "usbh", "usbs"};

/* find a string in a list, the strings is split by ",", such as
 * find "boot" in "bootloader,kernel,boot", it should match "boot",
 * not "bootloader".
 * @retval  NULL        can't find the string.
 * @retval !NULL        the pointer to the found string
 */
static const char *find_str_in_list(const char *list, const char *str, int split)
{
	int len;
	if (NULL == list || NULL == str)
		return NULL;

	if ('\0' == split)
		split = ',';

	len = strlen(str);
	while (*list) {
		const char *next_str = NULL;
		if ((split == list[len] || '\0' == list[len]) &&
			(0 == strncmp(list, str, len))) {
			return list;
		}
		/* goto next string */
		next_str = strchr(list, split);
		if (NULL == next_str)
			break;
		list = next_str + 1;
	}
	return NULL;
}

static int emmc_init(void)
{
	int ret = -1;
	if(curr_device >= 0)
		return 0;
	else {
		ret = run_command("mmc rescan", 0);
		if(ret) {
			printf("MMC device init fail\n");
			return CMD_RET_FAILURE;
		} else
			curr_device = 0;
	}

	mmc = find_mmc_device(curr_device);
	if (!mmc) {
		printf("MMC device init fail\n");
		return CMD_RET_FAILURE;
	}

	return 0;
}

static int boot_dev_select(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = -1;
	ulong index = 0;
	if(argc < 2) {
		printf("arguments ERROR\n");
		return CMD_RET_FAILURE;
	}

	if (argc > 1 && !(str2long(argv[1], &index))){
		printf("'%s' is not a number\n", argv[1]);
		return CMD_RET_FAILURE;
	}

	ret = run_command("mmc rescan", 0);
	if(ret) {
		printf("MMC device rescan fail\n");
		return CMD_RET_FAILURE;
	}

	mmc = find_mmc_device(index);
	if (!mmc) {
		printf("MMC device find fail\n");
		return CMD_RET_FAILURE;
	}

	curr_device = index;

	return 0;
}

static unsigned int get_blksize(void)
{
	int ret = -1;
	struct blk_desc * blkd = NULL;

	ret = emmc_init();
	if(ret)
		return -1;

	blkd = mmc_get_blk_desc(mmc);

	return blkd->blksz;
}

static unsigned int get_erase_grp_size(void)
{
	int ret = -1;

	ret = emmc_init();
	if(ret)
		return -1;

	return mmc->erase_grp_size;
}

static int emmc_erase(int part, unsigned int start_lba, unsigned int cnt)
{
	int ret = -1;
	char cmd[256];
	unsigned int blk_size = get_blksize();
	unsigned int erase_grp_size = get_erase_grp_size();

	if(((start_lba * blk_size) % erase_grp_size)
		|| ((cnt * blk_size) % erase_grp_size)) {
		printf("Caution! Your devices Erase group is 0x%x\n", erase_grp_size);
		printf("The erase range would be change to 0x%x ~ 0x%x",
				start_lba & ~(erase_grp_size - 1),
				((erase_grp_size + cnt + erase_grp_size)
				& ~(erase_grp_size - 1)) - 1);
	}

	ret = emmc_init();
	if(ret)
		return -1;

	if(part != curr_part) {
		//switch part: 0, 1, 2
		sprintf(cmd, "mmc dev 0 %d", part);
		ret = run_command(cmd, 0);
		if (ret)
			return -1;
		curr_part = part;
	}

	blk_derase(mmc_get_blk_desc(mmc), start_lba, cnt);

	return 0;
}

static int emmc_read(int part, unsigned int start_lba, unsigned int cnt, void * buff)
{
	int ret = -1;
	char cmd[256];
	unsigned int n;

	ret = emmc_init();
	if(ret)
		return -1;

	if(part != curr_part) {
		//switch part: 0, 1, 2
		sprintf(cmd, "mmc dev 0 %d", part);
		ret = run_command(cmd, 0);
		if (ret)
			return -1;
		curr_part = part;
	}

	// to avoid much print, we use blk_dwrite instead of "mmc write"
	n = blk_dread(mmc_get_blk_desc(mmc), start_lba, cnt, buff);
	if(cnt != n) {
		printf("%d blocks read: ERROR\n", n);
		return -1;
	}

	return 0;
}

static int emmc_write(int part, unsigned int start_lba, unsigned int cnt, void * buff, int is_verify)
{
	int ret = -1;
	char cmd[256];
	char * rbuff = NULL;
	char * wbuff = NULL;
	unsigned int r_lba, rstart, rest;
	unsigned int n = 0;

	ret = emmc_init();
	if(ret)
		return -1;

	if(part != curr_part) {
		//switch part: 0, 1, 2
		sprintf(cmd, "mmc dev 0 %d", part);
		ret = run_command(cmd, 0);
		if (ret)
			return -1;
		curr_part = part;
	}

	// to avoid much print, we use blk_dwrite instead of "mmc write"
	n = blk_dwrite(mmc_get_blk_desc(mmc), start_lba, cnt, buff);
	if(cnt != n) {
		printf("%d blocks written: ERROR\n", n);
		return -1;
	}

	if(is_verify) {
		#define VERIFY_BUF_LBA 1024
		rbuff = malloc_cache_aligned(VERIFY_BUF_LBA * get_blksize());
		rstart = start_lba;
		rest = cnt;
		wbuff = buff;
		do {
			r_lba = (rest >= VERIFY_BUF_LBA) ? VERIFY_BUF_LBA : rest;

			ret = emmc_read(part, rstart, r_lba, rbuff);
			if(ret) {
				free(rbuff);
				return -1;
			}

			if(memcmp(rbuff, wbuff, r_lba * get_blksize())) {
				free(rbuff);
				printf("%s: verify failed at %08x\n", __func__, rstart * get_blksize());
				return -1;
			}

			wbuff += r_lba * get_blksize();
			rstart += r_lba;
			rest -= r_lba;
		} while(rest > 0);
		free(rbuff);
	}

	return 0;
}

static int clean_mbr_gpt_table(void)
{
	int ret = -1;

	is_gpt_ready = 0;
	//both mbr and gpt area are erased
	ret = emmc_erase(EMMC_PART_USER, 0, GPT_LBA_NUM);
	if (ret)
		return -1;

	return 0;
}

static int read_gpt_partition_info(void)
{
	int ret = -1;
	void * tmp = NULL;

	if(is_gpt_ready)
		return 0;

	ret = emmc_init();
	if(ret)
		return -1;

	ret = run_command("mmc dev 0 0", 0);
	if (ret)
		return -1;
	curr_part = 0;

	tmp = malloc_cache_aligned(GPT_LBA_NUM * get_blksize());
	memset(tmp, 0, (GPT_LBA_NUM * get_blksize()));
	if(tmp == NULL) {
		printf("%s:alloc mem fail!!!\n", __func__);
		return -1;
	}

	ret = blk_dread(mmc_get_blk_desc(mmc), 0, GPT_LBA_NUM, tmp);
	if (ret != GPT_LBA_NUM) {
		printf("Read GPT fail %d!!!\n", ret);
		free(tmp);
		return -EIO;
	}

	if(is_valid_gpt_buf(mmc_get_blk_desc(mmc), tmp)) {
		printf("Invalid GPT!!!\n");
		free(tmp);
		return -1;
	}

	memcpy(&g_gpt_h, tmp + get_blksize(), sizeof(g_gpt_h));

	memcpy(&g_gpt_e, tmp + le64_to_cpu(g_gpt_h.partition_entry_lba) * get_blksize(), sizeof(g_gpt_e));

	//should be refreshed after gpt is updated
	is_gpt_ready = 1;

	free(tmp);
	return 0;
}

static void gpt_convert_efi_name_to_char(char *s, efi_char16_t *es, int n)
{
	char *ess = (char *)es;
	int i, j;

	memset(s, '\0', n);

	for (i = 0, j = 0; j < n; i += 2, j++) {
		s[j] = ess[i];
		if (!ess[i])
			return;
	}
}

static int get_partition_info(char * pt_name, struct pt_info * pi)
{
	unsigned int num = 0;
	if(read_gpt_partition_info())
		return -1;
	if(pt_name[0] == 'b') {
		num = pt_name[1] -  '0';
		if((num != EMMC_PART_BOOT1) && (num != EMMC_PART_BOOT2)) {
			printf("Only b1 and b2 are available.\n");
			return -1;
		}
		pi->part = num;
		pi->start_lba = 0;
		pi->cnt = mmc->capacity_boot / get_blksize();
		sprintf(pi->partition_name, "boot%lld", pi->part);
	} else if(pt_name[0] == 's' && pt_name[1] == 'd') {
		num = simple_strtoull(pt_name + 2, NULL, 10);
		pi->part = 0;
		pi->start_lba = g_gpt_e[num - 1].starting_lba;
		pi->cnt = g_gpt_e[num - 1].ending_lba - g_gpt_e[num - 1].starting_lba + 1;
		gpt_convert_efi_name_to_char(pi->partition_name, g_gpt_e[num - 1].partition_name, PARTNAME_SZ + 1);
	} else {
		printf("Invalide partition name %s!\n", pt_name);
		return -1;
	}

	return 0;
}

static lbaint_t emmcburn_sparse_write(struct sparse_storage *info,
		lbaint_t blk, lbaint_t blkcnt, const void *buffer)
{
	int ret = -1;
	struct pt_info * pi = (struct pt_info *)info->priv;

	ret = emmc_write(pi->part, blk, blkcnt, (void *)buffer, 1);
	if(ret)
		return 0;

	return blkcnt;
}

static lbaint_t emmcburn_sparse_reserve(struct sparse_storage *info,
		lbaint_t blk, lbaint_t blkcnt)
{
	return blkcnt;
}

static void emmcburn_sparse_fail(const char *reason, char *response)
{
	printf("%s\n", reason);
}

void setup_sparse_op(struct sparse_storage * sparse, struct pt_info * pi)
{
	sparse->priv = (void *)pi;
	sparse->blksz = get_blksize();
	sparse->start = pi->start_lba;
	sparse->size = pi->cnt;
	sparse->write = emmcburn_sparse_write;
	sparse->reserve = emmcburn_sparse_reserve;
	sparse->mssg = emmcburn_sparse_fail;
}

#if defined(CONFIG_CMD_UNZIP) && defined(CONFIG_IMAGE_SPARSE)
extern int write_gzipped_sparse_image(struct sparse_storage *info, const char *part_name, void *data, int len, char *response);
#endif

static int burn_images(struct pt_info pi, void * buff, unsigned size)
{
	int ret = -1;
	unsigned int cnt = 0;

#if defined(CONFIG_CMD_UNZIP)
	if (is_gzip_image(buff)) {
		printf("Image format: gzip\n");

#if defined(CONFIG_CMD_UNZIP) && defined(CONFIG_IMAGE_SPARSE)
		struct sparse_storage sparse;

		setup_sparse_op(&sparse, &pi);
		ret = write_gzipped_sparse_image(&sparse, pi.partition_name, buff, size, NULL);
		if(ret != -2) // not sparse image
			return ret;
#endif
		ret = gzwrite(buff, size, mmc_get_blk_desc(mmc),
			get_erase_grp_size() * 1024, pi.start_lba * get_blksize(), 0);
		if(ret)
			return -1;
		return 0;
	}
#endif

#if defined(CONFIG_IMAGE_SPARSE)
	if(is_sparse_image(buff)) {
		struct sparse_storage sparse;
		printf("Image format: sparse\n");

		setup_sparse_op(&sparse, &pi);
		ret = write_sparse_image(&sparse, pi.partition_name, buff, NULL);
		if(ret)
			return -1;
		return 0;
	}
#endif

	{
		printf("Image format: flat\n");
		cnt = (size + get_blksize() - 1) / get_blksize();
		ret = emmc_write(pi.part, pi.start_lba, cnt, buff, 1);
		if(ret) {
			return -1;
		}
	}

	return 0;
}

extern unsigned int get_max_malloc_size(void);
static int dl_and_burn_img(char * src, char * path, char * imgname, char * pt)
{
	int ret = -1;
	char cmd[512];
	void * buff = NULL;
	uint64_t max_alloc = 0;
	uint32_t image_size = 0;
	struct pt_info pi;
	static char spit_image[64] = {0};

	ret = get_partition_info(pt, &pi);
	if(ret)
		return -1;

	max_alloc = get_max_malloc_size();
	max_alloc = min(max_alloc, pi.cnt * get_blksize());
	printf("allocated 0x%llx bytes memory\n", max_alloc);
	buff = malloc_cache_aligned(max_alloc);

	sprintf(cmd, "imgload %s %s%s 0x%p", src, path, imgname, buff);

	printf("allocated memory address %p \n", buff);
	ret = run_command(cmd, 0);

	if(ret) {
		free(buff);
		return -1;
	}
	image_size = env_get_hex("filesize", 0);

	//erase the whole partition
	if(0 == strlen(spit_image) || strncmp(spit_image, imgname, strlen(spit_image))) {
		ret = emmc_erase(pi.part, pi.start_lba, pi.cnt);
		if(ret) {
			free(buff);
			return -1;
		}
		printf("erase partition %s success\n", pt);
	} else {
		printf("Split image sequence %s detected,skip partition erase\n", spit_image);
	}

	ret = burn_images(pi, buff, image_size);
	if(ret) {
		free(buff);
		return -1;
	}

	if(NULL != strstr(imgname,".0")) {
		memset(spit_image, 0, sizeof(spit_image));
		strncpy(spit_image, imgname, strlen(imgname)-strlen(strstr(imgname,".0")));
		printf("Split image sequence %s identified\n", spit_image);
	}

	printf("write image data success!\n");

	free(buff);
	return 0;
}

static int erase_partition_by_name(char * pt)
{
	int ret = -1;
	struct pt_info pi;

	ret = get_partition_info(pt, &pi);
	if(ret)
		return -1;

	//erase the whole partition
	ret = emmc_erase(pi.part, pi.start_lba, pi.cnt);

	return ret;
}

static int format_partition_by_name(char * pt)
{
	int ret = -1;
	struct pt_info pi;

	ret = get_partition_info(pt, &pi);
	if(ret)
		return -1;

	//erase the whole partition
	ret = emmc_erase(pi.part, pi.start_lba, pi.cnt);
	if(ret)
		return -1;

	ret = make_ext4fs(mmc_get_blk_desc(mmc), pi.start_lba, pi.cnt);

	return ret;
}


static int set_bootbus(void)
{
	int ret = -1;
	u8 ack = 1;
	/* always set default boot part to boot partition 1 with ack enabled */
#if defined(CONFIG_TARGET_VS680) || defined(CONFIG_TARGET_VS680_A0) || defined(CONFIG_TARGET_VS640)
	// FIXME: on vs680 and vs640, we have to set the ack bit to 0
	ack = 0;
#endif
	ret = mmc_set_part_conf(mmc, ack, 0x1, 0x0); //EXT_CSD[179]
	if (ret) {
		printf("mmc partconf fail.\n");
		return -1;
	}

	/* always set default bootbus mode to X8 SDR HS mode */
	ret = mmc_set_boot_bus_width(mmc, 0x2, 0x0, 0x1); //EXT_CSD[177]
	if (ret) {
		printf("mmc bootbus fail.\n");
		return -1;
	}
	printf("Set boot partition 1 to be boot part OK.\n");

	return 0;
}

static int process_input(char *input, int len)
{
	int i, ret = 0;
	debug("process_input input %s\n", input);
	for(i = 0; i<len; i++) {
		if (input[i] == 0)
			break;
		if (input[i] != ' ' &&
		    input[i] != '\t' &&
		    input[i] != '\r')
			input[ret++] = input[i];
	}

	for (i = ret; i < len; i++)
		input[i] = 0;

	debug("CMD_PART: process_input out\n%s\n", input);
	return ret;
}

static int get_line(char *input, int line, char **line_start, int *line_len)
{
	char *start = input;
	char *end = 0;
	*line_start = 0;
	*line_len = 0;

	do {
		line--;
		end = strchr(start, '\n');
		if (!end)
			break;
		if (!line)
			break;
		start = end +1;
	} while(1);

	if (line > 0)
		return -1;

	if (!end)
		end = input + strlen(input);
	else
		*end = 0;

	*line_start = start;
	*line_len = end - start;
	if (*line_len <= 1)
		return -1;

	// printf("get line %d \n%s\n", old_line, *line_start);
	return 0;
}

static int parse_arg(char *input, int len, int *argc, char *argv[])
{
	char *p = input;
	char *dot;
	int i = 0;
	*argc = 0;
//	printf("parse_arg %s\n", input);

	while (p < (input+len)) {
		argv[i++] = p;
		dot = strchr(p, ',');
		if (dot) {
			*dot = 0;
			p = dot + 1;
//			printf("parse arg got one %s\n", argv[i-1]);
		} else {
//			printf("parse arg got one %s\n", argv[i-1]);
			break;
		}
	}

	*argc = i;
	return i;
}

static void create_gpt_partition(disk_partition_t *part, char *part_name, loff_t start_bytes, loff_t size_bytes)
{
	char guid_str[UUID_STR_LEN + 1];

	part->start = start_bytes * 1024 * 1024 / 512;
	part->size = size_bytes * 1024 * 1024 / 512;
	part->blksz = 512;
	strcpy((char *)part->name, part_name);
	gen_rand_uuid_str(guid_str, UUID_STR_FORMAT_STD);
	memcpy(part->uuid, guid_str, (UUID_STR_LEN + 1));
}

static int parse_part_and_gen_gpt(void * buff, unsigned int size)
{
	int ret = -1;
	char * line = NULL;
	char * p_buff = buff;
	int len = 0;
	int import_argc;
	char* import_argv[4];
	loff_t pt_start = 0, pt_size = 0;
	disk_partition_t *partitions = NULL;
	int partition_num = 0;
	char guid_str[UUID_STR_LEN + 1];

	process_input(buff, size);

	ret = get_line(p_buff, 1, &line, &len);
	if(ret) {
		printf("part file is NULL!\n");
		return -1;
	}

	if((len != 4) && strncmp(line, "#GPT", 4) != 0) {
		printf("don't support part type except GPT!\n");
		return -1;
	}

	ret = clean_mbr_gpt_table();
	if (ret) {
		return -1;
	}

	p_buff += len + 1;

	partitions = malloc_cache_aligned(MAX_GPT_PT_NUM * sizeof(disk_partition_t));
	memset(partitions, 0,(MAX_GPT_PT_NUM * sizeof(disk_partition_t)));

	while(0 == get_line(p_buff, 1, &line, &len)) {
		debug("%s\n", line);
		p_buff += len + 1;
		parse_arg(line, len, &import_argc, import_argv);
		if(import_argc < 3) {
			free(partitions);
			printf("Invalid part file!\n");
			return -1;
		}
		if(partition_num == 0)
			pt_start = simple_strtoull(import_argv[1], NULL, 10);
		else
			pt_start += pt_size; //the end of last partition
		pt_size = simple_strtoull(import_argv[2], NULL, 10);
		printf("start to create partiton %s %lld %lld...\n", import_argv[0], pt_start, pt_size);
		create_gpt_partition(&partitions[partition_num], import_argv[0], pt_start, pt_size);
		partition_num ++;
	}

	gen_rand_uuid_str(guid_str, UUID_STR_FORMAT_GUID);
	ret = gpt_restore(mmc_get_blk_desc(mmc), guid_str, partitions, partition_num);
	if (ret) {
		free(partitions);
		printf("gpt_restore failed.\n");
		return -1;
	}
	is_gpt_ready = 0;

	printf("gpt update successfully.\n");

	free(partitions);
	return 0;
}

static int get_part_name_from_img_name(char * src, char * partlst, char * imgname)
{
	int ret = -1;
	int partition_num = 0;
	char cmd[512];
	char partlist_name[128] = {'\0'};
	char tmp_part_name[8] = {'\0'};
	void * buff = NULL;
	unsigned partlst_size = 0;
	char * line = NULL;
	char * p_buff = NULL;
	int len = 0;
	int import_argc;
	char* import_argv[4];

	if (src == NULL || partlst == NULL || imgname == NULL)
		return -1;

	buff = malloc_cache_aligned(0x1000); //4K for emmc_image_list, should be enough
	memset(buff, 0, 0x1000);
	sprintf(cmd, "imgload %s %s 0x%p", src, partlst, buff);

	ret = run_command(cmd, 0);
	if(ret) {
		free(buff);
		return -1;
	}

	partlst_size = env_get_hex("filesize", 0);
	process_input(buff, partlst_size);
	p_buff = buff;

	ret = get_line(p_buff, 1, &line, &len);
	if(ret) {
		printf("part file is NULL!\n");
		free(buff);
		return -1;
	}

	if((len != 4) && strncmp(line, "#GPT", 4) != 0) {
		printf("don't support part type except GPT!\n");
		free(buff);
		return -1;
	}

	p_buff += len + 1;
	while(0 == get_line(p_buff, 1, &line, &len)) {
		debug("%s\n", line);
		p_buff += len + 1;
		parse_arg(line, len, &import_argc, import_argv);
		if(import_argc < 3) {
			printf("Invalid part file!\n");
			free(buff);
			return -1;
		}
		if (find_str_in_list(imgname, import_argv[0], ',')) {
			sprintf(tmp_part_name, "sd%d", partition_num+1);
			if (strlen(partlist_name) > 0) {
				strcat(partlist_name, ",");
				strcat(partlist_name, tmp_part_name);
			}
			else {
				strcpy(partlist_name, tmp_part_name);
			}
			memset(tmp_part_name, 0, sizeof(tmp_part_name));
		}
		partition_num ++;
	}
	if (strlen(partlist_name) > 0) {
		memset(imgname, 0, strlen(imgname)+1);
		strcpy(imgname, partlist_name);
	}

	free(buff);
	return 0;
}

static void get_image_path_and_name(char * str, char * path, char * name)
{
	char * pname = NULL;
	int i = 0;

	//level1/level2/file; file
	pname = strrchr(str, '/');
	if(pname == NULL) {//file is in the root dir
		path[0] = '\0';
		strcpy(name, str);
	} else {
		strcpy(name, (pname + 1));
		for(i = 0; ; i++) {
			path[i] = str[i];
			if(pname == &str[i])
				break;
		}
		path[i + 1] = '\0';
	}
}

#if 1
void dump_buf(void *buff, int size)
{
	char *p = buff;
	int i;

	for (i = 0; i < size; i++) {
		if(0 == i % 16)
			printf("\n");
		printf("%c ", p[i]);
	}
	printf("\n");
}
#endif

static int pt_operation_serialization(char * src, char * imglst, struct pt_operation * ptop)
{
	int ret = -1;
	int num = 0;
	char cmd[512];
	void * buff = NULL;
	unsigned imglst_size = 0;
	char * line = NULL;
	char * p_buff = NULL;
	int len = 0;
	int import_argc;
	char* import_argv[4];

	//dl emmc_image_list
	buff = malloc_cache_aligned(0x1000); //4K for emmc_image_list, should be enough
	memset(buff, 0, 0x1000);
	sprintf(cmd, "imgload %s %s 0x%p", src, imglst, buff);

	ret = run_command(cmd, 0);
	if(ret) {
		free(buff);
		return -1;
	}

	imglst_size = env_get_hex("filesize", 0);

	//parse and burn
	process_input(buff, imglst_size);

	p_buff = buff;

	while(0 == get_line(p_buff, 1, &line, &len)) {
		debug("%s\n", line);
		p_buff += len + 1;
		parse_arg(line, len, &import_argc, import_argv);
		if(import_argc != 2) {
			free(buff);
			printf("Invalid image list file! %d, %d\n", num, import_argc);
			return -1;
		}
		if((strcmp(import_argv[0], "erase") == 0)) {
			ptop[num].op = ERASE_PT;
			strcpy(ptop[num].pt_name, import_argv[1]);
		} else if((strcmp(import_argv[0], "format") == 0)) {
			ptop[num].op = FORMAT_PT;
			strcpy(ptop[num].pt_name, import_argv[1]);
		} else {
			ptop[num].op = BURN_IMAGE;
			strcpy(ptop[num].imgname, import_argv[0]);
			strcpy(ptop[num].pt_name, import_argv[1]);
		}
		num++;
	}

	free(buff);
	return num;
}

static void print_img2sd_msg(struct pt_operation ptop, char * path, char * src)
{
	struct pt_info pi;
	get_partition_info(ptop.pt_name, &pi);
	printf("###################################################################\n");
	switch(ptop.op) {
		case ERASE_PT:
			printf("ERASE: start to erase partition: %s\n", ptop.pt_name);
			break;
		case FORMAT_PT:
			printf("FORMAT: start to format partition: %s\n", ptop.pt_name);
			break;
		case BURN_IMAGE:
			printf("WRITE: start to write %s/%s to partition %s\n", path, ptop.imgname, ptop.pt_name);
			printf("SOURCE: %s\n", src);
			break;
		default:
			printf("something wrong with img2sd!!!\n");
			break;
	}
	printf("###################################################################\n");
	printf("Partition %s info:\n", ptop.pt_name);
	printf("    Name: %s\n", pi.partition_name);
	printf("    Start address: 0x%llx LBA, 0x%llx Bytes\n", pi.start_lba, pi.start_lba * get_blksize());
	printf("    Size: 0x%llx LBA, 0x%llx Bytes\n", pi.cnt, pi.cnt * get_blksize());
}

static int do_emmc_part(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = -1;
	char cmd[512];
	void * buff = NULL;
	char * src = NULL;
	char * imgname = NULL;
	unsigned int part_size = 0;

	if(argc <= 1)
		goto ERROR;

	if(strcmp(argv[1], "clean") == 0) {
		clean_mbr_gpt_table();
		return 0;
	}

	if(strcmp(argv[1], "print") == 0) {
		ret = run_command("mmc part", 0);
		if (ret)
			goto ERROR;
		return 0;
	}

	if(strcmp(argv[1], "import") != 0)
		goto ERROR;

	if(argc == 3) {
#ifdef CONFIG_USB_BOOT
		src = srcstr[2];
#else
		src = srcstr[0];
#endif
		imgname = argv[2];
	} else if(argc == 4) {
		src = argv[2];
		imgname = argv[3];
	}

	buff = malloc_cache_aligned(0x1000); //4K for emmc_part_list, should be enough
	memset(buff, 0, 0x1000);
	sprintf(cmd, "imgload %s %s 0x%p", src, imgname, buff);

	ret = run_command(cmd, 0);
	if(ret) {
		free(buff);
		goto ERROR;
	}

	part_size = env_get_hex("filesize", 0);

	ret = parse_part_and_gen_gpt(buff, part_size);
	if(ret) {
		free(buff);
		goto ERROR;
	}

	free(buff);
	return 0;
ERROR:
	printf("CMD_PART: emmcpart failed\n");
	return -1;
}

static int do_img2sd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = -1, i = 0;
	char * src = NULL;
	char * img = NULL;
	char * pt = NULL;
	char * img_list = NULL;
	char * ignore_partition_list = NULL;
	char imgpath[256], name[32];
	struct pt_operation * ptop = NULL;
	int pt_op_num = 0;
	ulong timer = get_timer(0);

	if(argc < 3) {
		printf("too few arguments\n");
		goto ERROR;
	}
	//img2sd import [src] [image list file]
	if(strcmp(argv[1], "import") == 0) {
		if(argc == 3) {
#ifdef CONFIG_USB_BOOT
			src = srcstr[2];
#else
			src = srcstr[0];
#endif
			img_list = argv[2];
		} else if(argc == 4) {
			src = argv[2];
			img_list = argv[3];
		} else if (argc == 5) {
			src = argv[2];
			img_list = argv[3];
			ignore_partition_list = argv[4];
		}

		ptop = malloc_cache_aligned(MAX_GPT_PT_NUM * sizeof(struct pt_operation));
		memset(ptop, 0, (MAX_GPT_PT_NUM * sizeof(struct pt_operation)));

		//use ptop->imgname temperatorily. will be overwritten next function
		get_image_path_and_name(img_list, imgpath, ptop->imgname);

		pt_op_num = pt_operation_serialization(src, img_list, ptop);
		if(pt_op_num < 0) {
			printf("parse img_list fail\n");
			goto ERROR;
		}
	} else {
		ptop = malloc_cache_aligned(sizeof(struct pt_operation));
		if(strcmp(argv[1], "erase") == 0) {
			ptop->op = ERASE_PT;
			strcpy(ptop->pt_name, argv[2]);
			imgpath[0] = '\0';
		} else if(strcmp(argv[1], "format") == 0) {
			ptop->op = FORMAT_PT;
			strcpy(ptop->pt_name, argv[2]);
			imgpath[0] = '\0';
		} else {
			//img2sd [src] [image] [partition]
			ptop->op = BURN_IMAGE;
			if(argc == 3) {
#ifdef CONFIG_USB_BOOT
				src = srcstr[2];
#else
				src = srcstr[0];
#endif
				img = argv[1];
				pt = argv[2];
			} else if(argc == 4) {
				src = argv[1];
				img = argv[2];
				pt = argv[3];
			}
			strcpy(ptop->pt_name, pt);
			get_image_path_and_name(img, imgpath, name);
			strcpy(ptop->imgname, name);
		}
		pt_op_num = 1;
	}

	for(i = 0; i < pt_op_num; i++) {
		if(ignore_partition_list &&
			find_str_in_list(ignore_partition_list, ptop[i].pt_name, ',')) {
			printf("######ignore partition ===> %s\n", ptop[i].pt_name);
			continue;
		}

		print_img2sd_msg(ptop[i], imgpath, src);
		if(ptop[i].op == ERASE_PT) {
			ret = erase_partition_by_name(ptop[i].pt_name);
			if(ret) {
				free(ptop);
				printf("erase %s failed!!!\n", ptop[i].pt_name);
				goto ERROR;
			}
			printf("erase %s done!!!\n", ptop[i].pt_name);
		} else if(ptop[i].op == FORMAT_PT) {
			//will do it later
			ret = format_partition_by_name(ptop[i].pt_name);
			if(ret) {
				free(ptop);
				printf("format %s failed!!!\n", ptop[i].pt_name);
				goto ERROR;
			}
		} else if(ptop[i].op == BURN_IMAGE) {
			ret = dl_and_burn_img(src, imgpath, ptop[i].imgname, ptop[i].pt_name);
			if(ret) {
				free(ptop);
				printf("%s: [%s] burn failed\n", __func__, ptop[i].imgname);
				goto ERROR;
			}
		}
	}

	free(ptop);
	timer = get_timer(timer);
	printf("%s: time cost %ld ms\n", __func__, timer);
	return 0;
ERROR:
	printf("CMD_PART: img2sd failed\n");
	return -1;
}

static int do_list2emmc(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int c=0, error=0, develop_mode=0;
	int ret = -1;
	char cmd[512];
	unsigned int src = 0;
	char part_file[256] = "eMMCimg/emmc_part_list";
	char img_list[256] = "eMMCimg/emmc_image_list";

	char *ignore_partition_list = NULL;
	const char *short_options = "i:";
	optind = 0;
	while ((c = getopt(argc, argv, short_options)) != -1) {
		switch (c) {
		case 'i':
			ignore_partition_list = optarg;
			develop_mode = 1;
			printf("ignore list: %s.\n", ignore_partition_list);
			break;

		default:
			printf("do_list2emmc: unsupported option!!\n");
			error = 1;
			break;
		}
	}

	if (((argc - optind) > 2) || ((argc - optind) < 1) || error) {
		cmd_usage(cmdtp);
		goto ERROR;
	}

	//flash image needn't enable cache, so configure block and cache entry to 0
	ret = run_command("blkcache configure 0 0", 0);
	if(ret) {
		printf("blkcache configure fail\n");
		goto ERROR;
	}

	ulong timer = get_timer(0);
#if defined(CONFIG_NET)
	if(strcmp(argv[0], "tftp2emmc") == 0)
		src = 0;
#endif

#if defined(CONFIG_CMD_USB) && defined(CONFIG_CMD_FAT)
	if(strcmp(argv[0], "usb2emmc") == 0)
		src = 1;
#endif

#ifdef CONFIG_USB_BOOT
	if(strcmp(argv[0], "l2emmc") == 0)
		src = 2;
#endif

	if((argc - optind) == 1) {
		sprintf(part_file, "%s/emmc_part_list", argv[optind]);
		sprintf(img_list, "%s/emmc_image_list", argv[optind]);
	}
	if((argc - optind) == 2) {
		sprintf(part_file, "%s", argv[optind]);
		sprintf(img_list, "%s", argv[optind + 1]);
	}

	//parse part and gen gpt
	sprintf(cmd, "emmcpart import %s %s", srcstr[src], part_file);
	ret = run_command(cmd, 0);
	if(ret) {
		printf("import part_list fail\n");
		goto ERROR;
	}

	if (ignore_partition_list) {
		ret = get_part_name_from_img_name(srcstr[src], part_file, ignore_partition_list);
		if(ret) {
			printf("Fail to convert the partition name into image name!!\n");
			goto ERROR;
		}
	}

	//parse image list and burn image
	if (develop_mode) {
		sprintf(cmd, "img2sd import %s %s %s", srcstr[src], img_list, ignore_partition_list);
	} else {
		sprintf(cmd, "img2sd import %s %s", srcstr[src], img_list);
	}
	ret = run_command(cmd, 0);
	if(ret)
		goto ERROR;
	//set boot bus mode
	ret = set_bootbus();
	if(ret)
		goto ERROR;

	printf("CMD_PART: list2emmc success.\n");

	timer = get_timer(timer);
	printf("%s: time cost %ld ms\n", __func__, timer);

	//re-enable blk cache operation
	ret = run_command("blkcache configure 8 32", 0);
	if(ret) {
		printf("blkcache configure 8 32 fail\n");
		goto ERROR;
	}
	return 0;

ERROR:
	printf("CMD_PART: list2emmc failed\n");
	printf("fail to write image\n");
	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	emmcpart,	4,	0,	do_emmc_part,
	"u-boot partition tool",
	"emmcpart clean  - clean partition mbr and partition header\n"
	"emmcpart print  - print partition list\n"
	"emmcpart import [src] [part_list_file]\n"
	"                - partition according to assigned part_list_file\n"
	"example:\n"
	"    emmcpart import eMMCimg/emmc_part_list (by default from tftp)\n"
	"    emmcpart import usbs eMMCimg/emmc_part_list\n"
);

U_BOOT_CMD(
	img2sd, 5, 0, do_img2sd,
	"u-boot partition tool",
	"img2sd erase [partition]\n"
	"img2sd format [partition]\n"
	"img2sd [src] [image] [partition]\n"
	"img2sd import [src] [image list file] [skip partition list]\n"
	"    Note: 1. src:usbh-load image from usb drive\n"
	"                 usbs-load image from host(usbtool only)\n"
	"                 tftp-load image from tftp server\n"
	"          2. src is tftp by default if not set\n"
	"example:\n"
	"    img2sd tftp 10.37.116.100:eMMCimg/bootloader.subimg b1 --- (10.37.116.100 is tftp server IP addr)\n"
	"    img2sd usbh 0:1:eMMCimg/kernel.subimg sd6\n"
	"    img2sd usbh eMMCimg/kernel.subimg sd6\n"
	"    img2sd usbs eMMCimg/rootfs.subimg sd8 --- (usbtool only)\n"
	"    img2sd import usbs eMMCimg/emmc_image_list\n"
);

static char list2emmc_help[] =
"read emmc images from tftp/usb device/usb host, burn them to eMMC chip.";


/*
 * 1. does we still need -g -d option for GPT and DOS? should be no
 * 2. generate gpt image offline and burn it
*/
static char list2emmc_usage[] =
"\n###############################################################################"
"\n               usb2emmc/l2emmc/tftp2emmc usage manual"
"\n###############################################################################\n"
"[command] [option] $emmc_part_list_PATH $emmc_image_list_PATH\n"
"[command] [option] $eMMCimg_PATH \n"
"    options:\n"
"      -i skip these partitions in the list, each partition is sperated\n"
"         by ',', such as 'factory,cache'\n"
"example: \n"
"    tftp2emmc eMMCimg\n"
"    tftp2emmc eMMCimg/emmc_part_list eMMCimg/emmc_image_list\n"
"    tftp2emmc 10.37.116.100:eMMCimg\n"
"    usb2emmc eMMCimg\n"
"    usb2emmc 0:1:eMMCimg\n"
"    l2emmc eMMCimg -- (usbtool only)\n"
"    l2emmc -i factory eMMCimg --- (usbtool only)\n"
"\n"
;

U_BOOT_CMD(
	bootdev_sel, 2, 0, boot_dev_select,
	"select boot devices",
	"bootdev_sel x\n"
	"    - select mmc for boot devices\n"
);


#if defined(CONFIG_NET)
U_BOOT_CMD(
     tftp2emmc, 4, 0, do_list2emmc,
     list2emmc_help,
     list2emmc_usage
);
#endif

#if defined(CONFIG_CMD_USB) && defined(CONFIG_CMD_FAT)
U_BOOT_CMD(
     usb2emmc, 4, 0, do_list2emmc,
     list2emmc_help,
     list2emmc_usage
);
#endif

#ifdef CONFIG_USB_BOOT
U_BOOT_CMD(
     l2emmc, 4, 0, do_list2emmc,
     list2emmc_help,
     list2emmc_usage
);
#endif
