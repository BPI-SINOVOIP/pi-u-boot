// SPDX-License-Identifier: GPL-2.0-only
/*
 * Spacemit k1x qspi controller driver
 *
 * Copyright (c) 2023, spacemit Corporation.
 *
 */

#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <spi.h>
#include <spi-mem.h>
#include <dm.h>
#include <clk.h>
#include <reset.h>
#include <dm/device_compat.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <linux/iopoll.h>
#include <linux/bug.h>
#include <linux/ioport.h>

#define K1X_DUMP_QSPI_REG		0

/* QSPI PMUap register */
#define PMUA_QSPI_CLK_RES_CTRL		0xd4282860
#define QSPI_CLK_SEL(x)			((x) << 6)
#define QSPI_CLK_SEL_MASK		GENMASK(8, 6)
#define QSPI_CLK_EN			BIT(4)
#define QSPI_BUS_CLK_EN			BIT(3)
#define QSPI_CLK_RST			BIT(1)
#define QSPI_BUS_RST			BIT(0)

/* QSPI memory base */
#define QSPI_AMBA_BASE			0xb8000000
#define QSPI_FLASH_A1_BASE		QSPI_AMBA_BASE
#define QSPI_FLASH_A1_TOP		(QSPI_FLASH_A1_BASE + 0xa00000)
#define QSPI_FLASH_A2_BASE		QSPI_FLASH_A1_TOP
#define QSPI_FLASH_A2_TOP		(QSPI_FLASH_A2_BASE + 0x100000)
#define QSPI_FLASH_B1_BASE		QSPI_FLASH_A2_TOP
#define QSPI_FLASH_B1_TOP		(QSPI_FLASH_B1_BASE + 0x100000)
#define QSPI_FLASH_B2_BASE		QSPI_FLASH_B1_TOP
#define QSPI_FLASH_B2_TOP		(QSPI_FLASH_B2_BASE + 0x100000)

/* TX/RX/ABH buffer max */
#define QSPI_RX_BUFF_MAX 		SZ_128
#define QSPI_TX_BUFF_MAX 		SZ_256
#define QSPI_TX_BUFF_POP_MIN 		16
#define QSPI_AHB_BUFF_MAX_SIZE		SZ_512

#define QSPI_WAIT_BIT_CLEAR		0
#define QSPI_WAIT_BIT_SET		1

#define k1x_QSPI_DEFAULT_CLK_FREQ	26000000

/* QSPI Host Registers used by the driver */
#define QSPI_MCR			0x00
#define QSPI_MCR_ISD_MASK		GENMASK(19, 16)
#define QSPI_MCR_MDIS_MASK		BIT(14)
#define QSPI_MCR_CLR_TXF_MASK		BIT(11)
#define QSPI_MCR_CLR_RXF_MASK		BIT(10)
#define QSPI_MCR_DDR_EN_MASK		BIT(7)
#define QSPI_MCR_END_CFG_MASK		GENMASK(3, 2)
#define QSPI_MCR_SWRSTHD_MASK		BIT(1)
#define QSPI_MCR_SWRSTSD_MASK		BIT(0)

#define QSPI_TCR			0x04
#define QSPI_IPCR			0x08
#define QSPI_IPCR_SEQID(x)		((x) << 24)

#define QSPI_FLSHCR			0x0c

#define QSPI_BUF0CR			0x10
#define QSPI_BUF1CR			0x14
#define QSPI_BUF2CR			0x18
#define QSPI_BUF3CR			0x1c
#define QSPI_BUF3CR_ALLMST_MASK		BIT(31)
#define QSPI_BUF3CR_ADATSZ(x)		((x) << 8)
#define QSPI_BUF3CR_ADATSZ_MASK		GENMASK(15, 8)

#define QSPI_BFGENCR			0x20
#define QSPI_BFGENCR_SEQID(x)		((x) << 12)

#define QSPI_SOCCR			0x24

#define QSPI_BUF0IND			0x30
#define QSPI_BUF1IND			0x34
#define QSPI_BUF2IND			0x38

#define QSPI_SFAR			0x100
#define QSPI_SFACR			0x104

#define QSPI_SMPR			0x108
#define QSPI_SMPR_DDRSMP_MASK		GENMASK(18, 16)
#define QSPI_SMPR_FSDLY_MASK		BIT(6)
#define QSPI_SMPR_FSPHS_MASK		BIT(5)
#define QSPI_SMPR_HSENA_MASK		BIT(0)

#define QSPI_RBSR			0x10c

#define QSPI_RBCT			0x110
#define QSPI_RBCT_WMRK_MASK		GENMASK(4, 0)
#define QSPI_RBCT_RXBRD_MASK		BIT(8)

#define QSPI_TBSR			0x150
#define QSPI_TBDR			0x154
#define QSPI_TBCT			0x158

#define QSPI_SR				0x15c
#define QSPI_SR_BUSY			BIT(0)
#define QSPI_SR_IP_ACC_MASK		BIT(1)
#define QSPI_SR_AHB_ACC_MASK		BIT(2)
#define QSPI_SR_TXFULL			BIT(27)

#define QSPI_FR				0x160
#define QSPI_FR_TFF_MASK		BIT(0)
#define QSPI_FR_XIP_ON			BIT(1)
#define QSPI_FR_IPIEF			BIT(6)

#define QSPI_RSER			0x164
#define QSPI_RSER_TFIE			BIT(0)

#define QSPI_SPNDST			0x168
#define QSPI_SPTRCLR			0x16c
#define QSPI_SPTRCLR_IPPTRC		BIT(8)
#define QSPI_SPTRCLR_BFPTRC		BIT(0)

#define QSPI_SFA1AD			0x180
#define QSPI_SFA2AD			0x184
#define QSPI_SFB1AD			0x188
#define QSPI_SFB2AD			0x18c
#define QSPI_DLPR			0x190
#define QSPI_RBDR(x)			(0x200 + ((x) * 4))

#define QSPI_LUTKEY			0x300
#define QSPI_LUTKEY_VALUE		0x5af05af0

#define QSPI_LCKCR			0x304
#define QSPI_LCKER_LOCK			BIT(0)
#define QSPI_LCKER_UNLOCK		BIT(1)

#define QSPI_LUT_BASE			0x310
/* 16Bytes per sequence */
#define QSPI_LUT_REG(seqid, i)		(QSPI_LUT_BASE + (seqid) * 16 + (i) * 4)

/*
 * QSPI Sequence index.
 * index 0 is preset at boot for AHB read,
 * index 1 is used for other command.
 */
#define	SEQID_LUT_AHBREAD_ID		0
#define	SEQID_LUT_SHARED_ID		1

/* QSPI Instruction set for the LUT register */
#define LUT_INSTR_STOP			0
#define LUT_INSTR_CMD			1
#define LUT_INSTR_ADDR			2
#define LUT_INSTR_DUMMY			3
#define LUT_INSTR_MODE			4
#define LUT_INSTR_MODE2			5
#define LUT_INSTR_MODE4			6
#define LUT_INSTR_READ			7
#define LUT_INSTR_WRITE			8
#define LUT_INSTR_JMP_ON_CS		9
#define LUT_INSTR_ADDR_DDR		10
#define LUT_INSTR_MODE_DDR		11
#define LUT_INSTR_MODE2_DDR		12
#define LUT_INSTR_MODE4_DDR		13
#define LUT_INSTR_READ_DDR		14
#define LUT_INSTR_WRITE_DDR		15
#define LUT_INSTR_DATA_LEARN		16

/*
 * The PAD definitions for LUT register.
 *
 * The pad stands for the number of IO lines [0:3].
 * For example, the quad read needs four IO lines,
 * so you should use LUT_PAD(4).
 */
#define LUT_PAD(x) (fls(x) - 1)

/*
 * One sequence must be consisted of 4 LUT enteries(16Bytes).
 * LUT entries with the following register layout:
 * b'31                                                                     b'0
 *  ---------------------------------------------------------------------------
 *  |INSTR1[15~10]|PAD1[9~8]|OPRND1[7~0] | INSTR0[15~10]|PAD0[9~8]|OPRND0[7~0]|
 *  ---------------------------------------------------------------------------
 */
#define LUT_DEF(idx, ins, pad, opr)					\
	((((ins) << 10) | ((pad) << 8) | (opr)) << (((idx) & 0x1) * 16))

#define READ_FROM_CACHE_OP		0x03
#define READ_FROM_CACHE_OP_Fast		0x0b
#define READ_FROM_CACHE_OP_X2		0x3b
#define READ_FROM_CACHE_OP_X4		0x6b
#define READ_FROM_CACHE_OP_DUALIO	0xbb
#define READ_FROM_CACHE_OP_QUADIO	0xeb

u32 reg_offset_table[] = {
	QSPI_MCR,	QSPI_TCR,	QSPI_IPCR,	QSPI_FLSHCR,
	QSPI_BUF0CR,	QSPI_BUF1CR,	QSPI_BUF2CR,	QSPI_BUF3CR,
	QSPI_BFGENCR,	QSPI_SOCCR,	QSPI_BUF0IND,	QSPI_BUF1IND,
	QSPI_BUF2IND,	QSPI_SFAR,	QSPI_SFACR,	QSPI_SMPR,
	QSPI_RBSR,	QSPI_RBCT,	QSPI_TBSR,	QSPI_TBDR,
	QSPI_TBCT,	QSPI_SR,	QSPI_FR,	QSPI_RSER,
	QSPI_SPNDST,	QSPI_SPTRCLR,	QSPI_SFA1AD,	QSPI_SFA2AD,
	QSPI_SFB1AD,	QSPI_SFB2AD,	QSPI_DLPR,	QSPI_LUTKEY,
	QSPI_LCKCR
};

/* k1x qspi host priv */
struct k1x_qspi {
	struct udevice *dev;
	void __iomem *iobase;
	void __iomem *ahb_addr;
	u32 memmap_phy;
	u32 memmap_phy_size;
	u32 pmuap_reg;
    struct clk clk, bus_clk;
    struct reset_ctl_bulk resets;
	u32 qspi_id;
	u32 sfa1ad;
	u32 sfa2ad;
	u32 sfb1ad;
	u32 sfb2ad;

	u32 rxfifo;
	u32 txfifo;
	u32 ahb_buf_size;
	u32 ahb_read_enable;
	u32 tx_unit_size;
	u32 rx_unit_size;

	u32 cs_selected;
	u32 max_hz;
	u32 endian_xchg;
	u32 dma_enable;
};

enum qpsi_cs {
	QSPI_CS_A1 = 0,
	QSPI_CS_A2,
	QSPI_CS_B1,
	QSPI_CS_B2,
	QSPI_CS_MAX,
};

enum qpsi_mode {
	QSPI_NORMAL_MODE = 0,
	QSPI_DISABLE_MODE,
	QSPI_STOP_MODE,
};

static void qspi_writel(struct k1x_qspi *qspi, u32 val, void __iomem *addr)
{
	if (qspi->endian_xchg)
		out_be32(addr, val);
	else
		out_le32(addr, val);
}

static u32 qspi_readl(struct k1x_qspi *qspi, void __iomem *addr)
{
	if (qspi->endian_xchg)
		return in_be32(addr);
	else
		return in_le32(addr);
}

static void qspi_set_func_clk(struct k1x_qspi *qspi)
{
	reset_assert_bulk(&qspi->resets);
	clk_disable(&qspi->bus_clk);
	clk_disable(&qspi->clk);

	reset_deassert_bulk(&qspi->resets);
	clk_enable(&qspi->bus_clk);
	clk_set_rate(&qspi->clk, qspi->max_hz);
	clk_enable(&qspi->clk);
}

static int qspi_reset(struct k1x_qspi *qspi)
{
	uint32_t reg, sr, fr;
	int count = 0;

	do {
		sr = qspi_readl(qspi, qspi->iobase + QSPI_SR);
		fr = qspi_readl(qspi, qspi->iobase + QSPI_FR);
		if (!(sr & QSPI_SR_BUSY) && !(fr & QSPI_FR_XIP_ON))
			break;
		mdelay(3);
	} while(count++ < 1000);

	if (count >= 1000) {
		dev_err(qspi->dev, "reset failed\r\n");
		return -1;
	}

	/* qspi softreset first */
	reg = qspi_readl(qspi, qspi->iobase + QSPI_MCR);
	reg |= QSPI_MCR_SWRSTHD_MASK | QSPI_MCR_SWRSTSD_MASK;
	qspi_writel(qspi, reg, qspi->iobase + QSPI_MCR);
	reg = qspi_readl(qspi, qspi->iobase + QSPI_MCR);
	if ((reg & 0x3) != 0x3)
		dev_info(qspi->dev, "reset ignored 0x%x\r\n", reg);

	udelay(1);
	reg &= ~(QSPI_MCR_SWRSTHD_MASK | QSPI_MCR_SWRSTSD_MASK);
	qspi_writel(qspi, reg, qspi->iobase + QSPI_MCR);

	return 0;
}


static void qspi_enter_mode(struct k1x_qspi *qspi, uint32_t mode)
{
	uint32_t mcr;

	mcr = qspi_readl(qspi, qspi->iobase + QSPI_MCR);
	if (mode == QSPI_NORMAL_MODE)
		mcr &= ~QSPI_MCR_MDIS_MASK;
	else if (mode == QSPI_DISABLE_MODE)
		mcr |= QSPI_MCR_MDIS_MASK;
	qspi_writel(qspi, mcr, qspi->iobase + QSPI_MCR);
}


static int qspi_write_sfar(struct k1x_qspi *qspi, uint32_t val)
{
	uint32_t fr;
	int count = 0;

	do {
		qspi_writel(qspi, val, qspi->iobase + QSPI_SFAR);
		fr = qspi_readl(qspi, qspi->iobase + QSPI_FR);
		if (!(fr & QSPI_FR_IPIEF))
			break;

		fr &= QSPI_FR_IPIEF;
		qspi_writel(qspi, fr, qspi->iobase + QSPI_FR);

		mdelay(3);
	} while (count++ < 1000);

	if (count >= 1000) {
		dev_err(qspi->dev, "write sfar failed\r\n");
		return -1;
	}

	return 0;
}


/*
 * IP Command Trigger could not be executed Error Flag may happen for write
 * access to RBCT/SFAR register, need retry for these two register
 */
static int qspi_write_rbct(struct k1x_qspi *qspi, uint32_t val)
{
	uint32_t fr;
	int count = 0;

	do {
		qspi_writel(qspi, val, qspi->iobase + QSPI_RBCT);
		fr = qspi_readl(qspi, qspi->iobase + QSPI_FR);
		if (!(fr & QSPI_FR_IPIEF))
			break;
		fr &= QSPI_FR_IPIEF;
		qspi_writel(qspi, fr, qspi->iobase + QSPI_FR);

		mdelay(3);
	} while (count++ < 1000);

	if (count >= 1000) {
		dev_err(qspi->dev, "write sfar rbct\r\n");
		return -1;
	}

	return 0;
}

void qspi_init_ahbread(struct k1x_qspi *qspi, int seq_id)
{
	u32 buf_cfg = 0;

	/* Disable BUF0~BUF1, use BUF3 for all masters */
	qspi_writel(qspi, 0, qspi->iobase + QSPI_BUF0IND);
	qspi_writel(qspi, 0, qspi->iobase + QSPI_BUF1IND);
	qspi_writel(qspi, 0, qspi->iobase + QSPI_BUF2IND);

	buf_cfg = QSPI_BUF3CR_ALLMST_MASK |
			QSPI_BUF3CR_ADATSZ((qspi->ahb_buf_size / 8));

	/* AHB Master port */
	qspi_writel(qspi, 0xe, qspi->iobase + QSPI_BUF0CR);
	qspi_writel(qspi, 0xe, qspi->iobase + QSPI_BUF1CR);
	qspi_writel(qspi, 0xe, qspi->iobase + QSPI_BUF2CR);
	qspi_writel(qspi, buf_cfg, qspi->iobase + QSPI_BUF3CR); // other masters

	/* set AHB read sequence id */
	qspi_writel(qspi, QSPI_BFGENCR_SEQID(seq_id), qspi->iobase + QSPI_BFGENCR);
	dev_info(qspi->dev, "AHB buf size: %d\n", qspi->ahb_buf_size);
}

void qspi_dump_reg(struct k1x_qspi *qspi)
{
#if (K1X_DUMP_QSPI_REG)
	u32 reg = 0;
	void __iomem *base = qspi->iobase;
	int i;

	dev_notice(qspi->dev, "dump qspi host register:\n");
	for (i = 0; i < ARRAY_SIZE(reg_offset_table); i++) {
		if (i > 0 && (i % 4 == 0))
			dev_notice(qspi->dev, "\n");
		reg = qspi_readl(qspi, base + reg_offset_table[i]);
		dev_notice(qspi->dev, "offset[0x%03x]:0x%08x\t\t",
				reg_offset_table[i], reg);
	}

	dev_notice(qspi->dev, "\ndump AHB read LUT:\n");
	for (i = 0; i < 4; i++) {
		reg = qspi_readl(qspi, base + QSPI_LUT_REG(SEQID_LUT_AHBREAD_ID, i));
		dev_notice(qspi->dev, "lut_reg[0x%03x]:0x%08x\t\t",
				QSPI_LUT_REG(SEQID_LUT_AHBREAD_ID, i), reg);
	}

	dev_notice(qspi->dev, "\ndump shared LUT:\n");
	for (i = 0; i < 4; i++) {
		reg = qspi_readl(qspi, base + QSPI_LUT_REG(SEQID_LUT_SHARED_ID, i));
		dev_notice(qspi->dev, "lut_reg[0x%03x]:0x%08x\t\t",
				QSPI_LUT_REG(SEQID_LUT_SHARED_ID, i), reg);
	}
	dev_notice(qspi->dev, "\n");
#endif
}

static int k1x_qspi_readl_poll_tout(struct k1x_qspi *qspi, void __iomem *base,
					u32 mask, u32 timeout_us, u8 wait_set)
{
	u32 reg;

	if (qspi->endian_xchg)
		mask = swab32(mask);

	if (wait_set)
		return readl_poll_timeout(base, reg, (reg & mask), timeout_us);
	else
		return readl_poll_timeout(base, reg, !(reg & mask), timeout_us);
}

/*
 * If the slave device content being changed by Write/Erase, need to
 * invalidate the AHB buffer. This can be achieved by doing the reset
 * of controller after setting MCR0[SWRESET] bit.
 */
static inline void k1x_qspi_invalid(struct k1x_qspi *qspi)
{
	u32 reg;

	reg = qspi_readl(qspi, qspi->iobase + QSPI_MCR);
	reg |= QSPI_MCR_SWRSTHD_MASK | QSPI_MCR_SWRSTSD_MASK;
	qspi_writel(qspi, reg, qspi->iobase + QSPI_MCR);

	/*
	 * The minimum delay : 1 AHB + 2 SFCK clocks.
	 * Delay 1 us is enough.
	 */
	udelay(1);

	reg &= ~(QSPI_MCR_SWRSTHD_MASK | QSPI_MCR_SWRSTSD_MASK);
	qspi_writel(qspi, reg, qspi->iobase + QSPI_MCR);
}

static void k1x_qspi_prepare_lut(struct k1x_qspi *qspi,
				const struct spi_mem_op *op, u32 seq_id)
{
	u32 lutval[4] = {0,};
	int lutidx = 0;
	int i;

	/* qspi cmd */
	lutval[0] |= LUT_DEF(lutidx, LUT_INSTR_CMD,
			     LUT_PAD(op->cmd.buswidth),
			     op->cmd.opcode);
	lutidx++;

	/* addr bytes */
	if (op->addr.nbytes) {
		lutval[lutidx / 2] |= LUT_DEF(lutidx, LUT_INSTR_ADDR,
					      LUT_PAD(op->addr.buswidth),
					      op->addr.nbytes * 8);
		lutidx++;
	}

	/* dummy bytes, if needed */
	if (op->dummy.nbytes) {
		lutval[lutidx / 2] |= LUT_DEF(lutidx, LUT_INSTR_DUMMY,
					      LUT_PAD(op->dummy.buswidth),
					      op->dummy.nbytes * 8 /
					      op->dummy.buswidth);
		lutidx++;
	}

	/* read/write data bytes */
	if (op->data.nbytes) {
		lutval[lutidx / 2] |= LUT_DEF(lutidx,
					      op->data.dir == SPI_MEM_DATA_IN ?
					      LUT_INSTR_READ : LUT_INSTR_WRITE,
					      LUT_PAD(op->data.buswidth),
					      0);
		lutidx++;
	}

	/* stop condition. */
	lutval[lutidx / 2] |= LUT_DEF(lutidx, LUT_INSTR_STOP, 0, 0);

	/* unlock LUT */
	qspi_writel(qspi, QSPI_LUTKEY_VALUE, qspi->iobase + QSPI_LUTKEY);
	qspi_writel(qspi, QSPI_LCKER_UNLOCK, qspi->iobase + QSPI_LCKCR);

	/* fill LUT register */
	for (i = 0; i < ARRAY_SIZE(lutval); i++)
		qspi_writel(qspi, lutval[i], qspi->iobase + QSPI_LUT_REG(seq_id, i));

	/* lock LUT */
	qspi_writel(qspi, QSPI_LUTKEY_VALUE, qspi->iobase + QSPI_LUTKEY);
	qspi_writel(qspi, QSPI_LCKER_LOCK, qspi->iobase + QSPI_LCKCR);

	dev_dbg(qspi->dev, "opcode:0x%x, lut_reg[0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x]\n",
		op->cmd.opcode, lutval[0], lutval[1], lutval[2], lutval[3]);
}

static void k1x_qspi_select_mem(struct k1x_qspi *qspi, int chip_select)
{
	u64 size_kb;

	size_kb = (qspi->memmap_phy + qspi->memmap_phy_size) & 0xFFFFFC00;
	qspi_writel(qspi, size_kb, qspi->iobase + (QSPI_SFA1AD + 4 * chip_select));

	dev_dbg(qspi->dev, "slave device[cs:%d] selected\n", chip_select);
}

static void k1x_qspi_ahb_read(struct k1x_qspi *qspi,
				const struct spi_mem_op *op)
{
	u32 len = op->data.nbytes;

	/* Read out the data directly from the AHB buffer. */
	dev_dbg(qspi->dev, "ahb read %d bytes from address:0x%llx\n",
				len, (qspi->memmap_phy + op->addr.val));
	memcpy(op->data.buf.in, (qspi->ahb_addr + op->addr.val), len);
}

static void k1x_qspi_fill_txfifo(struct k1x_qspi *qspi,
				 const struct spi_mem_op *op)
{
	void __iomem *base = qspi->iobase;
	int i;
	u32 val;

	for (i = 0; i < ALIGN_DOWN(op->data.nbytes, 4); i += 4) {
		memcpy(&val, op->data.buf.out + i, 4);
		qspi_writel(qspi, val, base + QSPI_TBDR);
	}

	if (i < op->data.nbytes) {
		memcpy(&val, op->data.buf.out + i, op->data.nbytes - i);
		qspi_writel(qspi, val, base + QSPI_TBDR);
	}

	/*
	 * There must be atleast 128bit data available in TX FIFO
	 * for any pop operation otherwise QSPI_FR[TBUF] will be set
	 */
	for (i = op->data.nbytes; i < QSPI_TX_BUFF_POP_MIN; i += 4)
		qspi_writel(qspi, 0, base + QSPI_TBDR);
}

static void k1x_qspi_read_rxfifo(struct k1x_qspi *qspi,
			  const struct spi_mem_op *op)
{
	void __iomem *base = qspi->iobase;
	int i;
	u8 *buf = op->data.buf.in;
	u32 val;

	dev_dbg(qspi->dev, "ip read %d bytes\n", op->data.nbytes);
	for (i = 0; i < ALIGN_DOWN(op->data.nbytes, 4); i += 4) {
		val = qspi_readl(qspi, base + QSPI_RBDR(i / 4));
		memcpy(buf + i, &val, 4);
	}

	if (i < op->data.nbytes) {
		val = qspi_readl(qspi, base + QSPI_RBDR(i / 4));
		memcpy(buf + i, &val, op->data.nbytes - i);
	}
}

static int k1x_qspi_do_op(struct k1x_qspi *qspi, const struct spi_mem_op *op)
{
	void __iomem *base = qspi->iobase;
	int err = 0;

	/* dump reg if need */
	qspi_dump_reg(qspi);

	/* trigger LUT */
	qspi_writel(qspi, op->data.nbytes | QSPI_IPCR_SEQID(SEQID_LUT_SHARED_ID),
		    base + QSPI_IPCR);

	/* wait for the transaction complete */
	err = k1x_qspi_readl_poll_tout(qspi, base + QSPI_FR, QSPI_FR_TFF_MASK,
					100*1000, QSPI_WAIT_BIT_SET);
	if (!err)
		err = k1x_qspi_readl_poll_tout(qspi, base + QSPI_SR, QSPI_SR_BUSY,
					300*1000, QSPI_WAIT_BIT_CLEAR);
	if (err)
		dev_err(qspi->dev, "opcode:0x%x transaction timeout!\n", op->cmd.opcode);

	/* read RX buffer for IP command read */
	if (!err && op->data.nbytes && op->data.dir == SPI_MEM_DATA_IN) {
		qspi_dump_reg(qspi);
		k1x_qspi_read_rxfifo(qspi, op);
	}

	return err;
}

static void dump_spi_mem_op_info(struct k1x_qspi *qspi,
				const struct spi_mem_op *op)
{
	dev_dbg(qspi->dev, "cmd.opcode:0x%x\n", op->cmd.opcode);
	dev_dbg(qspi->dev, "cmd.buswidth:%d\n", op->cmd.buswidth);
	dev_dbg(qspi->dev, "addr.nbytes:%d,\n", op->addr.nbytes);
	dev_dbg(qspi->dev, "addr.buswidth:%d\n", op->addr.buswidth);
	dev_dbg(qspi->dev, "addr.val:0x%llx\n", op->addr.val);
	dev_dbg(qspi->dev, "dummy.nbytes:%d\n", op->dummy.nbytes);
	dev_dbg(qspi->dev, "dummy.buswidth:%d\n", op->dummy.buswidth);
	dev_dbg(qspi->dev, "%s data.nbytes:%d\n",
		(op->data.dir == SPI_MEM_DATA_IN) ? "read" :"write",
		op->data.nbytes);
	dev_dbg(qspi->dev, "data.buswidth:%d\n", op->data.buswidth);
	dev_dbg(qspi->dev, "data.buf:0x%p\n", op->data.buf.in);
}


static int is_read_from_cache_opcode(u8 opcode)
{
	int ret;

	ret = ((opcode == READ_FROM_CACHE_OP) ||
		(opcode == READ_FROM_CACHE_OP_Fast) ||
		(opcode == READ_FROM_CACHE_OP_X2) ||
		(opcode == READ_FROM_CACHE_OP_X4) ||
		(opcode == READ_FROM_CACHE_OP_DUALIO) ||
		(opcode == READ_FROM_CACHE_OP_QUADIO));

	return ret;
}

static int k1x_qspi_exec_op(struct spi_slave *slave,
			    const struct spi_mem_op *op)
{
	struct k1x_qspi *qspi;
	struct udevice *bus;
	void __iomem *base;
	u32 mask;
	u32 reg;
	int err;

	bus = slave->dev->parent;
	qspi = dev_get_priv(bus);
	base = qspi->iobase;

	dump_spi_mem_op_info(qspi, op);

	/* wait for controller being ready */
	mask = QSPI_SR_BUSY | QSPI_SR_IP_ACC_MASK | QSPI_SR_AHB_ACC_MASK;
	err = k1x_qspi_readl_poll_tout(qspi, base + QSPI_SR, mask, 100*1000, QSPI_WAIT_BIT_CLEAR);
	if (err) {
		dev_err(qspi->dev, "controller not ready!\n");
		return err;
	}

	/* clear TX/RX buffer before transaction */
	reg = qspi_readl(qspi, base + QSPI_MCR);
	reg |= QSPI_MCR_CLR_TXF_MASK | QSPI_MCR_CLR_RXF_MASK;
	qspi_writel(qspi, reg, base + QSPI_MCR);

	/*
	 * reset the sequence pointers whenever the sequence ID is changed by
	 * updating the SEDID filed in QSPI_IPCR OR QSPI_BFGENCR.
	 */
	reg = qspi_readl(qspi, base + QSPI_SPTRCLR);
	reg |= (QSPI_SPTRCLR_IPPTRC | QSPI_SPTRCLR_BFPTRC);
	qspi_writel(qspi, reg, base + QSPI_SPTRCLR);

	/* set the flash address into the QSPI_SFAR */
	err = qspi_write_sfar(qspi, qspi->memmap_phy + op->addr.val);
	if (err) {
		return err;
	}

	/* clear QSPI_FR before trigger LUT command */
	reg = qspi_readl(qspi, base + QSPI_FR);
	if (reg)
		qspi_writel(qspi, reg, base + QSPI_FR);

	/*
	 * read page command 13h must be done by IP command.
	 * read from cache through the AHB bus by accessing the mapped memory.
	 * In all other cases we use IP commands to access the flash.
	 */
	if (op->data.nbytes > (qspi->rxfifo - 4) &&
		op->data.dir == SPI_MEM_DATA_IN &&
		qspi->ahb_read_enable &&
		is_read_from_cache_opcode(op->cmd.opcode)) {
		k1x_qspi_prepare_lut(qspi, op, SEQID_LUT_AHBREAD_ID);
		k1x_qspi_ahb_read(qspi, op);
	} else {
		/* IP command */
		k1x_qspi_prepare_lut(qspi, op, SEQID_LUT_SHARED_ID);
		if (op->data.nbytes && op->data.dir == SPI_MEM_DATA_OUT)
			k1x_qspi_fill_txfifo(qspi, op);

		err = k1x_qspi_do_op(qspi, op);
	}

	/* invalidate the data in the AHB buffer. */
	k1x_qspi_invalid(qspi);

	return err;
}

static int k1x_qspi_check_buswidth(struct k1x_qspi *qspi, u8 width)
{
	switch (width) {
	case 1:
	case 2:
	case 4:
		return 0;
	}

	return -ENOTSUPP;
}

static bool k1x_qspi_supports_op(struct spi_slave *slave,
				 const struct spi_mem_op *op)
{
	struct k1x_qspi *qspi;
	struct udevice *bus;
	int ret;

	bus = slave->dev->parent;
	qspi = dev_get_priv(bus);

	ret = k1x_qspi_check_buswidth(qspi, op->cmd.buswidth);

	if (op->addr.nbytes)
		ret |= k1x_qspi_check_buswidth(qspi, op->addr.buswidth);

	if (op->dummy.nbytes)
		ret |= k1x_qspi_check_buswidth(qspi, op->dummy.buswidth);

	if (op->data.nbytes)
		ret |= k1x_qspi_check_buswidth(qspi, op->data.buswidth);

	if (ret)
		return false;

	/* address bytes should be equal to or less than 4 bytes */
	if (op->addr.nbytes > 4)
		return false;

	/* check controller TX/RX buffer limits and alignment */
	if (op->data.dir == SPI_MEM_DATA_IN &&
	    (op->data.nbytes > qspi->rx_unit_size ||
	    (op->data.nbytes > qspi->rxfifo - 4 && !IS_ALIGNED(op->data.nbytes, 4)))) {
		return false;
	}

	if (op->data.dir == SPI_MEM_DATA_OUT && op->data.nbytes > qspi->tx_unit_size)
		return false;

	/*
	 * If requested address value is greater than controller assigned
	 * memory mapped space, return error as it didn't fit in the range.
	 */
	if (op->addr.val >= qspi->memmap_phy_size)
		return false;

	/* number of dummy clock cycles should be <= 64 cycles */
	if (op->dummy.buswidth &&
	    (op->dummy.nbytes * 8 / op->dummy.buswidth > 64))
		return false;

	return true;
}

static int k1x_qspi_adjust_op_size(struct spi_slave *slave,
				   struct spi_mem_op *op)
{
	struct k1x_qspi *qspi;
	struct udevice *bus;

	bus = slave->dev->parent;
	qspi = dev_get_priv(bus);

	if (op->data.dir == SPI_MEM_DATA_OUT) {
		if (op->data.nbytes > qspi->tx_unit_size)
			op->data.nbytes = qspi->tx_unit_size;
	} else {
		if (op->data.nbytes > qspi->rx_unit_size) {
			op->data.nbytes = qspi->rx_unit_size;
		} else if (op->data.nbytes > qspi->rxfifo - 4 && !IS_ALIGNED(op->data.nbytes, 4)) {
			op->data.nbytes = qspi->rxfifo - 4;
		}
	}

	return 0;
}

static int k1x_qspi_host_init(struct k1x_qspi *qspi)
{
	void __iomem *base = qspi->iobase;
	u32 reg;
	int ret = 0;

	/* set PMUap */
	qspi_set_func_clk(qspi);

	/* rest qspi */
	ret = qspi_reset(qspi);
	if (ret < 0) {
		goto dis_clk;
	}

	/* clock settings */
	qspi_enter_mode(qspi, QSPI_DISABLE_MODE);

	/* sampled by sfif_clk_b; half cycle delay; */
	if (qspi->max_hz < 104000000)
		qspi_writel(qspi, 0x0, base + QSPI_SMPR);
	else
		qspi_writel(qspi, QSPI_SMPR_FSPHS_MASK, base + QSPI_SMPR);

	/* Fix wirte failure issue*/
	qspi_writel(qspi, 0x8, base + QSPI_SOCCR);

	/* Give the default source address */
	ret = qspi_write_sfar(qspi, qspi->memmap_phy);
	if (ret < 0) {
		goto dis_clk;
	}
	qspi_writel(qspi, 0x0, base + QSPI_SFACR);

	 /* config ahb read */
	qspi_init_ahbread(qspi, SEQID_LUT_AHBREAD_ID);

	/* Set flash memory map */
	qspi_writel(qspi, (qspi->memmap_phy + qspi->sfa1ad) & 0xfffffc00, base + QSPI_SFA1AD);
	qspi_writel(qspi, (qspi->memmap_phy + qspi->sfa2ad) & 0xfffffc00, base + QSPI_SFA2AD);
	qspi_writel(qspi, (qspi->memmap_phy + qspi->sfb1ad) & 0xfffffc00, base + QSPI_SFB1AD);
	qspi_writel(qspi, (qspi->memmap_phy + qspi->sfb2ad) & 0xfffffc00, base + QSPI_SFB2AD);

	/* ISD3FB, ISD2FB, ISD3FA, ISD2FA = 1; END_CFG=0x3 */
	reg = qspi_readl(qspi, base + QSPI_MCR);
	reg |= QSPI_MCR_END_CFG_MASK | QSPI_MCR_ISD_MASK;
	qspi_writel(qspi, reg, base + QSPI_MCR);

	/* Module enabled */
	qspi_enter_mode(qspi, QSPI_NORMAL_MODE);

	/* Read using the IP Bus registers QSPI_RBDR0 to QSPI_RBDR31*/
	ret = qspi_write_rbct(qspi, QSPI_RBCT_RXBRD_MASK);
	if (ret < 0) {
		goto dis_clk;
	}

	/* clear all interrupt status */
	qspi_writel(qspi, 0xffffffff, base + QSPI_FR);

	dev_dbg(qspi->dev, "dump registers after qspi host init\n");
	qspi_dump_reg(qspi);
	return 0;

dis_clk:
	reset_assert_bulk(&qspi->resets);
	clk_disable(&qspi->bus_clk);
	clk_disable(&qspi->clk);
	return ret;
}

static int k1x_qspi_probe(struct udevice *bus)
{
	struct k1x_qspi *host = dev_get_priv(bus);

	return k1x_qspi_host_init(host);
}

static int k1x_qspi_claim_bus(struct udevice *dev)
{
	struct k1x_qspi *qspi;
	struct udevice *bus;
	struct dm_spi_slave_plat *slave_plat = dev_get_parent_plat(dev);

	bus = dev->parent;
	qspi = dev_get_priv(bus);

	k1x_qspi_select_mem(qspi, slave_plat->cs);

	return 0;
}

static int k1x_qspi_set_speed(struct udevice *bus, uint speed)
{
	/* TODO: if need */
	return 0;
}

static int k1x_qspi_set_mode(struct udevice *bus, uint mode)
{
	/* TODO: if need */
	return 0;
}

static int k1x_qspi_ofdata_to_platdata(struct udevice *bus)
{
	struct k1x_qspi *qspi = dev_get_priv(bus);
	int ret;
	struct resource res;
	fdt_addr_t iobase;
	fdt_addr_t iobase_size;
	fdt_addr_t ahb_addr;
	fdt_addr_t ahb_size;
	const void *blob = gd->fdt_blob;
	int node = dev_of_offset(bus);

	qspi->dev = bus;

	ret = dev_read_resource_byname(bus, "qspi-base", &res);
	if (ret) {
		dev_err(bus, "can't get qspi-base addresses(ret:%d)!\n", ret);
		return ret;
	}
	iobase = (fdt_addr_t)res.start;
	iobase_size = resource_size(&res);
	qspi->iobase = map_physmem(iobase, iobase_size, MAP_NOCACHE);

	ret = dev_read_resource_byname(bus, "qspi-mmap", &res);
	if (ret) {
		dev_err(bus, "can't get qspi-mmap addresses(ret:%d)!\n", ret);
		return ret;
	}

	ahb_addr = (fdt_addr_t)res.start;
	ahb_size = resource_size(&res);
	qspi->ahb_addr = map_physmem(ahb_addr, ahb_size, MAP_NOCACHE);
	qspi->memmap_phy_size = ahb_size;
	qspi->memmap_phy = (u32)ahb_addr;

    ret = clk_get_by_index(bus, 0, &qspi->clk);
	if (ret) {
		dev_err(bus, "can not find the clock\n");
		return ret;
	}

    ret = clk_get_by_index(bus, 1, &qspi->bus_clk);
	if (ret) {
		dev_err(bus, "can not find bus clock\n");
		return ret;
	}

    ret = reset_get_bulk(bus, &qspi->resets);
    if (ret) {
		dev_err(bus, "can not find resets\n");
		return ret;
	}

	qspi->qspi_id = fdtdec_get_int(blob, node, "qspi-id", 0);
	qspi->sfa1ad = fdtdec_get_int(blob, node, "qspi-sfa1ad", (QSPI_FLASH_A1_TOP - QSPI_AMBA_BASE));
	qspi->sfa2ad = fdtdec_get_int(blob, node, "qspi-sfa2ad", (QSPI_FLASH_A2_TOP - QSPI_AMBA_BASE));
	qspi->sfb1ad = fdtdec_get_int(blob, node, "qspi-sfb1ad", (QSPI_FLASH_B1_TOP - QSPI_AMBA_BASE));
	qspi->sfb2ad = fdtdec_get_int(blob, node, "qspi-sfb2ad", (QSPI_FLASH_B2_TOP - QSPI_AMBA_BASE));

	qspi->pmuap_reg = fdtdec_get_int(blob, node, "qspi-pmuap-reg", PMUA_QSPI_CLK_RES_CTRL);
	qspi->max_hz = fdtdec_get_int(blob, node, "spi-max-frequency", k1x_QSPI_DEFAULT_CLK_FREQ);
	qspi->rxfifo = fdtdec_get_int(blob, node, "qspi-rxbuf", QSPI_RX_BUFF_MAX);
	qspi->txfifo = fdtdec_get_int(blob, node, "qspi-txfifo", QSPI_TX_BUFF_MAX);
	qspi->ahb_buf_size = fdtdec_get_int(blob, node, "qspi-ahbbuf", QSPI_AHB_BUFF_MAX_SIZE);
	qspi->ahb_read_enable = fdtdec_get_int(blob, node, "qspi-ahbread", 1);
	qspi->endian_xchg = fdtdec_get_int(blob, node, "qspi-little", 0);

	qspi->cs_selected = QSPI_CS_A1;

	dev_info(bus, "qspi iobase:0x%pa, ahb_addr:0x%pa, max_hz:%dHz\n",
				&iobase, &ahb_addr, qspi->max_hz);
	dev_info(bus, "rx buf size:%d, tx buf size:%d, ahb buf size=%d\n",
				qspi->rxfifo, qspi->txfifo, qspi->ahb_buf_size);
	dev_info(bus, "AHB read %s\n", qspi->ahb_read_enable ? "enabled" : "disabled");

	qspi->tx_unit_size = qspi->txfifo;
	if (qspi->ahb_read_enable)
		qspi->rx_unit_size = SZ_4K;
	else
		qspi->rx_unit_size = qspi->rxfifo;

	return 0;
}

static const struct spi_controller_mem_ops k1x_qspi_mem_ops = {
	.adjust_op_size = k1x_qspi_adjust_op_size,
	.supports_op = k1x_qspi_supports_op,
	.exec_op = k1x_qspi_exec_op,
};

static const struct dm_spi_ops k1x_qspi_ops = {
	.claim_bus	= k1x_qspi_claim_bus,
	.set_speed	= k1x_qspi_set_speed,
	.set_mode	= k1x_qspi_set_mode,
	.mem_ops	= &k1x_qspi_mem_ops,
};

static const struct udevice_id k1x_qspi_ids[] = {
	{ .compatible = "spacemit,k1x-qspi", },
	{ }
};

U_BOOT_DRIVER(k1x_qspi) = {
	.name	= "k1x_qspi",
	.id	= UCLASS_SPI,
	.of_match = k1x_qspi_ids,
	.ops	= &k1x_qspi_ops,
	.of_to_plat = k1x_qspi_ofdata_to_platdata,
	.priv_auto = sizeof(struct k1x_qspi),
	.probe	= k1x_qspi_probe,
};
