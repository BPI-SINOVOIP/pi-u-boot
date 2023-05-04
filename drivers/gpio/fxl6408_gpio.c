/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2016 Broadcom Limited.
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Cheng Lu <Cheng.Lu@synaptics.com>
 */

#include <common.h>
#include <dm.h>
#include <i2c.h>
#include <asm-generic/gpio.h>

#define FXL6408_DEVICE_ID		0x01
# define FXL6408_RST_INT		BIT(1)
# define FXL6408_SW_RST			BIT(0)

/* Bits set here indicate that the GPIO is an output. */
#define FXL6408_IO_DIR			0x03
/* Bits set here, when the corresponding bit of IO_DIR is set, drive
 * the output high instead of low.
 */
#define FXL6408_OUTPUT			0x05
/* Bits here make the output High-Z, instead of the OUTPUT value. */
#define FXL6408_OUTPUT_HIGH_Z		0x07
/* Bits here define the expected input state of the GPIO.
 * INTERRUPT_STAT bits will be set when the INPUT transitions away
 * from this value.
 */
#define FXL6408_INPUT_DEFAULT_STATE	0x09
/* Bits here enable either pull up or pull down according to
 * FXL6408_PULL_DOWN.
 */
#define FXL6408_PULL_ENABLE		0x0b
/* Bits set here (when the corresponding PULL_ENABLE is set) enable a
 * pull-up instead of a pull-down.
 */
#define FXL6408_PULL_UP			0x0d
/* Returns the current status (1 = HIGH) of the input pins. */
#define FXL6408_INPUT_STATUS		0x0f
/* Mask of pins which can generate interrupts. */
#define FXL6408_INTERRUPT_MASK		0x11
/* Mask of pins which have generated an interrupt.  Cleared on read. */
#define FXL6408_INTERRUPT_STAT		0x13

static int fxl6408_i2c_write_le8(struct udevice *dev, uint offset, u8 word)
{
	struct dm_i2c_chip *chip = dev_get_parent_platdata(dev);
	u8 buf[1];
	int ret;

	buf[0] = word;
	ret = dm_i2c_write(dev, offset, buf, 1);
	if (ret)
		printf("%s i2c write failed to addr %x\n", __func__,
			chip->chip_addr);

	return ret;
}

static int fxl6408_i2c_read_le8(struct udevice *dev, uint offset)
{
	struct dm_i2c_chip *chip = dev_get_parent_platdata(dev);
	u8 buf[1];
	int ret;

	ret = dm_i2c_read(dev, offset, buf, 1);
	if (ret < 0) {
		printf("%s i2c read failed from addr %x\n", __func__,
			chip->chip_addr);
		return ret;
	}

	return buf[0];
}

static int fxl6408_gpio_direction_input(struct udevice *dev, unsigned off)
{
	u8 val;

	val = fxl6408_i2c_read_le8(dev, FXL6408_IO_DIR);
	if (val < 0)
		return val;
	val &= ~BIT(off);
	fxl6408_i2c_write_le8(dev, FXL6408_IO_DIR, val);
	return 0;
}

static int fxl6408_gpio_direction_output(struct udevice *dev, unsigned off, int val)
{
	u8 reg_output, reg_io_dir;

	reg_output = fxl6408_i2c_read_le8(dev, FXL6408_OUTPUT);
	if (reg_output < 0)
		return reg_output;

	if (val)
		reg_output |= BIT(off);
	else
		reg_output &= ~BIT(off);
	fxl6408_i2c_write_le8(dev, FXL6408_OUTPUT, reg_output);

	reg_io_dir = fxl6408_i2c_read_le8(dev, FXL6408_IO_DIR);
	reg_io_dir |= BIT(off);
	fxl6408_i2c_write_le8(dev, FXL6408_IO_DIR, reg_io_dir);

	return 0;
}

static int fxl6408_gpio_get_value(struct udevice *dev, unsigned off)
{
	u8 reg;

	reg = fxl6408_i2c_read_le8(dev, FXL6408_INPUT_STATUS);
	if (reg < 0)
		return reg;
	return (reg & BIT(off)) != 0;
}

static int fxl6408_gpio_set_value(struct udevice *dev, unsigned off, int val)
{
	return fxl6408_gpio_direction_output(dev, off, val);
}

static int fxl6408_gpio_probe(struct udevice  *dev)
{
	u8 device_id;
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);

	uc_priv->gpio_count = 8;
	/* Check the device ID register to see if it's responding.
	 * This also clears RST_INT as a side effect, so we won't get
	 * the "we've been power cycled" interrupt once we enable
	 * interrupts.
	 */
	device_id = fxl6408_i2c_read_le8(dev, FXL6408_DEVICE_ID);
	if (device_id < 0) {
		dev_err(dev, "FXL6408 probe returned %d\n", device_id);
		return device_id;
	} else if (device_id >> 5 != 5) {
		dev_err(dev, "FXL6408 probe returned DID: 0x%02x\n", device_id);
		return -ENODEV;
	}

	/* Disable High-Z of outputs, so that our OUTPUT updates
	 * actually take effect.
	 */
	fxl6408_i2c_write_le8(dev, FXL6408_OUTPUT_HIGH_Z, 0);

	return 0;
}

static const struct dm_gpio_ops fxl6408_gpio_ops = {
	.direction_input	= fxl6408_gpio_direction_input,
	.direction_output	= fxl6408_gpio_direction_output,
	.get_value		= fxl6408_gpio_get_value,
	.set_value		= fxl6408_gpio_set_value,
};

static const struct udevice_id fxl6408_gpio_ids[] = {
	{ .compatible = "fcs,fxl6408" },
	{ }
};

U_BOOT_DRIVER(fxl6408_driver) = {
	.name	= "fxl6408_gpio",
	.id	= UCLASS_GPIO,
	.ops	= &fxl6408_gpio_ops,
	.of_match = fxl6408_gpio_ids,
	.probe	= fxl6408_gpio_probe,
};
