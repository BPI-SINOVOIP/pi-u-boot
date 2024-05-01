// SPDX-License-Identifier: GPL-2.0+

#include <env.h>
#include <i2c.h>
#include <asm/io.h>
#include <common.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

#define MUX_MODE0	0					/* func 0 */
#define MUX_MODE1	BIT(0)				/* func 1 */
#define MUX_MODE2	BIT(1)				/* func 2 */
#define MUX_MODE3	BIT(0) | BIT(1)		/* func 3 */
#define MUX_MODE4	BIT(2)				/* func 4 */
#define MUX_MODE5	BIT(0) | BIT(2)		/* func 5 */
#define EDGE_NONE	BIT(6)				/* edge-detection is unabled */
#define PAD_1V8_DS2	BIT(12)				/* voltage:1.8v, driver strength: 2 */
#define PULL_UP		BIT(14) | BIT(15)	/* pull-up */

#define I2C_PIN_CONFIG(x)	((x) | EDGE_NONE | PULL_UP | PAD_1V8_DS2)

char *spacemit_i2c_eeprom[] = {
	"atmel,24c02",
};

struct tlv_eeprom {
	uint8_t type;
	uint8_t length;
};

struct eeprom_config {
	uint8_t bus;
	uint16_t addr;
	uint8_t pin_function;
	uint32_t scl_pin_reg;
	uint32_t sda_pin_reg;
};

const struct eeprom_config eeprom_info[] = {
	// eeprom @deb1 & deb2: I2C2, pin group(GPIO_84, GPIO_85)
	{2, 0x50, MUX_MODE4, 0xd401e154, 0xd401e158},
	// eeprom @evb: I2C6, pin group(GPIO_118, GPIO_119)
	{6, 0x50, MUX_MODE2, 0xd401e228, 0xd401e22c},
};

int spacemit_eeprom_read(uint8_t chip, uint8_t *buffer, uint8_t id)
{
	struct tlv_eeprom tlv;
	int ret;
	uint8_t buf[1] = {0};
	uint8_t len[1] = {0};
	uint16_t i = 0;
	uint8_t j;

	tlv.type = 0;
	tlv.length = 0;

	for (i = 11; i <= 256; i = i + tlv.length + 2) {
		ret = i2c_read(chip, i, 1, buf, 1);
		tlv.type = *buf;

		ret = i2c_read(chip, i + 1, 1, len, 1);
		tlv.length = *len;

		if (tlv.length == 0) {
			pr_err("Error: wrong tlv length\n");
			return -1;
		}

		if (tlv.type == id) {
			for(j = 0; j < tlv.length; j++) {
				ret = i2c_read(chip, i + 2 + j, 1, (char *)buffer, 1);
				buffer++;
			}
			return 0;
		}
	}

	pr_info("No 0x%x tlv type in eeprom\n", id);
	return -2;
}

static void i2c_set_pinctrl(uint32_t value, uint32_t reg_addr)
{
	writel(value, (void __iomem *)(size_t)reg_addr);
}

static uint32_t i2c_get_pinctrl(uint32_t reg_addr)
{
	return readl((void __iomem *)(size_t)reg_addr);
}

int k1x_eeprom_init(void)
{
	static int saddr = -1, i;
	uint8_t bus;
	uint32_t scl_pin_backup, sda_pin_backup;

	if (saddr >= 0)
		return saddr;

	for (i = 0; i < ARRAY_SIZE(eeprom_info); i++) {
		bus = eeprom_info[i].bus;
		saddr = eeprom_info[i].addr;

		scl_pin_backup = i2c_get_pinctrl(eeprom_info[i].scl_pin_reg);;
		sda_pin_backup = i2c_get_pinctrl(eeprom_info[i].sda_pin_reg);;
		i2c_set_pinctrl(I2C_PIN_CONFIG(eeprom_info[i].pin_function), eeprom_info[i].scl_pin_reg);
		i2c_set_pinctrl(I2C_PIN_CONFIG(eeprom_info[i].pin_function), eeprom_info[i].sda_pin_reg);

		if ((i2c_set_bus_num(bus) < 0) || (i2c_probe(saddr) < 0)) {
			pr_err("%s: probe i2c(%d) @eeprom %d failed\n", __func__, bus, saddr);
			i2c_set_pinctrl(scl_pin_backup, eeprom_info[i].scl_pin_reg);
			i2c_set_pinctrl(sda_pin_backup, eeprom_info[i].sda_pin_reg);
		}
		else {
			pr_info("find eeprom in bus %d, address %d\n", bus, saddr);
			return saddr;
		}
	}

	return -EINVAL;
}
