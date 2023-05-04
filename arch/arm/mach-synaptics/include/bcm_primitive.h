/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */
#ifndef INCLUDE_BCM_PRIMITIVES_H
#define INCLUDE_BCM_PRIMITIVES_H

// Following definitions need to be consistent with BCM f/w
typedef enum _DRM_PRIMITIVE
{
    // Dummy
    DRM_GET_VERSION                 = 0x0000,

    // Secure boot
    VERIFY_IMAGE                    = 0x1000,
    VERIFY_SPLIT_IMAGE              = 0x1001,

    //WideWine
    DRM_WV_INITIALIZE               = 0x2000,
    DRM_WV_TERMINATE                = 0x2001,
    DRM_WV_OPENSESSION              = 0x2002,
    DRM_WV_CLOSESESSION             = 0x2003,
    DRM_WV_GENERATEDERIVEDKEYS      = 0x2004,
    DRM_WV_LOADKEYS_CHECKSIGNATURE  = 0x2005,
    DRM_WV_LOADKEYS                 = 0x2006,
    DRM_WV_LOADKEYS_UPDATE          = 0x2007,
    DRM_WV_REFRESHKEYS_CHECKSIGNATURE = 0x2008,
    DRM_WV_REFRESHKEYS              = 0x2009,
    DRM_WV_REFRESHKEYS_UPDATE       = 0x200A,
    DRM_WV_GENERATENONCE            = 0x200B,
    DRM_WV_GENERATESIGNATURE        = 0x200C,
    DRM_WV_REWRAPDEVICERSAKEY       = 0x200D,
    DRM_WV_LOADDEVICERSAKEY         = 0x200E,
    DRM_WV_GENERATERSASIGNATURE     = 0x200F,
    DRM_WV_DERIVEKEYSFROMSESSIONKEY = 0x2010,
    DRM_WV_GETKEYDATA               = 0x2011,
    DRM_WV_GETRANDOM                = 0x2012,
    DRM_WV_REMOVEKEY                = 0x2013,

    // GTV CA
    DRM_GTV_CA_LOADKEY_ADVANCE      = 0x3000,
    DRM_GTV_CA_SIGN                 = 0x3001,
    DRM_GTV_CA_UNLOADKEY            = 0x3002,

    // OpenCrypto
    DRM_OPENCRYPTO                  = 0x4000,

    // Gen random data
    DRM_GET_RANDOM                  = 0x4003,

    // HDCP
    DRM_HDCP_HDMI_TX_LOADKEYS       = 0x5000,

    // PlayReady

    // Standard TEE

    //USB Device Mode
    EROM_TURN_OFF_USB_DEVICE = 0x8000,
    EROM_RESET_USB_CONSOLE_BUF = 0x8001,
    EROM_TURN_ON_DEVICE        = 0x8002,
    EROM_FLUSH_USB_CONSOLE        = 0x8003,
} DRM_PRIMITIVE;


#endif //INCLUDE_BCM_PRIMITIVES_H

