// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2017 Microchip
 *		      Wenyou Yang <wenyou.yang@microchip.com>
 */

#include <common.h>
#include <dm.h>
#include <eeprom.h>
#include <env.h>
#include <i2c_eeprom.h>
#include <i2c.h>
#include <malloc.h>
#include <net.h>
#include <vsprintf.h>
#include <asm/io.h>
#include <hash.h>
#include <u-boot/crc.h>
#include <u-boot/sha256.h>

#define CHIPID_LEN	32
#define CHIPID_ADDR	0x1045114C

#if (IS_ENABLED(CONFIG_I2C_EEPROM))

#define SIGNATURE_LEN	8
#define VERSION_LEN		8
#define RESERVED_LEN	8
#define SERIAL_LEN		24
#define MAC_ADDR_LEN	12

#define SIGNATURE_OFFSET	0
#define VERSION_OFFSET		8
#define RESERVED_OFFSET		16
#define SERIAL_OFFSET		24
#define ETHADDR_OFFSET		48
#define ETH1ADDR_OFFSET		60

struct bpi_boardinfo {
	unsigned char signature[SIGNATURE_LEN];
	unsigned char version[VERSION_LEN];
	unsigned char reserved[RESERVED_LEN];
	unsigned char serial[SERIAL_LEN];
	unsigned char ethaddr[MAC_ADDR_LEN];
	unsigned char eth1addr[MAC_ADDR_LEN];
} __attribute__ ((__packed__));

int bpi_get_boardinfo(void)
{
	struct bpi_boardinfo *info;
	struct udevice *dev;
	const char signature[SIGNATURE_LEN] = "Bananapi";
	unsigned char signature_buf[SIGNATURE_LEN + 1] = {0};
	unsigned char version_buf[VERSION_LEN + 1] = {0};
	unsigned char reserved_buf[RESERVED_LEN + 1] = {0};
	unsigned char serial_buf[SERIAL_LEN + 1] = {0};
	unsigned char ethaddr_buf[MAC_ADDR_LEN + 1] = {0};
	unsigned char eth1addr_buf[MAC_ADDR_LEN + 1] = {0};
	u8 ethaddr_str[6];
	u8 eth1addr_str[6];
	int ret;
	int node;

	node = fdt_path_offset(gd->fdt_blob, "eeprom0");
	if (node < 0) {
		printf("%s: No eeprom0 path offset\n", __func__);
		return node;
	}

	ret = uclass_get_device_by_of_offset(UCLASS_I2C_EEPROM, node, &dev);
	if (ret) {
		printf("%s: Could not find EEPROM\n", __func__);
		return ret;
	}

	ret = i2c_set_chip_offset_len(dev, 1);
	if (ret)
		return ret;

	info = malloc(sizeof(struct bpi_boardinfo));
	if (!info)
		return -ENOMEM;

	ret = i2c_eeprom_read(dev, 0, (uint8_t *)info, sizeof(struct bpi_boardinfo));
	if (ret) {
		printf("%s: i2c_eeprom_read() failed: %d\n", __func__, ret);
		free(info);
		return ret;
	}

	memcpy(signature_buf, info->signature, SIGNATURE_LEN);
	memcpy(version_buf, info->version, VERSION_LEN);
	memcpy(reserved_buf, info->reserved, RESERVED_LEN);
	memcpy(serial_buf, info->serial, SERIAL_LEN);
	memcpy(ethaddr_buf, info->ethaddr, MAC_ADDR_LEN);
	memcpy(eth1addr_buf, info->eth1addr, MAC_ADDR_LEN);

	/* Check EEPROM signature. */
	if (memcmp(info->signature, signature, SIGNATURE_LEN)) {
		printf("Invalid I2C EEPROM signature.\n");
		free(info);
		return -EIO;
	}

	printf("signature: %s\n",  signature_buf);
	printf("version: %s\n",  version_buf);
	printf("reserved: %s\n",  reserved_buf);
	printf("serial: %s\n",  serial_buf);
	printf("ethaddr: %s\n",  ethaddr_buf);
	printf("eth1addr: %s\n",  eth1addr_buf);

	for (int i = 0; i < 6; i++) {
		sscanf(&ethaddr_buf[i * 2], "%2hhx", &ethaddr_str[i]);
		sscanf(&eth1addr_buf[i * 2], "%2hhx", &eth1addr_str[i]);
	}

	env_set("serial", serial_buf);
	if (is_valid_ethaddr(ethaddr_str))
		eth_env_set_enetaddr("ethaddr", ethaddr_str);
	if (is_valid_ethaddr(eth1addr_str))
		eth_env_set_enetaddr("eth1addr", eth1addr_str);

	free(info);

	return 0;
}
#endif

int bpi_set_boardinfo(void)
{
	char *chipid = NULL;
	u32 soc_chipid[4];
	u8 hash[SHA256_SUM_LEN];
	int size = sizeof(hash);
	u8 eth_addr[6];
	int ret;

	chipid = malloc(CHIPID_LEN + 1);
	if (!chipid)
		return -1;

	soc_chipid[0] = readl(CHIPID_ADDR);
	soc_chipid[1] = readl(CHIPID_ADDR + 0x4);
	soc_chipid[2] = readl(CHIPID_ADDR + 0x8);
	soc_chipid[3] = readl(CHIPID_ADDR + 0xc);

	sprintf(chipid, "%08x%08x%08x%08x", soc_chipid[0], soc_chipid[1], soc_chipid[2], soc_chipid[3] );
	env_set("chipid", chipid);
	env_set("serial", chipid);
	printf("chipid: %s\n", chipid);

	if (env_get("ethaddr"))
		return 0;

	if (!chipid) {
		printf("%s: could not retrieve chipid'\n", __func__);
		return -1;
	}

	ret = hash_block("sha256", (void *)chipid, strlen(chipid), hash, &size);
	if (ret) {
		printf("%s: failed to calculate SHA256\n", __func__);
		return -1;
	}

	/* Copy 6 bytes of the hash to base the MAC address on */
	memcpy(eth_addr, hash, 6);
	eth_addr[0] &= 0xfe;
	eth_addr[0] |= 0x02;

	if (is_valid_ethaddr(eth_addr)) {
		eth_env_set_enetaddr("ethaddr", eth_addr);

		eth_addr[5]++;
		if (is_valid_ethaddr(eth_addr))
			eth_env_set_enetaddr("eth1addr", eth_addr);
	} else {
		printf("invalid mac address(%pM) from chipid\n", eth_addr);
	}

	free(chipid);

	return 0;
}

