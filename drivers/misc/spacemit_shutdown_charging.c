#include <common.h>
#include <fdt_support.h>
#include <log.h>
#include <irq.h>
#include <dm.h>
#include <dm/lists.h>
#include <power-domain.h>
#include <power/regulator.h>
#include <asm/sbi.h>
#include <power/battery.h>
#include <asm/csr.h>
#include <asm/io.h>
#include <led.h>
#include <asm-generic/gpio.h>
#include <power/charger.h>
#include <power/spacemit/spacemit_pmic.h>
#include <linux/delay.h>
#include <backlight.h>

/* ===== Macros ===== */
#define SBI_HSM_SUSP_TOP_BIT         0x92000000

#define PLIC_PMIC_PRIO_REG           0xe0000100
#define PLIC_PMIC_THRESH_REG         0xe0201000
#define PLIC_PMIC_EN_REG             0xe0002088
#define PLIC_PMIC_PENDING_REG        0xe0001008

#define RTC_CLK_SEL_EXTERNAL_OSC     0x8
#define RTC_EN                       0x4
#define RTC_TICK_TYPE_1S            0x0
#define RTC_TICK_TYPE_1MIN          0x1
#define RTC_TICK_EN                  0x40
#define RTC_CRYSTAL_EN               0x1
#define RTC_OUT_32K_EN               0x2
#define RTC_TICK_IRQ                 0x20

#define WAKEUP_SOURCE_POWER_KEY_EVENT  0x0
#define WAKEUP_SOURCE_POWER_KEY_INTER  0x1
#define WAKEUP_SOURCE_RTC_WAKEUP_CTRL  0x2
#define WAKEUP_SOURCE_RTC_WAKEUP_EVENT 0x3
#define WAKEUP_SOURCE_RTC_WAKEUP_IRQ   0x4
#define SYS_SHUTDOWN                   0x5
#define SYS_REBOOT_FLAG                0x6

#define SYS_SHUTDOWN_BIT       0x4

#define BATTERY_RED_LIGHT_INDEX   0x1
#define BATTERY_GREEN_LIGHT_INDEX 0x0

#define MAX_GPIO_COUNT         3
#define MAX_REGULATOR_COUNT    7
#define MAX_CHARGER_COUNT      2

#define CHARGER_LIGHT_FLASHES_CNT 3

#define BACKLIGHT_ON_BRIGHTNESS 50

/* ===== Data Structures ===== */
struct pt_regs {
	unsigned long long sepc;
	unsigned long long ra;
	unsigned long long sp;
	unsigned long long gp;
	unsigned long long tp;
	unsigned long long t0;
	unsigned long long t1;
	unsigned long long t2;
	unsigned long long s0;
	unsigned long long s1;
	unsigned long long a0;
	unsigned long long a1;
	unsigned long long a2;
	unsigned long long a3;
	unsigned long long a4;
	unsigned long long a5;
	unsigned long long a6;
	unsigned long long a7;
	unsigned long long s2;
	unsigned long long s3;
	unsigned long long s4;
	unsigned long long s5;
	unsigned long long s6;
	unsigned long long s7;
	unsigned long long s8;
	unsigned long long s9;
	unsigned long long s10;
	unsigned long long s11;
	unsigned long long t3;
	unsigned long long t4;
	unsigned long long t5;
	unsigned long long t6;
	/* Supervisor */
	unsigned long long sstatus;
	unsigned long long sbadaddr;
	unsigned long long scause;
};

struct suspend_context {
	/* Saved and restored by low-level functions */
	struct pt_regs regs;
	/* Saved and restored by high-level functions */
	unsigned long scratch;
	unsigned long tvec;
	unsigned long ie;
} __aligned(64);

struct shutdown_charge {
	/* Electricity meter (battery device) */
	struct udevice *ele_meter;

	/* Wakeup source regulators */
	struct udevice *wkup_set[MAX_REGULATOR_COUNT];
	const char **wkup_name;

	/* LED charging indicators */
	struct gpio_desc led_indicators[MAX_GPIO_COUNT];
	int gpio_cnt;
	u32 valid_level;

	/* Backlight for mipi display */
	struct udevice *backlight;

	/* Charger devices */
	struct udevice *charger[MAX_CHARGER_COUNT];
	const char **charger_name;
	int charger_cnt;

	/* Power domain */
	struct power_domain pm_domain;

	/* Battery capacity thresholds */
	int threshold0;
	int threshold1;
	int threshold2;
};

/* ===== External Declarations ===== */
extern int __cpu_suspend_enter(void *context);
extern int __cpu_resume_enter(void *context);
extern void flush_dcache_range(unsigned long start, unsigned long end);
extern void battery_level_display(int target_level);
extern void clear_battery_display(void);
#ifdef CONFIG_TYPEC_HUSB239
extern int husb239_detect_pd(void);
#endif

/* Global suspend context */
static struct suspend_context context = { 0 };

/* ---------------------------------------------------------------------- */
/*                      Suspend/Resume Helpers                            */
/* ---------------------------------------------------------------------- */
static int sbi_suspend_finisher(unsigned long suspend_type,
								unsigned long resume_addr,
								unsigned long opaque)
{
	struct sbiret ret;

	flush_dcache_range((unsigned long)&context,
						(unsigned long)(&context) + sizeof(struct suspend_context));

	ret = sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_SUSPEND,
					suspend_type, resume_addr, opaque, 0, 0, 0);

	return (ret.error) ? ret.error : 0;
}

static void suspend_save_csrs(struct suspend_context *context)
{
	context->scratch = csr_read(CSR_SSCRATCH);
	context->tvec    = csr_read(CSR_STVEC);
	context->ie      = csr_read(CSR_SIE);
}

static void suspend_restore_csrs(struct suspend_context *context)
{
	csr_write(CSR_SSCRATCH, context->scratch);
	csr_write(CSR_STVEC,    context->tvec);
	csr_write(CSR_SIE,      context->ie);
}

int cpu_suspend(unsigned long arg,
                int (*finish)(unsigned long arg, unsigned long entry, unsigned long context))
{
	int rc = 0;

	if (!finish) {
		return -EINVAL;
	}

	suspend_save_csrs(&context);

	if (__cpu_suspend_enter(&context)) {
		rc = finish(arg,
					(unsigned long)__cpu_resume_enter,
					(unsigned long)&context);
		if (!rc) {
			rc = -EOPNOTSUPP;
		}
	}

	suspend_restore_csrs(&context);

	return rc;
}


/* ---------------------------------------------------------------------- */
/*                  Backlight Control Functions                           */
/* ---------------------------------------------------------------------- */
static int spacemit_display_backlight_on(struct shutdown_charge *priv, int brightness)
{
	if (!priv->backlight)
		return -ENODEV;

	return backlight_set_brightness(priv->backlight, brightness);
}

static int spacemit_display_backlight_off(struct shutdown_charge *priv)
{
	if (!priv->backlight)
		return -ENODEV;

	return backlight_set_brightness(priv->backlight, BACKLIGHT_OFF);
}


/* ---------------------------------------------------------------------- */
/*                  Device Initialization (init_devices)                  */
/* ---------------------------------------------------------------------- */

/* 1) Initialize battery (electricity meter) */
static int init_electricity_meter(struct udevice *dev, struct shutdown_charge *priv)
{
	int ret = uclass_get_device_by_phandle(UCLASS_BATTERY, dev,
											"electricity-meter",
											&priv->ele_meter);
	if (ret) {
		printf("%s:%d, failed to get electricity meter\n", __func__, __LINE__);
		return ret;
	}
	return 0;
}

/* 2) Initialize power domain */
static int init_power_domain(struct udevice *dev, struct shutdown_charge *priv)
{
	int ret = power_domain_get(dev, &priv->pm_domain);
	if (ret) {
		printf("%s:%d, failed to get power domain\n", __func__, __LINE__);
	}
	return ret;
}

/* 3) Initialize wakeup source regulators */
static int init_wkup_regulators(struct udevice *dev, struct shutdown_charge *priv)
{
	int ret = dev_read_string_list(dev, "wk-name", &priv->wkup_name);
	if (ret < 0) {
		printf("%s:%d, failed to read wk source names\n", __func__, __LINE__);
		return ret;
	}

	int count = dev_read_string_count(dev, "wk-name");
	for (int i = 0; i < count; ++i) {
		ret = uclass_get_device_by_phandle(UCLASS_REGULATOR,
											dev,
											priv->wkup_name[i],
											&priv->wkup_set[i]);
		if (ret) {
			printf("%s:%d, failed to get regulator: %s\n",
					__func__, __LINE__, priv->wkup_name[i]);
			return ret;
		}
	}
	return 0;
}

/* 4) Initialize chargers */
static int init_chargers(struct udevice *dev, struct shutdown_charge *priv)
{
	int ret;

	priv->charger_cnt = dev_read_string_count(dev, "charger-name");
	if (priv->charger_cnt < 0) {
		printf("%s:%d, failed to get charger count\n", __func__, __LINE__);
		return priv->charger_cnt;
	}

	ret = dev_read_string_list(dev, "charger-name", &priv->charger_name);
	if (ret < 0) {
		printf("%s:%d, failed to read charger names\n", __func__, __LINE__);
		return ret;
	}

	for (int i = 0; i < priv->charger_cnt; ++i) {
		ret = uclass_get_device_by_phandle(UCLASS_CHARGER,
											dev,
											priv->charger_name[i],
											&priv->charger[i]);
		if (ret) {
			printf("%s:%d, failed to get charger: %s\n",
					__func__, __LINE__, priv->charger_name[i]);
			return ret;
		}
	}
	return 0;
}

/* 5) Initialize LED GPIO for charging indicators */
static int init_leds(struct udevice *dev, struct shutdown_charge *priv)
{
	int ret = gpio_request_list_by_name(dev,
										"charge-light",
										priv->led_indicators,
										MAX_GPIO_COUNT,
										GPIOD_IS_OUT);
	if (ret < 0) {
		printf("%s:%d, failed to get charge-light GPIO\n", __func__, __LINE__);
		return ret;
	}
	priv->gpio_cnt = ret;

	ret = dev_read_u32_array(dev, "valid-level", &priv->valid_level, 1);
	if (ret) {
		printf("%s:%d, failed to read valid-level\n", __func__, __LINE__);
		return ret;
	}

	/* Turn off LEDs by default */
	for (int i = 0; i < priv->gpio_cnt; ++i) {
		dm_gpio_set_value(&priv->led_indicators[i], !(!!priv->valid_level));
	}
	return 0;
}

/* 6) Initialize backlight for mipi display */
static int init_backlight(struct udevice *dev, struct shutdown_charge *priv)
{
	int ret = uclass_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT,
											dev,
											"backlight",
											&priv->backlight);
	if (ret) {
		printf("%s:%d, failed to get backlight\n", __func__, __LINE__);
		return ret;
	}
	return 0;
}

/* 7) Update charging status */
static int update_charging_status(struct shutdown_charge *priv)
{
	int ret;
	int battery_status;

#ifdef CONFIG_TYPEC_HUSB239
	ret = husb239_detect_pd();
	if (ret < 0) {
		printf("Failed to detect PD\n");
	}
#endif

	battery_status = battery_get_status(priv->ele_meter);
	if (battery_status < 0) {
		printf("Failed to get battery status\n");
		return battery_status;
	}

	for (int i = 0; i < priv->charger_cnt; ++i) {
		ret = charger_get_status(priv->charger[i]);
		if (ret < 0) {
			printf("Failed to get charger %d status\n", i);
			continue;
		}

		// Set charging current based on battery status
		switch (battery_status) {
			case BAT_STATE_VERY_LOW:
				charger_set_current(priv->charger[i], 500000);  // Use 500mA low current charging when voltage below 3V
				break;
			case BAT_STATE_NEED_CHARGING:
			case BAT_STATE_NORMAL:
				charger_set_current(priv->charger[i], 2000000);  // Normal charging with 2A
				break;
			default:
				charger_set_current(priv->charger[i], 0);  // Stop charging for other states
				break;
		}
	}

	return 0;
}

/*
 * Main init_devices() which calls all sub-initialization functions
 */
static int init_devices(struct udevice *dev, struct shutdown_charge *priv)
{
	int ret;

	priv->threshold0 = dev_read_u32_default(dev, "battery-threshold0", 5);
	priv->threshold1 = dev_read_u32_default(dev, "battery-threshold1", 10);
	priv->threshold2 = dev_read_u32_default(dev, "battery-threshold2", 100);

	ret = init_electricity_meter(dev, priv);
	if (ret) {
		return ret;
	}

	ret = init_power_domain(dev, priv);
	if (ret) {
		return ret;
	}

	ret = init_wkup_regulators(dev, priv);
	if (ret) {
		return ret;
	}

	ret = init_chargers(dev, priv);
	if (ret) {
		return ret;
	}

	ret = init_leds(dev, priv);
	if (ret) {
		return ret;
	}

	ret = init_backlight(dev, priv);
	if (ret) {
		return ret;
	}

	return 0;
}

/* ---------------------------------------------------------------------- */
/*       Checking Status and Basic Wakeup/PLIC Configuration/Cleanup      */
/* ---------------------------------------------------------------------- */

/*
 * Check reboot flag, power key event, battery capacity, charger status
 */
static int check_reboot_or_powerup(struct shutdown_charge *priv,
								unsigned int *pwr_key_status,
								unsigned int *reboot_flag,
								int *capacity_out)
{
	int ret;
	int status = 0;

	/* Read power key event status */
	*pwr_key_status = regulator_get_value(priv->wkup_set[WAKEUP_SOURCE_POWER_KEY_EVENT]);

	/* Read battery capacity (SoC) */
	ret = battery_get_soc(priv->ele_meter);
	if (ret < 0) {
		printf("%s:%d, failed to get battery capacity\n", __func__, __LINE__);
		return ret;
	}
	*capacity_out = ret;

	/* Read charger status */
	for (int i = 0; i < priv->charger_cnt; ++i) {
		status |= charger_get_status(priv->charger[i]);
	}

	/* Read and clear reboot flag */
	*reboot_flag = regulator_get_value(priv->wkup_set[SYS_REBOOT_FLAG]);
	printf("reboot flag before clear: 0x%x\n", *reboot_flag);

	*reboot_flag &= ~(1 << SYS_REBOOT_FLAG_BIT);
	regulator_set_value_force(priv->wkup_set[SYS_REBOOT_FLAG], *reboot_flag);

	/* Read again after clear */
	unsigned int flag_after = regulator_get_value(priv->wkup_set[SYS_REBOOT_FLAG]);
	printf("reboot flag after clear: 0x%x\n", flag_after);

	return status; /* Return charger status */
}

static bool need_exit_early(struct shutdown_charge *priv,
						unsigned int reboot_flag,
						unsigned int pwr_key_status,
						int capacity,
						int charger_status)
{
	if ((reboot_flag & SYS_REBOOT_FLAG_BIT) && (capacity >= priv->threshold1)) {
		printf("reboot_flag & SYS_REBOOT_FLAG_BIT && (capacity >= priv->threshold1)\n");
		return true;
	}

	if ((pwr_key_status & PWRKEY_RISING_EVENT) &&
		(capacity >= priv->threshold1)) {
		printf("(pwr_key_status & PWRKEY_RISING_EVENT) && (capacity >= priv->threshold1)\n");
		return true;
	}

	return false;
}

/*
 * Configure wakeup sources and PLIC
 */
static void config_wakeup_and_plic(struct shutdown_charge *priv,
								unsigned int *prio,
								unsigned int *thresh)
{
	/* Clear power key pending events */
	regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_POWER_KEY_EVENT],
								PWRKEY_RISING_EVENT |
								PWRKEY_FAILING_EVENT |
								PWRKEY_LONG_PRESS_EVENT |
								PWRKEY_SHORT_PRESS_EVENT);

	/* Enable RTC basic functions */
	regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_CTRL],
								RTC_CLK_SEL_EXTERNAL_OSC |
								RTC_EN |
								RTC_OUT_32K_EN |
								RTC_CRYSTAL_EN);

	/* Clear RTC tick event */
	regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_EVENT], 0xff);

	/* Read and set PLIC priority and threshold */
	*prio   = readl((void __iomem *)PLIC_PMIC_PRIO_REG);
	writel(8, (void __iomem *)PLIC_PMIC_PRIO_REG);

	*thresh = readl((void __iomem *)PLIC_PMIC_THRESH_REG);
	writel(7, (void __iomem *)PLIC_PMIC_THRESH_REG);

	/* Enable PLIC interrupt */
	writel(1, (void __iomem *)PLIC_PMIC_EN_REG);

	/* Clear pending */
	writel(0, (void __iomem *)PLIC_PMIC_PENDING_REG);
}

/*
 * Cleanup PLIC and RTC after exiting the loop
 */
static void cleanup_wakeup_and_plic(struct shutdown_charge *priv,
									unsigned int prio,
									unsigned int thresh)
{
	/* Restore PLIC config */
	writel(0, (void __iomem *)PLIC_PMIC_PENDING_REG);
	writel(thresh, (void __iomem *)PLIC_PMIC_THRESH_REG);
	writel(prio,   (void __iomem *)PLIC_PMIC_PRIO_REG);
	writel(0,      (void __iomem *)PLIC_PMIC_EN_REG);

	/* Disable power key interrupt & clear pending */
	regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_POWER_KEY_INTER], 0);
	regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_POWER_KEY_EVENT],
								PWRKEY_RISING_EVENT |
								PWRKEY_FAILING_EVENT |
								PWRKEY_LONG_PRESS_EVENT |
								PWRKEY_SHORT_PRESS_EVENT);

	/* Disable RTC tick */
	int ctrl_val = regulator_get_value(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_CTRL]);
	ctrl_val &= ~(RTC_TICK_EN | RTC_EN);
	regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_CTRL], ctrl_val);
	regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_EVENT], 0xff);

	/* Disable RTC completely */
	regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_CTRL], 0);
	regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_IRQ], 0);
}

/* ---------------------------------------------------------------------- */
/*   Helper: handle suspend/wakeup repeated logic (no multi-press count)  */
/* ---------------------------------------------------------------------- */
static void handle_suspend_and_wakeup(struct shutdown_charge *priv,
									unsigned int *pwr_key_status,
									unsigned long long *plugin_count,
									unsigned long long *plugout_count)
{
	/* 1) Enable power key failing event */
	regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_POWER_KEY_INTER],
								PWRKEY_FAILING_EVENT);

	/* 2) Enter low power state */
	spacemit_display_backlight_off(priv);

	power_domain_on(&priv->pm_domain);
	cpu_suspend(SBI_HSM_SUSP_TOP_BIT, sbi_suspend_finisher);
	power_domain_off(&priv->pm_domain);

	/* 3) After wakeup, read power key status again */
	*pwr_key_status = regulator_get_value(priv->wkup_set[WAKEUP_SOURCE_POWER_KEY_EVENT]);
	if (*pwr_key_status & PWRKEY_FAILING_EVENT) {

		printf("%s:%d, pwr_key_status:%x => user pressed power key\n",
				__func__, __LINE__, *pwr_key_status);

		*plugin_count             = 0;
		*plugout_count            = 0;
	}

	/* 4) Clear power key pending */
	regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_POWER_KEY_EVENT],
								PWRKEY_RISING_EVENT |
								PWRKEY_FAILING_EVENT |
								PWRKEY_LONG_PRESS_EVENT |
								PWRKEY_SHORT_PRESS_EVENT);

	/* 5) Disable power key interrupt */
	regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_POWER_KEY_INTER], 0);

	/* 6) Disable RTC tick */
	int ctrl_val = regulator_get_value(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_CTRL]);
	ctrl_val &= ~(RTC_TICK_EN | RTC_EN);
	regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_CTRL], ctrl_val);

	/* 7) Clear RTC event and irq */
	regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_EVENT], 0xff);
	regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_IRQ], 0);

	/* 8) Clear PLIC pending */
	writel(0, (void __iomem *)PLIC_PMIC_PENDING_REG);
}

static void set_led_state(struct shutdown_charge *priv, bool red_on, bool green_on)
{
	/*
	* If valid_level=1 => '1' means ON, '0' means OFF
	* If valid_level=0 => '0' means ON, '1' means OFF
	*/
	bool red_val   = priv->valid_level ? red_on   : !red_on;
	bool green_val = priv->valid_level ? green_on : !green_on;

	dm_gpio_set_value(&priv->led_indicators[BATTERY_RED_LIGHT_INDEX],   red_val);
	dm_gpio_set_value(&priv->led_indicators[BATTERY_GREEN_LIGHT_INDEX], green_val);
}

static void blink_red_led_n_times(struct shutdown_charge *priv, int count, int on_ms, int off_ms)
{
	for (int i = 0; i < count; i++) {
		set_led_state(priv, true, false);
		mdelay(on_ms);

		set_led_state(priv, false, false);
		mdelay(off_ms);
	}
}

static int handle_led_unplugged(struct shutdown_charge *priv,
								int capacity,
								unsigned int *pwr_key_status,
								unsigned long long *plugout_count,
								bool charger_changed)
{
	bool red_on = false;
	bool green_on = false;
	/* Example debug prints */
	printf("charger is NOT attached, capacity=%d%%\n", capacity);

	/* If unplugged for too long => shutdown logic (optional) */
	if (*plugout_count == CHARGER_LIGHT_FLASHES_CNT) {
		printf("Don't have charger, shutdown\n");
		blink_red_led_n_times(priv, 5, 200, 200);
		regulator_set_value_force(priv->wkup_set[SYS_SHUTDOWN], SYS_SHUTDOWN);
	}

	/* Set LED according to different capacity ranges */
	if (capacity < priv->threshold0) {
		/* Range 1: capacity < STHRESHOLD0 => shutdown led */
		printf("Range 1 (unplugged): very low battery\n");
		red_on   = false;
		green_on = false;
		set_led_state(priv, red_on, green_on);

		if ((*pwr_key_status & PWRKEY_FAILING_EVENT)) {
			printf("Detect power key pressed and charger is unplugged, but not enough capacity to power on\n");
			blink_red_led_n_times(priv, 5, 200, 200);
		}
	}
	else if (capacity < priv->threshold1) {
		/* Range 2: [STHRESHOLD0, STHRESHOLD1) => show battery level when power key pressed */
		printf("Range 2 (unplugged): somewhat low battery\n");
		red_on   = false;
		green_on = false;
		set_led_state(priv, red_on, green_on);

		if ((*pwr_key_status & PWRKEY_FAILING_EVENT)) {
			printf("Detect power key pressed and charger is unplugged, but not enough capacity to power on\n");
			spacemit_display_backlight_on(priv, BACKLIGHT_ON_BRIGHTNESS);
			battery_level_display(capacity);

			blink_red_led_n_times(priv, 5, 200, 200);
			mdelay(3000);
			clear_battery_display();
		}
	}
	else if (capacity < priv->threshold2) {
		/* Range 3: [STHRESHOLD1, STHRESHOLD2) => ready to boot and blink red led when power key pressed*/
		printf("Range 3 (unplugged): partial\n");
		red_on   = false;
		green_on = false;
		set_led_state(priv, red_on, green_on);

		if ((*pwr_key_status & PWRKEY_FAILING_EVENT)) {
			printf("Detect power key pressed, enough capacity to power on\n");
			blink_red_led_n_times(priv, 5, 200, 200);
			return 1; /* Trigger upper-layer logic to power on */
		}
	}
	else {
		/* Range 4: capacity >= STHRESHOLD2 => ready to boot when power key pressed*/
		printf("Range 4 (unplugged): battery full\n");
		red_on   = false;
		green_on = false;
		set_led_state(priv, red_on, green_on);

		if ((*pwr_key_status & PWRKEY_FAILING_EVENT)) {
			printf("Detect power key pressed, enough capacity to power on\n");
			return 1; /* Trigger upper-layer logic to power on */
		}
	}

	/* If no need to exit loop, return 0 */
	return 0;
}

static int handle_led_plugged(struct shutdown_charge *priv,
							int capacity,
							unsigned int *pwr_key_status,
							unsigned long long *plugin_count,
							bool charger_changed)
{
	bool red_on = false;
	bool green_on = false;
	/* Possible debug print */
	printf("charger is attached, capacity=%d%%\n", capacity);

	if (capacity < priv->threshold0) {
		/* Range 1: capacity < STHRESHOLD0 => red steady and blink red led when power key pressed */
		printf("Range 1 (plugged): very low battery \n");
		red_on   = true;
		green_on = false;
		set_led_state(priv, red_on, green_on);

		if ((*pwr_key_status & PWRKEY_FAILING_EVENT)) {
			printf("Detect power key pressed and charger is plugged, but not enough capacity to power on\n");
			blink_red_led_n_times(priv, 5, 200, 200);
		}
	}
	else if (capacity < priv->threshold1) {
		/* Range 2: [STHRESHOLD0, STHRESHOLD1) => red steady and show battery level when plugged and blink red led when power key pressed */
		printf("Range 2 (plugged): somewhat low \n");
		if (charger_changed) {
			spacemit_display_backlight_on(priv, BACKLIGHT_ON_BRIGHTNESS);
			battery_level_display(capacity);
		}

		red_on   = true;
		green_on = false;
		set_led_state(priv, red_on, green_on);

		if ((*pwr_key_status & PWRKEY_FAILING_EVENT)) {
			printf("Detect power key pressed and charger is plugged, but not enough capacity to power on\n");
			blink_red_led_n_times(priv, 5, 200, 200);
			mdelay(3000);
		} else {
			mdelay(5000);
		}

		clear_battery_display();
	}
	else if (capacity < priv->threshold2) {
		/* Range 3: [STHRESHOLD1, STHRESHOLD2) => red steady and show battery level when plugged and ready to boot when power key pressed */
		printf("Range 3 (plugged): partial\n");
		if (charger_changed) {
			spacemit_display_backlight_on(priv, BACKLIGHT_ON_BRIGHTNESS);
			battery_level_display(capacity);
		}

		red_on   = true;
		green_on = false;
		set_led_state(priv, red_on, green_on);

		/* If we want to detect power-key to power on, we could return 1 to "exit loop => system on" */
		if ((*pwr_key_status & PWRKEY_FAILING_EVENT)) {
			printf("Detect power key pressed and charger is plugged, enough capacity to power on\n");
			return 1; /* Trigger upper-layer logic to power on */
		} else {
			mdelay(5000);
		}

		clear_battery_display();
	}
	else {
		/* Range 4: capacity >= priv->threshold2 => green steady and show battery level when plugged and ready to boot when power key pressed */
		printf("Range 4 (plugged): battery full\n");
		if (charger_changed) {
			spacemit_display_backlight_on(priv, BACKLIGHT_ON_BRIGHTNESS);
			battery_level_display(capacity);
		}

		red_on   = false;
		green_on = true;
		set_led_state(priv, red_on, green_on);

		/* If we want to detect power-key to power on, we could return 1 to "exit loop => system on" */
		if ((*pwr_key_status & PWRKEY_FAILING_EVENT)) {
			printf("Detect power key pressed and charger is plugged, enough capacity to power on\n");
			return 1; /* Trigger upper-layer logic to power on */
		} else {
			mdelay(5000);
		}

		clear_battery_display();
	}

	/* If no need to exit loop, return 0 */
	return 0;
}

static int run_charging_loop(struct shutdown_charge *priv,
							unsigned int *pwr_key_status,
							unsigned long long *plugin_count,
							unsigned long long *plugout_count)
{
	int capacity;
	int charger_status;
	int last_charger_status = -1;
	printf(" run_charging_loop \n");

	while (1) {
		/* 1) Read battery capacity */
		capacity = battery_get_soc(priv->ele_meter);
		if (capacity < 0) {
			printf("%s:%d, failed to get battery capacity\n", __func__, __LINE__);
			return capacity;
		}

		/* 2) Determine if charger is plugged in */
		charger_status = 0;
		for (int i = 0; i < priv->charger_cnt; ++i) {
			charger_status |= charger_get_status(priv->charger[i]);
		}
		bool charger_changed = (charger_status != last_charger_status);

		/* 3) Scenario when unplugged */
		if (!charger_status) {
			*plugin_count          = 0;
			++(*plugout_count);

			/* Helper for LED states in "unplugged" scenario */
			int want_boot = handle_led_unplugged(priv,
												capacity,
												pwr_key_status,
												plugout_count,
												charger_changed);

			if (want_boot == 1) {
				return 1;
			}

			/* Enable RTC tick to allow suspend + wake up */
			int tmp_val = regulator_get_value(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_CTRL]);
			tmp_val |= (RTC_TICK_EN | RTC_EN);
			regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_CTRL], tmp_val);
			regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_IRQ], RTC_TICK_IRQ);
		}

		/* 4) Scenario when plugged in */
		else {
			*plugout_count = 0;
			++(*plugin_count);

			update_charging_status(priv);

			/* Helper for LED states in "plugged" scenario */
			int want_boot = handle_led_plugged(priv,
												capacity,
												pwr_key_status,
												plugin_count,
												charger_changed);
			/* If handle_led_plugged() returns 1 => means "request boot => exit loop" */
			if (want_boot == 1) {
				return 0;
			}

			/* Enable RTC tick to allow suspend + wake up */
			int tmp_val = regulator_get_value(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_CTRL]);
			tmp_val |= (RTC_TICK_EN | RTC_EN);
			regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_CTRL], tmp_val);
			regulator_set_value_force(priv->wkup_set[WAKEUP_SOURCE_RTC_WAKEUP_IRQ], RTC_TICK_IRQ);
		}

		last_charger_status = charger_status;

		/* 5) Handle suspend + wake cycle */
		handle_suspend_and_wakeup(priv,
									pwr_key_status,
									plugin_count,
									plugout_count);
	}

	return 0; /* not reached */
}

/* ---------------------------------------------------------------------- */
/*  Interface to start / manage charging after probe initialization       */
/* ---------------------------------------------------------------------- */
int shutdown_charge_manager(struct udevice *dev)
{
	int ret;
	struct shutdown_charge *priv = dev_get_priv(dev);

	unsigned int pwr_key_status = 0;
	unsigned int reboot_flag = 0;
	unsigned long long plugin_count = 0, plugout_count = 0;
	unsigned int prio = 0, thresh = 0;
	int capacity = 0;

	/* 1) Check initial conditions */
	int charger_status = check_reboot_or_powerup(priv,
												&pwr_key_status,
												&reboot_flag,
												&capacity);
	if (charger_status < 0) {
		return charger_status;
	}

	/* 2) Possibly exit early if capacity is high enough & reboot was flagged */
	if (need_exit_early(priv, reboot_flag, pwr_key_status, capacity, charger_status)) {
		return 1;
	}

	/* 3) Set up wakeup / PLIC config */
	config_wakeup_and_plic(priv, &prio, &thresh);

	/* 4) Run charging loop */
	ret = run_charging_loop(priv,
							&pwr_key_status,
							&plugin_count,
							&plugout_count);

	/* 5) Cleanup upon exit */
	cleanup_wakeup_and_plic(priv, prio, thresh);

	return ret;
}

/* ---------------------------------------------------------------------- */
/*                       Driver's probe function                          */
/* ---------------------------------------------------------------------- */
static int shutdown_charge_probe(struct udevice *dev)
{
	struct shutdown_charge *priv = dev_get_priv(dev);

	int ret = init_devices(dev, priv);
	if (ret) {
		printf("%s:%d, init_devices failed\n", __func__, __LINE__);
		return ret;
	}

	/* Start charging management loop */
	shutdown_charge_manager(dev);

	return 0;
}

/* Device match table */
static const struct udevice_id shutdown_charging_ids[] = {
	{ .compatible = "k1,shutdown-charging" },
	{ }
};

U_BOOT_DRIVER(shutdown_charge) = {
	.name       = "shutdown-charge",
	.of_match   = shutdown_charging_ids,
	.id         = UCLASS_MISC,
	.probe      = shutdown_charge_probe,
	.priv_auto  = sizeof(struct shutdown_charge),
};
