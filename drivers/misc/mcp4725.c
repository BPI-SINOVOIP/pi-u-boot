#include <common.h>
#include <bootm.h>
#include <command.h>
#include <errno.h>
#include <image.h>
#include <malloc.h>
#include <nand.h>
#include <asm/byteorder.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <u-boot/zlib.h>
#include <dm.h>
#include <i2c.h>
#include <dm/lists.h>
#include <dm/root.h>
#include <linux/string.h>
#include <asm/io.h>
#include <mapmem.h>
#include <fdt_support.h>
#include <stdlib.h>

static int mcp5725_i2c_xfer(struct udevice * dev, struct i2c_msg * msg, int nmsgs)
{

        int ret;
	ret = dm_i2c_xfer(dev, msg, nmsgs);

        return 0;
}

static const struct dm_i2c_ops mcp5725_i2c_ops = {
        .xfer 		= mcp5725_i2c_xfer,
};


static const struct udevice_id mcp5725_i2c_ids[] = {
        { .compatible = "microchip,mcp4725" },
        { }
};

U_BOOT_DRIVER(i2c_mcp4725) = {
        .name     = "mcp4725",
        .id       = UCLASS_I2C_GENERIC,
        .of_match = mcp5725_i2c_ids,
        .ops      = &mcp5725_i2c_ops,
};


