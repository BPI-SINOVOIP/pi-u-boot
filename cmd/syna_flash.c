/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Shaojun Feng <Shaojun.feng@synaptics.com>
 */

#include <common.h>
#include <spi.h>
#include <spi_flash.h>
#include <dm/device-internal.h>

#define SPI_NOR_MAX_ID_LEN	6

struct flash_info {
	char		*name;

	/*
	 * This array stores the ID bytes.
	 * The first three bytes are the JEDIC ID.
	 * JEDEC ID zero means "no ID" (mostly older chips).
	 */
	u8		id[SPI_NOR_MAX_ID_LEN];
	u8		id_len;

	/* The size listed here is what works with SPINOR_OP_SE, which isn't
	 * necessarily called a "sector" by the vendor.
	 */
	unsigned int	sector_size;
	u16		n_sectors;

	u16		page_size;
	u16		addr_width;

	u16		flags;
};

extern const struct flash_info spi_nor_ids[];

flash_info_t flash_info[CONFIG_SYS_MAX_FLASH_BANKS]; /* info for FLASH chips */

static unsigned long flash_info_update(struct spi_flash *flash)
{
	const struct flash_info * finfo = spi_nor_ids;
	int i;

	for(; finfo->name != NULL; finfo++) {
		if((strlen(finfo->name) == strlen(flash->name)) &&
			(strncmp(finfo->name, flash->name, strlen(flash->name)) == 0)) {
			break;
		}
	}

	if (finfo->name == NULL) {
		printf("unrecognized sf device!\n");
		return 0;
	}

	memset(&flash_info[0], 0x0, sizeof(flash_info_t));

	flash_info[0].flash_id = (finfo->id[0] << 16) | (finfo->id[1] << 8) | finfo->id[2];
	flash_info[0].sector_count = flash->size / flash->sector_size;
	flash_info[0].size = flash->size;
	for (i = 0; i < flash_info[0].sector_count; i++) {
		flash_info[0].start[i] = 0xF0000000 + i * flash->sector_size;
		flash_info[0].protect[i] = 0;
	}
	return flash_info[0].size;
}

static int spi_flash_get(struct spi_slave **pslave)
{
	unsigned int bus = CONFIG_SF_DEFAULT_BUS;
	unsigned int cs = CONFIG_SF_DEFAULT_CS;
	unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
	unsigned int mode = CONFIG_SF_DEFAULT_MODE;

	char name[30], *str;
	struct udevice *dev = NULL;

	snprintf(name, sizeof(name), "spi_flash@%d:%d", bus, cs);
	str = strdup(name);
	if (!str)
		return -ENOMEM;
	return spi_get_bus_and_cs(bus, cs, speed, mode, "spi_flash_std",
				 str, &dev, pslave);
}

unsigned long flash_init(void)
{
	struct spi_slave *slave = NULL;
	int ret = 0;

	struct udevice *bus, *dev;

	for (ret = uclass_first_device(UCLASS_SPI, &dev);
	     dev;
	     ret = uclass_next_device(&dev)) {};

	ret = uclass_get_device_by_seq(UCLASS_SPI, CONFIG_SF_DEFAULT_BUS, &bus);
	if (ret < 0) {
		printf("Invalid bus %d (err=%d)\n", CONFIG_SF_DEFAULT_BUS, ret);
		return ret;
	}
	ret = spi_find_chip_select(bus, CONFIG_SF_DEFAULT_CS, &dev);
	if (ret == 0 && device_active(dev)) {
		device_remove(dev, DM_REMOVE_NORMAL);
		printf("Remove spi flash device and reprobe it.\n");
	}

	ret = spi_flash_get(&slave);
	if (ret < 0) {
		flash_info[0].flash_id = FLASH_UNKNOWN;
		return ret;
	}
	if (slave == NULL)
		return -1;

	debug("spi speed is %d Hz.\n", slave->max_hz);

	struct spi_flash *flash = dev_get_uclass_priv(slave->dev);

	return flash_info_update(flash);
}

void flash_print_info(flash_info_t *info)
{
	int i;

	// printf("%s, %d sectors, 0x%x bytes per sector.\n", mv_spi.name, mv_spi.scount, mv_spi.ssize);
	printf("  Flash ID: 0x%08lx\n\n", info->flash_id);
	printf("  Size: %ld MB in %d Sectors\n", info->size >> 20, info->sector_count);
	printf("  Sector Start Addresses:");
	for (i = 0; i < info->sector_count; i++) {
		if ((i % 4) == 0)
			printf ("\n   ");
		printf (" %08lX%s",
			info->start[i],
			info->protect[i] ? " (RO)" : "     "
		);
	}
	printf("\n");
}

int flash_erase(flash_info_t *info, int first, int last)
{
	struct spi_slave *slave = NULL;
	int ret = 0;

	ret = spi_flash_get(&slave);
	if (ret < 0)
		return ret;
	if (slave == NULL)
		return -1;

	struct spi_flash *flash = dev_get_uclass_priv(slave->dev);

	u32 offset = (u32)(first * flash->sector_size);
	size_t len = (last - first + 1) * flash->erase_size;
	ret = spi_flash_erase(flash, offset, len);
	debug("SF: %zu bytes @ %#x Erased: %s\n", len, offset,
	       ret ? "ERROR" : "OK");

	return ret == 0 ? 0 : 1;
}

int write_buff(flash_info_t *info, uchar *src, ulong addr, ulong cnt)
{
	struct spi_slave *slave = NULL;
	int ret = 0;

	ret = spi_flash_get(&slave);
	if (ret < 0)
		return ret;
	if (slave == NULL)
		return -1;

	struct spi_flash *flash = dev_get_uclass_priv(slave->dev);

	u32 offset = (u32)(addr - 0xF0000000);
	size_t len = cnt;
	ret = spi_flash_write(flash, offset, len, src);
	debug("SF: %zu bytes @ %#x Written: %s", len, offset,
		ret ? "ERROR" : "OK");
	return 0;
}

#ifdef CONFIG_SYS_FLASH_PROTECTION
int flash_real_protect(flash_info_t *info, long sector, int prot)
{
	return 0;
}
#endif

static int do_spinit(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct spi_slave *slave = NULL;
	int ret = 0;

	struct udevice *bus, *dev;

	for (ret = uclass_first_device(UCLASS_SPI, &dev);
	     dev;
	     ret = uclass_next_device(&dev)) {};

	ret = uclass_get_device_by_seq(UCLASS_SPI, CONFIG_SF_DEFAULT_BUS, &bus);
	if (ret) {
		printf("Invalid bus %d (err=%d)\n", CONFIG_SF_DEFAULT_BUS, ret);
		return ret;
	}
	ret = spi_find_chip_select(bus, CONFIG_SF_DEFAULT_CS, &dev);
	if (ret == 0 && device_active(dev)) {
		device_remove(dev, DM_REMOVE_NORMAL);
		printf("Remove spi flash device and reprobe it.\n");
	}

	ret = spi_flash_get(&slave);
	if (ret < 0)
		return ret;
	if (slave == NULL)
		return -1;

	debug("spi speed is %d Hz.\n", slave->max_hz);

	struct spi_flash *flash = dev_get_uclass_priv(slave->dev);

	ret = flash_info_update(flash);
	if (ret <= 0)
		return ret;

	return 0;
}

U_BOOT_CMD(
	spinit,		2,	0,	do_spinit,
	"init marvell spi flash",
	"	- init marvell spi flash"
);
