/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */

#include <config.h>
#include <common.h>
#include <dm.h>
#include <part.h>
#include <mmc.h>
#include <android_bootloader_message.h>
#include <misc.h>
#include <fs.h>
#include <div64.h>
#if CONFIG_IS_ENABLED(SYNA_BOOTFLAG_IN_SM) || \
	CONFIG_IS_ENABLED(SYNA_BOOTFLAG_IN_DTCM)
# include "sm.h"
# if CONFIG_IS_ENABLED(SYNA_FB_BOOT)
# include "bcm.h"
# include "miniloader.h"
# endif
#endif
#include "fastboot_syna.h"
#include "flash_ts.h"

#define FASTBOOT_VERSION		"0.4"

#define FASTBOOT_INTERFACE_CLASS	0xff
#define FASTBOOT_INTERFACE_SUB_CLASS	0x42
#define FASTBOOT_INTERFACE_PROTOCOL	0x03

#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0  (0x0200)
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1  (0x0040)
#define TX_ENDPOINT_MAXIMUM_PACKET_SIZE      (0x0040)

#define EP_BUFFER_SIZE			4096
#define MISC_BOOT_CONTROL_OFFSET_IN_MISC          (0x1000)
#define MISC_VIRTUAL_AB_MESSAGE_OFFSET_IN_MISC    (0x8000)
#define MISC_VIRTUAL_AB_MERGE_STATUS_NONE         (0x0)
#define MISC_VIRTUAL_AB_MERGE_STATUS_SNAPSHOTTED  (0x2)
#define MISC_VIRTUAL_AB_MERGE_STATUS_MERGING      (0x3)
#define MISC_VIRTUAL_AB_MESSAGE_VERSION           (2)
#define MISC_VIRTUAL_AB_MAGIC_HEADER              (0x56740AB0)


#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_RPMB_LOCK)
/* store lock flag in eMMC RPMB partition */
#define FASTBOOT_LOCKFLAG_EMMC_DEV	0
#define FASTBOOT_LOCKFLAG_RPMB_BLK_CNT	1

#define RPMB_UNLOCK_FLAG	(0xa5)
#define RPMB_AVB_MAGIC		(0x55595a59)
#define RPMB_BLK_SIZE		(256)

/* put flag info before Android RPMB blocks */
#define ANDROID_RPMB_AVB_BLKS 8
#endif

#if defined(CONFIG_SYNA_OEMBOOT_FLASH_MMC_DEV)
#define CONFIG_FASTBOOT_FLASH_MMC_DEV CONFIG_SYNA_OEMBOOT_FLASH_MMC_DEV
#endif
/*
 * EP_BUFFER_SIZE must always be an integral multiple of maxpacket size
 * (64 or 512 or 1024), else we break on certain controllers like DWC3
 * that expect bulk OUT requests to be divisible by maxpacket size.
 */

#define FASTBOOT_MAGIC		(0xaa5577ff)
#define OEM_UNLOCK_FLAG		(0xa55a)
#define FRP_UNLOCK_FLAG		(0xa5)

#define CHECK_NAME(a, b)  ((!strncmp((b), (a), strlen(a))) && \
			(strnlen((b), 16) == strlen(a)))

struct oem_lock_flag {
	unsigned int magic;
	unsigned int reserve1[6];
	unsigned int lock_flag;
	unsigned int lock_critical_flag;
	unsigned int reserve2[119];	/* round to 512 bytes */
};

struct frp_lock_flag {
	unsigned char reserve[510];
	unsigned char lock_critical_flag;
	unsigned char lock_flag;
};

#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_RPMB_LOCK)
struct rpmb_lock_flag {
	uint32_t magic;
	unsigned char reserve1[1];
	unsigned char unlock;
	unsigned char reserve2[250];
} __attribute__((packed));
#endif

// Holds Virtual A/B merge status information. Current version is 1. New fields
// must be added to the end.
struct __attribute__((__packed__)) misc_virtual_ab_message {
  uint8_t version;
  uint32_t magic;
  uint8_t merge_status;  // IBootControl 1.1, MergeStatus enum.
  uint8_t source_slot;   // Slot number when merge_status was written.
  uint8_t reserved[57];
};

#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_GUI_ENABLE)
extern void update_gui_lock_state(unsigned int lock_flag, unsigned int lock_critical_flag);
#endif
extern bool fts_exist; 	 //fts partition exist
extern bool devinfo_exist;  //devinfo partition exist

int get_lock_flag(unsigned int *lock_flag, unsigned int *lock_critical_flag)
{
#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_RPMB_LOCK)
	int ret = 0;
	int rpmb_lock_flag_blk = 0;
	struct rpmb_lock_flag p;
	struct mmc *mmc = find_mmc_device(FASTBOOT_LOCKFLAG_EMMC_DEV);

	if (!mmc) {
		pr_err("fastboot no mmc device at slot %x\n", FASTBOOT_LOCKFLAG_EMMC_DEV);
		return -1;
	}

	if (mmc_init(mmc)) {
		pr_err("fastboot mmc device %x init failed\n", FASTBOOT_LOCKFLAG_EMMC_DEV);
		return -1;
	}

	if (blk_select_hwpart_devnum(IF_TYPE_MMC, FASTBOOT_LOCKFLAG_EMMC_DEV, MMC_PART_RPMB)) {
		pr_err("fastboot Failed to select mmc RPMB part on device %x\n", FASTBOOT_LOCKFLAG_EMMC_DEV);
		return -1;
	}

	if (mmc->capacity_rpmb <= ANDROID_RPMB_AVB_BLKS * RPMB_BLK_SIZE) {
		pr_err("fastboot PMB size error on mmc device %x\n", FASTBOOT_LOCKFLAG_EMMC_DEV);
		return -1;
	}
	rpmb_lock_flag_blk = (mmc->capacity_rpmb/RPMB_BLK_SIZE) - ANDROID_RPMB_AVB_BLKS;

	memset(&p, 0, sizeof(struct rpmb_lock_flag));
	ret = mmc_rpmb_read(mmc, &p, rpmb_lock_flag_blk, FASTBOOT_LOCKFLAG_RPMB_BLK_CNT, NULL);
	if (ret != FASTBOOT_LOCKFLAG_RPMB_BLK_CNT) {
		pr_err("fastboot RPMB read failed\n");
		return -1;
	}

	debug("fastboot RPMB read magic[%x] lock_flag[%x]\n", p.magic, p.unlock);

	*lock_flag = (p.magic == RPMB_AVB_MAGIC && p.unlock == RPMB_UNLOCK_FLAG) ? 0 : 1;
	*lock_critical_flag = (p.magic == RPMB_AVB_MAGIC && p.unlock == RPMB_UNLOCK_FLAG) ? 0 : 1;

	return 0;
#else
	int ret = 0;

	if (!lock_flag || !lock_critical_flag)
		return -1;

	*lock_flag = 0;
	*lock_critical_flag = 0;

	struct blk_desc *dev = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev || dev->type == DEV_TYPE_UNKNOWN) {
		pr_err("invalid mmc device\n");
		return -ENOENT;
	}

	ret = blk_dselect_hwpart(dev, 0);
	if (ret) {
		return ret;
	}

	disk_partition_t info;
	if(devinfo_exist)
		ret = part_get_info_by_name(dev, "devinfo", &info);
	else
		ret = part_get_info_by_name(dev, "frp", &info);
	if (ret < 0) {
		ret = part_get_info_by_name(dev, "bl_normal", &info);
		lbaint_t lock_flag_lba = info.start + info.size - 2048;
		if (ret) {
			return ret;
		} else {
			debug("bl_normal start[%lx] size[%lx][%ld]\n", info.start, info.size, info.size);
			debug("bl_normal oem lock flag start[%lx]\n", lock_flag_lba);
		}

		struct oem_lock_flag p;
		memset(&p, 0, sizeof(struct oem_lock_flag));

		ret = blk_dread(dev, lock_flag_lba, 1, &p);
		if (ret != 1)
			return -1;
		*lock_flag = (p.lock_flag == OEM_UNLOCK_FLAG) ? 0 : 1;
		*lock_critical_flag = (p.lock_critical_flag == OEM_UNLOCK_FLAG) ? 0 : 1;
		debug("oem lock_flag[%x] lock_critical_flag[%x]\n",
			p.lock_flag, p.lock_critical_flag);
	} else {
		lbaint_t lock_flag_lba = info.start + info.size - 1;
		debug("frp start[%lx] size[%lx][%ld]\n", info.start, info.size, info.size);
		debug("frp oem lock flag start[%lx]\n", lock_flag_lba);

		struct frp_lock_flag p;
		memset(&p, 0, sizeof(struct frp_lock_flag));

		ret = blk_dread(dev, lock_flag_lba, 1, &p);
		if (ret != 1)
			return -1;
		*lock_flag = (p.lock_flag == FRP_UNLOCK_FLAG) ? 0 : 1;
		*lock_critical_flag = (p.lock_critical_flag == FRP_UNLOCK_FLAG) ? 0 : 1;
		debug("frp lock_flag[%x] lock_critical_flag[%x]\n",
			p.lock_flag, p.lock_critical_flag);
	}
	return 0;
#endif
}

int save_lock_flag(unsigned int lock_flag, unsigned lock_critical_flag)
{
#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_RPMB_LOCK)
	int ret = 0;
	struct rpmb_lock_flag p;
	int rpmb_lock_flag_blk = 0;
	struct mmc *mmc = find_mmc_device(FASTBOOT_LOCKFLAG_EMMC_DEV);

	if (!mmc) {
		pr_err("fastboot no mmc device at slot %x\n", FASTBOOT_LOCKFLAG_EMMC_DEV);
		return -1;
	}

	if (mmc_init(mmc)) {
		pr_err("fastboot mmc device %x init failed\n", FASTBOOT_LOCKFLAG_EMMC_DEV);
		return -1;
	}

	if (blk_select_hwpart_devnum(IF_TYPE_MMC, FASTBOOT_LOCKFLAG_EMMC_DEV, MMC_PART_RPMB)) {
		pr_err("fastboot Failed to select mmc RPMB part on device %x\n", FASTBOOT_LOCKFLAG_EMMC_DEV);
		return -1;
	}

	if (mmc->capacity_rpmb <= ANDROID_RPMB_AVB_BLKS * RPMB_BLK_SIZE) {
		pr_err("fastboot PMB size error on mmc device %x\n", FASTBOOT_LOCKFLAG_EMMC_DEV);
		return -1;
	}
	rpmb_lock_flag_blk = (mmc->capacity_rpmb/RPMB_BLK_SIZE) - ANDROID_RPMB_AVB_BLKS;

	/* read rpmb_lock_flag struct from rpmb */
	memset(&p, 0, sizeof(struct rpmb_lock_flag));
	ret = mmc_rpmb_read(mmc, &p, rpmb_lock_flag_blk, FASTBOOT_LOCKFLAG_RPMB_BLK_CNT, NULL);
	if (ret != FASTBOOT_LOCKFLAG_RPMB_BLK_CNT) {
		pr_err("fastboot RPMB read failed\n");
		return -1;
	}

	if (p.magic != RPMB_AVB_MAGIC) {
		debug("fastboot, magic number is invalid, keep as is\n");
		return 0;
	}

	if (p.unlock == (lock_flag ? 0 : RPMB_UNLOCK_FLAG)) {
		debug("fastboot, unlock status is OK\n");
		return 0;
	}
	p.unlock = lock_flag ? 0 : RPMB_UNLOCK_FLAG;

	ret = mmc_rpmb_write(mmc, &p, rpmb_lock_flag_blk, FASTBOOT_LOCKFLAG_RPMB_BLK_CNT, NULL);
	if (ret != FASTBOOT_LOCKFLAG_RPMB_BLK_CNT)
		return -1;

	debug("fastboot RPMB save lock_flag[%x]\n",	p.unlock);
#else
	int ret = 0;
	struct oem_lock_flag p;
	memset(&p, 0, sizeof(struct oem_lock_flag));

	struct blk_desc *dev = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev || dev->type == DEV_TYPE_UNKNOWN) {
		pr_err("invalid mmc device\n");
		return -ENOENT;
	}

	ret = blk_dselect_hwpart(dev, 0);
	if (ret) {
		return ret;
	}

	disk_partition_t info;
	if(devinfo_exist)
		ret = part_get_info_by_name(dev, "devinfo", &info);
	else
		ret = part_get_info_by_name(dev, "frp", &info);
	if (ret < 0) {
		ret = part_get_info_by_name(dev, "bl_normal", &info);
		lbaint_t lock_flag_lba = info.start + info.size - 2048;
		if (ret) {
			return ret;
		} else {
			debug("bl_normal start[%lx] size[%lx][%ld]\n", info.start, info.size, info.size);
			debug("bl_normal oem lock flag start[%lx]\n", lock_flag_lba);
		}

		struct oem_lock_flag p;
		memset(&p, 0, sizeof(struct oem_lock_flag));

		p.lock_flag = lock_flag ? 0 : OEM_UNLOCK_FLAG;
		p.lock_critical_flag = lock_critical_flag ? 0 : OEM_UNLOCK_FLAG;
		p.magic = FASTBOOT_MAGIC;
		ret = blk_dwrite(dev, lock_flag_lba, 1, &p);
		if (ret != 1)
			return -1;
		debug("fastboot oem save lock_flag[%x] lock_critical_flag[%x]\n",
			p.lock_flag, p.lock_critical_flag);
	} else {
		lbaint_t lock_flag_lba = info.start + info.size - 1;
		debug("frp start[%lx] size[%lx][%ld]\n", info.start, info.size, info.size);
		debug("frp lock flag start[%lx]\n", lock_flag_lba);

		struct frp_lock_flag p;
		memset(&p, 0, sizeof(struct frp_lock_flag));

		p.lock_flag = lock_flag ? 0 : FRP_UNLOCK_FLAG;
		p.lock_critical_flag = lock_critical_flag ? 0 : FRP_UNLOCK_FLAG;
		ret = blk_dwrite(dev, lock_flag_lba, 1, &p);
		if (ret != 1)
			return -1;
		debug("fastboot frp save lock_flag[%x] lock_critical_flag[%x]\n",
			p.lock_flag, p.lock_critical_flag);
	}
#endif

#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_GUI_ENABLE)
	update_gui_lock_state(lock_flag, lock_critical_flag);
#endif
	return 0;
}

int oem_check_lock(char *partition_name)
{
	unsigned int lock_flag, lock_critical_flag;
	int ret = get_lock_flag(&lock_flag, &lock_critical_flag);
	if (ret)
		return -1;

	if (CHECK_NAME("frp", partition_name)) {
		printf("frp partition cannot be flashed by fastboot\n");
		return 3;
	}
	if (CHECK_NAME("devinfo", partition_name)) {
		printf("devinfo partition cannot be flashed by fastboot\n");
		return 3;
	}

	if (lock_flag) {
		return 1; // all patition can't flash
	}

	if (lock_critical_flag) {
		if (CHECK_NAME("boot1", partition_name)
			|| CHECK_NAME("boot2", partition_name)
			|| CHECK_NAME("bl_normal", partition_name)
			|| CHECK_NAME("bl_recovery", partition_name)
			|| CHECK_NAME("bl_a", partition_name)
			|| CHECK_NAME("bl_b", partition_name)
			|| CHECK_NAME("factory_setting", partition_name)) {
			return 2;
		}
	}
	return 0;
}

int virtual_ab_merge_status_check(char *partition_name)
{
	if (CHECK_NAME("userdata", partition_name)
		|| CHECK_NAME("metadata", partition_name)
		|| CHECK_NAME("frp", partition_name)) {
		struct blk_desc *dev_desc;
		disk_partition_t info;
		lbaint_t strat_blk;
		char *pbuf;
		struct misc_virtual_ab_message virtual_ab_message;
		struct mmc *mmc = find_mmc_device(CONFIG_FASTBOOT_FLASH_MMC_DEV);

		if (mmc == NULL) {
			printf("invalid mmc device\n");
			return -1;
		}

		dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
		if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
			printf("invalid mmc device\n");
			return -1;
		}

		char *partition_name = "misc";
		if (part_get_info_by_name(dev_desc, partition_name, &info) == -1) {
			printf("cannot find partition: '%s'\n", partition_name);
			return -1;
		}

		pbuf = malloc(dev_desc->blksz);
		strat_blk = info.start + MISC_VIRTUAL_AB_MESSAGE_OFFSET_IN_MISC/dev_desc->blksz;
		blk_dread(dev_desc, strat_blk, 1, pbuf);
		memcpy((void*)&virtual_ab_message, (void*)pbuf, sizeof(struct misc_virtual_ab_message));
		free(pbuf);

		if (virtual_ab_message.version == MISC_VIRTUAL_AB_MESSAGE_VERSION &&
			virtual_ab_message.magic == MISC_VIRTUAL_AB_MAGIC_HEADER)
		{
			if (virtual_ab_message.merge_status == MISC_VIRTUAL_AB_MERGE_STATUS_SNAPSHOTTED ||
				virtual_ab_message.merge_status == MISC_VIRTUAL_AB_MERGE_STATUS_MERGING)
				return 1;
			else
				return 0;
		}
		//for case that immediatelly enter fastboot after burn eMMCimg
		else if(virtual_ab_message.version == 0 && virtual_ab_message.magic == 0 &&
				virtual_ab_message.merge_status == MISC_VIRTUAL_AB_MERGE_STATUS_NONE)
		{
			printf("misc partition doesn't have virtual ab message info!!\n");
			return 0;
		}
		else
		{
			printf("virtual_ab_message version(%d) or magic(0x%08x) is not expected!\n",\
				virtual_ab_message.version, virtual_ab_message.magic);
			return -1;
		}
	}
	else
		return 0;
}


#if CONFIG_IS_ENABLED(SYNA_BOOTFLAG_IN_DTCM)
static void set_syna_reboot_msg(const char *cmd)
{
	int len = 0;
	void *p = SM_MSG_EXTRA_BUF_ADDR;

	if (cmd && (len = strlen(cmd))) {
		len += 1;
		if (len > SM_MSG_EXTRA_BUF_SIZE - sizeof(len)) {
			len = SM_MSG_EXTRA_BUF_SIZE - sizeof(len);
			debug("cut off too long reboot args to %d bytes\n", len);
		}
		debug("reboot cmd is %s@%d\n", cmd, len);
		memcpy(p, &len, sizeof(len));
		memcpy(p + sizeof(len), cmd, len);
	} else
		memset(p, 0, sizeof(int));

}
#endif

#if CONFIG_IS_ENABLED(SYNA_FB_BOOT)
static int bcm_decrypt_miniloader(void)
{
	unsigned int addr, size;
	int boot_area;
	for (boot_area = 1; boot_area <= 2; boot_area++) {
		size = load_miniloader_image(CONFIG_FASTBOOT_FLASH_MMC_DEV, boot_area, &addr);
		void *addr_ptr = (void *)(ulong)addr;
		if (bcm_decrypt_image(addr_ptr, size, addr_ptr, CODETYPE_MINILOADER)) {
			debug("Verify min-loader fail from boot area %d "
				"data\n", boot_area);
			continue;
		} else {
			debug("Verify min-loader success");
			return 0;
		}
	}
	return -ENODEV;
}
#endif

static int write_ctrl_to_misc(void)
{
	struct blk_desc *dev_desc;
	disk_partition_t info;
	struct mmc *mmc = find_mmc_device(CONFIG_FASTBOOT_FLASH_MMC_DEV);

	if (mmc == NULL) {
		printf("invalid mmc device\n");
		return -1;
	}

	dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("invalid mmc device\n");
		return -1;
	}

	char *partition_name = "misc";
	if (part_get_info_by_name(dev_desc, partition_name, &info) == -1) {
		printf("cannot find partition: '%s'\n", partition_name);
		return -1;
	}

	blk_derase(dev_desc, info.start, info.size);
	struct bootloader_message *msg = (struct bootloader_message *)malloc(sizeof(struct bootloader_message));
	memset(msg, 0, sizeof(struct bootloader_message));
	strcpy(msg->command, "boot-fastboot");
	strcpy(msg->recovery, "recovery");
	blk_dwrite(dev_desc, info.start, info.size/dev_desc->blksz, msg);
	return 0;
}

static int write_bootcommand_to_misc(char * command)
{
	struct blk_desc *dev_desc;
	disk_partition_t info;
	struct mmc *mmc = find_mmc_device(CONFIG_FASTBOOT_FLASH_MMC_DEV);

	if (mmc == NULL) {
		printf("invalid mmc device\n");
		return -1;
	}

	dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("invalid mmc device\n");
		return -1;
	}

	char *partition_name = "misc";
	if (part_get_info_by_name(dev_desc, partition_name, &info) == -1) {
		printf("cannot find partition: '%s'\n", partition_name);
		return -1;
	}

	struct bootloader_message *msg = (struct bootloader_message *)malloc(sizeof(struct bootloader_message));
	memset(msg, 0, sizeof(struct bootloader_message));
	strcpy(msg->command, command);
	printf("start %08llx, size %08llx, blksz %08llx, command %s\n", info.start, info.size, dev_desc->blksz, command);
	blk_dwrite(dev_desc, info.start, info.size/dev_desc->blksz, msg);
	return 0;
}

int clear_bootloader_message_to_misc(void)
{
        struct blk_desc *dev_desc;
        disk_partition_t info;
        struct mmc *mmc = find_mmc_device(CONFIG_FASTBOOT_FLASH_MMC_DEV);

        if (mmc == NULL) {
                printf("invalid mmc device\n");
                return -1;
        }

        dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
        if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
                printf("invalid mmc device\n");
                return -1;
        }

        char *partition_name = "misc";
        if (part_get_info_by_name(dev_desc, partition_name, &info) == -1) {
                printf("cannot find partition: '%s'\n", partition_name);
                return -1;
        }

        struct bootloader_message *msg = (struct bootloader_message *)malloc(sizeof(struct bootloader_message));
        memset(msg, 0, sizeof(struct bootloader_message));
        blk_dread(dev_desc, info.start, 1, msg);
        if(!strcmp(msg->command, "bootonce-bootloader"))
        {
                memset(msg->command, 0, sizeof(msg->command));
                blk_dwrite(dev_desc, info.start, info.size/dev_desc->blksz, msg);
        }
        return 0;
}
#define FTS_NAME_LENGTH		128
#define FTS_VALUE_LENGTH	128

static void syna_reboot(unsigned int cmd, unsigned int dl_addr, unsigned int dl_size)
{
#if CONFIG_IS_ENABLED(SYNA_BOOTFLAG_IN_FTS)
	if(fts_exist)
	{
		struct udevice *dev = NULL;
		int ret;
		int res = uclass_get_device_by_name(UCLASS_MISC, "fts", &dev);
		if (res) {
			printf("Can't found misc device named fts\n");
			return;
		}
		char name[FTS_NAME_LENGTH], value[FTS_VALUE_LENGTH];
		struct flash_ts_key key = {name, value};
		if (cmd == FB_REBOOT_CMD_REBOOT_FB) {
			strncpy(name, "bootloader.command", FTS_NAME_LENGTH);
			strncpy(value, "bootonce-bootloader", FTS_VALUE_LENGTH);
			res = misc_write(dev, 0, (void *)&key, 0);
		} else if (cmd == FB_REBOOT_CMD_RECOVERY) {
			strncpy(name, "bootloader.command", FTS_NAME_LENGTH);
			strncpy(value, "boot-recovery", FTS_VALUE_LENGTH);
			res = misc_write(dev, 0, (void *)&key, 0);
		} else if (cmd == FB_REBOOT_CMD_FASTBOOT) {
			strncpy(name, "bootloader.command", FTS_NAME_LENGTH);
			strncpy(value, "boot-recovery", FTS_VALUE_LENGTH);
			res = misc_write(dev, 0, (void *)&key, 0);
			ret = write_ctrl_to_misc();
			if (ret) {
				printf("misc partition write failed!\n");
				return;
			}
		} else if (cmd == FB_REBOOT_CMD_BOOT) {
			printf("CMD_BOOT not support\n");
			return;
		}
		if (res) {
			printf("ERROR: Failed to set bootloader command\n");
			return;
		}
	}
	else if(devinfo_exist)
	{
		int ret;
		if (cmd == FB_REBOOT_CMD_REBOOT_FB) {
			ret = write_bootcommand_to_misc("bootonce-bootloader");
		} else if (cmd == FB_REBOOT_CMD_RECOVERY) {
			ret = write_bootcommand_to_misc("boot-recovery");
		} else if (cmd == FB_REBOOT_CMD_FASTBOOT) {
			ret = write_bootcommand_to_misc("boot-recovery");
			ret = write_ctrl_to_misc();
			if (ret) {
				printf("misc partition write failed!\n");
				return;
			}
		} else if (cmd == FB_REBOOT_CMD_BOOT) {
			printf("CMD_BOOT not support\n");
			return;
		}
	}

#elif CONFIG_IS_ENABLED(SYNA_BOOTFLAG_IN_SM)
	struct bootflag *tmp = (struct bootflag *)SM_BOOTMODE_ADDR;
	if (cmd == FB_REBOOT_CMD_REBOOT_FB) {
		tmp->flag = BOOTFLAG_FASTBOOT;
		tmp->cmd.command = CMD_REBOOT_FB;
	} else if (cmd == FB_REBOOT_CMD_RECOVERY) {
		tmp->flag = BOOTFLAG_RECOVERY;
		tmp->magic = BOOTFLAG_MAGIC;
	} else if (cmd == FB_REBOOT_CMD_BOOT) {
#if CONFIG_IS_ENABLED(SYNA_FB_BOOT)
		tmp->flag = BOOTFLAG_FASTBOOT;
		tmp->cmd.command = CMD_BOOT;
		tmp->cmd.param[0] = dl_addr;
		tmp->cmd.param[1] = dl_size;
		int res = bcm_decrypt_miniloader();
		if (res) {
			printf("Can't found miniloader image.\n");
			return;
		}
		jump_miniloader();
#else
		printf("CMD_BOOT not support\n");
		return;
#endif
	}
#endif

#if CONFIG_IS_ENABLED(SYNA_BOOTFLAG_IN_DTCM)
	if (cmd == FB_REBOOT_CMD_REBOOT_FB) {
		set_syna_reboot_msg("bootloader");
	} else if (cmd == FB_REBOOT_CMD_RECOVERY) {
		set_syna_reboot_msg("recovery");
	} else if (cmd == FB_REBOOT_CMD_REBOOT) {
		set_syna_reboot_msg(NULL);
	}else if (cmd == FB_REBOOT_CMD_BOOT) {
		printf("CMD_BOOT not support\n");
		return;
	}
#endif
}

static void compl_do_reboot_bootloader(void)
{
	syna_reboot(FB_REBOOT_CMD_REBOOT_FB, 0, 0);
}

static void compl_do_reboot_recovery(void)
{
	syna_reboot(FB_REBOOT_CMD_RECOVERY, 0, 0);
}

static void compl_do_reboot_fastboot(void)
{
	syna_reboot(FB_REBOOT_CMD_FASTBOOT, 0, 0);
}

int fb_set_reboot_bootloader_flag(void)
{
	compl_do_reboot_bootloader();
	return 0;
}

int fb_set_reboot_recovery_flag(void)
{
	compl_do_reboot_recovery();
	return 0;
}

int fb_set_reboot_fastboot_flag(void)
{
	compl_do_reboot_fastboot();
	return 0;
}

#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_AB)
#define BOOTCTRL_MAGIC				0x42424100
#define BOOT_CONTROL_VERSION		1
#define ANDROID_AB_COMMAND_KEY		"bootctrl.metadata"
#define BOOTCTRL_HAL_FTS_LENGH		0x20
#define MISC_BOOT_CONTROL_VERSION  2

typedef struct syna_slot_metadata {
	uint8_t priority : 4;
	uint8_t tries_remaining : 3;
	uint8_t successful_boot : 1;
}syna_slot_metadata_t;

typedef struct boot_ctrl {
	 uint32_t magic;		// Magic for identification - '\0ABB' (Boot Contrl Magic)
	 uint8_t version; 		// boot ctrl version, 1
	 syna_slot_metadata_t slot_info[2]; 	// Information about each slot
	 uint8_t reserved;		// reserved
}boot_ctrl_t;


typedef struct misc_slot_metadata {
    uint8_t priority;
    uint8_t tries_remaining;
    // 1 if this slot has booted successfully, 0 otherwise.
    uint8_t successful_boot;
    // 1 if this slot is corrupted from a dm-verity corruption, 0 otherwise.
    uint8_t verity_corrupted;
    uint8_t reserved;
} misc_slot_metadata_t;

typedef struct misc_boot_ctrl {
    uint32_t magic;
    uint8_t version;
    uint8_t recovery_tries_remaining;
    misc_slot_metadata_t slot_info[2];
    uint8_t reserved[16];
} misc_boot_ctrl_t;

static bool is_valid_bootctrl(void *pBootctrl)
{
	if (fts_exist)
	{
		boot_ctrl_t *pbctrl = (boot_ctrl_t *)pBootctrl;
		return ((BOOTCTRL_MAGIC == pbctrl->magic) && (BOOT_CONTROL_VERSION == pbctrl->version));
	}
	else
	{
		misc_boot_ctrl_t *pbctrl = (misc_boot_ctrl_t *)pBootctrl;
		return ((BOOTCTRL_MAGIC == pbctrl->magic) && (MISC_BOOT_CONTROL_VERSION == pbctrl->version));
	}
}

static bool slot_is_bootable(void *pSlot_metadata)
{
	if (fts_exist)
	{
		syna_slot_metadata_t *pABSlot = (syna_slot_metadata_t *)pSlot_metadata;
		return pABSlot->priority > 0 && (pABSlot->successful_boot || (pABSlot->tries_remaining > 0));
	}
	else
	{
		misc_slot_metadata_t *pABSlot = (misc_slot_metadata_t *)pSlot_metadata;
		return pABSlot->priority > 0 && (pABSlot->successful_boot || (pABSlot->tries_remaining > 0));
	}
}

static int write_bootctrl_metadata(void *pbootctrl)
{
	if(NULL == pbootctrl) {
		printf("ERROR: unvalid bootctrl metadata for write !\n");
		return -1;
	}

	if (fts_exist)
	{
		char metadata_str[32] = {0};
		struct udevice *dev = NULL;

		snprintf(metadata_str, 17, "%016llx", *((uint64_t *)pbootctrl));

		if (uclass_get_device_by_name(UCLASS_MISC, "fts", &dev)) {
			printf("Can't found misc device named fts\n");
			return -1;
		}

		struct flash_ts_key key = {ANDROID_AB_COMMAND_KEY, metadata_str};
		if (misc_write(dev, 0, (void *)&key, 0)) {
			printf("ERROR: Failed to write bootctrl metadata !\n");
			return -1;
		}
	}
	else //devinfo_exist=true
	{
		struct blk_desc *dev_desc;
		disk_partition_t info;
		lbaint_t strat_blk, size_blk;
		struct mmc *mmc = find_mmc_device(CONFIG_FASTBOOT_FLASH_MMC_DEV);

		if (mmc == NULL) {
			printf("invalid mmc device\n");
			return -1;
		}

		dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
		if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
			printf("invalid mmc device\n");
			return -1;
		}

		char *partition_name = "misc";
		if (part_get_info_by_name(dev_desc, partition_name, &info) == -1) {
			printf("cannot find partition: '%s'\n", partition_name);
			return -1;
		}
		strat_blk = info.start + 4096/dev_desc->blksz;
		size_blk = (sizeof(misc_boot_ctrl_t) + dev_desc->blksz)/dev_desc->blksz;
		blk_dwrite(dev_desc, strat_blk, size_blk, pbootctrl);
	}

	return 0;
}

static int get_bootctrl_metadata(void *pbootctrl)
{
	if(NULL == pbootctrl) {
		printf("Error: invalid parameter pbootctrl !\n");
		return -1;
	}

	if (fts_exist)
	{
		uint64_t bootctrl_data = 0x0;
		boot_ctrl_t * pbootctrl_local = (boot_ctrl_t *)(uintptr_t)(&bootctrl_data);

		char value[BOOTCTRL_HAL_FTS_LENGH] = {0};
		struct flash_ts_key key = {ANDROID_AB_COMMAND_KEY, value};
		struct udevice *dev = NULL;

		if (uclass_get_device_by_name(UCLASS_MISC, "fts", &dev)) {
			printf("Can't found misc device named fts\n");
			return -1;
		}

		if (misc_read(dev, 0, (void *)&key, BOOTCTRL_HAL_FTS_LENGH))
			return -1;

		printf("fts: %s: %s\n", key.name, key.value);
		if(strlen(key.value)) {
			bootctrl_data = simple_strtoul((key.value), NULL, 16);
			//Always display below message for bootctrl.metadata recognization
			printf("-----bootctrl_data:  0x%x-%x-----\n", (unsigned int)(bootctrl_data >> 32),
								(unsigned int)bootctrl_data);
			printf("Magic 0x%x, version: 0x%x, reserved 0x%x\n", pbootctrl_local->magic,
							pbootctrl_local->version, pbootctrl_local->reserved);
			memcpy(pbootctrl, (void *)pbootctrl_local, sizeof(boot_ctrl_t));
		}
	}
	else //devinfo_exist=true
	{
		struct blk_desc *dev_desc;
		disk_partition_t info;
		lbaint_t strat_blk;
		struct mmc *mmc = find_mmc_device(CONFIG_FASTBOOT_FLASH_MMC_DEV);
		char * misc_bootctrl;

		if (mmc == NULL) {
			printf("invalid mmc device\n");
			return -1;
		}

		dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
		if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
			printf("invalid mmc device\n");
			return -1;
		}

		char *partition_name = "misc";
		if (part_get_info_by_name(dev_desc, partition_name, &info) == -1) {
			printf("cannot find partition: '%s'\n", partition_name);
			return -1;
		}

		misc_bootctrl = malloc(dev_desc->blksz);
		strat_blk = info.start + 4096/dev_desc->blksz;
		blk_dread(dev_desc, strat_blk, 1, misc_bootctrl);
		memcpy(pbootctrl, (void*)misc_bootctrl, sizeof(misc_boot_ctrl_t));

		free(misc_bootctrl);
	}

	return 0;
}

static int init_bootctrl(void *pBootCtrl, int default_slot)
{
	if(default_slot > 1) {
		printf("invalid default slot number for bootctrl !\n");
		return -1;
	}

	if (fts_exist)
	{
		boot_ctrl_t * pbootctrl = (boot_ctrl_t *)pBootCtrl;

		pbootctrl->slot_info[default_slot].priority		    = 2;
		pbootctrl->slot_info[default_slot].tries_remaining	= 2;
		pbootctrl->slot_info[default_slot].successful_boot 	= 0;

		pbootctrl->slot_info[1 - default_slot].priority		    = 1;
		pbootctrl->slot_info[1 - default_slot].tries_remaining	= 2;
		pbootctrl->slot_info[1 - default_slot].successful_boot	= 0;

		pbootctrl->version	= BOOT_CONTROL_VERSION;
		pbootctrl->magic	= BOOTCTRL_MAGIC;

		if(write_bootctrl_metadata(pBootCtrl)) {
			printf("Error: fail to write bootctrl metadata !\n");
			return -1;
		}
	}
	else //devinfo_exist=true
	{
		misc_boot_ctrl_t * pbootctrl = (misc_boot_ctrl_t *)pBootCtrl;

		pbootctrl->slot_info[default_slot].priority		    = 8;
		pbootctrl->slot_info[default_slot].tries_remaining	= 2;
		pbootctrl->slot_info[default_slot].successful_boot 	= 0;

		pbootctrl->slot_info[1 - default_slot].priority		    = 3;
		pbootctrl->slot_info[1 - default_slot].tries_remaining	= 2;
		pbootctrl->slot_info[1 - default_slot].successful_boot	= 0;

		pbootctrl->version	= MISC_BOOT_CONTROL_VERSION;
		pbootctrl->magic	= BOOTCTRL_MAGIC;

		if(write_bootctrl_metadata(pBootCtrl)) {
			printf("Error: fail to write misc bootctrl metadata !\n");
			return -1;
		}
	}

	return 0;
}

int get_current_slot(void)
{
	int bootab_sel = BOOTSEL_INVALID;

	if (fts_exist)
	{
		boot_ctrl_t bctrl;

		memset((void *)&bctrl, 0, sizeof(boot_ctrl_t));
		if(get_bootctrl_metadata((void *)&bctrl)) {
			printf("Error: fail to get bootctrl.metadata !\n");
			goto out;
		}

		if(is_valid_bootctrl((void *)&bctrl)) {
			if(slot_is_bootable((void *)&bctrl.slot_info[0]) &&
				slot_is_bootable((void *)&bctrl.slot_info[1])) {
				if(bctrl.slot_info[0].priority >= bctrl.slot_info[1].priority) {
					bootab_sel = BOOTSEL_A;
				}
				else{
					bootab_sel = BOOTSEL_B;
				}
			}
			else if (slot_is_bootable((void *)&bctrl.slot_info[0])) {
				bootab_sel = BOOTSEL_A;
			}
			else if (slot_is_bootable((void *)&bctrl.slot_info[1])) {
				bootab_sel = BOOTSEL_B;
			}
			else {
				/* No bootable slots! */
				printf("No bootable slots found !!!\n");
				bootab_sel = BOOTSEL_INVALID;
				goto out;
			}
		}
		else {
			printf("invalid bootctrl, Magic:0x%x,Ver:0x%x.\n", bctrl.magic, bctrl.version);
			if(!init_bootctrl((void *)&bctrl, BOOTSEL_DEFAULT)) {
				printf("No valid metadata for bootctrl, initialize to default slot %d !\n", BOOTSEL_DEFAULT);
				bootab_sel = BOOTSEL_DEFAULT;
			}
		}
	}
	else //devinfo_exist=true
	{
		misc_boot_ctrl_t bctrl;

		memset((void *)&bctrl, 0, sizeof(misc_boot_ctrl_t));
		if(get_bootctrl_metadata((void *)&bctrl)) {
			printf("Error: fail to get bootctrl.metadata !\n");
			goto out;
		}

		if(is_valid_bootctrl((void *)&bctrl)) {
			if(slot_is_bootable((void *)&bctrl.slot_info[0]) &&
				slot_is_bootable((void *)&bctrl.slot_info[1])) {
				if(bctrl.slot_info[0].priority >= bctrl.slot_info[1].priority) {
					bootab_sel = BOOTSEL_A;
				}
				else{
					bootab_sel = BOOTSEL_B;
				}
			}
			else if (slot_is_bootable((void *)&bctrl.slot_info[0])) {
				bootab_sel = BOOTSEL_A;
			}
			else if (slot_is_bootable((void *)&bctrl.slot_info[1])) {
				bootab_sel = BOOTSEL_B;
			}
			else {
				/* No bootable slots! */
				printf("No bootable slots found !!!\n");
				bootab_sel = BOOTSEL_INVALID;
				goto out;
			}
		}
		else {
			printf("invalid bootctrl, Magic:0x%x,Ver:0x%x.\n", bctrl.magic, bctrl.version);
			if(!init_bootctrl((void *)&bctrl, BOOTSEL_DEFAULT)) {
				printf("No valid metadata for bootctrl, initialize to default slot %d !\n", BOOTSEL_DEFAULT);
				bootab_sel = BOOTSEL_DEFAULT;
			}
		}
	}

	printf("Slot index=%d got !\n", bootab_sel);

out:
	return bootab_sel;
}

int get_slot_count(void)
{
	return BOOTSEL_ALL;
}

bool is_slot_successful(int slot_index)
{
	bool slot_successful = (bool)0;

	if((unsigned int)slot_index > 0x1) {
		printf("unsupported slot index !\n");
		goto out;
	}

	if (fts_exist)
	{
		boot_ctrl_t bctrl;

		memset((void *)&bctrl, 0, sizeof(boot_ctrl_t));
		if(get_bootctrl_metadata((void *)&bctrl)) {
			printf("Error: fail to get bootctrl.metadata !\n");
			goto out;
		}

		if(is_valid_bootctrl((void *)&bctrl)) {
			if(slot_is_bootable((void *)&bctrl.slot_info[slot_index]) && (bctrl.slot_info[slot_index].successful_boot))
				slot_successful = (bool)1;
			else
				slot_successful = (bool)0;
		}
		else {
			printf("Warning: no valid bootctrl metadata !\n");
			slot_successful = (bool)0;
		}
	}
	else //devinfo_exist=true
	{
		misc_boot_ctrl_t bctrl;

		memset((void *)&bctrl, 0, sizeof(misc_boot_ctrl_t));
		if(get_bootctrl_metadata((void *)&bctrl)) {
			printf("Error: fail to get bootctrl.metadata !\n");
			goto out;
		}

		if(is_valid_bootctrl((void *)&bctrl)) {
			if(slot_is_bootable((void *)&bctrl.slot_info[slot_index]) && (bctrl.slot_info[slot_index].successful_boot))
				slot_successful = (bool)1;
			else
				slot_successful = (bool)0;
		}
		else {
			printf("Warning: no valid bootctrl metadata !\n");
			slot_successful = (bool)0;
		}
	}

out:
	return slot_successful;
}

bool is_slot_unbootable(int slot_index)
{
	bool slot_unbootable = (bool)1;

	if((unsigned int)slot_index > 0x1) {
		printf("unsupported slot index !\n");
		goto out;
	}

	if (fts_exist)
	{
		boot_ctrl_t bctrl;

		memset((void *)&bctrl, 0, sizeof(boot_ctrl_t));
		if(get_bootctrl_metadata((void *)&bctrl)) {
			printf("Error: fail to get bootctrl.metadata !\n");
			goto out;
		}

		if(is_valid_bootctrl((void *)&bctrl)) {
			if(slot_is_bootable((void *)&bctrl.slot_info[slot_index]))
				slot_unbootable = (bool)0;
			else
				slot_unbootable = (bool)1;
		}
		else {
			printf("Warning: no valid bootctrl metadata !\n");
			slot_unbootable = (bool)1;
		}
	}
	else //devinfo_exist=true
	{
		misc_boot_ctrl_t bctrl;

		memset((void *)&bctrl, 0, sizeof(misc_boot_ctrl_t));
		if(get_bootctrl_metadata((void *)&bctrl)) {
			printf("Error: fail to get bootctrl.metadata !\n");
			goto out;
		}

		if(is_valid_bootctrl((void *)&bctrl)) {
			if(slot_is_bootable((void *)&bctrl.slot_info[slot_index]))
				slot_unbootable = (bool)0;
			else
				slot_unbootable = (bool)1;
		}
		else {
			printf("Warning: no valid bootctrl metadata !\n");
			slot_unbootable = (bool)1;
		}
	}

out:
	return slot_unbootable;
}

int get_slot_retry_count(int slot_index)
{
	int retry_count = 0x0;

	if((unsigned int)slot_index > 0x1) {
		printf("unsupported slot index !\n");
		goto out;
	}

	if (fts_exist)
	{
		boot_ctrl_t bctrl;

		memset((void *)&bctrl, 0, sizeof(boot_ctrl_t));
		if(get_bootctrl_metadata((void *)&bctrl)) {
			printf("Error: fail to get bootctrl.metadata !\n");
			goto out;
		}

		retry_count = bctrl.slot_info[slot_index].tries_remaining;
	}
	else //devinfo_exist=true
	{
		misc_boot_ctrl_t bctrl;

		memset((void *)&bctrl, 0, sizeof(misc_boot_ctrl_t));
		if(get_bootctrl_metadata((void *)&bctrl)) {
			printf("Error: fail to get bootctrl.metadata !\n");
			goto out;
		}

		retry_count = bctrl.slot_info[slot_index].tries_remaining;
	}

out:
	return retry_count;
}

bool set_slot_active(int slot_index)
{
	if((unsigned int)slot_index > 0x1) {
		printf("invalid default slot number for bootctrl !\n");
		goto error;
	}

	if (fts_exist)
	{
		boot_ctrl_t bctrl = {0};
		syna_slot_metadata_t *slotp = NULL;

		memset((void *)&bctrl, 0, sizeof(boot_ctrl_t));
		if(get_bootctrl_metadata((void *)&bctrl)) {
			printf("Error: fail to get bootctrl.metadata !\n");
			goto error;
		}

		if(is_valid_bootctrl((void *)&bctrl)) {
			slotp = &bctrl.slot_info[slot_index];
			slotp->successful_boot = 0;
			slotp->priority = 15;
			slotp->tries_remaining = 2;

			slotp = &bctrl.slot_info[1-slot_index];
			if(slotp->priority >= 15)
				slotp->priority = 14;
			if(write_bootctrl_metadata((void *)&bctrl)) {
				printf("Fail to write bootctrl metadata !\n");
				goto error;
			}
		}
		else {
			printf("invalid bootctrl, Magic:0x%x,Ver:0x%x.\n", bctrl.magic, bctrl.version);
			printf("No valid metadata for bootctrl, initialize to default slot %d !\n", slot_index);
			if(init_bootctrl((void *)&bctrl, slot_index))
				goto error;
		}
	}
	else //devinfo_exist=true
	{
		misc_boot_ctrl_t bctrl;
		misc_slot_metadata_t *slotp = NULL;

		memset((void *)&bctrl, 0, sizeof(misc_boot_ctrl_t));
		if(get_bootctrl_metadata((void *)&bctrl)) {
			printf("Error: fail to get bootctrl.metadata !\n");
			goto error;
		}

		if(is_valid_bootctrl((void *)&bctrl)) {
			slotp = &bctrl.slot_info[slot_index];
			slotp->successful_boot = 0;
			slotp->priority = 15;
			slotp->tries_remaining = 2;

			slotp = &bctrl.slot_info[1-slot_index];
			if(slotp->priority >= 15)
				slotp->priority = 14;
			if(write_bootctrl_metadata((void *)&bctrl)) {
				printf("Fail to write bootctrl metadata !\n");
				goto error;
			}
		}
		else {
			printf("invalid bootctrl, Magic:0x%x,Ver:0x%x.\n", bctrl.magic, bctrl.version);
			printf("No valid metadata for bootctrl, initialize to default slot %d !\n", slot_index);
			if(init_bootctrl((void *)&bctrl, slot_index))
				goto error;
		}
	}

	return (bool)1;

error:
	printf("Fail to set slot %d active !\n", slot_index);
	return (bool)0;
}
#endif

static struct part_driver *f_part_driver_lookup_type(struct blk_desc *dev_desc)
{
	struct part_driver *drv =
		ll_entry_start(struct part_driver, part_driver);
	const int n_ents = ll_entry_count(struct part_driver, part_driver);
	struct part_driver *entry;

	if (dev_desc->part_type == PART_TYPE_UNKNOWN) {
		for (entry = drv; entry != drv + n_ents; entry++) {
			int ret;

			ret = entry->test(dev_desc);
			if (!ret) {
				dev_desc->part_type = entry->part_type;
				return entry;
			}
		}
	} else {
		for (entry = drv; entry != drv + n_ents; entry++) {
			if (dev_desc->part_type == entry->part_type)
				return entry;
		}
	}

	/* Not found */
	return NULL;
}

static int f_part_get_info_by_name(struct blk_desc *dev_desc, const char *name,
			       disk_partition_t *info)
{
	struct part_driver *part_drv;
	int ret;
	int i;

	part_drv = f_part_driver_lookup_type(dev_desc);
	if (!part_drv)
		return -1;
	for (i = 1; i < part_drv->max_entries; i++) {
		ret = part_drv->get_info(dev_desc, i, info);
		if (ret != 0) {
			/* no more entries in table */
			break;
		}
		if (strcmp(name, (const char *)info->name) == 0) {
			/* matched */
			return i;
		}
	}

	return -1;
}
static int f_part_get_info_by_name_or_alias(struct blk_desc *dev_desc,
		const char *name, disk_partition_t *info)
{
	int ret;

	ret = f_part_get_info_by_name(dev_desc, name, info);
	if (ret < 0) {
		/* strlen("fastboot_partition_alias_") + 32(part_name) + 1 */
		char env_alias_name[25 + 32 + 1];
		char *aliased_part_name;

		/* check for alias */
		strcpy(env_alias_name, "fastboot_partition_alias_");
		strncat(env_alias_name, name, 32);
		aliased_part_name = env_get(env_alias_name);
		if (aliased_part_name != NULL)
			ret = f_part_get_info_by_name(dev_desc,
					aliased_part_name, info);
	}
	return ret;
}

int f_mmc_get_part_info(char *part_name, struct blk_desc **dev_desc,
			       disk_partition_t *part_info)
{
	int r;

	*dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!*dev_desc) {
		printf("block device not found");
		return -ENOENT;
	}
	if (!part_name) {
		printf("partition not found");
		return -ENOENT;
	}

	r = f_part_get_info_by_name_or_alias(*dev_desc, part_name, part_info);
	if (r < 0) {
		printf("partition not found");
		return r;
	}

	return r;
}

int f_mmc_get_part_index(char *part_name)
{
	int r;
	struct blk_desc *dev_desc;
	disk_partition_t part_info;

	return f_mmc_get_part_info(part_name, &dev_desc, &part_info);
}

static int is_ext4_type(char *part_name)
{
	int r;
	struct blk_desc *dev_desc;
	disk_partition_t part_info;

	r = f_mmc_get_part_info(part_name, &dev_desc, &part_info);
	if (r >= 0) {
		//iomux_doenv(stdout, "serial");
		r = fs_set_blk_dev_with_part(dev_desc, r);
		//iomux_doenv(stdout, "serial,vidconsole");
		if (r < 0) {
			printf("failed to set partition\n");
			return 0;
		} else {
			printf("fs_type_name:%s\n", fs_get_type_name());
			return 1;
		}
	}
	else {
		printf("partition not found!");
		return -1;
	}
}

char *fastboot_check_partition_type(char *patition_name)
{
	char *s = NULL;
	if (CHECK_NAME("bootloader", patition_name)
		|| CHECK_NAME("bootloader2", patition_name)
		|| CHECK_NAME("fastboot_1st", patition_name)
		|| CHECK_NAME("fastboot_2nd", patition_name)
		|| CHECK_NAME("tzk_recovery", patition_name)
		|| CHECK_NAME("bl_recovery", patition_name)
		|| CHECK_NAME("recovery", patition_name)
		|| CHECK_NAME("tzk_normal", patition_name)
		|| CHECK_NAME("tzk_normalB", patition_name)
		|| CHECK_NAME("bl_normal", patition_name)
		|| CHECK_NAME("bl_normalB", patition_name)
		|| CHECK_NAME("boot", patition_name)
		|| CHECK_NAME("fastlogo", patition_name)
		|| CHECK_NAME("fts", patition_name)
		|| CHECK_NAME("misc", patition_name)
		|| CHECK_NAME("dtbo", patition_name)
		|| CHECK_NAME("super", patition_name)
		|| CHECK_NAME("vbmeta", patition_name)
		|| CHECK_NAME("vbmeta_system", patition_name)
		|| CHECK_NAME("vbmeta_vendor", patition_name)) {
		s = "emmc";
		return s;
	}
	else if (CHECK_NAME("userdata", patition_name)) {
		switch (is_ext4_type(patition_name)) {
		case 0:
			s = "f2fs";
			return s;
			break;
		case 1:
			s = "ext4";
			return s;
			break;
		case -1:
			s = "partition not found!";
			return s;
			break;
		}
	} else {
		s = "ext4";
		return s;
	}
	return s;
}

static void read_raw_image_from_offset(struct blk_desc *dev_desc, disk_partition_t *info,
		const char *part_name, void *buffer,
		unsigned int read_bytes, unsigned int offset)
{
	lbaint_t blkcnt;
	lbaint_t blks;
	lbaint_t start;

	/* determine number of blocks to write */
	blkcnt = ((read_bytes + (info->blksz - 1)) & ~(info->blksz - 1));
	blkcnt = lldiv(blkcnt, info->blksz);

	start = info->start;

	if(offset % (info->blksz)) {
		printf("offset should be aligned with block size\n");
		return;
	}

	start += (offset / (info->blksz));

	if (blkcnt > info->size) {
		printf("too large for partition: '%s'\n", part_name);
		return;
	}

	blks = blk_dread(dev_desc, start, blkcnt, buffer);
	if (blks != blkcnt) {
		printf("failed reading to device %d\n", dev_desc->devnum);
		return;
	}
}

void fb_mmc_flash_read_from_offset(const char *cmd, void *read_buffer,
			unsigned int read_bytes, unsigned int offset)
{
	struct blk_desc *dev_desc;
	disk_partition_t info;
	struct mmc *mmc = find_mmc_device(CONFIG_FASTBOOT_FLASH_MMC_DEV);

	if (mmc == NULL) {
		printf("invalid mmc device\n");
		return;
	}

	dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("invalid mmc device\n");
		return;
	}

	if ((strcmp(cmd, "bootloader") == 0) ||
		 (strcmp(cmd, "bootloader1") == 0)) {
		int ret = blk_dselect_hwpart(dev_desc, 1);
		if (ret) {
			printf("invalid mmc hwpart\n");
			return;
		}
		info.start = 0;
		info.size = dev_desc->lba;
		info.blksz = dev_desc->blksz;
		printf("%lx, %lx, %lx\n", info.start, info.size, info.blksz);
	} else if (strcmp(cmd, "bootloader2") == 0) {
		int ret = blk_dselect_hwpart(dev_desc, 2);
		if (ret) {
			printf("invalid mmc hwpart\n");
			return;
		}
		info.start = 0;
		info.size = dev_desc->lba;
		info.blksz = dev_desc->blksz;
		printf("%lx, %lx, %lx\n", info.start, info.size, info.blksz);
	} else if (f_part_get_info_by_name_or_alias(dev_desc, cmd, &info) < 0) {
		printf("cannot find partition: '%s'\n", cmd);
		return;
	}

	read_raw_image_from_offset(dev_desc, &info, cmd, read_buffer, read_bytes, offset);
}

void fb_mmc_flash_read(const char *cmd, void *read_buffer,
			unsigned int read_bytes)
{
	fb_mmc_flash_read_from_offset(cmd, read_buffer, read_bytes, 0);
}

