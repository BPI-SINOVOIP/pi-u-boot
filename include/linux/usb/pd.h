/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __LINUX_USB_PD_H
#define __LINUX_USB_PD_H

/* PDO: Power Data Object */
#define PDO_MAX_OBJECTS		7

enum pd_pdo_type {
	PDO_TYPE_FIXED = 0,
	PDO_TYPE_BATT = 1,
	PDO_TYPE_VAR = 2,
	PDO_TYPE_APDO = 3,
};

#define PDO_TYPE_SHIFT		30
#define PDO_TYPE_MASK		0x3

#define PDO_TYPE(t)	((t) << PDO_TYPE_SHIFT)

#define PDO_VOLT_MASK		0x3ff
#define PDO_CURR_MASK		0x3ff
#define PDO_PWR_MASK		0x3ff

#define PDO_FIXED_DUAL_ROLE		BIT(29)	/* Power role swap supported */
#define PDO_FIXED_SUSPEND		BIT(28) /* USB Suspend supported (Source) */
#define PDO_FIXED_HIGHER_CAP		BIT(28) /* Requires more than vSafe5V (Sink) */
#define PDO_FIXED_EXTPOWER		BIT(27) /* Externally powered */
#define PDO_FIXED_USB_COMM		BIT(26) /* USB communications capable */
#define PDO_FIXED_DATA_SWAP		BIT(25) /* Data role swap supported */
#define PDO_FIXED_UNCHUNK_EXT		BIT(24) /* Unchunked Extended Message supported (Source) */
#define PDO_FIXED_FRS_CURR_MASK		(BIT(24) | BIT(23)) /* FR_Swap Current (Sink) */
#define PDO_FIXED_FRS_CURR_SHIFT	23
#define PDO_FIXED_VOLT_SHIFT		10	/* 50mV units */
#define PDO_FIXED_CURR_SHIFT		0	/* 10mA units */

#define PDO_FIXED_VOLT(mv)	((((mv) / 50) & PDO_VOLT_MASK) << PDO_FIXED_VOLT_SHIFT)
#define PDO_FIXED_CURR(ma)	((((ma) / 10) & PDO_CURR_MASK) << PDO_FIXED_CURR_SHIFT)

#define PDO_FIXED(mv, ma, flags)			\
	(PDO_TYPE(PDO_TYPE_FIXED) | (flags) |		\
	 PDO_FIXED_VOLT(mv) | PDO_FIXED_CURR(ma))

#define VSAFE5V 5000 /* mv units */

#define PDO_BATT_MAX_VOLT_SHIFT	20	/* 50mV units */
#define PDO_BATT_MIN_VOLT_SHIFT	10	/* 50mV units */
#define PDO_BATT_MAX_PWR_SHIFT	0	/* 250mW units */

#define PDO_BATT_MIN_VOLT(mv) ((((mv) / 50) & PDO_VOLT_MASK) << PDO_BATT_MIN_VOLT_SHIFT)
#define PDO_BATT_MAX_VOLT(mv) ((((mv) / 50) & PDO_VOLT_MASK) << PDO_BATT_MAX_VOLT_SHIFT)
#define PDO_BATT_MAX_POWER(mw) ((((mw) / 250) & PDO_PWR_MASK) << PDO_BATT_MAX_PWR_SHIFT)

#define PDO_BATT(min_mv, max_mv, max_mw)			\
	(PDO_TYPE(PDO_TYPE_BATT) | PDO_BATT_MIN_VOLT(min_mv) |	\
	 PDO_BATT_MAX_VOLT(max_mv) | PDO_BATT_MAX_POWER(max_mw))

#define PDO_VAR_MAX_VOLT_SHIFT	20	/* 50mV units */
#define PDO_VAR_MIN_VOLT_SHIFT	10	/* 50mV units */
#define PDO_VAR_MAX_CURR_SHIFT	0	/* 10mA units */

#define PDO_VAR_MIN_VOLT(mv) ((((mv) / 50) & PDO_VOLT_MASK) << PDO_VAR_MIN_VOLT_SHIFT)
#define PDO_VAR_MAX_VOLT(mv) ((((mv) / 50) & PDO_VOLT_MASK) << PDO_VAR_MAX_VOLT_SHIFT)
#define PDO_VAR_MAX_CURR(ma) ((((ma) / 10) & PDO_CURR_MASK) << PDO_VAR_MAX_CURR_SHIFT)

#define PDO_VAR(min_mv, max_mv, max_ma)				\
	(PDO_TYPE(PDO_TYPE_VAR) | PDO_VAR_MIN_VOLT(min_mv) |	\
	 PDO_VAR_MAX_VOLT(max_mv) | PDO_VAR_MAX_CURR(max_ma))

enum pd_apdo_type {
	APDO_TYPE_PPS = 0,
};

#define PDO_APDO_TYPE_SHIFT	28	/* Only valid value currently is 0x0 - PPS */
#define PDO_APDO_TYPE_MASK	0x3

#define PDO_APDO_TYPE(t)	((t) << PDO_APDO_TYPE_SHIFT)

#define PDO_PPS_APDO_MAX_VOLT_SHIFT	17	/* 100mV units */
#define PDO_PPS_APDO_MIN_VOLT_SHIFT	8	/* 100mV units */
#define PDO_PPS_APDO_MAX_CURR_SHIFT	0	/* 50mA units */

#define PDO_PPS_APDO_VOLT_MASK	0xff
#define PDO_PPS_APDO_CURR_MASK	0x7f

#define PDO_PPS_APDO_MIN_VOLT(mv)	\
	((((mv) / 100) & PDO_PPS_APDO_VOLT_MASK) << PDO_PPS_APDO_MIN_VOLT_SHIFT)
#define PDO_PPS_APDO_MAX_VOLT(mv)	\
	((((mv) / 100) & PDO_PPS_APDO_VOLT_MASK) << PDO_PPS_APDO_MAX_VOLT_SHIFT)
#define PDO_PPS_APDO_MAX_CURR(ma)	\
	((((ma) / 50) & PDO_PPS_APDO_CURR_MASK) << PDO_PPS_APDO_MAX_CURR_SHIFT)

#define PDO_PPS_APDO(min_mv, max_mv, max_ma)				\
	(PDO_TYPE(PDO_TYPE_APDO) | PDO_APDO_TYPE(APDO_TYPE_PPS) |	\
	PDO_PPS_APDO_MIN_VOLT(min_mv) | PDO_PPS_APDO_MAX_VOLT(max_mv) |	\
	PDO_PPS_APDO_MAX_CURR(max_ma))

static inline enum pd_pdo_type pdo_type(u32 pdo)
{
	return (pdo >> PDO_TYPE_SHIFT) & PDO_TYPE_MASK;
}

static inline unsigned int pdo_fixed_voltage(u32 pdo)
{
	return ((pdo >> PDO_FIXED_VOLT_SHIFT) & PDO_VOLT_MASK) * 50;
}

static inline unsigned int pdo_min_voltage(u32 pdo)
{
	return ((pdo >> PDO_VAR_MIN_VOLT_SHIFT) & PDO_VOLT_MASK) * 50;
}

static inline unsigned int pdo_max_voltage(u32 pdo)
{
	return ((pdo >> PDO_VAR_MAX_VOLT_SHIFT) & PDO_VOLT_MASK) * 50;
}

static inline unsigned int pdo_max_current(u32 pdo)
{
	return ((pdo >> PDO_VAR_MAX_CURR_SHIFT) & PDO_CURR_MASK) * 10;
}

static inline unsigned int pdo_max_power(u32 pdo)
{
	return ((pdo >> PDO_BATT_MAX_PWR_SHIFT) & PDO_PWR_MASK) * 250;
}

static inline enum pd_apdo_type pdo_apdo_type(u32 pdo)
{
	return (pdo >> PDO_APDO_TYPE_SHIFT) & PDO_APDO_TYPE_MASK;
}

static inline unsigned int pdo_pps_apdo_min_voltage(u32 pdo)
{
	return ((pdo >> PDO_PPS_APDO_MIN_VOLT_SHIFT) &
		PDO_PPS_APDO_VOLT_MASK) * 100;
}

static inline unsigned int pdo_pps_apdo_max_voltage(u32 pdo)
{
	return ((pdo >> PDO_PPS_APDO_MAX_VOLT_SHIFT) &
		PDO_PPS_APDO_VOLT_MASK) * 100;
}

static inline unsigned int pdo_pps_apdo_max_current(u32 pdo)
{
	return ((pdo >> PDO_PPS_APDO_MAX_CURR_SHIFT) &
		PDO_PPS_APDO_CURR_MASK) * 50;
}

#endif /* __LINUX_USB_PD_H */
