/*
 * Renesas USB driver
 *
 * Copyright (C) 2011 Renesas Solutions Corp.
 * Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>
 *
 * Ported to u-boot
 * Copyright (C) 2016 GlobalLogic
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include <linux/err.h>
#include "common.h"
#include "rcar3.h"
#include <asm/io.h>
#include "rza.h"

/*
 *		image of renesas_usbhs
 *
 * ex) gadget case

 * mod.c
 * mod_gadget.c
 * mod_host.c		pipe.c		fifo.c
 *
 *			+-------+	+-----------+
 *			| pipe0 |------>| fifo pio  |
 * +------------+	+-------+	+-----------+
 * | mod_gadget |=====> | pipe1 |--+
 * +------------+	+-------+  |	+-----------+
 *			| pipe2 |  |  +-| fifo dma0 |
 * +------------+	+-------+  |  |	+-----------+
 * | mod_host   |	| pipe3 |<-|--+
 * +------------+	+-------+  |	+-----------+
 *			| ....  |  +--->| fifo dma1 |
 *			| ....  |	+-----------+
 */


#define USBHSF_RUNTIME_PWCTRL	(1 << 0)

/* status */
#define usbhsc_flags_init(p)   do {(p)->flags = 0; } while (0)
#define usbhsc_flags_set(p, b) ((p)->flags |=  (b))
#define usbhsc_flags_clr(p, b) ((p)->flags &= ~(b))
#define usbhsc_flags_has(p, b) ((p)->flags &   (b))

/*
 * platform call back
 *
 * renesas usb support platform callback function.
 * Below macro call it.
 * if platform doesn't have callback, it return 0 (no error)
 */
#define usbhs_platform_call(priv, func, args...)\
	(!(priv) ? -ENODEV :			\
	 !((priv)->pfunc.func) ? 0 :		\
	 (priv)->pfunc.func(args))

struct usbhs_regname {
	u32		regaddr;	/*Address of the usbhs register */
	const char *regname;	/*String Name of the register*/
} ;

/*Structure for debugfs to set the register name by the address*/
static const struct usbhs_regname usbhs_regnames[] = {
	{ 0x0,  "SYSCFG" }, { 0x2,  "BUSWAIT" }, { 0x4,  "SYSSTS" },
	{ 0x8,  "DVSTCTR"},	{ 0xC,  "TESTMODE" }, { 0x14,  "CFIFO" },
	{ 0x20,  "CFIFOSEL" }, { 0x22, "CFIFOCTR" }, { 0x28,  "D0FIFOSEL" },
	{ 0x2A,  "D0FIFOCTR" }, { 0x2C,  "D1FIFOSEL"}, { 0x2E,  "D1FIFOCTR" },
	{ 0x30,  "INTENB0" }, { 0x36,  "BRDYENB" }, { 0x38,  "NRDYENB" },
	{ 0x3A,  "BEMPENB" }, { 0x3C, "SOFCFG" }, { 0x40, "INTSTS0" },
	{ 0x46, "BRDYSTS" }, { 0x48, "NRDYSTS"  }, { 0x4A, "BEMPSTS" },
	{ 0x4C, "FRMNUM" }, { 0x4E, "UFRMNUM" }, { 0x50, "USBADDR" },
	{ 0x54, "USBREQ" }, { 0x56, "USBVAL" }, { 0x58, "USBINDX" },
	{ 0x5A, "USBLENG" }, { 0x5E, "DCPMAXP" }, { 0x60, "DCPCTR" },
	{ 0x64, "PIPESEL" }, { 0x68, "PIPECFG" }, { 0x6A, "PIPEBUF" },
	{ 0x6C, "PIPEMAXP" }, { 0x6E, "PIPEPERI" }, { 0x70, "PIPE1CTR" },
	{ 0x72, "PIPE2CTR" }, { 0x74, "PIPE3CTR" }, { 0x76, "PIPE4CTR" },
	{ 0x78, "PIPE5CTR" }, { 0x7A, "PIPE6CTR" }, { 0x7C, "PIPE7CTR" },
	{ 0x7E, "PIPE8CTR" }, { 0x80, "PIPE9CTR" }, { 0x82, "PIPEACTR" },
	{ 0x84, "PIPEBCTR" }, { 0x86, "PIPECCTR" }, { 0x88, "PIPEDCTR" },
	{ 0x8A, "PIPEECTR" }, { 0x8C, "PIPEFCTR" }, { 0x90, "PIPE1TRE" },
	{ 0x92, "PIPE1TRN" }, { 0x94, "PIPE2TRE" }, { 0x96, "PIPE2TRN" },
	{ 0x98, "PIPE3TRE" }, { 0x9A, "PIPE3TRN" }, { 0x9C, "PIPE4TRE" },
	{ 0x9E, "PIPE4TRN" }, { 0xA0, "PIPE5TRE" }, { 0xA2, "PIPE5TRN" },
	{ 0xA4, "PIPEBTRE" }, { 0xA6, "PIPEBTRN" }, { 0xA8, "PIPECTRE" },
	{ 0xAA, "PIPECTRN" }, { 0xAC, "PIPEDTRE" }, { 0xAE, "PIPEDTRN" },
	{ 0xB0, "PIPEETRE" }, { 0xB2, "PIPEETRN" }, { 0xB4, "PIPEFTRE" },
	{ 0xB6, "PIPEFTRN" }, { 0xB8, "PIPE9TRE" }, { 0xBA, "PIPE9TRN" },
	{ 0xBC, "PIPEATRE" }, { 0xBE, "PIPEATRN" }, { 0xF0, "D2FIFOSEL" },
	{ 0xF2, "D2FIFOCTR" }, { 0xF4, "D3FIFOSEL" }, { 0xF6, "D3FIFOCTR" },
	{ 0x102, "LPSTS" }, { 0x140, "BCCTRL" }, { 0x184, "UGCTRL2" },
};

/*
 *		common functions
 */
u16 usbhs_read(struct usbhs_priv *priv, u32 reg)
{
	return readw(priv->base + reg);
}

void usbhs_write(struct usbhs_priv *priv, u32 reg, u16 data)
{
	writew(data, priv->base + reg);
}

void usbhs_bset(struct usbhs_priv *priv, u32 reg, u16 mask, u16 data)
{
	u16 val = usbhs_read(priv, reg);

	val &= ~mask;
	val |= data & mask;

	usbhs_write(priv, reg, val);
}

struct usbhs_priv *usbhs_pdev_to_priv(struct platform_device *pdev)
{
	return dev_get_drvdata(&pdev->dev);
}

/*
 *		syscfg functions
 */
static void usbhs_sys_clock_ctrl(struct usbhs_priv *priv, int enable)
{
	usbhs_bset(priv, SYSCFG, SCKE, enable ? SCKE : 0);
}

void usbhs_sys_host_ctrl(struct usbhs_priv *priv, int enable)
{
	u16 mask = DCFM | DRPD | DPRPU | HSE | USBE;
	u16 val  = DCFM | DRPD | HSE | USBE;
	int has_otg = usbhs_get_dparam(priv, has_otg);

	if (has_otg)
		usbhs_bset(priv, DVSTCTR, (EXTLP | PWEN), (EXTLP | PWEN));

	/*
	 * if enable
	 *
	 * - select Host mode
	 * - D+ Line/D- Line Pull-down
	 */
	usbhs_bset(priv, SYSCFG, mask, enable ? val : 0);
}

void usbhs_sys_function_ctrl(struct usbhs_priv *priv, int enable)
{
	u16 mask = DCFM | DRPD | DPRPU | HSE | USBE;
	u16 val  = HSE | USBE;

	/* CNEN bit is required for function operation */
	if (usbhs_get_dparam(priv, has_cnen)) {
		mask |= CNEN;
		val  |= CNEN;
	}
	/*
	 * if enable
	 *
	 * - select Function mode
	 * - D+ Line Pull-up is disabled
	 *      When D+ Line Pull-up is enabled,
	 *      calling usbhs_sys_function_pullup(,1)
	 */
	usbhs_bset(priv, SYSCFG, mask, enable ? val : 0);
}

void usbhs_sys_function_pullup(struct usbhs_priv *priv, int enable)
{
	usbhs_bset(priv, SYSCFG, DPRPU, enable ? DPRPU : 0);
}

void usbhs_sys_set_test_mode(struct usbhs_priv *priv, u16 mode)
{
	usbhs_write(priv, TESTMODE, mode);
}

/*
 *		frame functions
 */
int usbhs_frame_get_num(struct usbhs_priv *priv)
{
	return usbhs_read(priv, FRMNUM) & FRNM_MASK;
}

/*
 *		usb request functions
 */
void usbhs_usbreq_get_val(struct usbhs_priv *priv, struct usb_ctrlrequest *req)
{
	u16 val;

	val = usbhs_read(priv, USBREQ);
	req->bRequest		= (val >> 8) & 0xFF;
	req->bRequestType	= (val >> 0) & 0xFF;

	req->wValue	= usbhs_read(priv, USBVAL);
	req->wIndex	= usbhs_read(priv, USBINDX);
	req->wLength	= usbhs_read(priv, USBLENG);
}

void usbhs_usbreq_set_val(struct usbhs_priv *priv, struct usb_ctrlrequest *req)
{
	usbhs_write(priv, USBREQ,  (req->bRequest << 8) | req->bRequestType);
	usbhs_write(priv, USBVAL,  req->wValue);
	usbhs_write(priv, USBINDX, req->wIndex);
	usbhs_write(priv, USBLENG, req->wLength);

	usbhs_bset(priv, DCPCTR, SUREQ, SUREQ);
}

/*
 *		bus/vbus functions
 */
void usbhs_bus_send_sof_enable(struct usbhs_priv *priv)
{
	u16 status = usbhs_read(priv, DVSTCTR) & (USBRST | UACT);

	if (status != USBRST) {
		struct device *dev __attribute__((unused));
		dev_err(dev, "usbhs should be reset\n");
	}

	usbhs_bset(priv, DVSTCTR, (USBRST | UACT), UACT);
}

void usbhs_bus_send_reset(struct usbhs_priv *priv)
{
	usbhs_bset(priv, DVSTCTR, (USBRST | UACT), USBRST);
}

int usbhs_bus_get_speed(struct usbhs_priv *priv)
{
	u16 dvstctr = usbhs_read(priv, DVSTCTR);

	switch (RHST & dvstctr) {
	case RHST_LOW_SPEED:
		return USB_SPEED_LOW;
	case RHST_FULL_SPEED:
		return USB_SPEED_FULL;
	case RHST_HIGH_SPEED:
		return USB_SPEED_HIGH;
	}

	return USB_SPEED_UNKNOWN;
}

int usbhs_vbus_ctrl(struct usbhs_priv *priv, int enable)
{
	struct platform_device *pdev = usbhs_priv_to_pdev(priv);

	return usbhs_platform_call(priv, set_vbus, pdev, enable);
}

static void usbhsc_bus_init(struct usbhs_priv *priv)
{
	usbhs_write(priv, DVSTCTR, 0);

	usbhs_vbus_ctrl(priv, 0);
}

/*
 *		device configuration
 */
int usbhs_set_device_config(struct usbhs_priv *priv, int devnum,
			   u16 upphub, u16 hubport, u16 speed)
{
	struct device *dev __attribute__((unused));
	u16 usbspd = 0;
	u32 reg = DEVADD0 + (2 * devnum);

	if (devnum > 10) {
		dev_err(dev, "cannot set speed to unknown device %d\n", devnum);
		return -EIO;
	}

	if (upphub > 0xA) {
		dev_err(dev, "unsupported hub number %d\n", upphub);
		return -EIO;
	}

	switch (speed) {
	case USB_SPEED_LOW:
		usbspd = USBSPD_SPEED_LOW;
		break;
	case USB_SPEED_FULL:
		usbspd = USBSPD_SPEED_FULL;
		break;
	case USB_SPEED_HIGH:
		usbspd = USBSPD_SPEED_HIGH;
		break;
	default:
		dev_err(dev, "unsupported speed %d\n", speed);
		return -EIO;
	}

	usbhs_write(priv, reg,	UPPHUB(upphub)	|
				HUBPORT(hubport)|
				USBSPD(usbspd));

	return 0;
}

/*
 *		interrupt functions
 */
void usbhs_xxxsts_clear(struct usbhs_priv *priv, u16 sts_reg, u16 bit)
{
	u16 pipe_mask = (u16)GENMASK(usbhs_get_dparam(priv, pipe_size), 0);

	usbhs_write(priv, sts_reg, ~(1 << bit) & pipe_mask);
}

/*
 *		local functions
 */
static void usbhsc_set_buswait(struct usbhs_priv *priv)
{
	int wait = usbhs_get_dparam(priv, buswait_bwait);

	/* set bus wait if platform have */
	if (wait)
		usbhs_bset(priv, BUSWAIT, 0x000F, wait);
}

/*
 *		platform default param
 */

/* commonly used on newer SH-Mobile and R-Car SoCs */
static struct renesas_usbhs_driver_pipe_config usbhsc_new_pipe[] = {
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_CONTROL, 64, 0x00, false),
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_ISOC, 1024, 0x08, true),
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_ISOC, 1024, 0x28, true),
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_BULK, 512, 0x48, true),
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_BULK, 512, 0x58, true),
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_BULK, 512, 0x68, true),
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_INT, 64, 0x04, false),
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_INT, 64, 0x05, false),
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_INT, 64, 0x06, false),
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_BULK, 512, 0x78, true),
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_BULK, 512, 0x88, true),
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_BULK, 512, 0x98, true),
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_BULK, 512, 0xa8, true),
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_BULK, 512, 0xb8, true),
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_BULK, 512, 0xc8, true),
	RENESAS_USBHS_PIPE(USB_ENDPOINT_XFER_BULK, 512, 0xd8, true),
};

/*
 *		power control
 */
static void usbhsc_power_ctrl(struct usbhs_priv *priv, int enable)
{
	struct platform_device *pdev = usbhs_priv_to_pdev(priv);
	struct device *dev __attribute__((unused));

	pr_dbg("++%s(0x%p, %d)\n", __func__, pdev, enable);

	if (enable) {
		/* enable PM */
		pm_runtime_get_sync(dev);

		/* enable platform power */
		usbhs_platform_call(priv, power_ctrl, pdev, priv->base, enable);

		/* USB on */
		usbhs_sys_clock_ctrl(priv, enable);
	} else {
		/* USB off */
		usbhs_sys_clock_ctrl(priv, enable);

		/* disable platform power */
		usbhs_platform_call(priv, power_ctrl, pdev, priv->base, enable);

		/* disable PM */
		pm_runtime_put_sync(dev);
	}
	pr_dbg("--%s\n", __func__);
}

/*
 *		hotplug
 */
static void usbhsc_hotplug(struct usbhs_priv *priv)
{
	struct platform_device *pdev = usbhs_priv_to_pdev(priv);
	int id;
	int enable;
	int ret;

	/*
	 * get vbus status from platform
	 */

	pr_dbg("++%s\n", __func__);

	/*
	 * Hack: We need to enable it here to avoid entering host mode
	 * and fifo select error
	 * Since this is u-boot, we may sacrifice real hotbplug function.
	 */
	enable = 1;
	/*
	 * get id from platform
	 */
	id = usbhs_platform_call(priv, get_id, pdev);

	pr_dbg("perform enable\n");
	ret = usbhs_mod_change(priv, id);
	if (ret < 0)
		return;

	dev_dbg(&pdev->dev, "%s enable\n", __func__);

	usbhsc_power_ctrl(priv, enable);

	/* bus init */
	usbhsc_set_buswait(priv);
	usbhsc_bus_init(priv);

	/* module start */
	usbhs_mod_call(priv, start, priv);

	pr_dbg("--%s\n", __func__);
}

/*
 *		notify hotplug
 */

void usbhsc_notify_hotplug(struct usbhs_priv *priv)
{
	pr_dbg("+-%s\n", __func__);
	usbhsc_hotplug(priv);
}

static int usbhsc_drvcllbck_notify_hotplug(struct platform_device *pdev)
{
	struct usbhs_priv *priv = usbhs_pdev_to_priv(pdev);
	int delay = usbhs_get_dparam(priv, detection_delay);

	pr_dbg("+-%s\n", __func__);
	mdelay(delay);
	usbhsc_notify_hotplug(priv);

	return 0;
}

/*
 *		platform functions
 */


#define USBHS_BASE	0x15820000
int usbhs_probe(struct platform_device *pdev)
{
	struct usbhs_priv *priv;
	int ret;

	pr_dbg("++%s\n", __func__);

	/* usb private data */
	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->base = (void *)USBHS_BASE;
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);


	/*
	 * care platform info
	 */
	pr_dbg("priv->dparam.type = %ld\n", priv->dparam.type);
	priv->dparam.type = USBHS_TYPE_G2L;

	priv->pfunc = usbhs_g2l_ops;
	if (!priv->dparam.pipe_configs) {
		priv->dparam.pipe_configs = usbhsc_new_pipe;
		priv->dparam.has_cnen = 1;
		priv->dparam.cfifo_byte_addr = 1;
		priv->dparam.pipe_size = ARRAY_SIZE(usbhsc_new_pipe);
	}

	if (!priv->dparam.pio_dma_border)
		priv->dparam.pio_dma_border = 64; /* 64byte */

	if (priv->pfunc.get_vbus)
		usbhsc_flags_set(priv, USBHSF_RUNTIME_PWCTRL);

	/*
	 * priv settings
	 */
	priv->pdev	= pdev;
	spin_lock_init(usbhs_priv_to_lock(priv));

	/* Fix fifo selection error */
	if (priv->pfunc.power_ctrl) {
		platform_set_drvdata(pdev, priv);
		usbhsc_power_ctrl(priv, 1);
	}

	/* call pipe and module init */
	ret = usbhs_pipe_probe(priv);
	if (ret < 0)
		return ret;

	ret = usbhs_fifo_probe(priv);
	if (ret < 0)
		goto probe_end_pipe_exit;

	ret = usbhs_mod_probe(priv);
	if (ret < 0)
		goto probe_end_fifo_exit;

	/* dev_set_drvdata should be called after usbhs_mod_init */
	platform_set_drvdata(pdev, priv);

	/*
	 * deviece reset here because
	 * USB device might be used in boot loader.
	 */
	usbhs_sys_clock_ctrl(priv, 0);

	/*
	 * platform call
	 *
	 * USB phy setup might depend on CPU/Board.
	 * If platform has its callback functions,
	 * call it here.
	 */
	ret = usbhs_platform_call(priv, hardware_init, pdev);
	if (ret < 0) {
		dev_err(&pdev->dev, "platform init failed.\n");
		goto probe_end_mod_exit;
	}

	/* reset phy for connection */
	usbhs_platform_call(priv, phy_reset, pdev);

	/* power control */
	pm_runtime_enable(&pdev->dev);
	if (!usbhsc_flags_has(priv, USBHSF_RUNTIME_PWCTRL)) {
		usbhsc_power_ctrl(priv, 1);
		usbhs_mod_autonomy_mode(priv);
	}

	/*
	 * manual call notify_hotplug for cold plug
	 */
	usbhsc_drvcllbck_notify_hotplug(pdev);

	dev_info(&pdev->dev, "probed\n");

	pr_dbg("--%s\n", __func__);

	return ret;

probe_end_mod_exit:
	usbhs_mod_remove(priv);
probe_end_fifo_exit:
	usbhs_fifo_remove(priv);
probe_end_pipe_exit:
	usbhs_pipe_remove(priv);

	dev_info(&pdev->dev, "probe failed\n");

	pr_dbg("--%s(-1)\n", __func__);

	return ret;
}

int usbhs_remove(struct platform_device *pdev)
{
	struct usbhs_priv *priv = usbhs_pdev_to_priv(pdev);

	pr_dbg("++%s\n", __func__);

	/* power off */
	if (!usbhsc_flags_has(priv, USBHSF_RUNTIME_PWCTRL))
		usbhsc_power_ctrl(priv, 0);

	usbhs_mod_remove(priv);
	usbhs_fifo_remove(priv);
	usbhs_pipe_remove(priv);

	pr_dbg("--%s\n", __func__);

	return 0;
}

void usbhs_dump_regs(struct usbhs_priv *priv)
{
	int i;
	u16 reg16;
	u32 reg32;

	if (!priv) {
		printf("Error: usbhs_priv structure is NULL\n");
		return;
	}
	printf("\r\n");
	for (i = 0; i < ARRAY_SIZE(usbhs_regnames)-1; i++) {
		reg16  = usbhs_read(priv, usbhs_regnames[i].regaddr);
		printf("%s [0x%x] = 0x%x\n", usbhs_regnames[i].regname,
							 usbhs_regnames[i].regaddr,
							 reg16);
	}
	/*Last register is a special one since it's 32-bit*/

	reg32 = readl(priv->base + usbhs_regnames[i].regaddr);

	printf("%s [0x%x] = 0x%x\n", usbhs_regnames[i].regname,
						 usbhs_regnames[i].regaddr,
						 reg32);
}


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Renesas USB driver");
MODULE_AUTHOR("Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>");

