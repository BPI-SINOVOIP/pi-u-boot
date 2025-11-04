/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Defines macros and constants for Renesas RZ/G2L pin controller pin
 * muxing functions.
 */
#ifndef __DT_BINDINGS_RZG2L_PINCTRL_H
#define __DT_BINDINGS_RZG2L_PINCTRL_H

#define RZG2L_PINS_PER_PORT	8

/*
 * Store the pin index from its port and position number in bits[11-0].
 * And store its peripheral function mode identifier in 3 bits [14-12]
 */
#define RZG2L_PINMUX(port, pos, func)	\
	(((port) * RZG2L_PINS_PER_PORT + (pos)) | ((func) << 12))

#endif /* __DT_BINDINGS_RZG2L_PINCTRL_H */
