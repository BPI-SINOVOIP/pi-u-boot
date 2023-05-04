/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2019 Synaptics Incorporated
 * Author: Shaojun Feng <Shaojun.feng@synaptics.com>
 *
 * Copyright (C) 2016 Marvell Technology Group Ltd.
 * Author: Xiaoming Lu <xmlu@marvell.com>
 */
#include <common.h>
#include <dm.h>
#include <spi.h>
#include <asm/io.h>
#include <clk.h>

#undef SPI_DEBUG

#ifdef SPI_DEBUG
#define spi_debug(fmt, args...)	printf(fmt, ##args)
#else
#define spi_debug(fmt, args...)
#endif /* SPI_DEBUG */

DECLARE_GLOBAL_DATA_PTR;

#define POLLING_MODE

struct syna_spi_regs {
	unsigned int ctrl0;
	unsigned int ctrl1;
	unsigned int ssienr;
	unsigned int mwcr;
	unsigned int ser;
	unsigned int baud;
	unsigned int txftlr;
	unsigned int rxftlr;
	unsigned int txflr;
	unsigned int rxflr;
	unsigned int sr;
	unsigned int imr;
	unsigned int isr;
	unsigned int risr;
	unsigned int txoicr;
	unsigned int rxoicr;
	unsigned int rxuicr;
	unsigned int msticr;
	unsigned int icr;
	unsigned int dmacr;
	unsigned int dmatdlr;
	unsigned int dmardlr;
	unsigned int idr;
	unsigned int _reserved;
	unsigned int dr;
};

struct syna_spi_platdata {
	struct syna_spi_regs *reg;
	unsigned int ref_clock_hz;
	unsigned int max_write_size;
	unsigned char wbuffer[260];
	int wnum;
	int rnum;
	volatile int tx_done;
	volatile int rx_done;
};

struct syna_spi_priv {
	struct syna_spi_regs *reg;
};

/* SSI system clock */
/* SPI output clock can be calculated as: GALOIS_SPI_SYS_CLOCK/SPI_REG_BAUD */
#define GALOIS_SPI_SYS_CLOCK	100000000


/* SPI master TX/RX FIFO size */
// #define SPI_TX_FIFO_SIZE	128
// #define SPI_RX_FIFO_SIZE	128
// #define SPI_TX_FIFO_TH		(SPI_TX_FIFO_SIZE/2)
// #define SPI_RX_FIFO_TH		(SPI_RX_FIFO_SIZE/2)
#define SPI_TX_MAX_DATA_LEN		CONFIG_ENV_SECT_SIZE
#define SPI_RX_MAX_DATA_LEN		CONFIG_ENV_SECT_SIZE
#define MAX_SPI_CMD_LEN 		10

/* port register accessors **************************************************/
static inline u32 bsrl(void * addr)
{
	u32 data = readl(addr);
	spi_debug("syna spi read: addr = %p, data = %x\n", addr, data);
	return data;
}

static inline void bswl(u32 data, void * addr)
{
	spi_debug("syna spi write: addr = %p, data = %x\n", addr, data);
	writel(data, addr);
}

static void set_ssi_protocol(struct syna_spi_platdata *plat)
{
	struct syna_spi_regs *reg = plat->reg;
	int value;
	value = bsrl(&reg->ctrl0);
	value &= 0xffcf;	/* protocol = 0: Motorola SPI */
	bswl(value, &reg->ctrl0);
}

static void set_transfer_mode(struct syna_spi_platdata *plat, int mode)
{
	struct syna_spi_regs *reg = plat->reg;
	int value;
	value = bsrl(&reg->ctrl0);
	value &= 0xfcff;
	value |= mode << 8;
	bswl(value, &reg->ctrl0);
}

static void syna_spi_set_clock_mode(struct syna_spi_regs *reg, int mode)
{
	int value;

	/* check status first */
	while(bsrl(&reg->sr) & 0x01);

	/* config SPI master clock mode */
	value = bsrl(&reg->ctrl0);
	value &= 0xff3f;
	value |= (mode<<6);
	bswl(value, &reg->ctrl0);
}

static void syna_spi_set_framewidth(struct syna_spi_regs *reg, int width)
{
	int value;

	/* check status first */
	while(bsrl(&reg->sr) & 0x01);

	/* config SPI master data frame width */
	value = bsrl(&reg->ctrl0);
	value &= 0xfff0;
	value |= width-1;
	bswl(value, &reg->ctrl0);
}

static int syna_spi_claim_bus(struct udevice *dev)
{
	struct udevice *bus = dev->parent;
	struct syna_spi_platdata *plat = dev_get_platdata(bus);
	struct syna_spi_regs *reg = plat->reg;

	bswl(0, &reg->ssienr);

	return 0;
}

static int syna_spi_release_bus(struct udevice *dev)
{
	struct udevice *bus = dev->parent;
	struct syna_spi_platdata *plat = dev_get_platdata(bus);
	struct syna_spi_regs *reg = plat->reg;

	bswl(3, &reg->ctrl1);
	bswl(3, &reg->rxftlr);
	/* Enable SPI */
	bswl(1, &reg->ssienr);

	return 0;
}

static int syna_spi_xfer(struct udevice *dev, unsigned int bitlen,
			  const void *dout, void *din, unsigned long flags)
{
	struct udevice *bus = dev->parent;
	struct syna_spi_platdata *plat = dev_get_platdata(bus);
	int i, j;
	struct syna_spi_regs *reg = plat->reg;

	debug("spi_xfer: dout %p din %p bitlen %u\n", dout, din, bitlen);

	if (flags & SPI_XFER_MMAP) {
		debug("SPI_XFER_MMAP\n");
		bswl(3, &reg->ctrl1);
		bswl(3, &reg->rxftlr);
		/* Enable SPI */
		bswl(1, &reg->ssienr);
		return 0;
	}

	if (flags & SPI_XFER_MMAP_END) {
		debug("SPI_XFER_MMAP_END\n");
		/* Disable SPI */
		bswl(0, &reg->ssienr);
		return 0;
	}

	if (bitlen % 8) {
		flags |= SPI_XFER_END;
		plat->wnum = 0;
		plat->rnum = 0;
		printf("%s: wrong parameter, bitlen should be an integer multiple of 8.\n",
			__func__);
		return -EPERM;
	}

	/* assume spi core configured to do 8 bit transfers */
	unsigned int bytes = bitlen / 8;

	if (flags & SPI_XFER_BEGIN) {
		plat->wnum = 0;
		plat->rnum = 0;
	}

	if (!dout && !din) {
		flags |= SPI_XFER_END;
		plat->wnum = 0;
		plat->rnum = 0;
		printf("%s: wrong parameter, in dout and din, one should be NULL and the\n"
			"other should not be NULL\n",
			__func__);
		return -EPERM;
	} else if (dout && din) {
		printf("%s: wrong parameter, in dout and din, one should be NULL and the\n"
			"other should not be NULL\n",
			__func__);
		printf("try to get dout length via strlen: ");
		memcpy(plat->wbuffer + plat->wnum, (unsigned char *)dout, strlen(dout));
		plat->wnum += strlen(dout);
		plat->rnum = bytes - plat->wnum;
		printf("wnum = %d, rnum = %d\n", plat->wnum, plat->rnum);
		flags |= SPI_XFER_END;
	} else if (dout) {
		memcpy(plat->wbuffer + plat->wnum, (unsigned char *)dout, bytes);
		plat->wnum += bytes;
		if (plat->wnum > SPI_TX_MAX_DATA_LEN) {
			plat->wnum = 0;
			printf("bitlen is too long for FIFO, wnum = %d\n", plat->wnum);
			return -ENOBUFS;
		}
	} else if (din) {
		if (!(flags & SPI_XFER_END)) {
			flags |= SPI_XFER_END;
			printf("%s: wrong parameter, when din is not NULL, this xfer should be an\n"
				"end, which means the SPI_XFER_END flags should be asserted.\n",
				__func__);
			printf("%s: assert SPI_XFER_END flag.\n", __func__);
		}
		plat->rnum = bytes;
		if (plat->rnum > SPI_RX_MAX_DATA_LEN) {
			printf("bitlen is too long for FIFO, rnum = %d\n", plat->rnum);
			return -ENOBUFS;
		}
	}

	if (!(flags & SPI_XFER_END)) {
		return 0;
	}

	const unsigned char *txp = plat->wbuffer;
	unsigned char *rxp = din;

	int wnum = plat->wnum;
	int rnum = plat->rnum;
	plat->wnum = 0;
	plat->rnum = 0;

	spi_debug("start xfer: txp = %p, rxp = %p, wnum = %d, rnum = %d\n", txp, rxp, wnum, rnum);

	if (wnum == 0 && rnum == 0)
		return 0;

	/* check status first */
	while (bsrl(&reg->sr) & 0x01);

	/* setup transfer */
	if ((txp && wnum>0) && (rxp && rnum>0))	{	/* half-duplex write, read transfer */
		/* half-duplex tranfer mode */
		set_transfer_mode(plat, 3);

		/* set number of read frames */
		bswl(rnum - 1, &reg->ctrl1);

		/* Enable SPI */
		bswl(1, &reg->ssienr);

		/* write TX FIFO */
		for (i = 0; i < wnum; i++)
			bswl(txp[i], &reg->dr);

		/* Configure the chip-select in the SER register to start transfer */
		bswl(1 << spi_chip_select(dev), &reg->ser);

		i = 0;
		while (i < rnum) {
			/* Wait RX fifo not empty */
			while (!(bsrl(&reg->sr) & 0x08));

			if (bsrl(&reg->sr) & 0x10) break;

			j = bsrl(&reg->rxflr);
			while (j--) {
				rxp[i++] = bsrl(&reg->dr);
			}
		}

	} else if (txp && wnum>0) {	/* write-only transfer */
		/* transmit-only */
		set_transfer_mode(plat, 1);

		/* Enable SPI */
		bswl(1, &reg->ssienr);

		/* write TX FIFO */
		for (i = 0; i < wnum; i++)
			bswl(txp[i], &reg->dr);

		/* Configure the chip-select in the SER register to start transfer*/
		bswl(1 << spi_chip_select(dev), &reg->ser);

	} else if (rxp && rnum>0){ /* read-only transfer */
		/* set transfer mode to receive-only */
		set_transfer_mode(plat, 2);

		/* set number of read frames */
		bswl(rnum-1, &reg->ctrl1);

		/* Enable SPI */
		bswl(1, &reg->ssienr);

		/* write TX FIFO to start transfer */
		bswl(0, &reg->dr);

		/* Configure the chip-select in the SER register to start transfer*/
		bswl(1 << spi_chip_select(dev), &reg->ser);

		for (i = 0; i < rnum; i++) {
			/* RX fifo is empty */
			while (!(bsrl(&reg->sr) & 0x08));

			if (bsrl(&reg->sr) & 0x10) break;

			rxp[i++] = bsrl(&reg->dr);
		}
	}

	/* Wait transmission finish */
	while ((bsrl(&reg->sr) != 0x06));
	/* Deselect slave device */
	bswl(0 << spi_chip_select(dev), &reg->ser);
	/* Disable SPI */
	bswl(0, &reg->ssienr);

	return 0;
}

static int syna_spi_set_speed(struct udevice *bus, uint hz)
{
	struct syna_spi_platdata *plat = dev_get_platdata(bus);
	struct syna_spi_regs *reg = plat->reg;

	int value;

	debug("Set spi speed %d Hz\n", hz);

	/* check status first */
	while (bsrl(&reg->sr) & 0x01);

	unsigned base_clk = plat->ref_clock_hz;

	/* config SPI master speed */
	if (hz > base_clk) value = 1;
	else value = (base_clk + hz - 1000) / hz;

	bswl(value, &reg->baud);

	return 0;
}

static int syna_spi_set_mode(struct udevice *bus, uint mode)
{
	return 0;
}

static const struct dm_spi_ops syna_spi_ops = {
	.claim_bus	= syna_spi_claim_bus,
	.release_bus	= syna_spi_release_bus,
	.xfer		= syna_spi_xfer,
	.set_speed	= syna_spi_set_speed,
	.set_mode	= syna_spi_set_mode,
	/*
	 * cs_info is not needed, since we require all chip selects to be
	 * in the device tree explicitly
	 */
};

static int syna_spi_child_pre_probe(struct udevice *dev)
{
	struct udevice *bus = dev_get_parent(dev);
	struct syna_spi_platdata *plat = dev_get_platdata(bus);
	struct spi_slave *slave = dev_get_parent_priv(dev);

	/*
	 * this controller can only write a small number of bytes at
	 * once! The limit is typically 64 bytes.
	 */
	slave->max_write_size = plat->max_write_size;

	return 0;
}

static int syna_spi_ofdata_to_platdata(struct udevice *bus)
{
	int ret;
	struct syna_spi_platdata *plat = dev_get_platdata(bus);
	u32 freq, mxwsz;

	plat->reg = (struct syna_spi_regs *)dev_read_addr(bus);

	ret = dev_read_u32(bus, "clock-frequency", &freq);
	if(ret) {
		debug("Set default reference clock %d Hz\n", GALOIS_SPI_SYS_CLOCK);
		plat->ref_clock_hz = GALOIS_SPI_SYS_CLOCK;
	} else
		plat->ref_clock_hz = freq;

	ret = dev_read_u32(bus, "max-write-size", &mxwsz);
	if(ret) {
			debug("Set default max write size %d\n", SPI_TX_MAX_DATA_LEN - MAX_SPI_CMD_LEN);
			plat->max_write_size = SPI_TX_MAX_DATA_LEN - MAX_SPI_CMD_LEN;
		} else
			plat->max_write_size = mxwsz;

	plat->wnum = 0;
	plat->rnum = 0;

	return 0;
}

static int syna_spi_clock_init(struct udevice *bus)
{
	int ret;
	struct clk_bulk	clks;

	ret = clk_get_bulk(bus, &clks);
	if (ret == -ENOSYS)
		return 0;
	if (ret)
		return ret;

	ret = clk_enable_bulk(&clks);
	if (ret) {
		clk_release_bulk(&clks);
		return ret;
	}

	return 0;
}

static int syna_spi_probe(struct udevice *bus)
{
	struct syna_spi_platdata *plat = dev_get_platdata(bus);
	struct syna_spi_regs *reg = plat->reg;

	syna_spi_clock_init(bus);

	/* Disable SPI */
	bswl(0, &reg->ssienr);

	/* Configure SSI as SPI master device */
	set_ssi_protocol(plat);		/* SPI protocol */
	set_transfer_mode(plat, 3);	/* half-duplex tranfer mode */

	syna_spi_set_clock_mode(reg, 3);
	syna_spi_set_speed(bus, plat->ref_clock_hz);
	syna_spi_set_framewidth(reg, 8);

	return 0;
}

static const struct udevice_id syna_spi_ids[] = {
	{ .compatible = "synaptics,dw-spi" },
	{ }
};

U_BOOT_DRIVER(syna_spi) = {
	.name = "syna_dw_spi",
	.id = UCLASS_SPI,
	.of_match = syna_spi_ids,
	.ops = &syna_spi_ops,
	.ofdata_to_platdata = syna_spi_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct syna_spi_platdata),
	.priv_auto_alloc_size = sizeof(struct syna_spi_priv),
	.child_pre_probe = syna_spi_child_pre_probe,
	.probe = syna_spi_probe,
};
