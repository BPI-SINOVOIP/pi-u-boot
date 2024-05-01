#include <common.h>
#include <dm.h>
#include <malloc.h>
#include <spi.h>
#include <spi_flash.h>
#include <blk.h>
#include <dm/device-internal.h>


static unsigned long nor_blk_read(struct udevice *bdev, lbaint_t start, lbaint_t blkcnt, void *buffer)
{

	// Retrieve the original SPI NOR device from nor_blk device's private data
	struct udevice *orig_dev = dev_get_priv(bdev);
	if (!orig_dev) {
		printf("%s:%d: Failed to get original device\n", __func__, __LINE__);
		return -ENODEV;
	}

	struct spi_flash *flash = dev_get_uclass_priv(orig_dev);
	struct mtd_info *mtd = &flash->mtd;
	if (!mtd) {
		printf("%s:%d: Failed to get MTD info\n", __func__, __LINE__);
		return -ENODEV;
	}

	// Calculate the offset and length for the read operation
	loff_t offset = (loff_t)start * SPI_NOR_BLOCK_SIZE;
	size_t len = blkcnt * SPI_NOR_BLOCK_SIZE;

	size_t retlen = 0;
	int result = mtd->_read(mtd, offset, len, &retlen, buffer);
	if (result) {
		printf("%s:%d: MTD read error %d\n", __func__, __LINE__, result);
		return result;
	}

	unsigned long blocks_read = retlen / SPI_NOR_BLOCK_SIZE;
	return blocks_read;
}

static const struct blk_ops nor_blk_ops = {
	.read = nor_blk_read,
};

int nor_blk_probe(struct udevice *dev)
{
	struct blk_desc *desc = dev_get_uclass_plat(dev);
	if (!desc) {
		printf("Failed to get block device descriptor\n");
		return -ENODEV;
	}

	// The private data should already be the SPI NOR device ('nor_dev')
	struct udevice *nor_dev = dev_get_priv(dev);
	if (!nor_dev) {
		printf("Failed to get the SPI NOR device from private data\n");
		return -ENODEV;
	}

	// Retrieve the SPI flash structure from the SPI NOR device
	struct spi_flash *flash = dev_get_uclass_priv(nor_dev);
	if (!flash) {
		printf("Failed to get SPI flash data\n");
		return -ENODEV;
	}

	// Configure block device descriptor properties based on the flash data
	desc->blksz = SPI_NOR_BLOCK_SIZE;
	desc->lba = flash->mtd.size / desc->blksz;

	return 0;
}

U_BOOT_DRIVER(nor_blk) = {
	.name  = "nor_blk",
	.id    = UCLASS_BLK,
	.probe = nor_blk_probe,
	.ops   = &nor_blk_ops,
	.priv_auto = sizeof(struct blk_desc),
};