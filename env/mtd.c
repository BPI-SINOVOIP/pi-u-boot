#include <common.h>
#include <env.h>
#include <env_internal.h>
#include <linux/mtd/mtd.h>
#include <jffs2/load_kernel.h>
#include <mtd.h>
#include <dm.h>
#include <env.h>
#include <env_internal.h>
#include <malloc.h>
#include <spi.h>
#include <spi_flash.h>
#include <search.h>
#include <errno.h>
#include <uuid.h>
#include <asm/cache.h>
#include <asm/global_data.h>
#include <dm/device-internal.h>
#include <u-boot/crc.h>
#include <config.h>

static struct udevice *env_mtd_dev = NULL;

static int env_mtd_init(void) {
	if (!env_mtd_dev) {
		if (uclass_get_device(UCLASS_MTD, 0, &env_mtd_dev)) {
			return -ENODEV;
		}
	}
	return 0;
}

static struct udevice *find_mtd_device(void) {
	struct udevice *dev;
	if (uclass_get_device(UCLASS_MTD, 0, &dev)) {
		printf("Cannot find any MTD device\n");
		return NULL;
	}
	return dev;
}

static int env_mtd_erase(void) {
	struct udevice *dev = find_mtd_device();
	if (!dev) {
		printf("Failed to initialize MTD device\n");
		return -ENODEV;
	}

	struct mtd_info *mtd = dev_get_uclass_priv(dev);

#ifdef CONFIG_ENV_OFFSET_REDUND
	loff_t offset = (gd->env_valid == ENV_VALID) ? CONFIG_ENV_OFFSET_REDUND : CONFIG_ENV_OFFSET;
#else
	loff_t offset = CONFIG_ENV_OFFSET;
#endif

	struct erase_info erase_opts = {
		.addr = offset,
		.len = CONFIG_ENV_SIZE,
	};

	if (mtd_erase(mtd, &erase_opts)) {
		return -EIO;
	}
	return 0;
}
#if defined(CONFIG_ENV_OFFSET_REDUND)

static int env_mtd_load(void) {
	struct udevice *dev = find_mtd_device();
	if (!dev) {
		printf("MTD device not initialized\n");
		return -ENODEV;
	}

	struct mtd_info *mtd = dev_get_uclass_priv(dev);
	size_t retlen;
	char *buf1 = malloc(CONFIG_ENV_SIZE);
	char *buf2 = malloc(CONFIG_ENV_SIZE);
	if (!buf1 || !buf2) {
		free(buf1);
		free(buf2);
		return -ENOMEM;
	}

	int read1_fail = mtd_read(mtd, CONFIG_ENV_OFFSET, CONFIG_ENV_SIZE, &retlen, buf1);
	int read2_fail = mtd_read(mtd, CONFIG_ENV_OFFSET_REDUND, CONFIG_ENV_SIZE, &retlen, buf2);

	int ret = env_import_redund(buf1, read1_fail, buf2, read2_fail, H_EXTERNAL);
	if (ret == 0) {
		gd->env_valid = (read1_fail == 0) ? ENV_VALID : ENV_REDUND;
		printf("Loaded environment from %s MTD location\n", 
			   (gd->env_valid == ENV_VALID) ? "primary" : "redundant");
	} else {
		printf("Failed to load environment from MTD device\n");
		gd->env_valid = ENV_INVALID;
	}

	free(buf1);
	free(buf2);
	return (gd->env_valid == ENV_INVALID) ? -EIO : 0;
}

static int env_mtd_save(void) {
	struct udevice *dev = find_mtd_device();
	if (!dev) {
		printf("MTD device not initialized\n");
		return -ENODEV;
	}

	struct mtd_info *mtd = dev_get_uclass_priv(dev);
	env_t env_new;
	char *buf = env_new.data;

	int ret = env_export(&env_new);
	if (ret)
		return -EIO;

	env_new.flags = ENV_REDUND_ACTIVE;
	loff_t env_new_offset, env_offset;

	if (gd->env_valid == ENV_VALID) {
		env_new_offset = CONFIG_ENV_OFFSET_REDUND;
		env_offset = CONFIG_ENV_OFFSET;
	} else {
		env_new_offset = CONFIG_ENV_OFFSET;
		env_offset = CONFIG_ENV_OFFSET_REDUND;
	}

	size_t retlen;

	ret = env_mtd_erase();
	if (ret) {
		printf("Failed to erase new environment location on MTD device\n");
		return -EIO;
	}

	ret = mtd_write(mtd, env_new_offset, CONFIG_ENV_SIZE, &retlen, buf);
	if (ret) {
		printf("Failed to write new environment to MTD device\n");
		return -EIO;
	}

	char flag = ENV_REDUND_OBSOLETE;
	ret = mtd_write(mtd, env_offset + offsetof(env_t, flags), sizeof(env_new.flags), &retlen, &flag);
	if (ret) {
		printf("Failed to mark old environment as obsolete\n");
		return -EIO;
	}

	printf("Environment successfully saved to MTD device\n");
	gd->env_valid = gd->env_valid == ENV_REDUND ? ENV_VALID : ENV_REDUND;
	return 0;
}

#else

static int env_mtd_load(void) {
	struct udevice *dev = find_mtd_device();
	if (!dev) {
		printf("MTD device not initialized\n");
		return -ENODEV;
	}

	struct mtd_info *mtd = dev_get_uclass_priv(dev);
	size_t retlen;
	char *buf = malloc(CONFIG_ENV_SIZE);
	if (!buf) {
		return -ENOMEM;
	}

	int ret = mtd_read(mtd, CONFIG_ENV_OFFSET, CONFIG_ENV_SIZE, &retlen, buf);
	if (ret) {
		printf("Failed to read environment from MTD device\n");
		free(buf);
		return -EIO;
	}

	env_import(buf, 1, H_EXTERNAL);
	free(buf);
	return 0;
}

static int env_mtd_save(void) {
	struct udevice *dev = find_mtd_device();
	if (!dev) {
		printf("MTD device not initialized\n");
		return -ENODEV;
	}

	struct mtd_info *mtd = dev_get_uclass_priv(dev);
	env_t env_new;
	char *buf = env_new.data;

	int ret = env_export(&env_new);
	if (ret)
		return -EIO;

	ret = env_mtd_erase();
	if (ret) {
		printf("Failed to erase environment on MTD device\n");
		return -EIO;
	}

	size_t retlen;
	ret = mtd_write(mtd, CONFIG_ENV_OFFSET, CONFIG_ENV_SIZE, &retlen, buf);
	if (ret) {
		printf("Failed to write environment to MTD device\n");
		return -EIO;
	}

	return 0;
}

#endif

U_BOOT_ENV_LOCATION(mtd) = {
	.location = ENVL_MTD,
	ENV_NAME("mtdENV")
	.load     = env_mtd_load,
	.save     = ENV_SAVE_PTR(env_mtd_save),
	.erase    = ENV_ERASE_PTR(env_mtd_erase),
	.init     = env_mtd_init,
};


