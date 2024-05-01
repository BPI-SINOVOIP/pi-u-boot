// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */


#include<malloc.h>
#include<common.h>
#include<dma.h>
#include<dma-uclass.h>
#include<cpu_func.h>
#include<asm/io.h>
#include<dm/device.h>
#include<dm.h>
#include<dm/ofnode.h>
#include<dm/uclass.h>
#include<linux/kernel.h>
#include"axi-dma.h"

static int axi_dma_transfer(struct udevice *dev,int direction,void *dst,void *src,size_t len)
{
	struct axi_dma_dev *ud = dev_get_priv(dev);
	struct axi_dma_chan *uc = ud->axi_chan[0];

	u32 data_width,burst_axi_len,burst_len;
	u32 reg = 0;
	u64 block_ts;
	data_width = DEF_WIDTH;
	burst_len = DWAXIDMAC_BURST_TRANS_LEN_4;

	block_ts = len>>data_width;
	if(block_ts >= ud->max_block_ts) {
		block_ts = ud->max_block_ts;
		printf("transfer size too large\n");
	}

	/*write dma_chan_ctl register*/
	reg = CH_CTL_H_LLI_VALID|CH_CTL_H_LLI_LAST;
	burst_axi_len = DEF_AXI_BURST_LEN;
	reg |= (CH_CTL_H_ARLEN_EN |
		burst_axi_len << CH_CTL_H_ARLEN_POS |
		CH_CTL_H_AWLEN_EN |
		burst_axi_len << CH_CTL_H_AWLEN_POS);
	writel(reg,(volatile void __iomem*)&uc->regs->ctl_hi);
	reg = (burst_len << CH_CTL_L_DST_MSIZE_POS |
		burst_len << CH_CTL_L_SRC_MSIZE_POS |
		data_width << CH_CTL_L_DST_WIDTH_POS |
		data_width << CH_CTL_L_SRC_WIDTH_POS |
		DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_DST_INC_POS |
		DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_SRC_INC_POS);
	writel(reg,(volatile void __iomem*)&uc->regs->ctl_lo);

	/*write dma_chan_cfg register*/
	reg = (DWAXIDMAC_TT_FC_MEM_TO_MEM_DMAC << CH_CFG_H_TT_FC_POS |
			DWAXIDMAC_HS_SEL_HW << CH_CFG_H_HS_SEL_DST_POS |
			DWAXIDMAC_HS_SEL_HW << CH_CFG_H_HS_SEL_SRC_POS);
	writel(reg,(volatile void __iomem*)&uc->regs->cfg_hi);

	/*write dma sar,dar,block_ts*/
	writeq((u64)src,(volatile void __iomem*)&uc->regs->sar);
	writeq((u64)dst,(volatile void __iomem*)&uc->regs->dar);
	writeq(block_ts-1,(volatile void __iomem*)&uc->regs->block_ts);
	return 0;
}

static int axi_dma_of_xlate(struct dma *dma,struct ofnode_phandle_args *args)
{
	struct axi_dma_dev *ud = dev_get_priv(dma->dev);
	struct axi_dma_chan *uc;

	dma->id = args->args[0];
	uc = ud->axi_chan[dma->id];
	uc->config->hs_if = args->args[1];
	uc->config->burst_len = args->args[2];
	return 0;
}

static int axi_dma_enable(struct dma* dma)
{
	struct axi_dma_dev *ud = dev_get_priv(dma->dev);
	struct axi_dma_chan *uc = ud->axi_chan[dma->id];
	u32 val;

	if(uc->status == UNUSED)
		printf("channel %ld has nothing to transfer\n",dma->id);
	writel(0,&uc->regs->int_en);
	val = readl(ud->iorebase+DMAC_CHEN);
	val |= BIT(dma->id);
	val |= BIT(dma->id)<<DMAC_CHAN_EN_WE_SHIFT;
	writel(val,ud->iorebase+DMAC_CHEN);
	while(1) {
		val = readl(ud->iorebase+DMAC_CHEN);
		if(val&&BIT(dma->id) == 0){
			uc->status = UNUSED;
			printf("dma_transfer complete\n");
		}
		break;
	}

	return 0;
}

static int axi_dma_disable(struct dma* dma)
{
	u32 val;
	struct axi_dma_dev *ud = dev_get_priv(dma->dev);

	/*disable dma_channel*/
	val = readl(ud->iorebase+DMAC_CHAN_EN_SHIFT);
	val &= ~BIT(dma->id);
	val |= BIT(dma->id)<<DMAC_CHAN_EN_WE_SHIFT;
	writel(val,ud->iorebase+DMAC_CHAN_EN_SHIFT);

	/*disable dma*/
	val = readl(ud->iorebase+DMAC_CFG);
	val &= ~DMAC_EN_MASK;
	writel(val,ud->iorebase+DMAC_CFG);
	return 0;
}

static int axi_dma_alloc_chan(struct axi_dma_dev *ud)
{
	int i;

	/*find a idle chan*/
	for(i = 1;i < DMA_CHAN_NUM;i++) {
		if(ud->axi_chan[i]->status == UNUSED) {
			ud->axi_chan[i]->status = USED;
			return i;
		}
	}
	return 0;
}
static int axi_dma_request(struct dma *dma)
{
	struct axi_dma_dev *ud = dev_get_priv(dma->dev);
	struct axi_dma_chan *uc = ud->axi_chan[dma->id];

	/*alloc chan0 for memcpy*/
	if(dma->id == 0) {
		if(uc->status == UNUSED) {
			printf("channel %ld is ready\n",dma->id);
			uc->status = USED;
			return 0;
		}
		else {
			printf("channel %ld is busy\n",dma->id);
			return -1;
		}
	}
	else {
		/*alloc requested channel ,if busy ,return another*/
		if(uc->status == UNUSED) {
			printf("channel %ld is ready\n",dma->id);
			uc->status = USED;
			return 0;
		}
		else {
			int i = axi_dma_alloc_chan(ud);
			if(dma->id > 0) {
				printf("channel %ld is busy,change to channel %d\n",dma->id,i);
				dma->id = (u64)i;
				return 0;
			}
			else
				printf("all chan busy\n");
		}
	}
	return -1;
}

static int axi_dma_rfree(struct dma* dma)
{
	struct axi_dma_dev *ud = dev_get_priv(dma->dev);
	struct axi_dma_chan *uc = ud->axi_chan[dma->id];

	u32 val;
	/*disable dma_chan set use flag to unused*/
	writel(0,(volatile void __iomem*)&uc->regs->int_en);
	val = readl(ud->iorebase+DMAC_CHAN_EN_SHIFT);
	val &= ~BIT(dma->id);
	val |= BIT(dma->id)<<DMAC_CHAN_EN_WE_SHIFT;
	writel(val,(volatile void __iomem*)ud->iorebase+DMAC_CHAN_EN_SHIFT);
	uc->status = UNUSED;

	return 0;
}

static int axi_dma_send(struct dma* dma,void *src,size_t len,void *metadata)
{
	struct axi_dma_dev *ud = dev_get_priv(dma->dev);
	struct axi_dma_chan *uc = ud->axi_chan[dma->id];
	u32 data_width,burst_len,burst_axi_len;
	u32 reg = 0;
	u64 block_ts;
	data_width = DEF_WIDTH;
	block_ts = len>>data_width;

	if(uc->status!=USED)
		return -1;

	/*write back data in tx_buffer*/
	flush_cache((u64)src,len);
	block_ts = len>>data_width;
	if(block_ts >= ud->max_block_ts) {
		block_ts = ud->max_block_ts;
		printf("transfer size too large\n");
	}

	/*write dma_chan_ctl register*/
	if(!uc->config->burst_len)
		burst_len = DWAXIDMAC_BURST_TRANS_LEN_4;
	else
		burst_len = uc->config->burst_len;
	reg = CH_CTL_H_LLI_VALID|CH_CTL_H_LLI_LAST;
	reg |= (CH_CTL_H_ARLEN_EN |
			burst_axi_len << CH_CTL_H_ARLEN_POS |
			CH_CTL_H_AWLEN_EN |
			burst_axi_len << CH_CTL_H_AWLEN_POS);
	writel(reg,(volatile void __iomem*)&uc->regs->ctl_hi);
	reg = (burst_len << CH_CTL_L_DST_MSIZE_POS |
			burst_len << CH_CTL_L_SRC_MSIZE_POS |
			data_width << CH_CTL_L_DST_WIDTH_POS |
			data_width << CH_CTL_L_SRC_WIDTH_POS |
			DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_DST_INC_POS |
			DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_SRC_INC_POS);
	writel(reg,(volatile void __iomem*)&uc->regs->ctl_lo);

	/*write dma_chan_cfg register*/
	reg = (DWAXIDMAC_TT_FC_MEM_TO_PER_DMAC << CH_CFG_H_TT_FC_POS |
			DWAXIDMAC_HS_SEL_HW << CH_CFG_H_HS_SEL_DST_POS |
			DWAXIDMAC_HS_SEL_HW << CH_CFG_H_HS_SEL_SRC_POS) |
			uc->config->hs_if << CH_CFG2_L_DST_PER_POS;
	writel(reg,(volatile void __iomem*)&uc->regs->cfg_hi);

	/*write dma sar,dar,block_ts*/
	writeq((u64)src,(volatile void __iomem*)&uc->regs->sar);
	writeq((u64)metadata,(volatile void __iomem*)&uc->regs->dar);
	writeq(block_ts-1,(volatile void __iomem*)&uc->regs->block_ts);

	return 0;
}

static int axi_dma_receive(struct dma *dma, void **dst,void *metadata)
{
	struct axi_dma_dev *ud = dev_get_priv(dma->dev);
	struct axi_dma_chan *uc = ud->axi_chan[dma->id];
	u32 data_width,burst_len,burst_axi_len,len;
	u32 reg = 0;
	u64 block_ts;
	data_width = DEF_WIDTH;

	len = uc->info->buf_len;
	*dst = (void*)uc->info->address;
	u64 dar = (u64)*dst;
	if(uc->status!=USED)
		return -1;

	/*invalid receive buffer cache*/
	invalidate_dcache_range(dar,dar+len);
	block_ts = len>>data_width;
	if(block_ts >= ud->max_block_ts) {
		block_ts = ud->max_block_ts;
		printf("transfer size too large\n");
	}

	/*write dma_chan_ctl register*/
	if(!uc->config->burst_len)
		burst_len = DWAXIDMAC_BURST_TRANS_LEN_4;
	else
		burst_len = uc->config->burst_len;
	reg = CH_CTL_H_LLI_VALID|CH_CTL_H_LLI_LAST;
	reg |= (CH_CTL_H_ARLEN_EN |
			burst_axi_len << CH_CTL_H_ARLEN_POS |
			CH_CTL_H_AWLEN_EN |
			burst_axi_len << CH_CTL_H_AWLEN_POS);
	writel(reg,(volatile void __iomem*)&uc->regs->ctl_hi);
	reg = (burst_len << CH_CTL_L_DST_MSIZE_POS |
			burst_len << CH_CTL_L_SRC_MSIZE_POS |
			data_width << CH_CTL_L_DST_WIDTH_POS |
			data_width << CH_CTL_L_SRC_WIDTH_POS |
			DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_DST_INC_POS |
			DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_SRC_INC_POS);
	writel(reg,(volatile void __iomem*)&uc->regs->ctl_lo);

	/*write dma_chan_cfg register*/
	reg = (DWAXIDMAC_TT_FC_PER_TO_MEM_DMAC << CH_CFG_H_TT_FC_POS |
			DWAXIDMAC_HS_SEL_HW << CH_CFG_H_HS_SEL_DST_POS |
			DWAXIDMAC_HS_SEL_HW << CH_CFG_H_HS_SEL_SRC_POS) |
			uc->config->hs_if << CH_CFG2_L_SRC_PER_POS;
	writel(reg,(volatile void __iomem*)&uc->regs->cfg_hi);

	/*write dma sar,dar,block_ts*/
	writeq((u64)metadata,(volatile void __iomem*)&uc->regs->sar);
	writeq(dar,(volatile void __iomem*)&uc->regs->dar);
	writeq(block_ts-1,(volatile void __iomem*)&uc->regs->block_ts);

	return 0;
}

static int axi_dma_prepare_rcv_buf(struct dma *dma,void *dst,size_t size)
{
	struct axi_dma_dev *ud = dev_get_priv(dma->dev);
	struct axi_dma_chan *uc = ud->axi_chan[dma->id];

	/*record info of rcv_buf*/
	uc->info->address = (u64)dst;
	uc->info->type = RX;
	uc->info->buf_len = size;

	/*invalid receive buffer cache*/
	invalidate_dcache_range((u64)dst,(u64)(dst+size));

	return 0;
}

static const struct dma_ops axi_dma_ops={
	.transfer = axi_dma_transfer,
	.of_xlate = axi_dma_of_xlate,
	.request = axi_dma_request,
	.rfree = axi_dma_rfree,
	.enable = axi_dma_enable,
	.disable = axi_dma_disable,
	.send = axi_dma_send,
	.receive = axi_dma_receive,
	.prepare_rcv_buf = axi_dma_prepare_rcv_buf,
};

#define CHAN_REG_SHIFT 0x100

static int axi_dma_probe(struct udevice *dev)
{
	struct dma_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct axi_dma_dev *ud = dev_get_priv(dev);
	u32 val = 0;
	u32 i,ret;

	ud->iorebase = dev_read_addr_ptr(dev);
	ud->max_block_ts = dev_read_u32_default(dev,"max_block_ts",0x400000);
	ud->n_channels = dev_read_u32_default(dev,"dma-channels",8);

	ret = reset_get_by_index(dev, 0, &ud->reset);
	if (ret)
		return ret;
	ret = reset_deassert(&ud->reset);
	if (ret) {
		pr_err("Failed to de-assert reset for DMA (error %d)\n",ret);
		return ret; 
	}

	ret = clk_get_by_index(dev,0,&ud->clk);
	if(ret)
		return ret;
	ret = clk_enable(&ud->clk);
	if (ret && ret != -ENOSYS && ret != -ENOTSUPP)
		return ret;

	val = readl(ud->iorebase+DMAC_CFG);
	val |= DMAC_EN_MASK;
	val &= ~INT_EN_MASK;
	writeq(val,ud->iorebase+DMAC_CFG);
	for(i=0;i<DMA_CHAN_NUM;i++) {
		ud->axi_chan[i]=malloc(sizeof(struct axi_dma_chan));
		ud->axi_chan[i]->regs=ud->iorebase+(i+1)*CHAN_REG_SHIFT;
		ud->axi_chan[i]->status = UNUSED;
	}
	uc_priv->supported = DMA_SUPPORTS_MEM_TO_MEM |
				 DMA_SUPPORTS_MEM_TO_DEV |
				 DMA_SUPPORTS_DEV_TO_MEM;

	return 0;
}

static int axi_dma_remove(struct udevice *dev)
{
	struct axi_dma_dev *ud = dev_get_priv(dev);
	int ret;

	ret = reset_assert(&ud->reset);
	if (ret)
		return ret;

#if CONFIG_IS_ENABLED(CLK)
	ret = clk_disable(&ud->clk);
	if (ret)
		return ret;

	clk_free(&ud->clk);
	if (ret)
		return ret;
#endif
	return 0;
}

static const struct udevice_id designware_dma_ids[] = {
	{ .compatible = "dw_axi_dma" },
	{ }
};

U_BOOT_DRIVER(dw_axi_dma) = {
	.name	= "dw_axi_dma",
	.id	= UCLASS_DMA,
	.of_match = designware_dma_ids,
	.ops	= &axi_dma_ops,
	.probe = axi_dma_probe,
	.remove = axi_dma_remove,
	.priv_auto	= sizeof(struct axi_dma_dev),
};

