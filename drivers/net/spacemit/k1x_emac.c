// SPDX-License-Identifier: GPL-2.0
/*
 * spacemit emac driver
 *
 * Copyright (C) 2023 Spacemit
 *
 */

#include <asm/gpio.h>
#include <asm/io.h>
#include <common.h>
#include <cpu_func.h>
#include <clk.h>
#include <dm.h>
#include <errno.h>
#include <memalign.h>
#include <miiphy.h>
#include <net.h>
#include <netdev.h>
#include <phy.h>
#include <reset.h>
#include <wait_bit.h>
#include "k1x_emac.h"

#define TX_PHASE                1
#define RX_PHASE                0

#define CLK_PHASE_REVERT        180


/* Clock */
#define K1X_APMU_BASE           0xd4282800

#define EMAC_AXI_CLK_ENABLE     BIT(0)
#define EMAC_AXI_CLK_RESET      BIT(1)
/* emac phy interface selection 0:RMII 1:RGMII */
#define EMAC_PHY_SEL_RGMII      BIT(2)

/*
 * only valid for rmii mode
 * 0: ref clock from external phy
 * 1: ref clock from soc
 */
#define REF_CLK_SEL             BIT(3)

/*
 * emac function clock select
 * 0: 208M
 * 1: 312M
 */
#define FUNC_CLK_SEL            BIT(4)

/* only valid for rmii, invert tx clk */
#define RMII_TX_CLK_SEL         BIT(6)

/* only valid for rmii, invert rx clk */
#define RMII_RX_CLK_SEL         BIT(7)

/*
 * only valid for rgmiii
 * 0: tx clk from rx clk
 * 1: tx clk from soc
 */
#define RGMII_TX_CLK_SEL        BIT(8)

#define PHY_IRQ_EN              BIT(12)
#define AXI_SINGLE_ID           BIT(13)

#define RMII_TX_PHASE_OFFSET            (16)
#define RMII_TX_PHASE_MASK              GENMASK(18, 16)
#define RMII_RX_PHASE_OFFSET            (20)
#define RMII_RX_PHASE_MASK              GENMASK(22, 20)

#define RGMII_TX_PHASE_OFFSET           (24)
#define RGMII_TX_PHASE_MASK             GENMASK(26, 24)
#define RGMII_RX_PHASE_OFFSET           (28)
#define RGMII_RX_PHASE_MASK             GENMASK(30, 28)

#define EMAC_RX_DLINE_EN                BIT(0)
#define EMAC_RX_DLINE_STEP_OFFSET       (4)
#define EMAC_RX_DLINE_STEP_MASK         GENMASK(5, 4)
#define EMAC_RX_DLINE_CODE_OFFSET       (8)
#define EMAC_RX_DLINE_CODE_MASK         GENMASK(15, 8)

#define EMAC_TX_DLINE_EN                BIT(16)
#define EMAC_TX_DLINE_STEP_OFFSET       (20)
#define EMAC_TX_DLINE_STEP_MASK         GENMASK(21, 20)
#define EMAC_TX_DLINE_CODE_OFFSET       (24)
#define EMAC_TX_DLINE_CODE_MASK         GENMASK(31, 24)

/* Descriptors */

#define EQOS_DESCRIPTOR_WORDS           4
#define EQOS_DESCRIPTOR_SIZE            (EQOS_DESCRIPTOR_WORDS * 4)
/* We assume ARCH_DMA_MINALIGN >= 16; 16 is the EQOS HW minimum */
#define EQOS_DESCRIPTOR_ALIGN           ARCH_DMA_MINALIGN
#define EQOS_DESCRIPTORS_TX             4
#define EQOS_DESCRIPTORS_RX             32
#define EQOS_DESCRIPTORS_NUM            (EQOS_DESCRIPTORS_TX + EQOS_DESCRIPTORS_RX)
#define EQOS_DESCRIPTORS_SIZE           ALIGN(EQOS_DESCRIPTORS_NUM * \
                                            EQOS_DESCRIPTOR_SIZE, ARCH_DMA_MINALIGN)
#define EQOS_BUFFER_ALIGN               ARCH_DMA_MINALIGN
#define EQOS_MAX_PACKET_SIZE            ALIGN(1568, ARCH_DMA_MINALIGN)
#define EQOS_RX_BUFFER_SIZE             (EQOS_DESCRIPTORS_RX * EQOS_MAX_PACKET_SIZE)
#define CACHE_FLUSH_CNT                 (ARCH_DMA_MINALIGN / EQOS_DESCRIPTOR_SIZE)

/*
 * Warn if the cache-line size is larger than the descriptor size. In such
 * cases the driver will likely fail because the CPU needs to flush the cache
 * when requeuing RX buffers, therefore descriptors written by the hardware
 * may be discarded. Architectures with full IO coherence, such as x86, do not
 * experience this issue, and hence are excluded from this condition.
 *
 * This can be fixed by defining CONFIG_SYS_NONCACHED_MEMORY which will cause
 * the driver to allocate descriptors from a pool of non-cached memory.
 *
 * #if EQOS_DESCRIPTOR_SIZE < ARCH_DMA_MINALIGN
 * #if !defined(CONFIG_SYS_NONCACHED_MEMORY) && \
 *    !CONFIG_IS_ENABLED(SYS_DCACHE_OFF) && !defined(CONFIG_X86)
 * #warning Cache line size is larger than descriptor size
 * #endif
 * #endif
 */

struct emac_desc {
    u32 des0;
    u32 des1;
    u32 des2;
    u32 des3;
};

#define EMAC_DESC_OWN           BIT(31)
#define EMAC_DESC_FD            BIT(30)
#define EMAC_DESC_LD            BIT(29)
#define EMAC_DESC_EOR           BIT(26)
#define EMAC_DESC_BUFF_SIZE1    GENMASK(11, 0)

enum clk_tuning_way {
    /* fpga rgmii/rmii clk tuning register */
    CLK_TUNING_BY_REG,
    /* rgmii evb delayline register */
    CLK_TUNING_BY_DLINE,
    /* rmii evb only revert tx/rx clock for clk tuning */
    CLK_TUNING_BY_CLK_REVERT,
    CLK_TUNING_MAX,
};

struct emac_priv {
    struct udevice *dev;
    void __iomem *io_base;
    struct clk mac_clk;
    struct reset_ctl reset;
    struct mii_dev *mii;
    struct phy_device *phy;
    int phy_interface;
    int duplex;
    int speed;
    void *descs;
    struct emac_desc *tx_descs;
    struct emac_desc *rx_descs;
    int tx_desc_idx, rx_desc_idx;
    void *tx_dma_buf;
    void *rx_dma_buf;
    bool started;
    int phy_reset_gpio;
    int ldo_gpio;
    int phy_addr;
    int tx_phase;
    int rx_phase;
    int clk_tuning_enable;
    int ref_clk_frm_soc;
    void __iomem *ctrl_reg;
    void __iomem *dline_reg;
    int clk_tuning_way;
};

void print_pkt(unsigned char *buf, int len)
{
    int i = 0;

    printf("data len = %d byte, buf addr: %p\n", len, buf);
    for (i = 0; i < len; i = i + 8) {
        printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
            *(buf + i),
            *(buf + i + 1),
            *(buf + i + 2),
            *(buf + i + 3),
            *(buf + i + 4),
            *(buf + i + 5),
            *(buf + i + 6),
            *(buf + i + 7));
    }
}

void print_desc(unsigned char *buf, int len)
{
    int i;

    printf("descriptor len = %d byte, buf addr: %p\n",
           len, buf);
    for (i = 0; i < len; i = i + 4) {
        printf("0x%02x 0x%02x 0x%02x 0x%02x\n",
            *(buf + i + 3),
            *(buf + i + 2),
            *(buf + i + 1),
            *(buf + i));
    }
}

static inline void emac_wr(struct emac_priv *priv, u32 reg, u32 val)
{
    writel(val, (priv->io_base + reg));
}

static inline int emac_rd(struct emac_priv *priv, u32 reg)
{
    return readl(priv->io_base + reg);
}

int emac_reset_hw(struct emac_priv *priv)
{
    /* disable all the interrupts */
    emac_wr(priv, MAC_INTERRUPT_ENABLE, 0x0000);
    emac_wr(priv, DMA_INTERRUPT_ENABLE, 0x0000);

    /* disable transmit and receive units */
    emac_wr(priv, MAC_RECEIVE_CONTROL, 0x0000);
    emac_wr(priv, MAC_TRANSMIT_CONTROL, 0x0000);

    /* stop the DMA */
    emac_wr(priv, DMA_CONTROL, 0x0000);

    /* reset mac, statistic counters */
    emac_wr(priv, MAC_GLOBAL_CONTROL, 0x0018);

    emac_wr(priv, MAC_GLOBAL_CONTROL, 0x0000);
    return 0;
}

int emac_init_hw(struct emac_priv *priv)
{
    u32 val = 0;

    /* MAC Init
     * disable transmit and receive units
     */
    emac_wr(priv, MAC_RECEIVE_CONTROL, 0x0000);
    emac_wr(priv, MAC_TRANSMIT_CONTROL, 0x0000);

    /* enable mac address 1 filtering */
    emac_wr(priv, MAC_ADDRESS_CONTROL, 0x0001);

    /* zero initialize the multicast hash table */
    emac_wr(priv, MAC_MULTICAST_HASH_TABLE1, 0x0000);
    emac_wr(priv, MAC_MULTICAST_HASH_TABLE2, 0x0000);
    emac_wr(priv, MAC_MULTICAST_HASH_TABLE3, 0x0000);
    emac_wr(priv, MAC_MULTICAST_HASH_TABLE4, 0x0000);

    emac_wr(priv, MAC_TRANSMIT_FIFO_ALMOST_FULL, 0x1f8);

    emac_wr(priv, MAC_TRANSMIT_PACKET_START_THRESHOLD,
        TX_STORE_FORWARD_MODE);

    emac_wr(priv, MAC_RECEIVE_PACKET_START_THRESHOLD, 0xc);

    /* reset dma */
    emac_wr(priv, DMA_CONTROL, 0x0000);

    emac_wr(priv, DMA_CONFIGURATION, 0x01);
    mdelay(10);
    emac_wr(priv, DMA_CONFIGURATION, 0x00);
    mdelay(10);

    val |= MREGBIT_WAIT_FOR_DONE;
    val |= MREGBIT_STRICT_BURST;
    val |= MREGBIT_DMA_64BIT_MODE;

    val |= MREGBIT_BURST_16WORD;

    emac_wr(priv, DMA_CONFIGURATION, val);

    return 0;
}

static void emac_configure_tx(struct emac_priv *priv)
{
    u32 val;

    /* set the transmit base address */
    val = (ulong)(priv->tx_descs);

    emac_wr(priv, DMA_TRANSMIT_BASE_ADDRESS, val);

    debug("%s tx descriptor:0x%x\n", __func__,
           emac_rd(priv,DMA_TRANSMIT_BASE_ADDRESS));
    /* Tx Inter Packet Gap value and enable the transmit */
    val = emac_rd(priv, MAC_TRANSMIT_CONTROL);
    val &= (~MREGBIT_IFG_LEN);
    val |= MREGBIT_TRANSMIT_ENABLE;
    val |= MREGBIT_TRANSMIT_AUTO_RETRY;
    emac_wr(priv, MAC_TRANSMIT_CONTROL, val);

    emac_wr(priv, DMA_TRANSMIT_AUTO_POLL_COUNTER, 0x00);

    /* start tx dma */
    val = emac_rd(priv, DMA_CONTROL);
    val |= MREGBIT_START_STOP_TRANSMIT_DMA;
    emac_wr(priv, DMA_CONTROL, val);
}

static void emac_configure_rx(struct emac_priv *priv)
{
    u32 val;

    /* set the receive base address */
    val = (ulong)(priv->rx_descs);
    emac_wr(priv, DMA_RECEIVE_BASE_ADDRESS, val);

    debug("%s rx descriptor:0x%x\n", __func__,
            emac_rd(priv,DMA_RECEIVE_BASE_ADDRESS));
    /* enable the receive */
    val = emac_rd(priv, MAC_RECEIVE_CONTROL);
    val |= MREGBIT_RECEIVE_ENABLE;
    val |= MREGBIT_STORE_FORWARD;
    emac_wr(priv, MAC_RECEIVE_CONTROL, val);

    /* start rx dma */
    val = emac_rd(priv, DMA_CONTROL);
    val |= MREGBIT_START_STOP_RECEIVE_DMA;
    emac_wr(priv, DMA_CONTROL, val);
}

/* tx and rX descriptors are 16 bytes. This causes problems with the cache
 * maintenance on CPUs where the cache-line size exceeds the size of these
 * descriptors. What will happen is that when the driver receives a packet
 * it will be immediately requeued for the hardware to reuse. The CPU will
 * therefore need to flush the cache-line containing the descriptor, which
 * will cause all other descriptors in the same cache-line to be flushed
 * along with it. If one of those descriptors had been written to by the
 * device those changes (and the associated packet) will be lost.
 *
 * to work around this, we make use of non-cached memory if available. If
 * descriptors are mapped uncached there's no need to manually flush them
 * or invalidate them.
 *
 * note that this only applies to descriptors. The packet data buffers do
 * not have the same constraints since they are 1536 bytes large, so they
 * are unlikely to share cache-lines.
 */
static void *emac_alloc_descs(unsigned int num)
{
#ifdef CONFIG_SYS_NONCACHED_MEMORY
    return (void *)noncached_alloc(EQOS_DESCRIPTORS_SIZE,
                      EQOS_DESCRIPTOR_ALIGN);
#else
    return memalign(EQOS_DESCRIPTOR_ALIGN, EQOS_DESCRIPTORS_SIZE);
#endif
}

static void emac_free_descs(void *descs)
{
#ifdef CONFIG_SYS_NONCACHED_MEMORY
    /* FIXME: noncached_alloc() has no opposite */
#else
    free(descs);
#endif
}

static void emac_inval_desc(void *desc)
{
#ifndef CONFIG_SYS_NONCACHED_MEMORY
    unsigned long start = (unsigned long)desc & ~(ARCH_DMA_MINALIGN - 1);
    unsigned long end = ALIGN(start + EQOS_DESCRIPTOR_SIZE,
                  ARCH_DMA_MINALIGN);

    invalidate_dcache_range(start, end);
#endif
}

static void emac_flush_desc(void *desc)
{
#ifndef CONFIG_SYS_NONCACHED_MEMORY
    unsigned long start = (unsigned long)desc & ~(ARCH_DMA_MINALIGN - 1);
    unsigned long end = ALIGN(start + EQOS_DESCRIPTOR_SIZE,
                  ARCH_DMA_MINALIGN);
    flush_dcache_range(start, end);
#endif
}

static void emac_inval_buffer(void *buf, size_t size)
{
    unsigned long start = (unsigned long)buf & ~(ARCH_DMA_MINALIGN - 1);
    unsigned long end = ALIGN(start + size, ARCH_DMA_MINALIGN);

    invalidate_dcache_range(start, end);
}

static void emac_flush_buffer(void *buf, size_t size)
{
    unsigned long start = (unsigned long)buf & ~(ARCH_DMA_MINALIGN - 1);
    unsigned long end = ALIGN(start + size, ARCH_DMA_MINALIGN);

    flush_dcache_range(start, end);
}

bool emac_is_rmii(struct emac_priv *priv)
{
    return priv->phy_interface == PHY_INTERFACE_MODE_RMII;
}

static int emac_mdio_read(struct mii_dev *bus, int mdio_addr, 
                int mdio_devad, int mdio_reg)
{
    struct emac_priv *priv = bus->priv;
    u32 cmd = 0;
    u32 val;

    cmd |= mdio_addr & 0x1F;
    cmd |= (mdio_reg & 0x1F) << 5;
    cmd |= MREGBIT_START_MDIO_TRANS | MREGBIT_MDIO_READ_WRITE;

    emac_wr(priv, MAC_MDIO_DATA, 0x0);
    emac_wr(priv, MAC_MDIO_CONTROL, cmd);

    val = emac_rd(priv, MAC_MDIO_CONTROL);
    val = val >> 15 & 0x01;

    while (val != 0) {
        val = emac_rd(priv, MAC_MDIO_CONTROL);
        val = val >> 15 & 0x01;
        mdelay(5);
    }

    val = emac_rd(priv, MAC_MDIO_DATA);

    return val;
}

static int emac_mdio_write(struct mii_dev *bus, int mdio_addr, int mdio_devad,
                int mdio_reg, u16 mdio_val)
{
    struct emac_priv *priv = bus->priv;
    u32 val;
    u32 cmd = 0;

    emac_wr(priv, MAC_MDIO_DATA, mdio_val);

    cmd |= mdio_addr & 0x1F;
    cmd |= (mdio_reg & 0x1F) << 5;
    cmd |= MREGBIT_START_MDIO_TRANS;

    emac_wr(priv, MAC_MDIO_CONTROL, cmd);

    val = emac_rd(priv, MAC_MDIO_CONTROL);
    val = val >> 15 & 0x01;

    while (val != 0) {
        val = emac_rd(priv, MAC_MDIO_CONTROL);
        val = val >> 15 & 0x01;
    }
    return 0;
}

static int emac_adjust_link(struct udevice *dev)
{
    u32 ctrl;
    struct emac_priv *priv = dev_get_priv(dev);

    debug("%s(dev=%p):\n", __func__, dev);

    ctrl = emac_rd(priv, MAC_GLOBAL_CONTROL);

    if (priv->phy->duplex != priv->duplex) {
        if (!priv->phy->duplex)
            ctrl &= ~MREGBIT_FULL_DUPLEX_MODE;
        else
            ctrl |= MREGBIT_FULL_DUPLEX_MODE;

        priv->duplex = priv->phy->duplex;
    }

    if (priv->phy->speed != priv->speed) {
        ctrl &= ~MREGBIT_SPEED;
        switch (priv->phy->speed) {
        case SPEED_1000:
            ctrl |= MREGBIT_SPEED_1000M;
            break;
        case SPEED_100:
            ctrl |= MREGBIT_SPEED_100M;
            break;
        case SPEED_10:
            ctrl |= MREGBIT_SPEED_10M;
            break;
        }
        priv->speed = priv->phy->speed;
    }
    emac_wr(priv, MAC_GLOBAL_CONTROL, ctrl);
    printf("%s link:%d speed:%d duplex:%s\n",
           __func__, priv->phy->link,
           priv->phy->speed,
           priv->phy->duplex ? "full" : "half");

    return 0;
}

static int emac_write_hwaddr(struct udevice *dev)
{
    struct eth_pdata *plat = dev_get_plat(dev);
    struct emac_priv *priv = dev_get_priv(dev);

    /* This function may be called before start() or after stop(). At that
     * time, on at least some configurations of the EQoS HW, all clocks to
     * the EQoS HW block will be stopped, and a reset signal applied. If
     * any register access is attempted in this state, bus timeouts or CPU
     * hangs may occur. This check prevents that.
     *
     * A simple solution to this problem would be to not implement
     * write_hwaddr(), since start() always writes the MAC address into HW
     * anyway. However, it is desirable to implement write_hwaddr() to
     * support the case of SW that runs subsequent to U-Boot which expects
     * the MAC address to already be programmed into the EQoS registers,
     * which must happen irrespective of whether the U-Boot user (or
     * scripts) actually made use of the EQoS device, and hence
     * irrespective of whether start() was ever called.
     *
     * Note that this requirement by subsequent SW is not valid for
     * Tegra186, and is likely not valid for any non-PCI instantiation of
     * the EQoS HW block. This function is implemented solely as
     * future-proofing with the expectation the driver will eventually be
     * ported to some system where the expectation above is true.
     */
    /* Update the MAC address */
    emac_wr(priv, MAC_ADDRESS1_HIGH,
        (plat->enetaddr[1] << 8 | plat->enetaddr[0]));
    emac_wr(priv, MAC_ADDRESS1_MED,
        (plat->enetaddr[3] << 8 | plat->enetaddr[2]));
    emac_wr(priv, MAC_ADDRESS1_LOW,
        (plat->enetaddr[5] << 8 | plat->enetaddr[4]));

    return 0;
}

static int emac_phy_reset(struct emac_priv *priv)
{

#ifdef CONFIG_GPIO  /* gpio driver is not ready for fpga platform */
    int ret;

    ret = gpio_direction_output(priv->phy_reset_gpio, 1);
    if (ret < 0) {
        pr_err("gpio_direction_output(phy_reset, assert) failed: %d", ret);
        return ret;
    }

    udelay(2);

    ret = gpio_direction_output(priv->phy_reset_gpio, 0);
    if (ret < 0) {
        pr_err("gpio_direction_output(phy_reset, deassert) failed: %d", ret);
        return ret;
    }

    mdelay(10);

    ret = gpio_direction_output(priv->phy_reset_gpio, 1);
    if (ret < 0) {
        pr_err("gpio_direction_output(phy_reset, assert) failed: %d", ret);
        return ret;
    }
    mdelay(15);
#else
    void __iomem *reg;
    u32 reg_gbase = 0, reg_goff = 0, bit_no = 0;

    if (priv->phy_reset_gpio < 96) {
        reg_goff = (priv->phy_reset_gpio >> 5) * (0x4);
    } else {
        reg_goff = 0x100;
    }
    reg_gbase = 0xD4019000 + reg_goff;
    bit_no = (priv->phy_reset_gpio) & 0x1f;

    reg =  (void *)(ulong)(reg_gbase + 0xc);
    u32 val = readl(reg);
    val |= 1 << bit_no;
    writel(val, reg);

    udelay(2);

    reg =  (void *)(ulong)(reg_gbase + 0x18);
    val = readl(reg);
    val |= 1 << bit_no;
    writel(val, reg);

    mdelay(10);
    reg =  (void *)(ulong)(reg_gbase + 0x24);
    val = readl(reg);
    val |= 1 << bit_no;
    writel(val, reg);

    mdelay(15);
    reg =  (void *)(ulong)(reg_gbase + 0x18);
    val = readl(reg);
    val |= 1 << bit_no;
    writel(val, reg);

#endif
    return 0;
}

static int emac_start(struct udevice *dev)
{
    struct emac_priv *priv = dev_get_priv(dev);
    int ret, i;

    debug("%s(dev=%p):\n", __func__, dev);

    priv->tx_desc_idx = 0;
    priv->rx_desc_idx = 0;

    emac_phy_reset(priv);

    priv->phy = phy_connect(priv->mii, priv->phy_addr, dev,
                    priv->phy_interface);
    if (!priv->phy) {
        ret = -1;
        pr_err("phy_connect() failed");
        goto err_connect_phy;
    }

    if (emac_is_rmii(priv))
        priv->phy->supported &= ~(SUPPORTED_1000baseT_Half | SUPPORTED_1000baseT_Full);

    ret = phy_config(priv->phy);
    if (ret < 0) {
        pr_err("phy_config() failed: %d", ret);
        goto err_shutdown_phy;
    }

    ret = phy_startup(priv->phy);
    if (ret < 0) {
        pr_err("phy_startup() failed: %d", ret);
        goto err_shutdown_phy;
    }

    if (!priv->phy->link) {
        pr_err("No link");
        goto err_shutdown_phy;
    }

    ret = emac_adjust_link(dev);
    if (ret < 0) {
        pr_err("emac_adjust_link() failed: %d", ret);
        goto err_shutdown_phy;
    }

    /* Set up descriptors */
    memset(priv->descs, 0, EQOS_DESCRIPTORS_SIZE);
    for (i = 0; i < EQOS_DESCRIPTORS_RX; i++) {
        struct emac_desc *rx_desc = &priv->rx_descs[i];
        rx_desc->des2 = (u32)(ulong)(priv->rx_dma_buf +
                    (i * EQOS_MAX_PACKET_SIZE));
        rx_desc->des1 = EQOS_MAX_PACKET_SIZE & 0xFFF;

        if (i == (EQOS_DESCRIPTORS_RX - 1))
            rx_desc->des1 |= EMAC_DESC_EOR;

        rx_desc->des0 |= EMAC_DESC_OWN;
        if (!((i+1) % CACHE_FLUSH_CNT))
            emac_flush_desc(rx_desc);
    }

    emac_inval_buffer(priv->rx_dma_buf, EQOS_RX_BUFFER_SIZE);

    emac_init_hw(priv);

    emac_write_hwaddr(dev);

    emac_configure_tx(priv);

    emac_configure_rx(priv);

    priv->started = true;
    return 0;

err_shutdown_phy:
    phy_shutdown(priv->phy);
    priv->phy = NULL;
err_connect_phy:
    pr_err("FAILED: %d", ret);
    return ret;
}

void emac_stop(struct udevice *dev)
{
    struct emac_priv *priv = dev_get_priv(dev);

    debug("%s(dev=%p):\n", __func__, dev);

    if (!priv->started)
        return;
    priv->started = false;

    emac_reset_hw(priv);
    if (priv->phy)
        phy_shutdown(priv->phy);

    priv->speed = -1;
    priv->duplex = -1;
}

int emac_send(struct udevice *dev, void *packet, int length)
{
    struct emac_priv *priv = dev_get_priv(dev);
    struct emac_desc *tx_desc;
    int i;

    debug("%s(dev=%p, packet=%p, length=%d):\n", __func__, dev, packet,
          length);

    memcpy(priv->tx_dma_buf, packet, length);
    emac_flush_buffer(priv->tx_dma_buf, length);

    tx_desc = &priv->tx_descs[priv->tx_desc_idx];
    priv->tx_desc_idx++;
    priv->tx_desc_idx %= EQOS_DESCRIPTORS_TX;

    memset(tx_desc, 0x0, sizeof(struct emac_desc));

    tx_desc->des2 = (ulong)priv->tx_dma_buf;
    tx_desc->des1 = EMAC_DESC_BUFF_SIZE1 & length;
    tx_desc->des1 |= EMAC_DESC_FD | EMAC_DESC_LD;

    if (priv->tx_desc_idx == 0)
        tx_desc->des1 |= EMAC_DESC_EOR;

    /* Make sure that if HW sees the _OWN emac_wr below, it will see all the
     * writes to the rest of the descriptor too.
     */
    mb();
    tx_desc->des0 = EMAC_DESC_OWN;
    emac_flush_desc(tx_desc);

    emac_wr(priv, DMA_TRANSMIT_POLL_DEMAND, 0xFF);

    for (i = 0; i < 1000; i++) {
        emac_inval_desc(tx_desc);
        if (!(readl(&tx_desc->des0) & EMAC_DESC_OWN))
            return 0;
        mdelay(1);
    }

    printf("%s: TX timeout\n", __func__);

    return -ETIMEDOUT;
}

int emac_recv(struct udevice *dev, int flags, uchar **packetp)
{
    struct emac_priv *priv = dev_get_priv(dev);
    struct emac_desc *rx_desc;
    int length;

    rx_desc = &priv->rx_descs[priv->rx_desc_idx];

    emac_inval_desc(rx_desc);

    if (rx_desc->des0 & EMAC_DESC_OWN) {
        debug("%s: RX packet not available\n", __func__);
        return -EAGAIN;
    }

    *packetp = priv->rx_dma_buf +
        (priv->rx_desc_idx * EQOS_MAX_PACKET_SIZE);
    /* use frame length */
    if (rx_desc->des0 & EMAC_DESC_LD)
        length = (rx_desc->des0 & 0x3fff) - ETHERNET_FCS_SIZE;
    else
        length = EQOS_MAX_PACKET_SIZE;

    emac_inval_buffer(*packetp, length);
    return length;
}

int emac_free_pkt(struct udevice *dev, uchar *packet, int length)
{
    struct emac_priv *priv = dev_get_priv(dev);
    uchar *packet_expected;
    struct emac_desc *rx_desc;
    int desc_idx;

    debug("%s(packet=%p, length=%d)\n", __func__, packet, length);

    packet_expected = priv->rx_dma_buf +
        (priv->rx_desc_idx * EQOS_MAX_PACKET_SIZE);
    if (packet != packet_expected) {
        printf("%s: Unexpected packet (expected %p)\n", __func__,
               packet_expected);
        return -EINVAL;
    }

    emac_inval_buffer((void *)packet, length);

    if (!((priv->rx_desc_idx + 1) % CACHE_FLUSH_CNT)) {
        for (desc_idx = (priv->rx_desc_idx + 1 - CACHE_FLUSH_CNT); desc_idx <= priv->rx_desc_idx; desc_idx++) {
            rx_desc = &priv->rx_descs[desc_idx];
            memset(rx_desc, 0x0, sizeof(struct emac_desc));

            rx_desc->des1 = EQOS_MAX_PACKET_SIZE & 0xFFF;
            if (desc_idx == (EQOS_DESCRIPTORS_RX - 1))
                rx_desc->des1 |= EMAC_DESC_EOR;

            rx_desc->des2 = (u32)(ulong)(priv->rx_dma_buf +
                    (desc_idx * EQOS_MAX_PACKET_SIZE));

            /* Make sure that if HW sees the _OWN write below, it will see all the
             * writes to the rest of the descriptor too.
             */
            mb();
            rx_desc->des0 |= EMAC_DESC_OWN;
        }

        emac_flush_desc(rx_desc);
        emac_wr(priv, DMA_RECEIVE_POLL_DEMAND, 0xFF);
    }
    priv->rx_desc_idx++;

    priv->rx_desc_idx %= EQOS_DESCRIPTORS_RX;
    return 0;
}

void emac_enable_axi_single_id_mode(struct emac_priv *priv, int en)
{
    u32 val;

    val = readl(priv->ctrl_reg);
    if (en)
        val |= AXI_SINGLE_ID;
    else
        val &= ~AXI_SINGLE_ID;
    writel(val, priv->ctrl_reg);
}

int emac_phy_interface_select(struct emac_priv *priv)
{
    u32 val;

    val = readl(priv->ctrl_reg);
    if (emac_is_rmii(priv)) {
        printf("RMII interface\n");
        val &= ~EMAC_PHY_SEL_RGMII;
        if (priv->ref_clk_frm_soc)
            val |= REF_CLK_SEL;
        else
            val &= ~REF_CLK_SEL;
    } else {
        printf("RGMII interface\n");
        val |= EMAC_PHY_SEL_RGMII;
        if (priv->ref_clk_frm_soc)
            val |= RGMII_TX_CLK_SEL;
        else
            val &= ~RGMII_TX_CLK_SEL;
    }
    writel(val, priv->ctrl_reg);
    return 0;
}

int emac_enable_clk(struct emac_priv *priv)
{
    /* enable mac clock */
    clk_enable(&priv->mac_clk);
    reset_deassert(&priv->reset);

#if 0   /* CLK driver is not ready on fpga platform */
    /* enable phy clock */
    clk_enable_pll(PLL1, DIV_8);
    clk_enable_gate(CLK_104);
#endif
    return 0;
}

int emac_disable_clk(struct emac_priv *priv)
{
    /* disable mac clock */
    reset_assert(&priv->reset);
    clk_disable(&priv->mac_clk);

#if 0   /* CLK driver is not ready on fpga platform */
    /* disable phy clock */
    clk_disable_gate(CLK_104);
    clk_disable_pll(PLL1, DIV_8);
#endif
    return 0;
}

static int emac_bind(struct udevice *bus)
{
    return 0;
}

static int emac_probe_resources_core(struct udevice *dev)
{
    struct emac_priv *priv = dev_get_priv(dev);
    int ret;

    debug("%s(dev=%p):\n", __func__, dev);

    priv->descs = emac_alloc_descs(EQOS_DESCRIPTORS_TX +
                       EQOS_DESCRIPTORS_RX);
    if (!priv->descs) {
        debug("%s: emac_alloc_descs() failed\n", __func__);
        ret = -ENOMEM;
        goto err;
    }
    priv->tx_descs = (struct emac_desc *)priv->descs;
    priv->rx_descs = (priv->tx_descs + EQOS_DESCRIPTORS_TX);
    debug("%s: tx_descs=%p, rx_descs=%p\n", __func__, priv->tx_descs,
          priv->rx_descs);

    priv->tx_dma_buf = memalign(EQOS_BUFFER_ALIGN, EQOS_MAX_PACKET_SIZE);
    if (!priv->tx_dma_buf) {
        debug("%s: memalign(tx_dma_buf) failed\n", __func__);
        ret = -ENOMEM;
        goto err_free_descs;
    }
    debug("%s: tx_dma_buf=%p\n", __func__, priv->tx_dma_buf);

    priv->rx_dma_buf = memalign(EQOS_BUFFER_ALIGN, EQOS_RX_BUFFER_SIZE);
    if (!priv->rx_dma_buf) {
        debug("%s: memalign(rx_dma_buf) failed\n", __func__);
        ret = -ENOMEM;
        goto err_free_tx_dma_buf;
    }
    debug("%s: rx_dma_buf=%p\n", __func__, priv->rx_dma_buf);

    debug("%s: OK\n", __func__);
    return 0;

err_free_tx_dma_buf:
    free(priv->tx_dma_buf);
err_free_descs:
    emac_free_descs(priv->descs);
err:
    debug("%s: returns %d\n", __func__, ret);
    return ret;
}

static int emac_remove_resources_core(struct udevice *dev)
{
    struct emac_priv *priv = dev_get_priv(dev);

    debug("%s(dev=%p):\n", __func__, dev);

    free(priv->rx_dma_buf);
    free(priv->tx_dma_buf);
    emac_free_descs(priv->descs);

    debug("%s: OK\n", __func__);
    return 0;
}

static int emac_eth_ofdata_to_platdata(struct udevice *dev)
{
    struct eth_pdata *pdata = dev_get_plat(dev);
    struct emac_priv *priv = dev_get_priv(dev);
    u32 ctrl_reg;

    pdata->iobase = devfdt_get_addr(dev);
    /* Interface mode is required */
    pdata->phy_interface = dev_read_phy_mode(dev);
    priv->phy_interface = pdata->phy_interface;
    if (pdata->phy_interface == PHY_INTERFACE_MODE_NA) {
        printf("error: phy-mode is not set\n");
        return -ENODEV;
    }

    priv->phy_addr = dev_read_u32_default(dev, "phy-addr", 0);
    priv->ref_clk_frm_soc = !dev_read_bool(dev, "ref-clock-from-phy");

    ctrl_reg = dev_read_u32_default(dev, "ctrl-reg", 0);
    if (!ctrl_reg) {
        printf("ethernet ctrl_reg NOT CONFIG!!!!\n");
        return -EINVAL;
    }

    priv->ctrl_reg = (void *)((ulong)(K1X_APMU_BASE + ctrl_reg));

    priv->phy_reset_gpio = dev_read_s32_default(dev, "phy-reset-pin", -1);
    if(priv->phy_reset_gpio < 0)
        goto err_tuning_gpio;

    priv->ldo_gpio = dev_read_s32_default(dev, "ldo-pwr-pin", -1);

    priv->clk_tuning_enable = dev_read_bool(dev, "clk_tuning_enable");
    if (priv->clk_tuning_enable) {
        if (dev_read_bool(dev, "clk-tuning-by-reg"))
            priv->clk_tuning_way = CLK_TUNING_BY_REG;
        else if (dev_read_bool(dev, "clk-tuning-by-clk-revert"))
            priv->clk_tuning_way = CLK_TUNING_BY_CLK_REVERT;
        else if (dev_read_bool(dev, "clk-tuning-by-delayline")) {
            priv->clk_tuning_way = CLK_TUNING_BY_DLINE;
            ctrl_reg = dev_read_u32_default(dev, "dline-reg", 0);
            if (!ctrl_reg) {
                printf("ethernet dline_reg NOT CONFIG!!!!\n");
                return -EINVAL;
            }
            priv->dline_reg = (void *)((ulong)(K1X_APMU_BASE + ctrl_reg));
        } else
            priv->clk_tuning_way = CLK_TUNING_BY_REG;

        priv->tx_phase = dev_read_u32_default(dev, "tx-phase", 0);
        priv->rx_phase = dev_read_u32_default(dev, "rx-phase", 0);

        debug("tx_phase:%d  rx_phase:%d clk_tuning:%d\n",
              priv->tx_phase, priv->rx_phase, priv->clk_tuning_enable);
    }
    return 0;
err_tuning_gpio:
    printf("error: gpio get failed from dts\n");
    return -EINVAL;
}

static int clk_phase_rgmii_set(struct emac_priv *priv, bool is_tx)
{
    u32 val;

    switch (priv->clk_tuning_way) {
    case CLK_TUNING_BY_REG:
        val = readl(priv->ctrl_reg);
        if (is_tx) {
            val &= ~RGMII_TX_PHASE_MASK;
            val |= (priv->tx_phase & 0x7) << RGMII_TX_PHASE_OFFSET;
        } else {
            val &= ~RGMII_RX_PHASE_MASK;
            val |= (priv->rx_phase & 0x7) << RGMII_RX_PHASE_OFFSET;
        }
        writel(val, priv->ctrl_reg);
        break;
    case CLK_TUNING_BY_DLINE:
        val = readl(priv->dline_reg);
        if (is_tx) {
            val &= ~EMAC_TX_DLINE_CODE_MASK;
            val |= priv->tx_phase << EMAC_TX_DLINE_CODE_OFFSET;
            val |= EMAC_TX_DLINE_EN;
        } else {
            val &= ~EMAC_RX_DLINE_CODE_MASK;
            val |= priv->rx_phase << EMAC_RX_DLINE_CODE_OFFSET;
            val |= EMAC_RX_DLINE_EN;
        }
        writel(val, priv->dline_reg);
        break;
    default:
        printf("wrong clk tuning way:%d !!\n", priv->clk_tuning_way);
        return -1;
    }
    debug("%s tx phase:%d rx phase:%d\n",
        __func__, priv->tx_phase, priv->rx_phase);
    return 0;
}

static int clk_phase_rmii_set(struct emac_priv *priv, bool is_tx)
{
    u32 val;

    switch (priv->clk_tuning_way) {
    case CLK_TUNING_BY_REG:
        val = readl(priv->ctrl_reg);
        if (is_tx) {
            val &= ~RMII_TX_PHASE_MASK;
            val |= (priv->tx_phase & 0x7) << RMII_TX_PHASE_OFFSET;
        } else {
            val &= ~RMII_RX_PHASE_MASK;
            val |= (priv->rx_phase & 0x7) << RMII_RX_PHASE_OFFSET;
        }
        writel(val, priv->ctrl_reg);
        break;
    case CLK_TUNING_BY_CLK_REVERT:
        val = readl(priv->ctrl_reg);
        if (is_tx) {
            if (priv->tx_phase == CLK_PHASE_REVERT)
                val |= RMII_TX_CLK_SEL;
            else
                val &= ~RMII_TX_CLK_SEL;
        } else {
            if (priv->rx_phase == CLK_PHASE_REVERT)
                val |= RMII_RX_CLK_SEL;
            else
                val &= ~RMII_RX_CLK_SEL;
        }
        writel(val, priv->ctrl_reg);
        break;
    default:
        printf("wrong clk tuning way:%d !!\n", priv->clk_tuning_way);
        return -1;
    }
    debug("%s tx phase:%d rx phase:%d\n",
        __func__, priv->tx_phase, priv->rx_phase);
    return 0;
}

static int emac_set_clock_phase(struct udevice *dev, int is_tx)
{
    struct emac_priv *priv = dev_get_priv(dev);

    if (priv->clk_tuning_enable) {
        if (emac_is_rmii(priv))
            clk_phase_rmii_set(priv, is_tx);
        else
            clk_phase_rgmii_set(priv, is_tx);
    }

    return 0;
}

static int emac_probe(struct udevice *dev)
{
    struct emac_priv *priv = dev_get_priv(dev);
    struct eth_pdata *pdata = dev_get_plat(dev);
    int ret;

    debug("%s(dev=%p):\n", __func__, dev);
    priv->dev = dev;

    priv->io_base = (void *)(pdata->iobase);

    ret = emac_probe_resources_core(dev);
    if (ret < 0) {
        pr_err("emac_probe_resources_core() failed: %d", ret);
        return ret;
    }

    ret = clk_get_by_index(dev, 0, &priv->mac_clk);
    if (ret) {
        pr_err("It has no clk: %d\n", ret);
        return ret;
    }

    ret = reset_get_by_index(dev, 0, &priv->reset);
    if (ret) {
        pr_err("It has no reset: %d\n", ret);
        return ret;
    }

    emac_enable_clk(priv);

    priv->mii = mdio_alloc();
    if (!priv->mii) {
        pr_err("mdio_alloc() failed");
        ret = -ENOMEM;
        goto err_remove_resources_core;
    }
    priv->mii->read = emac_mdio_read;
    priv->mii->write = emac_mdio_write;
    priv->mii->priv = priv;
    strncpy(priv->mii->name, dev->name, MDIO_NAME_LEN - 1);

    ret = mdio_register(priv->mii);
    if (ret < 0) {
        pr_err("mdio_register() failed: %d", ret);
        goto err_free_mdio;
    }

    emac_phy_interface_select(priv);

    emac_enable_axi_single_id_mode(priv, 1);

#ifdef CONFIG_GPIO  /* gpio driver is not ready for fpga platform! */
    ret = gpio_request(priv->phy_reset_gpio, "phy-reset-pin");
    if (ret < 0) {
        pr_err("gpio_request_by_name(phy reset) failed: %d", ret);
        goto err_free_mdio;
    }

    if (priv->ldo_gpio >= 0) {
        ret = gpio_request(priv->ldo_gpio, "ldo-pwr-pin");
        if (ret < 0) {
            pr_err("gpio_request_by_name(ldo pwr) failed: %d", ret);
            goto err_free_gpio;
        }
        gpio_direction_output(priv->ldo_gpio, 1);
    }
#endif
    if (priv->clk_tuning_enable) {
        emac_set_clock_phase(dev, TX_PHASE);
        emac_set_clock_phase(dev, RX_PHASE);
    }
    debug("%s: OK\n", __func__);
    return 0;

#ifdef CONFIG_GPIO  /* gpio driver is not ready for fpga platform! */
err_free_gpio:
    gpio_free(priv->phy_reset_gpio);
#endif
err_free_mdio:
    mdio_free(priv->mii);
err_remove_resources_core:
    emac_disable_clk(priv);
    emac_remove_resources_core(dev);

    debug("%s: returns %d\n", __func__, ret);
    return ret;
}

static int emac_remove(struct udevice *dev)
{
    struct emac_priv *priv = dev_get_priv(dev);

    debug("%s(dev=%p):\n", __func__, dev);

    emac_disable_clk(priv);
    mdio_unregister(priv->mii);
    mdio_free(priv->mii);
    emac_remove_resources_core(dev);

    return 0;
}

static const struct eth_ops emac_ops = {
    .start = emac_start,
    .stop = emac_stop,
    .send = emac_send,
    .recv = emac_recv,
    .free_pkt = emac_free_pkt,
    .write_hwaddr = emac_write_hwaddr,
};

static const struct udevice_id emac_ids[] = {
    {
        .compatible = "spacemit,k1x-emac",
    },
    { }
};

U_BOOT_DRIVER(k1x_emac) = {
    .name = "k1x_emac",
    .id = UCLASS_ETH,
    .of_match = emac_ids,
    .of_to_plat = emac_eth_ofdata_to_platdata,
    .probe = emac_probe,
    .remove = emac_remove,
    .bind = emac_bind,
    .ops = &emac_ops,
    .priv_auto = sizeof(struct emac_priv),
    .plat_auto = sizeof(struct eth_pdata),
};
