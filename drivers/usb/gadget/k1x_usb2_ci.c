/*
 * Copyright 2023, Spacemit Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <config.h>
#include <cpu_func.h>
#include <net.h>
#include <malloc.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <asm/io.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include "../host/ehci.h"
#include "k1x_usb2_ci.h"

/*
 * Check if the system has too long cachelines. If the cachelines are
 * longer then 128b, the driver will not be able flush/invalidate data
 * cache over separate QH entries. We use 128b because one QH entry is
 * 64b long and there are always two QH list entries for each endpoint.
 */
#if ARCH_DMA_MINALIGN > 128
#error This driver can not work on systems with caches longer than 128b
#endif

/*
 * Every QTD must be individually aligned, since we can program any
 * QTD's address into HW. Cache flushing requires ARCH_DMA_MINALIGN,
 * and the USB HW requires 32-byte alignment. Align to both:
 */
#define ILIST_ALIGN		roundup(ARCH_DMA_MINALIGN, 32)
/* Each QTD is this size */
#define ILIST_ENT_RAW_SZ	sizeof(struct ept_queue_item)
/*
 * Align the size of the QTD too, so we can add this value to each
 * QTD's address to get another aligned address.
 */
#define ILIST_ENT_SZ		roundup(ILIST_ENT_RAW_SZ, ARCH_DMA_MINALIGN)
/* For each endpoint, we need 2 QTDs, one for each of IN and OUT */
#define ILIST_SZ		(NUM_ENDPOINTS * 2 * ILIST_ENT_SZ)

//#define DEBUG 1
#ifndef DEBUG
#define DBG(x...) do {} while (0)
#else
#define DBG(x...) printf(x)
#endif

static const char *reqname(unsigned r)
{
	switch (r) {
	case USB_REQ_GET_STATUS: return "GET_STATUS";
	case USB_REQ_CLEAR_FEATURE: return "CLEAR_FEATURE";
	case USB_REQ_SET_FEATURE: return "SET_FEATURE";
	case USB_REQ_SET_ADDRESS: return "SET_ADDRESS";
	case USB_REQ_GET_DESCRIPTOR: return "GET_DESCRIPTOR";
	case USB_REQ_SET_DESCRIPTOR: return "SET_DESCRIPTOR";
	case USB_REQ_GET_CONFIGURATION: return "GET_CONFIGURATION";
	case USB_REQ_SET_CONFIGURATION: return "SET_CONFIGURATION";
	case USB_REQ_GET_INTERFACE: return "GET_INTERFACE";
	case USB_REQ_SET_INTERFACE: return "SET_INTERFACE";
	default: return "*UNKNOWN*";
	}
}

static struct usb_endpoint_descriptor ep0_out_desc = {
	.bLength = sizeof(struct usb_endpoint_descriptor),
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0,
	.bmAttributes =	USB_ENDPOINT_XFER_CONTROL,
};

static struct usb_endpoint_descriptor ep0_in_desc = {
	.bLength = sizeof(struct usb_endpoint_descriptor),
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes =	USB_ENDPOINT_XFER_CONTROL,
};

#define EP_MAX_PACKET_SIZE	0x200
#define EP0_MAX_PACKET_SIZE	64

#define WAIT_FOR_SETUP		0
#define DATA_STATE_XMIT		1
#define DATA_STATE_RECV		2
#define WAIT_FOR_OUT_STATUS	3
static unsigned int ep0_state = WAIT_FOR_SETUP;
static unsigned short testmode;
static unsigned short windex;
static int mv_pullup(struct usb_gadget *gadget, int is_on);
static int mv_ep_enable(struct usb_ep *ep,
		const struct usb_endpoint_descriptor *desc);
static int mv_ep_disable(struct usb_ep *ep);
static int mv_ep_queue(struct usb_ep *ep,
		struct usb_request *req, gfp_t gfp_flags);
static int mv_ep_dequeue(struct usb_ep *ep, struct usb_request *req);
static struct usb_request *
mv_ep_alloc_request(struct usb_ep *ep, unsigned int gfp_flags);
static void mv_ep_free_request(struct usb_ep *ep, struct usb_request *_req);

static struct usb_gadget_ops mv_udc_ops = {
	.pullup = mv_pullup,
};

static struct usb_ep_ops mv_ep_ops = {
	.enable         = mv_ep_enable,
	.disable        = mv_ep_disable,
	.queue          = mv_ep_queue,
	.dequeue	= mv_ep_dequeue,
	.alloc_request  = mv_ep_alloc_request,
	.free_request   = mv_ep_free_request,
};

/* Init values for USB endpoints. */
static const struct usb_ep mv_ep_init[2] = {
	[0] = {	/* EP 0 */
		.maxpacket	= 64,
		.name		= "ep0",
		.ops		= &mv_ep_ops,
	},
	[1] = {	/* EP 1..n */
		.maxpacket	= 512,
		.name		= "ep-",
		.ops		= &mv_ep_ops,
	},
};

static struct mv_drv controller = {
	.gadget	= {
		.name	= "mv_udc",
		.ops	= &mv_udc_ops,
		.is_dualspeed = 1,
		.max_speed = USB_SPEED_HIGH,
	},
};

static struct ehci_ctrl *ehci_ctrl;
/**
 * mv_get_qh() - return queue head for endpoint
 * @ep_num:	Endpoint number
 * @dir_in:	Direction of the endpoint (IN = 1, OUT = 0)
 *
 * This function returns the QH associated with particular endpoint
 * and it's direction.
 */
static struct ept_queue_head *mv_get_qh(int ep_num, int dir_in)
{
	return &controller.epts[(ep_num * 2) + dir_in];
}

/**
 * mv_get_qtd() - return queue item for endpoint
 * @ep_num:	Endpoint number
 * @dir_in:	Direction of the endpoint (IN = 1, OUT = 0)
 *
 * This function returns the QH associated with particular endpoint
 * and it's direction.
 */
static struct ept_queue_item *mv_get_qtd(int ep_num, int dir_in)
{
	return controller.items[(ep_num * 2) + dir_in];
}

/**
 * mv_flush_qh - flush cache over queue head
 * @ep_num:	Endpoint number
 *
 * This function flushes cache over QH for particular endpoint.
 */
static void mv_flush_qh(int ep_num)
{
	struct ept_queue_head *head = mv_get_qh(ep_num, 0);
	const ulong start = (ulong)head;
	const ulong end = start + 2 * sizeof(*head);

	flush_dcache_range(start, end);
	dmb();
}

/**
 * mv_invalidate_qh - invalidate cache over queue head
 * @ep_num:	Endpoint number
 *
 * This function invalidates cache over QH for particular endpoint.
 */
static void mv_invalidate_qh(int ep_num)
{
	struct ept_queue_head *head = mv_get_qh(ep_num, 0);
	ulong start = (ulong)head;
	ulong end = start + 2 * sizeof(*head);

	invalidate_dcache_range(start, end);
}

/**
 * mv_flush_qtd - flush cache over queue item
 * @ep_num:	Endpoint number
 *
 * This function flushes cache over qTD pair for particular endpoint.
 */
static void mv_flush_qtd(int ep_num)
{
	struct ept_queue_item *item = mv_get_qtd(ep_num, 0);
	const ulong start = (ulong)item;
	const ulong end = start + 2 * ILIST_ENT_SZ;

	flush_dcache_range(start, end);
	dmb();
}

/**
 * mv_invalidate_qtd - invalidate cache over queue item
 * @ep_num:	Endpoint number
 *
 * This function invalidates cache over qTD pair for particular endpoint.
 */
static void mv_invalidate_qtd(int ep_num)
{
	struct ept_queue_item *item = mv_get_qtd(ep_num, 0);
	const ulong start = (ulong)item;
	const ulong end = start + 2 * ILIST_ENT_SZ;

	invalidate_dcache_range(start, end);
}

static struct usb_request *
mv_ep_alloc_request(struct usb_ep *ep, unsigned int gfp_flags)
{
	struct mv_ep *mv_ep = container_of(ep, struct mv_ep, ep);
	int num = -1;
	struct mv_req *mv_req;

	if (mv_ep->desc)
		num = mv_ep->desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	if (num == 0 && controller.ep0_req)
		return &controller.ep0_req->req;

	mv_req = memalign(ARCH_DMA_MINALIGN, sizeof(*mv_req));
	if (!mv_req)
		return NULL;

	INIT_LIST_HEAD(&mv_req->queue);
	mv_req->b_buf = 0;

	if (num == 0)
		controller.ep0_req = mv_req;

	return &mv_req->req;
}

static void mv_ep_free_request(struct usb_ep *ep, struct usb_request *req)
{
	struct mv_ep *mv_ep = container_of(ep, struct mv_ep, ep);
	struct mv_req *mv_req = container_of(req, struct mv_req, req);
	int num = -1;

	if (mv_ep->desc)
		num = mv_ep->desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	if (num == 0) {
		if (!controller.ep0_req)
			return;
		controller.ep0_req = 0;
	}

	if (mv_req->b_buf)
		free(mv_req->b_buf);
	free(mv_req);
}

static void ep_enable(int num, int in, int maxpacket)
{
	struct mv_udc *udc = (struct mv_udc *)controller.ctrl->hccr;
	unsigned n;

	n = readl(&udc->epctrl[num]);
	if (in)
		n |= (CTRL_TXE | CTRL_TXR | CTRL_TXT_BULK);
	else
		n |= (CTRL_RXE | CTRL_RXR | CTRL_RXT_BULK);

	if (num != 0) {
		struct ept_queue_head *head = mv_get_qh(num, in);

		head->config = CONFIG_MAX_PKT(maxpacket) | CONFIG_ZLT;
		mv_flush_qh(num);
	}
	writel(n, &udc->epctrl[num]);
}

static int mv_ep_enable(struct usb_ep *ep,
		const struct usb_endpoint_descriptor *desc)
{
	struct mv_ep *mv_ep = container_of(ep, struct mv_ep, ep);
	int num, in;
	num = desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	in = (desc->bEndpointAddress & USB_DIR_IN) != 0;
	mv_ep->desc = desc;
	ep->desc = desc;

	if (num) {
		int max = get_unaligned_le16(&desc->wMaxPacketSize);

		if ((max > 64) && (controller.gadget.speed == USB_SPEED_FULL))
			max = 64;
		if (ep->maxpacket != max) {
			DBG("%s: from %d to %d\n", __func__,
			    ep->maxpacket, max);
			ep->maxpacket = max;
		}
	}
	ep_enable(num, in, ep->maxpacket);
	DBG("%s: num=%d maxpacket=%d\n", __func__, num, ep->maxpacket);
	return 0;
}

static int mv_ep_disable(struct usb_ep *ep)
{
	struct mv_ep *mv_ep = container_of(ep, struct mv_ep, ep);

	mv_ep->desc = NULL;
	ep->desc = NULL;
	return 0;
}

static void ep_set_stall(struct mv_udc *udc, u8 ep_num, u8 direction, int stall)
{
	u32 epctrlx;

	epctrlx = readl(&udc->epctrl[ep_num]);

	if (stall) {
		if (direction == USB_DIR_IN)
			epctrlx |= EPCTRL_TX_EP_STALL;
		else
			epctrlx |= EPCTRL_RX_EP_STALL;
	} else {
		if (direction == USB_DIR_IN) {
			epctrlx &= ~EPCTRL_TX_EP_STALL;
			epctrlx |= EPCTRL_TX_DATA_TOGGLE_RST;
		} else {
			epctrlx &= ~EPCTRL_RX_EP_STALL;
			epctrlx |= EPCTRL_RX_DATA_TOGGLE_RST;
		}
	}
	writel(epctrlx, &udc->epctrl[ep_num]);
}
static int ep_is_stall(struct mv_udc *udc, u8 ep_num, u8 direction)
{
	u32 epctrlx;

	epctrlx = readl(&udc->epctrl[ep_num]);

	if (direction == USB_DIR_OUT)
		return (epctrlx & EPCTRL_RX_EP_STALL) ? 1 : 0;
	else
		return (epctrlx & EPCTRL_TX_EP_STALL) ? 1 : 0;
}

static int mv_bounce(struct mv_req *mv_req, int in)
{
	struct usb_request *req = &mv_req->req;
	unsigned long addr = (unsigned long)req->buf;
	unsigned long hwaddr;
	uint32_t aligned_used_len;

	/* Input buffer address is not aligned. */
	if (addr & (ARCH_DMA_MINALIGN - 1))
		goto align;

	/* Input buffer length is not aligned. */
	if (req->length & (ARCH_DMA_MINALIGN - 1))
		goto align;

	/* The buffer is well aligned, only flush cache. */
	mv_req->hw_len = req->length;
	mv_req->hw_buf = req->buf;
	goto flush;

align:
	if (mv_req->b_buf && req->length > mv_req->b_len) {
		free(mv_req->b_buf);
		mv_req->b_buf = 0;
	}

	if (!mv_req->b_buf) {
		mv_req->b_len = roundup(req->length, ARCH_DMA_MINALIGN);
		mv_req->b_buf = memalign(ARCH_DMA_MINALIGN, mv_req->b_len);
		if (!mv_req->b_buf)
			return -ENOMEM;
	}
	mv_req->hw_len = mv_req->b_len;
	mv_req->hw_buf = mv_req->b_buf;

	if (in)
		memcpy(mv_req->hw_buf, req->buf, req->length);

flush:
	hwaddr = (unsigned long)mv_req->hw_buf;
	if (!hwaddr)
		return 0;
	aligned_used_len = roundup(req->length, ARCH_DMA_MINALIGN);
	flush_dcache_range(hwaddr, hwaddr + aligned_used_len);

	return 0;
}

static void mv_debounce(struct mv_req *mv_req, int in)
{
	struct usb_request *req = &mv_req->req;
	unsigned long addr = (unsigned long)req->buf;
	unsigned long hwaddr = (unsigned long)mv_req->hw_buf;
	uint32_t aligned_used_len;

	if (in)
		return;

	aligned_used_len = roundup(req->actual, ARCH_DMA_MINALIGN);
	invalidate_dcache_range(hwaddr, hwaddr + aligned_used_len);

	if (addr == hwaddr)
		return;	/* not a bounce */

	memcpy(req->buf, mv_req->hw_buf, req->actual);
}

static void mv_ep_submit_next_request(struct mv_ep *mv_ep)
{
	struct mv_udc *udc = (struct mv_udc *)controller.ctrl->hccr;
	struct ept_queue_item *item;
	struct ept_queue_head *head;
	int bit, num, len, in;
	struct mv_req *mv_req;

	mv_ep->req_primed = true;

	num = mv_ep->desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	in = (mv_ep->desc->bEndpointAddress & USB_DIR_IN) != 0;
	item = mv_get_qtd(num, in);
	head = mv_get_qh(num, in);

	mv_req = list_first_entry(&mv_ep->queue, struct mv_req, queue);
	len = mv_req->req.length;

	item->info = INFO_BYTES(len) | INFO_ACTIVE;
	item->page0 =  (uint32_t)(ulong)mv_req->hw_buf;
	item->page1 = ((uint32_t)(ulong)mv_req->hw_buf & 0xfffff000) + 0x1000;
	item->page2 = ((uint32_t)(ulong)mv_req->hw_buf & 0xfffff000) + 0x2000;
	item->page3 = ((uint32_t)(ulong)mv_req->hw_buf & 0xfffff000) + 0x3000;
	item->page4 = ((uint32_t)(ulong)mv_req->hw_buf & 0xfffff000) + 0x4000;

	head->next = (unsigned)(ulong)item;
	head->info = 0;


	/*
	 * When sending the data for an IN transaction, the attached host
	 * knows that all data for the IN is sent when one of the following
	 * occurs:
	 * a) A zero-length packet is transmitted.
	 * b) A packet with length that isn't an exact multiple of the ep's
	 *    maxpacket is transmitted.
	 * c) Enough data is sent to exactly fill the host's maximum expected
	 *    IN transaction size.
	 *
	 * One of these conditions MUST apply at the end of an IN transaction,
	 * or the transaction will not be considered complete by the host. If
	 * none of (a)..(c) already applies, then we must force (a) to apply
	 * by explicitly sending an extra zero-length packet.
	 */
	/*  IN    !a     !b                              !c */
#if 0
	if (in && len && !(len % mv_ep->ep.maxpacket) && mv_req->req.zero) {
		/*
		 * Each endpoint has 2 items allocated, even though typically
		 * only 1 is used at a time since either an IN or an OUT but
		 * not both is queued. For an IN transaction, item currently
		 * points at the second of these items, so we know that we
		 * can use the other to transmit the extra zero-length packet.
		 */
		struct ept_queue_item *other_item = mv_get_qtd(num, 0);
		item->next = (ulong)other_item;
		item = other_item;
		item->info = INFO_ACTIVE;
	}
#endif

	item->next = TERMINATE;
	item->info |= INFO_IOC;

	mv_flush_qtd(num);

	DBG("ept%d %s queue len %x, req %p buffer %p\n",
	    num, in ? "in" : "out", len, mv_req, mv_req->hw_buf);
	mv_flush_qh(num);

	udelay(10);

	if (in)
		bit = EPT_TX(num);
	else
		bit = EPT_RX(num);

	writel(bit, &udc->epprime);
}

static int mv_ep_queue(struct usb_ep *ep,
		struct usb_request *req, gfp_t gfp_flags)
{
	struct mv_ep *mv_ep = container_of(ep, struct mv_ep, ep);
	struct mv_req *mv_req = container_of(req, struct mv_req, req);
	int in, ret;
	int __maybe_unused num;

	num = mv_ep->desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	in = (mv_ep->desc->bEndpointAddress & USB_DIR_IN) != 0;

	if (!num && mv_ep->req_primed) {
		/*
		 * The flipping of ep0 between IN and OUT relies on
		 * ci_ep_queue consuming the current IN/OUT setting
		 * immediately. If this is deferred to a later point when the
		 * req is pulled out of ci_req->queue, then the IN/OUT setting
		 * may have been changed since the req was queued, and state
		 * will get out of sync. This condition doesn't occur today,
		 * but could if bugs were introduced later, and this error
		 * check will save a lot of debugging time.
		 */
		pr_err("%s: ep0 transaction already in progress\n", __func__);
		return -EPROTO;
	}

	ret = mv_bounce(mv_req, in);
	if (ret)
		return ret;

	DBG("ept%d %s pre-queue req %p, buffer %p\n",
		num, in ? "in" : "out", mv_req, mv_req->hw_buf);
	list_add_tail(&mv_req->queue, &mv_ep->queue);

	if (!mv_ep->req_primed)
		mv_ep_submit_next_request(mv_ep);

	return 0;
}

static int mv_ep_dequeue(struct usb_ep *ep, struct usb_request *req)
{
	struct mv_ep *mv_ep = container_of(ep, struct mv_ep, ep);
	struct mv_req *mv_req;

	list_for_each_entry(mv_req, &mv_ep->queue, queue) {
		if (&mv_req->req == req)
			break;
	}

	if (&mv_req->req != req)
		return -EINVAL;

	list_del_init(&mv_req->queue);

	if (mv_req->req.status == -EINPROGRESS) {
		mv_req->req.status = -ECONNRESET;
		if (mv_req->req.complete)
			mv_req->req.complete(ep, req);
	}

	return 0;
}

/*
 *  Test Mode Selectors, these are not defined
 *  in ch9.h, so define here
 */
#define TEST_J      1
#define TEST_K      2
#define TEST_SE0_NAK    3
#define TEST_PACKET 4
#define TEST_FORCE_EN   5
#define TEST_DISABLE 0

static void mv_set_ptc(struct mv_udc *udc, u32 mode)
{
	u32 portsc;
	portsc = readl(&udc->portsc);
	portsc |= mode << 16;
	writel(portsc, &udc->portsc);
}

static void mv_udc_testmode(struct mv_udc *udc, u16 index)
{
	if (index <= TEST_FORCE_EN){
		mv_set_ptc(udc, index);
	}else{
		pr_info("This test mode(%d) is not supported\n", index);
	}
}

static void handle_ep_complete(struct mv_ep *ep)
{
	struct ept_queue_item *item;
	int num, in, len;
	struct mv_req *mv_req;
	struct mv_udc *udc = (struct mv_udc *)controller.ctrl->hccr;

	num = ep->desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	in = (ep->desc->bEndpointAddress & USB_DIR_IN) != 0;

	item = mv_get_qtd(num, in);
	mv_invalidate_qtd(num);

	len = (item->info >> 16) & 0x7fff;
	if (item->info & 0xff)
		pr_err("EP%d/%s FAIL info=%x pg0=%x\n",
		       num, in ? "in" : "out", item->info, item->page0);

	mv_req = list_first_entry(&ep->queue, struct mv_req, queue);
	list_del_init(&mv_req->queue);
	ep->req_primed = false;

	if (!list_empty(&ep->queue))
		mv_ep_submit_next_request(ep);

	mv_req->req.actual = mv_req->req.length - len;
	mv_debounce(mv_req, in);

	DBG("ept%d %s req %p complete %x\n",
			num, in ? "in" : "out", mv_req, len);

	mv_req->req.status = 0;

	mv_req->req.complete(&ep->ep, &mv_req->req);
	if (num == 0) {
		switch (ep0_state) {
		case DATA_STATE_XMIT:
			/* receive status phase */
			mv_req->req.length = 0;
			ep->desc = &ep0_out_desc;
			ep0_state = WAIT_FOR_OUT_STATUS;
			usb_ep_queue(&ep->ep, &mv_req->req, 0);
			break;
		case DATA_STATE_RECV:
			/* send status phase */
			mv_req->req.length = 0;
			ep->desc = &ep0_in_desc;
			ep0_state = WAIT_FOR_OUT_STATUS;
			usb_ep_queue(&ep->ep, &mv_req->req, 0);
			break;
		case WAIT_FOR_OUT_STATUS:
			ep0_state = WAIT_FOR_SETUP;
			if (testmode == USB_DEVICE_TEST_MODE) {
				pr_info("enter test mode too!!!\n");
				mv_udc_testmode(udc, windex);
				windex = 0x0;
				testmode = 0x0;
			}
			break;
		}
	}
}

#define SETUP(type, request) (((type) << 8) | (request))

static void handle_setup(void)
{
	struct mv_ep *mv_ep = &controller.ep[0];
	struct mv_req *mv_req;
	struct usb_request *req;
	struct mv_udc *udc = (struct mv_udc *)controller.ctrl->hccr;
	struct ept_queue_head *head;
	struct usb_ctrlrequest r;
	int status = 0;
	int num, dir, _num, _dir, i;
	char *buf;

	mv_req = controller.ep0_req;

	req = &mv_req->req;

	head = mv_get_qh(0, 0);	/* EP0 OUT */

	mv_invalidate_qh(0);
	memcpy(&r, head->setup_data, sizeof(struct usb_ctrlrequest));
	writel(EPT_RX(0), &udc->epsetupstat);
	pr_info("handle setup %s, 0x%x, 0x%x index 0x%x value 0x%x length 0x%x\n",
	    reqname(r.bRequest), r.bRequestType, r.bRequest, r.wIndex,
	    r.wValue, r.wLength);

	list_del_init(&mv_req->queue);
	mv_ep->req_primed = false;

	switch (SETUP(r.bRequestType, r.bRequest)) {
	case SETUP(USB_RECIP_ENDPOINT, USB_REQ_CLEAR_FEATURE):
	case SETUP(USB_RECIP_ENDPOINT, USB_REQ_SET_FEATURE):
		_num = r.wIndex & 15;
		_dir = (r.wIndex & 0x80) ? USB_DIR_IN : USB_DIR_OUT;

		if ((r.wValue == 0) && (r.wLength == 0)) {
			req->length = 0;
			for (i = 0; i < NUM_ENDPOINTS * 2; i++) {
				if (!controller.ep[i].desc)
					continue;
				num = controller.ep[i].desc->bEndpointAddress
					& USB_ENDPOINT_NUMBER_MASK;
				dir = (controller.ep[i].desc->bEndpointAddress
						& USB_DIR_IN) != 0 ? USB_DIR_IN : USB_DIR_OUT;
				if ((num == _num) && (dir == _dir)) {
					if (r.bRequest == USB_REQ_SET_FEATURE) {
						ep_set_stall(udc, num, dir, 1);
					} else {
						ep_enable(num, dir, controller.ep[i].ep.maxpacket);
						ep_set_stall(udc, num, dir, 0);
					}
					controller.ep[0].desc = &ep0_in_desc;
					ep0_state = WAIT_FOR_OUT_STATUS;
					usb_ep_queue(controller.gadget.ep0, req, 0);
					break;
				}
			}
		}
		return;

	case SETUP(USB_RECIP_DEVICE, USB_REQ_SET_FEATURE):
		if (r.wValue == USB_DEVICE_TEST_MODE) {
			pr_info("enter test mode\n");
			testmode = r.wValue;
			windex = (r.wIndex >> 8);
		}
		req->length = 0;
		controller.ep[0].desc = &ep0_in_desc;
		ep0_state = WAIT_FOR_OUT_STATUS;
		usb_ep_queue(controller.gadget.ep0, req, 0);
		return;

	case SETUP(USB_RECIP_DEVICE, USB_REQ_CLEAR_FEATURE):
		if (r.wValue == USB_DEVICE_TEST_MODE) {
			pr_info("leave test mode\n");
			mv_udc_testmode(udc, TEST_DISABLE);
		}
		return;

	case SETUP(USB_RECIP_DEVICE, USB_REQ_SET_ADDRESS):
		/*
		 * write address delayed (will take effect
		 * after the next IN txn)
		 */

		writel((r.wValue << 25) | (1 << 24), &udc->devaddr);

		req->length = 0;
		controller.ep[0].desc = &ep0_in_desc;
		ep0_state = WAIT_FOR_OUT_STATUS;
		usb_ep_queue(controller.gadget.ep0, req, 0);
		return;

	case SETUP(USB_DIR_IN | USB_RECIP_DEVICE, USB_REQ_GET_STATUS):
		req->length = 2;
		buf = (char *)req->buf;
		buf[0] = 1 << USB_DEVICE_SELF_POWERED;
		buf[1] = 0;
		controller.ep[0].desc = &ep0_in_desc;
		usb_ep_queue(controller.gadget.ep0, req, 0);
		ep0_state = DATA_STATE_XMIT;
		return;

	case SETUP(USB_DIR_IN | USB_RECIP_ENDPOINT, USB_REQ_GET_STATUS):
		_num = r.wIndex & 15;
		_dir = (r.wIndex & 0x80) ? USB_DIR_IN : USB_DIR_OUT;
		req->length = 2;
		buf = (char *)req->buf;
		buf[0] = ep_is_stall(udc, _num, _dir) << USB_ENDPOINT_HALT;
		buf[1] = 0;
		controller.ep[0].desc = &ep0_in_desc;
		usb_ep_queue(controller.gadget.ep0, req, 0);
		ep0_state = DATA_STATE_XMIT;
		return;
	}

	if (r.wLength) {
		if (r.bRequestType & USB_DIR_IN)
			controller.ep[0].desc = &ep0_in_desc;
		else
			controller.ep[0].desc = &ep0_out_desc;
	} else {
		controller.ep[0].desc = &ep0_in_desc;
	}

	/* pass request up to the gadget driver */
	if (controller.driver)
		status = controller.driver->setup(&controller.gadget, &r);
	else
		status = -ENODEV;

	if (!status) {
		if (r.wLength)
			/* DATA phase from gadget, STATUS phase from udc */
			ep0_state = (r.bRequestType & USB_DIR_IN)
				?  DATA_STATE_XMIT : DATA_STATE_RECV;
		else
			ep0_state = WAIT_FOR_OUT_STATUS;
		return;
	}
	DBG("STALL reqname %s type %x value %x, index %x\n",
	    reqname(r.bRequest), r.bRequestType, r.wValue, r.wIndex);
	writel((1<<16) | (1 << 0), &udc->epctrl[0]);
}

static void stop_activity(void)
{
	int i, num, in;
	struct ept_queue_head *head;
	struct mv_udc *udc = (struct mv_udc *)controller.ctrl->hccr;

	controller.ep[0].desc = &ep0_out_desc;
	ep0_state = WAIT_FOR_SETUP;

	writel(readl(&udc->epsetupstat), &udc->epsetupstat);
	writel(readl(&udc->epcomp), &udc->epcomp);
	writel(0xffffffff, &udc->epflush);

	/* error out any pending reqs */
	for (i = 0; i < NUM_ENDPOINTS; i++) {
		if (i != 0)
			writel(0, &udc->epctrl[i]);
		if (controller.ep[i].desc) {
			num = controller.ep[i].desc->bEndpointAddress
				& USB_ENDPOINT_NUMBER_MASK;
			in = (controller.ep[i].desc->bEndpointAddress
				& USB_DIR_IN) != 0;
			head = mv_get_qh(num, in);
			head->next = TERMINATE;
			head->info = 0;
			mv_flush_qh(num);
			controller.ep[i].req_primed = false;
		}
	}
}

void udc_irq(void)
{
	struct mv_udc *udc = (struct mv_udc *)controller.ctrl->hccr;
	unsigned n = readl(&udc->usbsts);
	writel(n, &udc->usbsts);
	int bit, i, num, in;

	n &= (STS_SLI | STS_URI | STS_SEI | STS_PCI | STS_UI | STS_UEI);
	if (n == 0)
		return;

	if (n & STS_SEI){
		pr_err("-- system error -- \n");
	}

	if (n & STS_URI) {
		DBG("-- reset --:epsetupstat= 0x%x\n", readl(&udc->epsetupstat));
		stop_activity();
	}
	if (n & STS_SLI){
		pr_info("-- suspend --\n");
	}

	if (n & STS_PCI) {
		int max = 64;
		int speed = USB_SPEED_FULL;

		bit = (readl(&udc->portsc) >> 26) & 3;
//		DBG("-- portchange %x %s\n", bit, (bit == 2) ? "High" : "Full");
		if (bit == 2) {
			speed = USB_SPEED_HIGH;
			max = 512;
		}
		controller.gadget.speed = speed;
		for (i = 1; i < NUM_ENDPOINTS * 2; i++) {
			if (controller.ep[i].ep.maxpacket > max)
				controller.ep[i].ep.maxpacket = max;
		}
	}

	if (n & STS_UEI)
		DBG("<UEI %x>\n", readl(&udc->epcomp));

	if (n & STS_UI) {
		DBG("-- STS_UI --\n");
		n = readl(&udc->epcomp);
		if (n != 0) {
			writel(n, &udc->epcomp);

			for (i = 0; i < 2 * NUM_ENDPOINTS; i++) {
				if (controller.ep[i].desc) {
					num = controller.ep[i].desc->bEndpointAddress
						& USB_ENDPOINT_NUMBER_MASK;
					in = (controller.ep[i].desc->bEndpointAddress
					      & USB_DIR_IN) != 0;
					bit = (in) ? EPT_TX(num) : EPT_RX(num);

					if (n & bit)
						handle_ep_complete(&controller.ep[i]);
				}
			}
		}
		n = readl(&udc->epsetupstat);
		if (n & EPT_RX(0))
			handle_setup();
	}
}

int usb_gadget_handle_interrupts(int index)
{
	u32 value;
	struct mv_udc *udc = (struct mv_udc *)controller.ctrl->hccr;

	value = readl(&udc->usbsts);
	if (value)
		udc_irq();

	return value;
}

void udc_disconnect(void)
{
	struct mv_udc *udc = (struct mv_udc *)controller.ctrl->hccr;
	/* disable pullup */
	stop_activity();
	writel(USBCMD_FS2, &udc->usbcmd);
	udelay(800);
	if (controller.driver)
		controller.driver->disconnect(&controller.gadget);
}

static int mv_pullup(struct usb_gadget *gadget, int is_on)
{
	struct mv_udc *udc = (struct mv_udc *)controller.ctrl->hccr;
	u32 value;

	pr_info("k1xci_udc: pullup %d \n", is_on);

	if (is_on) {
		/* RESET */
		writel(USBCMD_ITC(MICRO_8FRAME) | USBCMD_RST, &udc->usbcmd);
		udelay(200);

		writel((unsigned)(ulong)controller.epts, &udc->epinitaddr);

		/* select DEVICE mode */
		value = readl(&udc->usbmode);
		value &= ~USBMODE_CM_MASK;
		value |= USBMODE_DEVICE;
		writel(value, &udc->usbmode);

		writel(0xffffffff, &udc->epflush);

		//writel(STS_UI | STS_PCI | STS_URI, &udc->usbintr);
		//writel(STS_UI | STS_PCI | STS_URI, &udc->usbsts);

		/* Turn on the USB connection by enabling the pullup resistor */
		value = readl(&udc->usbcmd);
		value |= (USBCMD_ITC(MICRO_8FRAME) | USBCMD_RUN);
		writel(value, &udc->usbcmd);
	} else {
		udc_disconnect();
	}

	return 0;
}

static int mvudc_probe(void)
{
	struct ept_queue_head *head;
	uint8_t *imem;
	int i;

	const int num = 2 * NUM_ENDPOINTS;

	const int eplist_min_align = 4096;
	const int eplist_align = roundup(eplist_min_align, ARCH_DMA_MINALIGN);
	const int eplist_raw_sz = num * sizeof(struct ept_queue_head);
	const int eplist_sz = roundup(eplist_raw_sz, ARCH_DMA_MINALIGN);

	/* The QH list must be aligned to 4096 bytes. */
	controller.epts = memalign(eplist_align, eplist_sz);
	if (!controller.epts)
		return -ENOMEM;
	memset(controller.epts, 0, eplist_sz);

	ehci_ctrl = malloc(sizeof(struct ehci_ctrl));
	ehci_ctrl->hccr = (struct ehci_hccr *)(K1X_USB_BASE + 0x100);
	controller.ctrl = ehci_ctrl;

	controller.items_mem = memalign(ILIST_ALIGN, ILIST_SZ);
	if (!controller.items_mem) {
		free(controller.epts);
		return -ENOMEM;
	}
	memset(controller.items_mem, 0, ILIST_SZ);

	for (i = 0; i < 2 * NUM_ENDPOINTS; i++) {
		/*
		 * Configure QH for each endpoint. The structure of the QH list
		 * is such that each two subsequent fields, N and N+1 where N is
		 * even, in the QH list represent QH for one endpoint. The Nth
		 * entry represents OUT configuration and the N+1th entry does
		 * represent IN configuration of the endpoint.
		 */
		head = controller.epts + i;
		if (i < 2)
			head->config = CONFIG_MAX_PKT(EP0_MAX_PACKET_SIZE)
				| CONFIG_ZLT | CONFIG_IOS;
		else
			head->config = CONFIG_MAX_PKT(EP_MAX_PACKET_SIZE)
				| CONFIG_ZLT;
		head->next = TERMINATE;
		head->info = 0;

		imem = controller.items_mem + (i * ILIST_ENT_SZ);
		controller.items[i] = (struct ept_queue_item *)imem;

		if (i & 1) {
			mv_flush_qh(i/2);
			mv_flush_qtd(i/2);
		}
	}

	INIT_LIST_HEAD(&controller.gadget.ep_list);

	/* Init EP 0 */
	memcpy(&controller.ep[0].ep, &mv_ep_init[0], sizeof(*mv_ep_init));
	controller.ep[0].desc = &ep0_out_desc;
	INIT_LIST_HEAD(&controller.ep[0].queue);
	controller.ep[0].req_primed = false;
	controller.gadget.ep0 = &controller.ep[0].ep;
	INIT_LIST_HEAD(&controller.gadget.ep0->ep_list);

	/* Init EP 1..n */
	for (i = 1; i < 2 * NUM_ENDPOINTS; i++) {
		memcpy(&controller.ep[i].ep, &mv_ep_init[1],
		       sizeof(*mv_ep_init));
		INIT_LIST_HEAD(&controller.ep[i].queue);
		controller.ep[i].req_primed = false;
		list_add_tail(&controller.ep[i].ep.ep_list,
			      &controller.gadget.ep_list);
		controller.ep[i].desc = NULL;
	}

	mv_ep_alloc_request(&controller.ep[0].ep, 0);
	if (!controller.ep0_req) {
		free(controller.items_mem);
		free(controller.epts);
		return -ENOMEM;
	}

	pr_info("k1xci_udc probe\n");

	return 0;
}

void usbphy_init(void)
{
	uint32_t loops, temp;

	pr_info("k1xci_udc: phy_init \n");
	reg32_modify(PMUA_USB_CLK_RES_CTRL, 0, PMUA_USB_CLK_RES_CTRL_USB_AXICLK_EN);
	reg32_modify(PMUA_USB_CLK_RES_CTRL, PMUA_USB_CLK_RES_CTRL_USB_AXI_RST, 0);
	reg32_modify(PMUA_USB_CLK_RES_CTRL, 0, PMUA_USB_CLK_RES_CTRL_USB_AXI_RST);

	udelay(200);

	loops = 200;
	do {
		temp = reg32_read(USB2_PHY_REG01);
		if (temp & USB2_PLL_PLLREADY)
			break;
		udelay(5);
	} while(--loops);

	if (loops == 0){
		pr_err("Wait PHY_REG01[PLLREADY] timeout \n");
	}

	reg32_write(USB2_PHY_REG01, 0x60ef);
	reg32_write(USB2_PHY_REG0D, 0x1c);
}

int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	struct mv_udc *udc;
	int ret;

	if (!driver)
		return -EINVAL;
	if (!driver->bind || !driver->setup || !driver->disconnect)
		return -EINVAL;

	usbphy_init();

	ret = mvudc_probe();
	if (ret) {
		DBG("udc probe failed, returned %d\n", ret);
		return ret;
	}

	udc = (struct mv_udc *)controller.ctrl->hccr;
	/* select ULPI phy */
	writel(PTS(PTS_ENABLE) | PFSC, &udc->portsc);

	ret = driver->bind(&controller.gadget);
	if (ret) {
		DBG("driver->bind() returned %d\n", ret);
		return ret;
	}
	controller.driver = driver;

	return 0;
}

int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	udc_disconnect();
	driver->unbind(&controller.gadget);
	controller.driver = NULL;

	mv_ep_free_request(&controller.ep[0].ep, &controller.ep0_req->req);
	free(controller.items_mem);
	free(controller.epts);

	return 0;
}
