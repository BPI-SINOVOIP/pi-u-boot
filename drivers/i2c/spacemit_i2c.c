// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 Spacemit
 */

#include <common.h>
#include <dm.h>
#include <reset.h>
#include <clk.h>
#include <i2c.h>
#include <asm/io.h>
#include <linux/delay.h>
#include "spacemit_i2c.h"

/* All transfers are described by this data structure */
struct spacemit_i2c_msg {
	u8 condition;
	u8 acknack;
	u8 direction;
	u8 data;
};

struct spacemit_i2c {
	u32 icr;
	u32 isr;
	u32 isar;
	u32 idbr;
	u32 ilcr;
	u32 iwcr;
	u32 irst_cyc;
	u32 ibmr;
};

/*
 * i2c_reset: - reset the host controller
 *
 */
static void i2c_reset(struct spacemit_i2c *base)
{
	u32 icr_mode;

	/* Save bus mode (standard or fast speed) for later use */
	icr_mode = readl(&base->icr) & ICR_MODE_MASK;
	writel(readl(&base->icr) & ~ICR_IUE, &base->icr); /* disable unit */
	writel(readl(&base->icr) | ICR_UR, &base->icr);	  /* reset the unit */
	udelay(100);
	writel(readl(&base->icr) & ~ICR_IUE, &base->icr); /* disable unit */

#ifdef CONFIG_SYS_I2C_SLAVE
	writel(CONFIG_SYS_I2C_SLAVE, &base->isar); /* set our slave address */
#else
	writel(0x00, &base->isar); /* set our slave address */
#endif
	/* set control reg values */
	writel(I2C_ICR_INIT | icr_mode, &base->icr);
	writel(I2C_ISR_INIT, &base->isr); /* set clear interrupt bits */
	writel(readl(&base->icr) | ICR_IUE, &base->icr); /* enable unit */
	udelay(1e0);
}

/*
 * i2c_isr_set_cleared: - wait until certain bits of the I2C status register
 *	                  are set and cleared
 *
 * @return: 1 in case of success, 0 means timeout (no match within 10 ms).
 */
static int i2c_isr_set_cleared(struct spacemit_i2c *base, unsigned long set_mask,
			       unsigned long cleared_mask)
{
	int timeout = 1000, isr;

	do {
		isr = readl(&base->isr);
		udelay(10);
		if (timeout-- < 0)
			return 0;
	} while (((isr & set_mask) != set_mask)
		|| ((isr & cleared_mask) != 0));

	return 1;
}

/*
 * i2c_transfer: - Transfer one byte over the i2c bus
 *
 * This function can tranfer a byte over the i2c bus in both directions.
 * It is used by the public API functions.
 *
 * @return:  0: transfer successful
 *          -1: message is empty
 *          -2: transmit timeout
 *          -3: ACK missing
 *          -4: receive timeout
 *          -5: illegal parameters
 *          -6: bus is busy and couldn't be aquired
 */
static int i2c_transfer(struct spacemit_i2c *base, struct spacemit_i2c_msg *msg)
{
	int ret;

	if (!msg)
		goto transfer_error_msg_empty;

	switch (msg->direction) {
	case I2C_WRITE:
		/* check if bus is not busy */
		if (!i2c_isr_set_cleared(base, 0, ISR_IBB))
			goto transfer_error_bus_busy;

		/* start transmission */
		writel(readl(&base->icr) & ~ICR_START, &base->icr);
		writel(readl(&base->icr) & ~ICR_STOP, &base->icr);
		writel(msg->data, &base->idbr);
		if (msg->condition == I2C_COND_START)
			writel(readl(&base->icr) | ICR_START, &base->icr);
		if (msg->condition == I2C_COND_STOP)
			writel(readl(&base->icr) | ICR_STOP, &base->icr);
		if (msg->acknack == I2C_ACKNAK_SENDNAK)
			writel(readl(&base->icr) | ICR_ACKNAK, &base->icr);
		if (msg->acknack == I2C_ACKNAK_SENDACK)
			writel(readl(&base->icr) & ~ICR_ACKNAK, &base->icr);
		writel(readl(&base->icr) & ~ICR_ALDIE, &base->icr);
		writel(readl(&base->icr) | ICR_TB, &base->icr);

		/* transmit register empty? */
		if (!i2c_isr_set_cleared(base, ISR_ITE, 0))
			goto transfer_error_transmit_timeout;

		/* clear 'transmit empty' state */
		writel(readl(&base->isr) | ISR_ITE, &base->isr);

		/* wait for ACK from slave */
		if (msg->acknack == I2C_ACKNAK_WAITACK)
			if (!i2c_isr_set_cleared(base, 0, ISR_ACKNAK))
				goto transfer_error_ack_missing;
		break;

	case I2C_READ:

		/* check if bus is not busy */
		if (!i2c_isr_set_cleared(base, 0, ISR_IBB))
			goto transfer_error_bus_busy;

		/* start receive */
		writel(readl(&base->icr) & ~ICR_START, &base->icr);
		writel(readl(&base->icr) & ~ICR_STOP, &base->icr);
		if (msg->condition == I2C_COND_START)
			writel(readl(&base->icr) | ICR_START, &base->icr);
		if (msg->condition == I2C_COND_STOP)
			writel(readl(&base->icr) | ICR_STOP, &base->icr);
		if (msg->acknack == I2C_ACKNAK_SENDNAK)
			writel(readl(&base->icr) | ICR_ACKNAK, &base->icr);
		if (msg->acknack == I2C_ACKNAK_SENDACK)
			writel(readl(&base->icr) & ~ICR_ACKNAK, &base->icr);
		writel(readl(&base->icr) & ~ICR_ALDIE, &base->icr);
		writel(readl(&base->icr) | ICR_TB, &base->icr);

		/* receive register full? */
		if (!i2c_isr_set_cleared(base, ISR_IRF, 0))
			goto transfer_error_receive_timeout;

		msg->data = readl(&base->idbr);

		/* clear 'receive empty' state */
		writel(readl(&base->isr) | ISR_IRF, &base->isr);
		break;
	default:
		goto transfer_error_illegal_param;
	}

	return 0;

transfer_error_msg_empty:
	debug("i2c_transfer: error: 'msg' is empty\n");
	ret = -1;
	goto i2c_transfer_finish;

transfer_error_transmit_timeout:
	debug("i2c_transfer: error: transmit timeout\n");
	ret = -2;
	goto i2c_transfer_finish;

transfer_error_ack_missing:
	debug("i2c_transfer: error: ACK missing\n");
	ret = -3;
	goto i2c_transfer_finish;

transfer_error_receive_timeout:
	debug("i2c_transfer: error: receive timeout\n");
	ret = -4;
	goto i2c_transfer_finish;

transfer_error_illegal_param:
	debug("i2c_transfer: error: illegal parameters\n");
	ret = -5;
	goto i2c_transfer_finish;

transfer_error_bus_busy:
	debug("i2c_transfer: error: bus is busy\n");
	ret = -6;
	goto i2c_transfer_finish;

i2c_transfer_finish:
	debug("i2c_transfer: ISR: 0x%04x\n", readl(&base->isr));
	i2c_reset(base);
	return ret;
}

static int __i2c_read(struct spacemit_i2c *base, uchar chip, u8 *addr, int alen,
		      uchar *buffer, int len)
{
	struct spacemit_i2c_msg msg;

	debug("i2c_read(chip=0x%02x, addr=0x%02x, alen=0x%02x, "
	      "len=0x%02x)\n", chip, *addr, alen, len);

	if (len == 0) {
		pr_err("reading zero byte is invalid\n");
		return -EINVAL;
	}

	i2c_reset(base);

	/* dummy chip address write */
	debug("i2c_read: dummy chip address write\n");
	msg.condition = I2C_COND_START;
	msg.acknack   = I2C_ACKNAK_WAITACK;
	msg.direction = I2C_WRITE;
	msg.data = (chip << 1);
	msg.data &= 0xFE;
	if (i2c_transfer(base, &msg))
		return -1;

	/*
	 * send memory address bytes;
	 * alen defines how much bytes we have to send.
	 */
	while (--alen >= 0) {
		debug("i2c_read: send address byte %02x (alen=%d)\n",
		      *addr, alen);
		msg.condition = I2C_COND_NORMAL;
		msg.acknack   = I2C_ACKNAK_WAITACK;
		msg.direction = I2C_WRITE;
		msg.data      = addr[alen];
		if (i2c_transfer(base, &msg))
			return -1;
	}

	/* start read sequence */
	debug("i2c_read: start read sequence\n");
	msg.condition = I2C_COND_START;
	msg.acknack   = I2C_ACKNAK_WAITACK;
	msg.direction = I2C_WRITE;
	msg.data      = (chip << 1);
	msg.data     |= 0x01;
	if (i2c_transfer(base, &msg))
		return -1;

	/* read bytes; send NACK at last byte */
	while (len--) {
		if (len == 0) {
			msg.condition = I2C_COND_STOP;
			msg.acknack   = I2C_ACKNAK_SENDNAK;
		} else {
			msg.condition = I2C_COND_NORMAL;
			msg.acknack   = I2C_ACKNAK_SENDACK;
		}

		msg.direction = I2C_READ;
		msg.data      = 0x00;
		if (i2c_transfer(base, &msg))
			return -1;

		*buffer = msg.data;
		debug("i2c_read: reading byte (%p)=0x%02x\n",
		      buffer, *buffer);
		buffer++;
	}

	i2c_reset(base);

	return 0;
}

static int __i2c_write(struct spacemit_i2c *base, uchar chip, u8 *addr, int alen,
		       uchar *buffer, int len)
{
	struct spacemit_i2c_msg msg;

	debug("i2c_write(chip=0x%02x, addr=0x%02x, alen=0x%02x, "
	      "len=0x%02x)\n", chip, *addr, alen, len);

	i2c_reset(base);

	/* chip address write */
	debug("i2c_write: chip address write\n");
	msg.condition = I2C_COND_START;
	msg.acknack   = I2C_ACKNAK_WAITACK;
	msg.direction = I2C_WRITE;
	msg.data = (chip << 1);
	msg.data &= 0xFE;
	if (i2c_transfer(base, &msg))
		return -1;

	/*
	 * send memory address bytes;
	 * alen defines how much bytes we have to send.
	 */
	while (--alen >= 0) {
		debug("i2c_read: send address byte %02x (alen=%d)\n",
		      *addr, alen);
		msg.condition = I2C_COND_NORMAL;
		msg.acknack   = I2C_ACKNAK_WAITACK;
		msg.direction = I2C_WRITE;
		msg.data      = addr[alen];
		if (i2c_transfer(base, &msg))
			return -1;
	}

	/* write bytes; send NACK at last byte */
	while (len--) {
		debug("i2c_write: writing byte (%p)=0x%02x\n",
		      buffer, *buffer);

		if (len == 0)
			msg.condition = I2C_COND_STOP;
		else
			msg.condition = I2C_COND_NORMAL;

		msg.acknack   = I2C_ACKNAK_WAITACK;
		msg.direction = I2C_WRITE;
		msg.data      = *(buffer++);

		if (i2c_transfer(base, &msg))
			return -1;
	}

	i2c_reset(base);

	return 0;
}

#if CONFIG_IS_ENABLED(SYS_I2C_LEGACY)

#define SPACEMIT_APBC_BASE		0xd4015000	/* APB Clock Unit */
#define REG_APBC_APBC_TWSI0_CLK_RST	(SPACEMIT_APBC_BASE + 0x2c)
#define REG_APBC_APBC_TWSI1_CLK_RST	(SPACEMIT_APBC_BASE + 0x30)
#define REG_APBC_APBC_TWSI2_CLK_RST	(SPACEMIT_APBC_BASE + 0x38)
#define REG_APBC_APBC_TWSI3_CLK_RST	(0xf0610000    + 0x08)
#define REG_APBC_APBC_TWSI4_CLK_RST	(SPACEMIT_APBC_BASE + 0x40)
#define REG_APBC_APBC_TWSI5_CLK_RST	(SPACEMIT_APBC_BASE + 0x4c)
#define REG_APBC_APBC_TWSI6_CLK_RST	(SPACEMIT_APBC_BASE + 0x60)
#define REG_APBC_APBC_TWSI7_CLK_RST	(SPACEMIT_APBC_BASE + 0x68)
#define REG_APBC_APBC_TWSI8_CLK_RST	(SPACEMIT_APBC_BASE + 0x20)

typedef enum {
	I2C_FUNCLK_33MHz = 0,	/*up to 3.4M bps for HS */
	I2C_FUNCLK_52MHz = 1,
	I2C_FUNCLK_62P4MHz = 2,	/*up to 1.8M bps for HS */
} I2C_FUNCTION_CLK;

static struct spacemit_i2c *i2c_base[] = {
	(struct spacemit_i2c *)0xd4010800,
	(struct spacemit_i2c *)0xd4011000,
	(struct spacemit_i2c *)0xd4012000,
	(struct spacemit_i2c *)0xf0614000,
	(struct spacemit_i2c *)0xd4012800,
	(struct spacemit_i2c *)0xd4013800,
	(struct spacemit_i2c *)0xd4018800,
	(struct spacemit_i2c *)0xd401d000,
	(struct spacemit_i2c *)0xd401d800,
};

static uint32_t apbc_clk_reg[] = {
	REG_APBC_APBC_TWSI0_CLK_RST,
	REG_APBC_APBC_TWSI1_CLK_RST,
	REG_APBC_APBC_TWSI2_CLK_RST,
	REG_APBC_APBC_TWSI3_CLK_RST,
	REG_APBC_APBC_TWSI4_CLK_RST,
	REG_APBC_APBC_TWSI5_CLK_RST,
	REG_APBC_APBC_TWSI6_CLK_RST,
	REG_APBC_APBC_TWSI7_CLK_RST,
	REG_APBC_APBC_TWSI8_CLK_RST,
};

static inline void mmio_write_32(uintptr_t addr, uint32_t val)
{
	*(volatile uint32_t *)addr = val;
}
static inline uint32_t mmio_read_32(uintptr_t addr)
{
	return *(volatile uint32_t *)addr;
}

void i2c_init_board(void)
{
	int i = 0;

	mmio_write_32(0xd4051024, (*(unsigned int *)0xd4051024) | (1 << 6));
	mmio_write_32(0xd4090104, (*(unsigned int *)0xd4090104) | (1 << 4));
	mmio_write_32(0xd4090108, (*(unsigned int *)0xd4090108) | (1 << 31));

	mmio_write_32(0xd401e228, (*(unsigned int *)0xd401e228) | (1 << 1));
        mmio_write_32(0xd401e22c, (*(unsigned int *)0xd401e22c) | (1 << 1));
	/* init the clk & reset or pinctrl */
	for (i = 0; i < sizeof(apbc_clk_reg) / sizeof(apbc_clk_reg[0]); ++i) {
		mmio_write_32(apbc_clk_reg[i], (I2C_FUNCLK_33MHz << 4) | 0x4);
		mmio_write_32(apbc_clk_reg[i], (I2C_FUNCLK_33MHz << 4) | 0x7);
		mmio_write_32(apbc_clk_reg[i], (I2C_FUNCLK_33MHz << 4) | 0x3);
	}
}

static void __i2c_init_chip(struct spacemit_i2c *base, int speed, int slaveaddr)
{
	u32 val;

	if (speed > 100000)
		val = ICR_FM;
	else
		val = ICR_SM;

	clrsetbits_le32(&base->icr, ICR_MODE_MASK, val);
}

static int __i2c_probe_chip(struct spacemit_i2c *base, uchar chip)
{
	struct spacemit_i2c_msg msg;

	i2c_reset(base);

	msg.condition = I2C_COND_START;
	msg.acknack   = I2C_ACKNAK_WAITACK;
	msg.direction = I2C_WRITE;
	msg.data      = (chip << 1) + 1;
	if (i2c_transfer(base, &msg))
		return -1;

	msg.condition = I2C_COND_STOP;
	msg.acknack   = I2C_ACKNAK_SENDNAK;
	msg.direction = I2C_READ;
	msg.data      = 0x00;
	if (i2c_transfer(base, &msg))
		return -1;

	return 0;
}

static void spacemit_i2c_init(struct i2c_adapter *adap, int speed, int slaveadd)
{
	__i2c_init_chip(i2c_base[adap->hwadapnr], speed, slaveadd);
}

/*
 * spacemit_i2c_probe: - Test if a chip answers for a given i2c address
 */
static int spacemit_i2c_probe(struct i2c_adapter *adap, uchar chip)
{
	return __i2c_probe_chip(i2c_base[adap->hwadapnr], chip);
}

/*
 * i2c_read: - Read multiple bytes from an i2c device
 *
 * The higher level routines take into account that this function is only
 * called with len < page length of the device (see configuration file)
 *
 * @chip:      address of the chip which is to be read
 * @addr:      i2c data address within the chip
 * @alen:      length of the i2c data address (1..2 bytes)
 * @buffer:    where to write the data
 * @len:       how much byte do we want to read
 * @return:    0 in case of success
 */
static int spacemit_i2c_read(struct i2c_adapter *adap, uchar chip, uint addr, int alen, uchar *buffer, int len)
{
	u8 addr_bytes[4];

	addr_bytes[0] = (addr >> 0) & 0xFF;
	addr_bytes[1] = (addr >> 8) & 0xFF;
	addr_bytes[2] = (addr >> 16) & 0xFF;
	addr_bytes[3] = (addr >> 24) & 0xFF;

	return __i2c_read(i2c_base[adap->hwadapnr], chip, addr_bytes, alen, buffer, len);
}

/*
 * spacemit_i2c_write: -  Write multiple bytes to an i2c device
 *
 * The higher level routines take into account that this function is only
 * called with len < page length of the device (see configuration file)
 *
 * @chip:	address of the chip which is to be written
 * @addr:	i2c data address within the chip
 * @alen:	length of the i2c data address (1..2 bytes)
 * @buffer:	where to find the data to be written
 * @len:	how much byte do we want to read
 * @return:	0 in case of success
 */
static int spacemit_i2c_write(struct i2c_adapter *adap, uchar chip, uint addr, int alen, uchar *buffer, int len)
{
	u8 addr_bytes[4];

	addr_bytes[0] = (addr >> 0) & 0xFF;
	addr_bytes[1] = (addr >> 8) & 0xFF;
	addr_bytes[2] = (addr >> 16) & 0xFF;
	addr_bytes[3] = (addr >> 24) & 0xFF;

	return __i2c_write(i2c_base[adap->hwadapnr], chip, addr_bytes, alen, buffer, len);
}

U_BOOT_I2C_ADAP_COMPLETE(spacemit_i2c0, spacemit_i2c_init, spacemit_i2c_probe,
		spacemit_i2c_read, spacemit_i2c_write,
		NULL, 100000, 0x31,
		0);
U_BOOT_I2C_ADAP_COMPLETE(spacemit_i2c1, spacemit_i2c_init, spacemit_i2c_probe,
		spacemit_i2c_read, spacemit_i2c_write,
		NULL, 100000, 0x31,
		1);
U_BOOT_I2C_ADAP_COMPLETE(spacemit_i2c2, spacemit_i2c_init, spacemit_i2c_probe,
		spacemit_i2c_read, spacemit_i2c_write,
		NULL, 100000, 0x31,
		2);
U_BOOT_I2C_ADAP_COMPLETE(spacemit_i2c3, spacemit_i2c_init, spacemit_i2c_probe,
		spacemit_i2c_read, spacemit_i2c_write,
		NULL, 100000, 0x31,
		3);
U_BOOT_I2C_ADAP_COMPLETE(spacemit_i2c4, spacemit_i2c_init, spacemit_i2c_probe,
		spacemit_i2c_read, spacemit_i2c_write,
		NULL, 100000, 0x31,
		4);
U_BOOT_I2C_ADAP_COMPLETE(spacemit_i2c5, spacemit_i2c_init, spacemit_i2c_probe,
		spacemit_i2c_read, spacemit_i2c_write,
		NULL, 100000, 0x31,
		5);
U_BOOT_I2C_ADAP_COMPLETE(spacemit_i2c6, spacemit_i2c_init, spacemit_i2c_probe,
		spacemit_i2c_read, spacemit_i2c_write,
		NULL, 100000, 0x50,
		6);
U_BOOT_I2C_ADAP_COMPLETE(spacemit_i2c7, spacemit_i2c_init, spacemit_i2c_probe,
		spacemit_i2c_read, spacemit_i2c_write,
		NULL, 100000, 0x31,
		7);
U_BOOT_I2C_ADAP_COMPLETE(spacemit_i2c8, spacemit_i2c_init, spacemit_i2c_probe,
		spacemit_i2c_read, spacemit_i2c_write,
		NULL, 100000, 0x31,
		8);
#else /* SYS_I2C_LEGACY */

struct spacemit_i2c_priv {
	struct spacemit_i2c *base;
	struct reset_ctl_bulk resets;
#if CONFIG_IS_ENABLED(CLK)
        struct clk clk;
#endif
	u32 clk_rate;

};

static int spacemit_i2c_xfer(struct udevice *bus, struct i2c_msg *msg, int nmsgs)
{
	struct spacemit_i2c_priv *i2c = dev_get_priv(bus);
	struct i2c_msg *dmsg, *omsg, dummy;

	memset(&dummy, 0, sizeof(struct i2c_msg));

	/*
	 * We expect either two messages (one with an offset and one with the
	 * actual data) or one message (just data or offset/data combined)
	 */
	if (nmsgs > 2 || nmsgs == 0) {
		debug("%s: Only one or two messages are supported.", __func__);
		return -1;
	}

	omsg = nmsgs == 1 ? &dummy : msg;
	dmsg = nmsgs == 1 ? msg : msg + 1;

	if (dmsg->flags & I2C_M_RD)
		return __i2c_read(i2c->base, dmsg->addr, omsg->buf,
				  omsg->len, dmsg->buf, dmsg->len);
	else
		return __i2c_write(i2c->base, dmsg->addr, omsg->buf,
				   omsg->len, dmsg->buf, dmsg->len);
}

static int spacemit_i2c_set_bus_speed(struct udevice *bus, unsigned int speed)
{
	struct spacemit_i2c_priv *priv = dev_get_priv(bus);
	u32 val;

	if (speed > 100000)
		val = ICR_FM;
	else
		val = ICR_SM;
	clrsetbits_le32(&priv->base->icr, ICR_MODE_MASK, val);

	return 0;
}

static int spacemit_i2c_bind(struct udevice *bus)
{
	return 0;
}

static int spacemit_i2c_probe(struct udevice *bus)
{
	struct spacemit_i2c_priv *priv = dev_get_priv(bus);
	int ret;

	ret = reset_get_bulk(bus, &priv->resets);
        ret = reset_deassert_bulk(&priv->resets);
        if (ret){
                debug("I2C probe: failed to reset \n");
                return ret;
        }

#if CONFIG_IS_ENABLED(CLK)
        ret = clk_get_by_name(bus, NULL, &priv->clk);
        if (ret)
                return ret;

        ret = clk_enable(&priv->clk);
        if (ret && ret != -ENOSYS && ret != -ENOTSUPP) {
                clk_free(&priv->clk);
                debug("I2C probe: failed to enable clock\n");
                return ret;
        }
#endif
	priv->base = (void *)devfdt_get_addr_ptr(bus);
	ret = dev_read_u32(bus, "clock-frequency", &priv->clk_rate);
        if (ret) {
                pr_info("Default to 100kHz\n");
		/* default clock rate: 100k */
                priv->clk_rate = 100000;
        }

	ret = spacemit_i2c_set_bus_speed(bus, priv->clk_rate);
	return 0;
}

static const struct dm_i2c_ops spacemit_i2c_ops = {
	.xfer		= spacemit_i2c_xfer,
	.set_bus_speed	= spacemit_i2c_set_bus_speed,
};

static const struct udevice_id spacemit_i2c_ids[] = {
	{ .compatible = "spacemit,i2c" },
	{ }
};

U_BOOT_DRIVER(i2c_spacemit) = {
	.name	= "i2c_spacemit",
	.id	= UCLASS_I2C,
	.of_match = spacemit_i2c_ids,
	.bind	= spacemit_i2c_bind,
	.probe	= spacemit_i2c_probe,
	.priv_auto = sizeof(struct spacemit_i2c_priv),
	.ops	= &spacemit_i2c_ops,
};
#endif /* CONFIG_DM_I2C */
