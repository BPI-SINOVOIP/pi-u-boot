#include <common.h>
#include <console.h>
#include <dm.h>
#include <flash_ts.h>
#include <misc.h>
#include <usb.h>
#include <mmc.h>

DECLARE_GLOBAL_DATA_PTR;

typedef struct boot_info {
	char mac_addr[32];
	char serialno[64];
	char command[64];
	char boot_option[32];
	int  debeg_level;
	char reserved1[32];
	char reserved2[32];
	char reserved3[32];
} boot_info_t;

bool fts_exist = false; 	 //fts partition exist
bool devinfo_exist = false;  //devinfo partition exist
static boot_info_t Boot_info;

#define FTS_NAME_LENGTH		128
#define FTS_VALUE_LENGTH	128

static void check_devinfo_partition(void)
{
	struct blk_desc *dev_desc = NULL;
	disk_partition_t info;

	struct mmc *mmc = find_mmc_device(0);
	if (mmc == NULL) {
		printf("invalid mmc device\n");
	}

	dev_desc = blk_get_dev("mmc", 0);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("invalid mmc device\n");
	}

	if (part_get_info_by_name(dev_desc, "fts", &info) != -1)
	{
		fts_exist = true;
		printf("Found misc device named fts\n");
	}
	else if (part_get_info_by_name(dev_desc, "devinfo", &info) != -1)
	{
		char * pBuff = malloc(info.blksz);

		memset((void *)&Boot_info, 0, sizeof(boot_info_t));
		if(pBuff)
		{
			blk_dread(dev_desc, info.start, 1, pBuff);
			memcpy((void *)&Boot_info, pBuff, sizeof(boot_info_t));
			free(pBuff);
		}
		devinfo_exist = true;
		printf("Found misc device named devinfo\n");
	}
	else
	{
		printf("Can't found misc device named fts or devinfo \n");
	}

}

#if CONFIG_IS_ENABLED(FASTBOOT)
static int getenv_fastboot(void)
{
	const char *value_fdt = NULL;

	if (fts_exist)
	{
		struct udevice *dev = NULL;
		int res = uclass_get_device_by_name(UCLASS_MISC, "fts", &dev);
		if (res) {
			printf("Can't found misc device named fts\n");
			return -ENODEV;
		}
		char name[FTS_NAME_LENGTH], value[FTS_VALUE_LENGTH];
		struct flash_ts_key key = {name, value};

		strncpy(name, "serialno", FTS_NAME_LENGTH);
		res = misc_read(dev, 0, (void *)&key, FTS_VALUE_LENGTH);
		if (strnlen(value, FTS_VALUE_LENGTH) > 0) {
			printf("Serial#: %s\n", value);
			env_set("serial#", value);
		} else {
			puts("Serial#: 123456789ABCDEF(default)\n");
			env_set("serial#", "123456789ABCDEF");
		}
	}
	else if (devinfo_exist)
	{
		if (strnlen(Boot_info.serialno, FTS_VALUE_LENGTH) > 0) {
			printf("Serial#: %s\n", Boot_info.serialno);
			env_set("serial#", Boot_info.serialno);
		} else {
			puts("Serial#: 123456789ABCDEF(default)\n");
			env_set("serial#", "123456789ABCDEF");
		}
	}

	if ((value_fdt = fdt_getprop(gd->fdt_blob, 0, "fastboot,product-name", NULL))) {
		printf("Product: %s(default)\n", value_fdt);
		env_set("product", value_fdt);
	} else {
		printf("Error: product name not specified.\n");
		env_set("product", "N/A");
	}

	return 0;
}
#endif

#if CONFIG_IS_ENABLED(DM_ETH)
static int getenv_ethaddr(void)
{
	if ((env_get("ethaddr") == NULL) || (env_get("eth1addr") == NULL)) {
		if (fts_exist)
		{
			struct udevice *dev = NULL;
			int res = uclass_get_device_by_name(UCLASS_MISC, "fts", &dev);
			if (res) {
				printf("Can't found misc device named fts\n");
				return -ENODEV;
			}
			char name[FTS_NAME_LENGTH], value[FTS_VALUE_LENGTH];
			struct flash_ts_key key = {name, value};
			strncpy(name, "macaddr", FTS_NAME_LENGTH);
			res = misc_read(dev, 0, (void *)&key, FTS_VALUE_LENGTH);
			if (strnlen(value, FTS_VALUE_LENGTH) <= 0) {
				if (env_get("ethaddr") == NULL)
					puts("get ethaddr error\n");
			} else {
				if (env_get("ethaddr") == NULL)
					env_set("ethaddr", value);
			}
		}
		else if (devinfo_exist)
		{
			if (strnlen(Boot_info.mac_addr, FTS_VALUE_LENGTH) > 0) {
				printf("ethaddr#: %s\n", Boot_info.mac_addr);
				env_set("ethaddr#", Boot_info.mac_addr);
			} else {
				puts("ethaddr#: 11:22:33:44:55:66(default)\n");
				env_set("ethaddr#", "11:22:33:44:55:66");
			}
		}
	}
	return 0;
}
#endif

int arch_misc_init(void)
{
	int ret = 0;

	check_devinfo_partition();

#if CONFIG_IS_ENABLED(FASTBOOT)
	ret = getenv_fastboot();
	if (ret)
		return ret;
#endif
#if CONFIG_IS_ENABLED(DM_ETH)
	ret = getenv_ethaddr();
	if (ret)
		return ret;
#endif
	return ret;
}
