/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#ifndef _VPP_API_H_
#define _VPP_API_H_

#include "ErrorCode.h"
#include "errcode_vpp.h"
#include "vpp_api_types.h"

#if 1
//macro's needed for porting
#define Vpp VPP_FL
#define MV_VPPOBJ MV_VPPOBJ_FL_
#endif

#ifdef VPP_IN_TRUST_ZONE
#ifdef TZ_3_0
//This is not required in uboot
//#define printk printf
#else
#define printf printk
#endif
#endif

#ifdef VPP_ENABLE_FLOW_PRINT
#define VPP_FLOW_PRINT(...)  printf(__VA_ARGS__)
#define VPP_FLOW_PRINT1(...)  dbg_printf(__VA_ARGS__)
#else
#define VPP_FLOW_PRINT(...)
#define VPP_FLOW_PRINT1(...)
#endif

#ifdef TZ_3_0
//SHM_SHARE_SZ allocated size of NW_noncacheable should not exceed 3MB !
//due to "NonSecure-NC" region size limitation
#define SHM_SHARE_SZ 3*1024*1024
#endif

#if !defined(VPP_IN_TRUST_ZONE)
typedef void (*voidfunc_t)(void);
#endif

/*Enable to configure PLANE alpha for GFX1 plane*/
#define CONFIG_ENABLE_GFX1_PLANE_ALPHA
#if (CONFIG_MV_AMP_ENABLE_CLKPWR_GATING == true)
/* Enable EN_VPP_DYNAMIC_CLKGATE to dynamically clock gate various IPs in VPP based on if they are using or soft bypass mode */
#define EN_VPP_DYNAMIC_CLKGATE
#endif
#ifdef EN_VPP_DYNAMIC_CLKGATE
/* We can do SRAM gating only if respective block is clk gated
 * SRAM gating is avaiable in BG4_CDp/CDp_A0
 */
#if (BERLIN_CHIP_VERSION==BERLIN_BG4_CDP) || (BERLIN_CHIP_VERSION==BERLIN_BG4_CDP_A0)
#define EN_VPP_SRAM_CLKGATE
#define EN_VPP_DOLBY_CLKGATE
#define EN_VPP_NEW_NON_DOLBY_CLKGATE
#define EN_VPP_NON_DOLBY_NEW_BG4CDPA0_CLKGATE
#define EN_DOLBY_DYNAMIC_CLKGATE
#endif
#endif

/* VPP SCL Coeff Macros */
#define VPP_FRC_SCL_MAIN_LAY_MAX 2 /* Main and VIP SCL */
#define VPP_FRC_SCL_GFX_LAY_MAX  2 /* PIP and OSD */
#define VPP_FRC_SCL_LAY_MAX      2 /* Max amoung all the scalers */
#define VPP_FRC_SCL_NUM_OF_PHASES 17
#define VPP_FRC_SCL_NUM_OF_COEFF  8
#define VPP_FRC_SCL_COEFF_TAB_SIZE  (VPP_FRC_SCL_NUM_OF_PHASES * VPP_FRC_SCL_NUM_OF_COEFF * sizeof(UINT16))

/* size if calculated as 7 values, 7 address and sizeof(UINT32) for each of them */
#define VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS_PERPHASE (7*2)
#define VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS (VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS_PERPHASE * VPP_FRC_SCL_NUM_OF_PHASES)
#define VPP_FRC_COEFFTAB_BCMBUF_SIZE_PERPHASE (VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS_PERPHASE*sizeof(UINT32))
#define VPP_FRC_COEFFTAB_BCMBUF_SIZE (VPP_FRC_COEFFTAB_BCMBUF_SIZE_PERPHASE * VPP_FRC_SCL_NUM_OF_PHASES)

#define CAPTURE_BUFFER_SIZE (4096*2160*2) //4K YUV422

/* Enable one of the following macro for tiled mode only;
 * based on VDEC is sending (stride*4) or (stride*1) in m_buf_stride & m_buf_stride_UV
 * Note: legacy VDEC use stride factor = 1*/
//#define VPP_TILED_MODE_STRIDE_FACTOR    1
#define VPP_TILED_MODE_STRIDE_FACTOR    4

#if !defined(VPP_DIAGS)
//HDMI IS ENABLED by macro 'HDMI_SRV_SUPPORT'
//HDCP IS ENABLED by macro 'ENABLE_HDMI_HDCP'


//Enable HDCP only on silicon
#define ENABLE_HDMI_HDCP

//HDCP is disabled for time being
#if 1
#undef ENABLE_HDMI_HDCP
#endif

#endif

//Enable DHUB Clear for respective channel whenever Underflow is detected
//#define ENABLE_DHUB_CLEAR_UPON_UF
//Enable logging of Underflow condition irrespective of whether DHUB clear is done or not
//#define ENABLE_UF_DETECTION_LOGGING
//Enable logging for all planes
//#define ENABLE_UF_DETECTION_LOGGING_FOR_ALL

//Enable the cropping and scaling support for interlaced output
#define VPP_ENABLE_INTERLACED_OUTPUT_SCALING_CROPING

#if defined(VPP_DIAGS) || defined(VPP_ON_FPGA) || defined(VPP_TEST_APP)
//Feature macro for FPGA specific testing, VPP_TEST_APP should be defined for VPP_Test_APP
//Note: SC - dispservice does not require this; Please check with SH - dispservice
#define ENABLE_CROP_FOR_FPGA_TESTING
#define ENABLE_REFWIN_FOR_FPGA_TESTING
#endif

//Enable to read full resolution to crop tiled format and 420SP format
//#define ENABLE_FE_FULLCROP

#ifdef ENABLE_FE_FULLCROP
/* Enable to croma alligned linedropping when full FETG cropping is available.
  Eg. 2 contiguous Y lines and corresponding 1 UV alines for 2x line droppping and smilarly for other linedrops */
//#define ENABLE_CROMA_ALLIGNED_LINEDROP
#endif

//Macro to control debug prints
//#define ENABLE_CROP_DEBUG_PRINTS

//Enable debug prints
#ifdef ENABLE_CROP_DEBUG_PRINTS
#define VDB_print(...)  printf(__VA_ARGS__)
//Macro to control debug prints in vpp isr
#define ENABLE_CROP_CALC_DEBUG_PRINTS
#else
#define VDB_print(...)
#endif

//Enable debug prints - for displaying windows
#ifdef ENABLE_CROP_DEBUG_PRINTS
#define VPP_DEBUG_CROP_PREFIX   "VPPDBG:CROP:"
#if defined(VPP_DIAGS)
#define PRINT_WIN(planeID, win)      \
    printf(VPP_DEBUG_CROP_PREFIX " %s : plane [%d], x[%d], y[%d], width[%d], height[%d] \n",__FUNCTION__, planeID, win->x, win->y, win->width, win->height);
#define PRINT_WIN1(win)      \
    printf(VPP_DEBUG_CROP_PREFIX " %s : Win [%s], x[%d], y[%d], width[%d], height[%d] \n",__FUNCTION__, #win, (win)->x, (win)->y, (win)->width, (win)->height);
#else
//linux Terminal may require \n\r
#define PRINT_WIN(planeID, win)      \
    printf(VPP_DEBUG_CROP_PREFIX " %s : plane [%d], x[%d], y[%d], width[%d], height[%d] \n\r",__FUNCTION__, planeID, win->x, win->y, win->width, win->height);
#define PRINT_WIN1(win)      \
    printf(VPP_DEBUG_CROP_PREFIX " %s : Win [%s], x[%d], y[%d], width[%d], height[%d] \n\r",__FUNCTION__, #win, (win)->x, (win)->y, (win)->width, (win)->height);
#endif
#else //ENABLE_CROP_DEBUG_PRINTS
#define PRINT_WIN(planeID, win)
#define PRINT_WIN1(win)
#endif //ENABLE_CROP_DEBUG_PRINTS

// error code definitions
#define MV_VPP_OK          S_VPP_OK
#define MV_VPP_ENODEV      VPP_E_NODEV
#define MV_VPP_EBADPARAM   VPP_E_BADPARAM
#define MV_VPP_EBADCALL    VPP_E_BADCALL
#define MV_VPP_EUNSUPPORT  VPP_E_UNSUPPORT
#define MV_VPP_EIOFAIL     VPP_E_IOFAIL
#define MV_VPP_EUNCONFIG   VPP_E_UNCONFIG
#define MV_VPP_ECMDQFULL   VPP_E_CMDQFULL
#define MV_VPP_EFRAMEQFULL VPP_E_FRAMEQFULL
#define MV_VPP_EBCMBUFFULL VPP_E_BCMBUFFULL
#define MV_VPP_ENOMEM      VPP_E_NOMEM
#define MV_VPP_EVBIBUFFULL VPP_EVBIBUFFULL
#define MV_VPP_EHARDWAREBUSY VPP_EHARDWAREBUSY
#define MV_VPP_ENOSINKCNCTED  VPP_ENOSINKCNCTED
#define MV_VPP_ENOHDCPENABLED VPP_ENOHDCPENABLED

#define VPP_IOCTL_VBI_DMA_CFGQ 0xbeef0001
#define VPP_IOCTL_VBI_BCM_CFGQ 0xbeef0002
#define VPP_IOCTL_VDE_BCM_CFGQ 0xbeef0003
#define VPP_IOCTL_TIMING       0xbeef0004
#define VPP_IOCTL_START_BCM_TRANSACTION 0xbeef0005
#define VPP_IOCTL_BCM_SCHE_CMD 0xbeef0006
#define VPP_IOCTL_INTR_MSG     0xbeef0007
#define CEC_IOCTL_RX_MSG_BUF_MSG     0xbeef0008
#define VPP_IOCTL_GET_VSYNC             0xbeef0009
#define HDMITX_IOCTL_UPDATE_HPD         0xbeef000a
#define VPP_IOCTL_INIT                  0xbeef000b
#define ISR_IOCTL_SET_AFFINITY              0xbeef000c
#define VPP_IOCTL_GET_QUIESCENT_FLAG         0xbeef000d
#define VPP_IOCTL_GPIO_WRITE            0xbeef000e

#define CEC_IOCTL_INTR_MSG      0xbeef0001
#define CEC_IOCTL_GET_MSG       0xbeef0002
#define CEC_IOCTL_RX_MSG_BUF    0xbeef0003

#define VPP_CC_MSG_TYPE_VPP 0x00
#define VPP_CC_MSG_TYPE_CEC 0x01

// Callback function type
#if defined(CONFIG_VPP_RECOVERY_MODE_SUPPORT)|| defined(CONFIG_DOLBY_VISION_SUPPORT)
//for CDP A0 only
typedef HRESULT (*MV_VPP_EVENT_CALLBACK)(UINT32 EventCode, void *EventInfo, void *Context);
#else
#if !defined(VPP_IN_TRUST_ZONE)
typedef HRESULT (*MV_VPP_EVENT_CALLBACK)(UINT32 EventCode, void *EventInfo, void *Context);
#else
typedef HRESULT (*MV_VPP_EVENT_CALLBACK)(void *ptr,UINT32 EventCode, void *EventInfo, void *Context);
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* OVP related macros */
#define VPP_FRAME_IN_USE    0x10

#define VPP_ATTR_PARAM_GET_GLOBALPHA(val)          _BFGET_(val,31,16)
#define VPP_ATTR_PARAM_GET_PLANEID(val)            _BFGET_(val,15,0)
#define VPP_ATTR_PARAM_SET_GLOBALPHA(addr,val)     _BFSET_(addr,31,16,val)
#define VPP_ATTR_PARAM_SET_PLANEID(addr,val)       _BFSET_(addr,15,0,val)

#define TOTAL_HDMI_TX_REGISTERS               0x56
#define TOTAL_HDCP_TX_REGISTERS               0x19
#define HDMI_DUMP_REG_COUNT (TOTAL_HDMI_TX_REGISTERS + TOTAL_HDCP_TX_REGISTERS)

typedef struct VPP_INTR_MSG_T {
    UINT32 DhubSemMap;
    UINT32 Enable;
} VPP_INTR_MSG;

/* definition of display mode */
typedef enum {
    DISP_NON_DEFINED        = -1, /* non-defined */
    DISP_STILL_PIC = 0, /* display still picture mode */
    DISP_FRAME     = 1, /* display frame mode */
    DISP_PATGEN    = 2, /* display pattern generated by patgen, only valid for main */
}ENUM_DISP_MODE;

/*definition of picture control*/
typedef enum{
    FIRST_PICTURE_CTRL = 0,
    PICTURE_CTRL_BRIGHTNESS = 0,
    PICTURE_CTRL_CONTRAST,
    PICTURE_CTRL_HUE,
    PICTURE_CTRL_SATURATION,
    PICTURE_CTRL_COLORTEMPERATURE,
    PICTURE_CTRL_MAX
}ENUM_PICTURE_CTRL;


/* definition of de-interlacer working mode */
typedef enum {
    DEINT_2D   = 0, /* 2D mode, only valid for interlaced input */
    DEINT_3D   = 1, /* 3D mode, only valid for interlaced input */
}ENUM_DEINT_MODE;

/* defination of DEINT parameter cmd */
typedef enum {
    DEINT_CMD_INVALID = -1,
    VPP_CMD_DEINT_SET_USERCAD=0,
    VPP_CMD_DEINT_SET_EXCLCTRL,
    VPP_CMD_DEINT_SET_LPFCTRL,
    VPP_CMD_DEINT_SET_FILM_CADCTRL,
    VPP_CMD_DEINT_SET_PRESET_MODE,
    VPP_CMD_DEINT_SET_FORCED_2D,
    VPP_CMD_DEINT_SET_HARD_SOFT_FILM_MODE,
    VPP_CMD_DEINT_SET_OSD_CTRL,
    VPP_CMD_DEINT_SET_DEINT_ADV_CTRL,
    MAX_NUM_DEINT_CMD
}VPP_CMD_EXT_DEINT;

typedef enum {
    SCL_CMD_INVALID = -1,
    VPP_CMD_SCL_LOAD_CUST_COEFFS,
    VPP_CMD_SCL_RELOAD_DEF_COEFFS,
    MAX_NUM_SCL_CMD

}VPP_CMD_EXT_SCL;

/* definition of video component data format in frame buffer */
typedef enum {
    SRCFMT_MIN      = 0,
#if defined(VPP_DIAGS)
    SRCFMT_YUV422   = 0, /* only valid for video plane */
    SRCFMT_ARGB32    = 1, /* only valid for gfx/PG planes */
    SRCFMT_YUV420SP = 2,
    SRCFMT_YUV420SP_TILED_MODE0 = 3,
    SRCFMT_YUV420SP_TILED_MODE1 = 4,
    SRCFMT_YUV420SP_TILED_V4H8 = 5,
    SRCFMT_ARGB32_PM = 6,
    SRCFMT_XRGB32 = 7,
    SRCFMT_IPT420 = 8,
    SRCFMT_IPT422   = 9,
    SRCFMT_IPT444   = 10,
    SRCFMT_YUV444   = 11,
    SRCFMT_YUV444_DWA   = 12,
    SRCFMT_YUV422SP_DWA = 13,
    SRCFMT_YUV420SP_DWA = 14,
    SRCFMT_ARGB4444 = 15,
    SRCFMT_RGB565  = 16,
    SRCFMT_ARGB1555 = 17,
    SRCFMT_ARBG2101010 =18,
    SRCFMT_ARGB8332 =   19,
#else
    SRCFMT_YUV422   = 0, /* only valid for video plane */
    SRCFMT_ARGB32   = 1, /* only valid for gfx/PG planes */
    SRCFMT_ARGB24   = 2, /* only valid for gfx planes */
    SRCFMT_ARGB4444 = 3, /* only valid for gfx planes */
    SRCFMT_RGB565   = 4, /* only valid for gfx planes */
    SRCFMT_ARGB1555 = 5, /* only valid for gfx planes */
    SRCFMT_LUT8     = 6, /* only valid for gfx planes */
    SRCFMT_AYUV32   = 7, /* only valid for PG planes */
    SRCFMT_YUV420   = 8, /* Only for VPP 4K Bypass path */
    SRCFMT_YUV420SP = 9,
    SRCFMT_YUV444   = 10,
    SRCFMT_RGB444   = 11,
    SRCFMT_ARGB32_4K   = 12,/* Only for 4K Bypass Path */
    SRCFMT_YUV420SP_TILED_MODE0 = 13,
    SRCFMT_YUV420SP_TILED_MODE1 = 14,
    SRCFMT_ARGB32_PM = 15,
    SRCFMT_XRGB32 = 16,
    SRCFMT_YUV420SP_TILED_V4H8 = 17,
    SRCFMT_IPT420 = 18,
    SRCFMT_IPT422   = 19,
    SRCFMT_IPT444   = 20,
    SRCFMT_YUV420SP_TILED_AUTO_MODE = 21,
    SRCFMT_YUV444_DWA   = 22,
    SRCFMT_YUV422SP_DWA = 23,
    SRCFMT_YUV420SP_DWA = 24,
    SRCFMT_ARBG2101010 =  25,
    SRCFMT_ARGB8332 =   26,
#endif
    SRCFMT_MAX
}ENUM_SRC_FMT;

#if (BERLIN_CHIP_VERSION >= BERLIN_VS680)
#define IS_GFX_SRC_FORMAT(src_fmt) ((src_fmt == SRCFMT_XRGB32) || (src_fmt == SRCFMT_ARGB32_PM) || (src_fmt == SRCFMT_ARGB32) || (src_fmt == SRCFMT_IPT444) \
     || (src_fmt == SRCFMT_ARGB1555) || (src_fmt == SRCFMT_ARGB4444) || (src_fmt == SRCFMT_ARGB8332) || (src_fmt == SRCFMT_RGB565) \
     || (src_fmt == SRCFMT_ARBG2101010))
#else
#define IS_GFX_SRC_FORMAT(src_fmt) ((src_fmt == SRCFMT_XRGB32) || (src_fmt == SRCFMT_ARGB32_PM) || (src_fmt == SRCFMT_ARGB32) || (src_fmt == SRCFMT_IPT444))
#endif

/* Bit mask for dolby vision configurature on Base layer present. it shall be on (1) when dolby vision present*/
#define VPP_DOLBY_VISION_BL  (1 << 0)
    /* Bit mask for dolby vision configurature on Enhance layer present (1) or not (0) */
#define VPP_DOLBY_VISION_EL  (1 << 1)
    /* Bit mask for dolby vision configurature on Meta data present. it shall be on (1) when dolby vision present */
#define VPP_DOLBY_VISION_RPU (1 << 2)

typedef enum VPP_DOLBY_PRESENT_COMPONENTS_t
{
    VPP_DOLBY_PRESENT_MIN = 0,
    VPP_DOLBY_PRESENT_NONE              =   0x0,
    VPP_DOLBY_PRESENT_BL                =   0x1,
    VPP_DOLBY_PRESENT_BL_METADATA       =   0x5,
    VPP_DOLBY_PRESENT_BL_EL_METADATA    =   0x7,
    VPP_DOLBY_PRESENT_MAX
}VPP_DOLBY_PRESENT_COMPONENTS;

typedef enum VPP_THDR_PRESENT_COMPONENTS_t
{
    VPP_THDR_PRESENT_MIN            =   0x0,
    VPP_THDR_PRESENT_NONE           =   0x0,
    VPP_THDR_PRESENT_THDR           =   0x1,
    VPP_THDR_PRESENT_THDR_METADATA  =   0X3,
    VPP_THDR_PRESENT_MAX
}VPP_THDR_PRESENT_COMPONENTS;

typedef enum VPP_WRITEBACK_MODES_t
{
    VPP_WRITEBACK_MODE_MIN  =   0,
    VPP_WRITEBACK_MODE_DISABLE  =   VPP_WRITEBACK_MODE_MIN,
    VPP_WRITEBACK_MODE_DV_COMP,
    VPP_WRITEBACK_MODE_OFFLINE_SCL,
    VPP_WRITEBACK_MODE_EDR,
    VPP_WRITEBACK_MODE_DITHER,
    VPP_WRITEBACK_MODE_MAX
}ENUM_VPP_WRITEBACK_MODES;

typedef enum VPP_FE_DV_COMP_MODES_t
{
    VPP_FE_DV_COMP_MODE_MIN =   0,
    //Disable Composer
    VPP_FE_DV_COMP_MODE_BYPASS  =  VPP_FE_DV_COMP_MODE_MIN,
    //Test Modes
    VPP_FE_DV_COMP_MODE_TEST_COMPOSER,
    VPP_FE_DV_COMP_MODE_TEST_CSC,
    VPP_FE_DV_COMP_MODE_TEST_BL,
    VPP_FE_DV_COMP_MODE_TEST_EL,
    VPP_FE_DV_COMP_MODE_TEST_CVM,
    //Functional Mode
    VPP_FE_DV_COMP_MODE_COMPOSER,
    VPP_FE_DV_COMP_MODE_BL,
    VPP_FE_DV_COMP_MODE_TEST_EDR,
    VPP_FE_DV_COMP_MODE_TEST_DITHER,
    VPP_FE_DV_COMP_MODE_MAX
}VPP_FE_DV_COMP_MODES;

typedef enum
{
    VPP_SCAN_INVERSE_MIN = 0,
    VPP_SCAN_INVERSE_NORMAL = VPP_SCAN_INVERSE_MIN,
    VPP_SCAN_INVERSE_H,
    VPP_SCAN_INVERSE_V,
    VPP_SCAN_INVERSE_HV,
    VPP_SCAN_INVERSE_MAX
}ENUM_VPP_SCAN_INVERSE;

/* definition of video component data order in frame buffer */
typedef enum {
    ORDER_ARGB      = 0, /* only valid for gfx planes */
    ORDER_ABGR      = 1, /* only valid for gfx planes */
    ORDER_RGBA      = 2, /* only valid for gfx planes */
    ORDER_BGRA      = 3, /* only valid for gfx planes */
    ORDER_ARGB32_MAX = 4, /* only valid for gfx planes */
    ORDER_AVYU      = 0, /* only valid for PG plane */
    ORDER_AUYV      = 1, /* only valid for PG plane */
    ORDER_VYUA      = 2, /* only valid for PG plane */
    ORDER_UYVA      = 3, /* only valid for PG plane */
    ORDER_UYVY      = 0, /* only valid for video planes */
    ORDER_VYUY      = 1, /* only valid for video planes */
    ORDER_YUYV      = 2, /* only valid for video planes */
    ORDER_YVYU      = 3, /* only valid for video planes */
}ENUM_SRC_ORDER;

typedef enum
{
    ROTATION_MIN    = 0,
    ROTATION_0D     = 0,
    ROTATION_90D    = 1,
    ROTATION_180D   = 2,
    ROTATION_270D   = 3,
    ROTATION_H_FLIP = 4,
    ROTATION_V_FLIP = 5,
    ROTATION_MAX
}ENUM_ROTATION;

typedef enum
{
    SCAN_INVERSE_MIN = 0,
    SCAN_INVERSE_NORMAL = SCAN_INVERSE_MIN,
    SCAN_INVERSE_H,
    SCAN_INVERSE_V,
    SCAN_INVERSE_HV,
    SCAN_INVERSE_MAX
}ENUM_SCAN_INVERSE;


typedef enum {
    SRC_PRI_RESERVED1,
    SRC_PRI_BT709,      /**< ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B . */
    SRC_PRI_UNSPECIFIED,
    SRC_PRI_RESERVED2,
    SRC_PRI_BT470M,
    SRC_PRI_BT470BG,    /**< ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM. */
    SRC_PRI_SMPTE170M,  /**< ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC. */
    SRC_PRI_SMPTE470M,  /**< functionally identical to above. */
    SRC_PRI_FILM,
    SRC_PRI_BT2020     /**< ITU-R BT2020. */
}ENUM_SRC_PRI;

typedef enum {
    SRC_DEPTH_8BIT,    /**<  8 bit color depth. */
    SRC_DEPTH_10BIT,   /**< 10 bit color depth. */
    SRC_DEPTH_12BIT,   /**< 12 bit color depth. */
    SRC_DEPTH_16BIT    /**< 16 bit color depth. */
}ENUM_SRC_DEPTH;

typedef enum {
    SRC_3D_VID_FMT_CBI,       /**< checherboard interleaving */
    SRC_3D_VID_FMT_CI,        /**< column interleaving */
    SRC_3D_VID_FMT_RI,        /**< row interleaving */
    SRC_3D_VID_FMT_SBS,       /**< side by side */
    SRC_3D_VID_FMT_TAB,       /**< top and bottom */
    SRC_3D_VID_FMT_FS,        /**< frame sequential */
    SRC_3D_VID_FMT_FP         /**< frame packing (for HDMI input frame) */
}ENUM_SRC_3D_VID_FMT;

/* definition of CPCB output video format */
typedef enum {
    OUTPUT_PROGRESSIVE   = 0,
    OUTPUT_INTERLACED     = 1,
    OUTPUT_AUTO_SELECT     = 2,
}ENUM_OUTPUT_MODE;

/* definition of all the output color supported by VPP*/
typedef enum {
    OUTPUT_COLOR_FMT_INVALID     = -1,
    FIRST_OUTPUT_COLOR_FMT     = 0,
    OUTPUT_COLOR_FMT_RGB888      = 0,
    OUTPUT_COLOR_FMT_YCBCR444    = 1,
    OUTPUT_COLOR_FMT_YCBCR422    = 2,
    OUTPUT_COLOR_FMT_YCBCR420    = 3,
    OUTPUT_COLOR_FMT_sRGB888     = 4,
    OUTPUT_COLOR_FMT_IPT444      = 5,
    OUTPUT_COLOR_FMT_IPT422      = 6,
    OUTPUT_COLOR_FMT_YCBCR422_P3D65  = 7,
    OUTPUT_COLOR_FMT_YCBCR422_R2020  = 8,
    OUTPUT_COLOR_FMT_YCBCR420_P3D65  = 9,
    OUTPUT_COLOR_FMT_YCBCR420_R2020  = 10,
    OUTPUT_COLOR_FMT_YCBCR444_P3D65  = 11,
    OUTPUT_COLOR_FMT_YCBCR444_R2020  = 12,
    OUTPUT_COLOR_FMT_IPT420      = 13,
    MAX_NUM_OUTPUT_COLOR_FMTS
} ENUM_OUTPUT_COLOR_FMT;

/* definition of all the bit depths (output) supported by VPP*/
typedef enum {
    OUTPUT_BIT_DEPTH_INVALID = -1,
    FIRST_OUTPUT_BIT_DEPTH   = 0,
    OUTPUT_BIT_DEPTH_12BIT     = 0,
    OUTPUT_BIT_DEPTH_10BIT   = 1,
    OUTPUT_BIT_DEPTH_8BIT     = 2,
    MAX_NUM_OUTPUT_BIT_DEPTHS
} ENUM_OUTPUT_BIT_DEPTH;

/* definition of all the input depths (input) supported by VPP*/
typedef enum {
    INPUT_BIT_DEPTH_INVALID = -1,
    FIRST_INPUT_BIT_DEPTH   = 0,
    INPUT_BIT_DEPTH_8BIT    = 0,
    INPUT_BIT_DEPTH_10BIT   = 1,
    INPUT_BIT_DEPTH_12BIT   = 2,
    INPUT_BIT_DEPTH_16BIT   = 3,
    INPUT_BIT_DEPTH_32BIT   = 4,
    MAX_NUM_INPUT_BIT_DEPTHS
} ENUM_INPUT_BIT_DEPTH;

/* definition of aspect ratio */
typedef enum {
    ASPECT_RATIO_INVALID = -1,
    FIRST_ASPECT_RATIO   = 0,
    ASPECT_RATIO_NONE    = 0,
    ASPECT_RATIO_4BY3    = 1,
    ASPECT_RATIO_16BY9   = 2,
    MAX_ASPECT_RATIO
} ENUM_ASPECT_RATIO;

/* definition of scan information */
typedef enum {
    SCAN_DATA_INVALID = -1,
    FIRST_SCAN_DATA   = 0,
    SCAN_DATA_NONE    = 0,
    OVER_SCAN_DATA    = 1,
    UNDER_SCAN_DATA   = 2,
    MAX_SCAN_DATA
} ENUM_SCAN_DATA;

/* definition of VPP TG timing formats */
typedef enum {
    RES_INVALID   = -1,
    FIRST_RES     = 0,
    RES_NTSC_M    = 0,
    RES_NTSC_J    = 1,
    RES_PAL_M     = 2,
    RES_PAL_BGH   = 3,
    RES_525I60    = 4,
    RES_525I5994  = 5,
    RES_625I50    = 6,
    RES_525P60    = 7,
    RES_525P5994  = 8,
    RES_625P50    = 9,
    RES_720P30    = 10,
    RES_720P2997  = 11,
    RES_720P25    = 12,
    RES_720P60    = 13,
    RES_720P5994  = 14,
    RES_720P50    = 15,
    RES_1080I60   = 16,
    RES_1080I5994 = 17,
    RES_1080I50   = 18,
    RES_1080P30   = 19,
    RES_1080P2997 = 20,
    RES_1080P25   = 21,
    RES_1080P24   = 22,
    RES_1080P2398 = 23,
    RES_1080P60   = 24,
    RES_1080P5994 = 25,
    RES_1080P50   = 26,
    RES_LVDS_1080P48   = 27,
    RES_LVDS_1080P50   = 28,
    RES_LVDS_1080P60   = 29,
    RES_LVDS_2160P12   = 30,
    RES_VGA_480P60 = 31,
    RES_VGA_480P5994 = 32,
    FIRST_RES_3D = 33,
    RES_720P50_3D = 33,
    RES_720P60_3D = 34,
    RES_720P5994_3D = 35,
    RES_1080P24_3D = 36,
    RES_1080P2398_3D = 37,
    RES_1080P30_3D = 38,
    RES_1080P2997_3D = 39,
    RES_1080P25_3D = 40,
    RES_1080I60_FP = 41,
    RES_1080I5994_FP = 42,
    RES_1080I50_FP = 43,
    RES_LVDS_1920X540P60_3D = 44,
    RES_LVDS_1920X540P30_3D = 45,
    RES_LVDS_1920X540P24_3D = 46,
    RES_LVDS_720P100_3D = 47,
    RES_LVDS_720P120_3D = 48,
    RES_LVDS_1080P48_3D = 49,
    RES_LVDS_1080P50_3D = 50,
    RES_LVDS_1080P60_3D = 51,
    RES_LVDS_1920X540P100_3D = 52,
    RES_LVDS_1920X540P120_3D = 53,
    RES_LVDS_960X1080P100_3D = 54,
    RES_LVDS_960X1080P120_3D = 55,
    MAX_NUM_RES_3D = 55,
    RES_MIN_4Kx2K      = 56,
    RES_4Kx2K2398      = 56,
    RES_4Kx2K24        = 57,
    RES_4Kx2K24_SMPTE  = 58,
    RES_4Kx2K25        = 59,
    RES_4Kx2K2997      = 60,
    RES_4Kx2K30        = 61,
    RES_4Kx2K50        = 62,
    RES_4Kx2K5994      = 63,
    RES_4Kx2K60        = 64,
    RES_4Kx2K30_HDMI   = 65,
    RES_4Kx1K120       = 66,
    RES_MAX_4Kx2K      = 66,
    RES_720P_4Kx1K120_3D = 67,
    RES_720P100        = 68,
    RES_720P11988      = 69,
    RES_720P120        = 70,
    HFR_RES_MIN        = 71,
    RES_1080P100       = 71,
    RES_1080P11988     = 72,
    RES_1080P120       = 73,
    RES_1080P8991      = 74,
    RES_1080P90        = 75,
    HFR_RES_MAX        = 75,
    RES_4Kx2K2398_SMPTE = 76,
    RES_4Kx2K25_SMPTE   = 77,
    RES_4Kx2K2997_SMPTE = 78,
    RES_4Kx2K30_SMPTE   = 79,
    RES_4Kx2K50_SMPTE   = 80,
    RES_4Kx2K5994_SMPTE = 81,
    RES_4Kx2K60_SMPTE   = 82,
    RES_4Kx2K50_420     = 83,
    RES_4Kx2K5994_420   = 84,
    RES_4Kx2K60_420     = 85,
    RES_4Kx2K2398_3D    = 86,
    RES_4Kx2K24_3D      = 87,
    RES_4Kx2K25_3D      = 88,
    RES_4Kx2K2997_3D    = 89,
    RES_4Kx2K30_3D      = 90,
    RES_LVDS_1088P60    = 91, //Non Standard Resolution
    RES_LVDS_1366X768P60 = 92,
    RES_LVDS_1366X768P5994 = 93,
    RES_LVDS_1366X768P50 = 94,
    RES_LVDS_1366X768P48 = 95,
    RES_LVDS_1366X768P4796 = 96,
    RES_MIN_DSI          = 97,
    RES_DSI_540x960P60   = 97,
    RES_DSI_1920x1200P60 = 98,
    RES_DSI_800x1280P60  = 99,
    RES_MAX_DSI          = 99,
    RES_RESET,
    MAX_NUM_RESS
}ENUM_CPCB_TG_RES;

/* definition of VPP status */
typedef enum {
    STATUS_INACTIVE         = 0, /* for plane and channel */
    STATUS_INACTIVE_PENDING = 1, /* for plane and channel */
    STATUS_ACTIVE           = 2, /* for plane and channel */
    STATUS_ACTIVE_PENDING   = 3, /* for plane and channel */
    STATUS_DISP             = 4, /* for channel only */
    STATUS_DISP_LOGO        = 5, /* for plane only */
    STATUS_DISP_VIDEO       = 6, /* for plane only */
    STATUS_DISP_PATGEN      = 7, /* for main plane */
}ENUM_PLANE_STATUS;

/* definition of VPP input planes */
typedef enum {
    PLANE_INVALID = -1,
    FIRST_PLANE  = 0,
    PLANE_MAIN   = 0,
    PLANE_PIP    = 1,
    PLANE_GFX1   = 2,
#if (BERLIN_CHIP_VERSION < BERLIN_VS680)
    PLANE_OVP_EL = 3,
    PLANE_VMX    = PLANE_OVP_EL,
#else
    PLANE_GFX2   = 3,
    PLANE_AUX    = 4,
    PLANE_OVP_EL = 5,
    PLANE_MAIN_EL = 6,
    PLANE_VMX    = PLANE_MAIN_EL,
#endif
    MAX_NUM_PLANES
}ENUM_PLANE_ID;

typedef enum VPP_UNIQUE_FORMAT_ID
{
    VPP_UNIQUE_FORMAT_ID_MIN           = 0,
    VPP_UNIQUE_FORMAT_ID_8BIT_420_SP   = 0,
    VPP_UNIQUE_FORMAT_ID_10BIT_420_SP  = 1,
    VPP_UNIQUE_FORMAT_ID_16BIT_422_YC  = 2,
    VPP_UNIQUE_FORMAT_ID_32BIT_ARGB    = 3,
    VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_MODE0   = 4,
    VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_MODE1   = 5,
    VPP_UNIQUE_FORMAT_ID_10BIT_420_SP_TILED_MODE0  = 6,
    VPP_UNIQUE_FORMAT_ID_10BIT_420_SP_TILED_MODE1  = 7,
    VPP_UNIQUE_FORMAT_ID_20BIT_422_YC  = 8,
    VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_V4H8  = 9,
    VPP_UNIQUE_FORMAT_ID_16BIT_RGB565  = 10,
    VPP_UNIQUE_FORMAT_ID_16BIT_420_SP  = 11,
    VPP_UNIQUE_FORMAT_ID_10BIT_DWA_420_SP   =   12,
    VPP_UNIQUE_FORMAT_ID_10BIT_DWA_422_SP   =   13,
    VPP_UNIQUE_FORMAT_ID_10BIT_DWA_444      =   14,
    VPP_UNIQUE_FORMAT_ID_16BIT_ARGB4444     =   15,/*this should match with VIDEO_DATA_Y_ALIGN_BYTES in vpp_cpcb_ovl.c */
    VPP_UNIQUE_FORMAT_ID_16BIT_ARGB1555     =   16,
    VPP_UNIQUE_FORMAT_ID_16BIT_ARGB8332     =   17,
    VPP_UNIQUE_FORMAT_ID_MAX,
}VPP_UNIQUE_FORMAT_ID;

/* definition of VPP channels, namely pipelines */
typedef enum {
    CHAN_INVALID = -1,
    FIRST_CHAN  = 0,
    CHAN_MAIN   = 0,
    CHAN_PIP    = 1,
    CHAN_GFX1   = 2,
#if (BERLIN_CHIP_VERSION < BERLIN_VS680)
    CHAN_OVP_EL = 3,
    CHAN_VMX    = CHAN_OVP_EL,
#else
    CHAN_GFX2    = 3, //AUX channel for DP1->DP2 connection
    CHAN_AUX     = 4, //AUX channel for DP1->DP2 connection
    CHAN_OVP_EL  = 5,
    CHAN_MAIN_EL = 6,
    CHAN_VMX     = CHAN_MAIN_EL,
#endif
    MAX_NUM_CHANS
}ENUM_CHAN_ID;

/* definition of VPP CPCB plane(for Berlin) */
typedef enum {
    CPCB_PLANE_INVALID  = -1,
    FIRST_CPCB_PLANE    = 0,
    CPCB1_PLANE_1       = 0, // CPCB 1st plane
    CPCB1_PLANE_2       = 1, // CPCB 2nd plane
    CPCB1_PLANE_3       = 2, // CPCB 3rd plane
    CPCB1_PLANE_4       = 3, // CPCB 4th plane
#if (BERLIN_CHIP_VERSION < BERLIN_VS680)
    CPCB1_PLANE_5       = CPCB1_PLANE_4,
#else
    CPCB2_PLANE_1       = 4, // CPCB 4th plane
    CPCB1_PLANE_5       = 5, // OVP plane
#endif
    MAX_NUM_CPCB_PLANES
}ENUM_CPCB_PLANE;

/* definition of VPP CPCB plane z-order(for Berlin) */
typedef enum {
    CPCB_ZORDER_INVALID  = -1,
    FIRST_CPCB_ZORDER    = 0,
    CPCB_ZORDER_1        = 0, // CPCB z-order 1 (bottom)
    CPCB_ZORDER_2        = 1, // CPCB z-order 2
    CPCB_ZORDER_3        = 2, // CPCB z-order 3
    CPCB_ZORDER_4        = 3, // CPCB z-order 4
    MAX_NUM_CPCB_ZORDERS
}ENUM_CPCB_ZORDER;

/* definition of VPP CPCB video outputs(for Berlin) */
typedef enum {
    CPCB_INVALID  = -1,
    FIRST_CPCB    = 0,
    CPCB_1        = 0, // CPCB-0 for Berlin
#if (BERLIN_CHIP_VERSION >= BERLIN_VS680)
    CPCB_2        = 1, // CPCB-1 for Berlin
#endif
    MAX_NUM_CPCBS
}ENUM_CPCB_ID;

// Type of Channel
typedef enum tagVOP_TYPE
{
    VPP_VOP = 0,
    VPP_VOP2 =  1,
    VPP_MAX_VOP,
}VPP_VOP_TYPE, *PVPP_VOP_TYPE;

/* definition of VPP video outputs */
typedef enum {
    VOUT_INVALID   = -1,
    FIRST_VOUT     = 0,
    VOUT_HDMI      = 0,
    VOUT_HD        = 1,
    VOUT_SD        = 2,
#if (BERLIN_CHIP_VERSION >= BERLIN_VS680)
    VOUT_DSI       = 3,
#endif
    MAX_NUM_VOUTS
}ENUM_VOUT_ID;

/* definition of VDAC and DVIO */
typedef enum {
    PORT_INVALID   = -1,
    FIRST_PORT     = 0,
    PORT_COMPONENT = 0,
    PORT_CVBS      = 1,
    PORT_SVIDEO    = 2,
    PORT_HDMI      = 3,
#ifdef VPP_DIAGS
    PORT_TTL30     = 4,
#endif
    MAX_NUM_PORTS
}ENUM_VDAC_ID;

/* definition of GFX0/PG plane input mode */
typedef enum {
    GFX_INPUT_MODE_INVALID   = -1,
    GFX_INPUT_MODE_NORMAL     = 0,
    GFX_INPUT_MODE_MATTE      = 1, /* pre-multiplied ARGB/AYUV */
}ENUM_GFX_INPUT_MODE;

/* definition of alpha polarity */
typedef enum {
    ALPHA_POL_NORMAL = 0,
    ALPHA_POL_INVERSE = 1,
}ENUM_ALPHA_POL;

/*definition of global alpha enable or disable (disable means: per pixel alpha)*/
typedef enum
{
    GLOBAL_ALPHA_FLAG_DISABLE = 0,
    GLOBAL_ALPHA_FLAG_ENABLE,
}ENUM_GLOBAL_ALPHA_FLAG;


/* definition of all the horizontal and vertical scaling coefficient modes */
typedef enum {
    SCALAR_COEFF_INVALID                            = -1,
    FIRST_SCALAR_COEFF                              = 0,
    SCALAR_COEFF_MAIN_MIN                           = 0,
    SCALAR_COEFF_H_MAIN_ADP_HLUT_CS                 = 0,
    SCALAR_COEFF_H_MAIN_1D_HLUT_BLK5                = 1,
    SCALAR_COEFF_H_MAIN_1D_HLUT_PH8                 = 2,
    SCALAR_COEFF_H_MAIN_1D_HLUT_PH8_SOFT            = 3,
    SCALAR_COEFF_V_MAIN_ADP_HLUT_BS                 = 4,
    SCALAR_COEFF_V_MAIN_1D_VLUT_DBG1                = 5,
    SCALAR_COEFF_V_MAIN_1D_VLUT_2TAP                = 6,
    SCALAR_COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL    = 7,
    SCALAR_COEFF_V_MAIN_1D_VLUT_BLK5                = 8,
    SCALAR_COEFF_V_MAIN_1D_VLUT_6TAP                = 9,
    SCALAR_COEFF_MAIN_MAX                           = 9,
    SCALAR_COEFF_GFX_MIN                            = 10,
    SCALAR_COEFF_H_GFX_HLUT_BLK5_GFX                = 10,
    SCALAR_COEFF_H_GFX_HLUT_PV8                     = 11,
    SCALAR_COEFF_V_GFX_VLUT_CS_4TAP                 = 12,
    SCALAR_COEFF_V_GFX_VLUT_3TAP_CUBIC              = 13,
    SCALAR_COEFF_V_GFX_VLUT_1TAP                    = 14,
    SCALAR_COEFF_GFX_MAX                            = 14,
    MAX_NUM_PREDEFINED_COEFFS                       = 15,
    SCALAR_COEFF_AUTO                               = 15,
    MAX_NUM_SCALAR_COEFF_MODES
} ENUM_SCALAR_COEFF_MODE;

/*pattern generation mode*/
typedef enum {
    PATGEN_SOLID     = 0,
    PATGEN_INCREMENT = 1,
    PATGEN_INVERSE   = 2,
    PATGEN_RANDOM    = 3,
} ENUM_PATGEN_MODE;

/*pattern generation type*/
typedef enum {
    PATGEN_PROG     = 0,
    PATGEN_INT      = 1,
} ENUM_PATGEN_TYPE;

/*pattern generation cadence*/
typedef enum {
    PATGEN_NORMAL     = 0,
    PATGEN_3_2_CAD    = 1,
    PATGEN_2_2_CAD    = 2,
} ENUM_PATGEN_CADENCE;

/*Macrovision control*/
typedef enum {
    VPP_MV_INVALID = -1,
    VPP_MV_OFF = 0,
    VPP_MV_AGC,
    VPP_MV_AGC_2LINE,
    VPP_MV_AGC_4LINE,
    VPP_MV_CERT_TEST1,
    VPP_MV_CERT_TEST2,
    VPP_MAX_MV_MODE
} ENUM_VPP_MV_MODE;

/*WSS aspect ratio mode*/
typedef enum {
    FIRST_WSS_AR_MODE = 0,
    WSS_AR_4_3_FULL_FORMAT = 0,
    WSS_AR_14_9_LETTERBOX_CENTER,
    WSS_AR_14_9_LETTERBOX_TOP,
    WSS_AR_16_9_LETTERBOX_CENTER,
    WSS_AR_16_9_LETTERBOX_TOP,
    WSS_AR_16_9_ABOVE_LETTERBOX_CENTER,
    WSS_AR_14_9_FULL_FORMAT_CENTER,
    WSS_AR_16_9_FULL_FORMAT,
    MAX_WSS_AR_MODE
} ENUM_WSS_AR_MODE;

/*WSS source mode*/
typedef enum {
    FIRST_WSS_SOURCE_MODE = 0,
    WSS_SOURCE_MODE_CAMERA = 0,
    WSS_SOURCE_MODE_FILM,
    MAX_WSS_SOURCE_MODE
} ENUM_WSS_SOURCE_MODE;

/*WSS subtitle existence in teletext*/
typedef enum {
    FIRST_WSS_SUBTITLE_IN_TT_MODE = 0,
    WSS_NO_SUBTITLE_IN_TT = 0,
    WSS_SUBTITLE_IN_TT ,
    MAX_WSS_SUBTITLE_IN_TT_MODE
} ENUM_WSS_SUBTITLE_IN_TT_MODE;

/*WSS subtitle mode*/
typedef enum {
    FIRST_WSS_SUBTITLE_MODE = 0,
    WSS_NO_OPEN_SUBTITLE = 0,
    WSS_SUBTITLE_INSIDE_ACTIVE,
    WSS_SUBTITLE_OUTSIDE_ACTIVE,
    MAX_WSS_SUBTITLE_MODE
} ENUM_WSS_SUBTITLE_MODE;

/*WSS surround sound mode*/
typedef enum {
    FIRST_WSS_SURROUND_SOUND_MODE = 0,
    WSS_NO_SURROUND_SOUND = 0,
    WSS_SURROUND_SOUND_MODE,
    MAX_WSS_SURROUND_SOUND_MODE
} ENUM_WSS_SURROUND_SOUND_MODE;

/*WSS copyright mode*/
typedef enum {
    FIRST_WSS_COPYRIGHT_MODE = 0,
    WSS_NO_COPYRIGHT_ASSERTED = 0,
    WSS_COPYRIGHT_ASSERTED,
    MAX_WSS_COPYRIGHT_MODE
} ENUM_WSS_COPYRIGHT_MODE;

/*WSS generation mode*/
typedef enum {
    FIRST_WSS_GENERATION_MODE = 0,
    WSS_COPYING_NOT_RESTRICTED = 0,
    WSS_COPYING_RESTRICTED,
    MAX_WSS_GENERATION_MODE
} ENUM_WSS_GENERATION_MODE;

/*CGMS aspect ratio mode*/
typedef enum {
    FIRST_CGMS_AR_MODE = 0,
    CGMS_AR_NOT_INDICATED = 0,
    CGMS_AR_4_3_NORMAL = 0,
    CGMS_AR_16_9_NORMAL,
    CGMS_AR_4_3_LETTERBOX,
    MAX_CGMS_AR_MODE
} ENUM_CGMS_AR_MODE;

/*CGMS copy control infomation transfer mode*/
typedef enum {
    FIRST_CGMS_COPY_CTRL_INFO_TRANS_MODE = 0,
    CGMS_COPY_CTRL_INFO_TRANS = 0,
    CGMS_COPY_CTRL_INFO_NOT_TRANS,
    MAX_CGMS_COPY_CTRL_INFO_TRANS_MODE
} ENUM_CGMS_COPY_CTRL_INFO_TRANS_MODE;

/*CGMS copy control infomation mode*/
typedef enum {
    FIRST_CGMS_COPY_CTRL_INFO_MODE = 0,
    CGMS_COPY_CTRL_INFO_PERMITTED = 0,
    CGMS_COPY_CTRL_INFO_NOT_USED,
    CGMS_COPY_CTRL_INFO_ONE_GENERATION,
    CGMS_COPY_CTRL_INFO_NOT_PERMITTED,
    MAX_CGMS_COPY_CTRL_INFO_MODE
} ENUM_CGMS_COPY_CTRL_INFO_MODE;

/*CGMS APS mode*/
typedef enum {
    FIRST_CGMS_APS_MODE = 0,
    CGMS_APS_PSP_OFF = 0,
    CGMS_APS_PSP_ON_SPLIT_BURST_OFF,
    CGMS_APS_PSP_ON_SPLIT_BURST_2_LINE,
    CGMS_APS_PSP_ON_SPLIT_BURST_4_LINE,
    MAX_CGMS_APS_MODE
} ENUM_CGMS_APS_MODE;

/*CGMS source mode*/
typedef enum {
    FIRST_CGMS_SOURCE_MODE = 0,
    CGMS_SOURCE_NOT_ANALOG = 0,
    CGMS_SOURCE_ANALOG,
    MAX_CGMS_SOURCE_MODE
} ENUM_CGMS_SOURCE_MODE;

/*Enum for MTR_R chkSem ID*/
typedef enum VPP_MTRR_CHKSEM_ID
{
    VPP_MTRR_CHKSEM_ID_MIN = 0,
    VPP_MTRR_CHKSEM_ID_0 = VPP_MTRR_CHKSEM_ID_MIN,
    VPP_MTRR_CHKSEM_ID_1,
    VPP_MTRR_CHKSEM_ID_2,
    VPP_MTRR_CHKSEM_ID_3,
    VPP_MTRR_CHKSEM_ID_4,
    VPP_MTRR_CHKSEM_ID_5,
    VPP_MTRR_CHKSEM_ID_MAX,
}VPP_MTRR_CHKSEM_ID;

/*TT control mode*/
typedef enum {
    FIRST_TT_CTRL_MODE = 0,
    TT_CTRL_ENABLE = 0,
    TT_CTRL_UPDATE,
    TT_CTRL_DISABLE,
    MAX_TT_CTRL_MODE
} ENUM_TT_CTRL_MODE;

/* SS offset mode */
typedef enum {
    FIRST_SS_OFFSET_MODE = 0,
    SS_OFFSET_NONE = 0,
    SS_OFFSET_VIDEO,
    SS_OFFSET_DEFAULT,
    MAX_SS_OFFSET_MODE
} ENUM_SS_OFFSET_MODE;

#ifdef PE_3D_FMT_CONVERT
/* 3D source format */
typedef enum {
    FIRST_SRC_3D_FMT = 0,
    SRC_3D_FMT_FP = 0,
    SRC_3D_FMT_SBS,
    SRC_3D_FMT_TAB,
    SRC_3D_FMT_AUTO,
    SRC_3D_FMT_SBS_SENSIO,
    MAX_SRC_3D_FMT
} ENUM_SRC_3D_FMT;
#endif

typedef enum VPP_NGPTV_CMD_t
{
    VPP_NGPTV_CMD_MIN = 0,
    VPP_NGPTV_ENABLE_BYPASS = 1,
    VPP_NGPTV_ENABLE_WATERMARK,
    VPP_NGPTV_CONFIG_KEY_IN,
    VPP_NGPTV_CONFIG_SETTING_IN,
    VPP_NGPTV_CONFIG_PAYLOAD_IN,
    VPP_NGPTV_CONFIG_PARAMS,
    VPP_NGPTV_CMD_MAX
}VPP_NGPTV_CMD;

typedef enum VPP_HDMI_HDCP_VERSION_T
{
    VPP_HDMI_HDCP_VER_ERR = -1,
    VPP_HDMI_HDCP_VER_NONE = 0,
    VPP_HDMI_HDCP_VER_HDCP_1_x = 1,
    VPP_HDMI_HDCP_VER_HDCP_2_2 = 2,
    VPP_HDMI_HDCP_VER_AUTO = 3,
}VPP_HDMI_HDCP_VERSION;

typedef struct VPP_HDMITX_REG_DUMP_T {
    UINT32 uiRegAddr[425]; /*Main the total Register same as BG2CDP to match allignment*/
    UINT32 uiRegVal[425];
} VPP_HDMITX_REG_DUMP;


/*Enums for the various option passed to enClkGate()
 *
 * FORMAT => 0xDDddXXOO; Where DD:Debug prints, dd:disable, XX:Future option, OO: clkgate options
 */
typedef enum __ENUM_VPP_CLKGATE_ENABLE_OPT_ {
    //The basic clock-gate options
    VPP_CLKGATE_ENABLE_OPT_CLKGATE = 0x01,
    VPP_CLKGATE_ENABLE_OPT_PWRGATE = 0x02,

    //Disable options
    VPP_CLKGATE_ENABLE_OPT_DISABLE_HDMIRX   = 0x00010000,

    //The debug options
    VPP_CLKGATE_ENABLE_OPT_DBG_CLKGATE = 0x01000000,
    VPP_CLKGATE_ENABLE_OPT_DBG_PWRGATE = 0x02000000,
}VPP_CLKGATE_ENABLE_OPT;

#define IS_VPP_CLKGATE_ENABLE(clkGateOpt, OPTION)       (clkGateOpt&VPP_CLKGATE_ENABLE_OPT_##OPTION)
#define IS_VPP_CLKGATE_DISABLE(clkGateOpt, OPTION)      (clkGateOpt&VPP_CLKGATE_ENABLE_OPT_DISABLE_##OPTION)
#define IS_VPP_CLKGATE_DBG_ENABLE(clkGateOpt, OPTION)   (clkGateOpt&VPP_CLKGATE_ENABLE_OPT_DBG_##OPTION)

/*Enums which be can be enabled for clk gate signature capture option
 *
 * format => 0xFFCCDDOO; Where FF:Future use,CC:Count, DD:Delayed trigger,OO:actual options
 */
#if 0
typedef union __VPP_CLKGATE_OPT__ {
    UINT32 opt;
    struct {
        UINT32 clkGateOpt:8;
        UINT32 clkGateDelayedOpt:8;
        UINT32 clkGateDelayedCount:8;
        UINT32 clkGateOptFuture:8;
    };
}VPP_CLKGATE_OPT;

typedef enum __ENUM_VPP_CLKGATE_OPT__ {
    //This option should present at 1st byte from LSB
    VPP_CLKGATE_OPT_PRINT      = 0x01,
    VPP_CLKGATE_OPT_RECORD_RAW = 0x02,
    //This option should present at 2nd byte from LSB
    VPP_CLKGATE_DLY_OPT_REPEAT = 0x01,        //Repeat or don't Repeat delayed operation
    VPP_CLKGATE_DLY_OPT_SRCFMT = 0x02,        //Delayed print when scrfmt is changed
    VPP_CLKGATE_DLY_OPT_REF_DISP_WIN = 0x04,  //Delayed print when refwin/dispwin is changed
    //This option should present at 3rd byte from LSB
    VPP_CLKGATE_DLY_COUNT_MASK = 0x00FF0000,    //User defined delay in terms of interrupts, otherwise delay count will be used
}ENUM_VPP_CLKGATE_OPT;
#endif
#define VPP_CLKGATE_DLY_DEFAULT_COUNT                   5   //Show Delayed print after 5 interrupt

#define IS_VPP_CLKGATE_OPTION_ENABLE(var, OPT_PRINT)    (var&VPP_CLKGATE_OPT_##OPT_PRINT)
#define IS_VPP_CLKGATE_OPTION_ENABLE_PRINT(var)         IS_VPP_CLKGATE_OPTION_ENABLE(var, PRINT)
#define IS_VPP_CLKGATE_OPTION_ENABLE_RECORD_RAW(var)    IS_VPP_CLKGATE_OPTION_ENABLE(var, RECORD_RAW)
#define IS_VPP_CLKGATE_OPTION_ENABLE_REGDUMP(var)       IS_VPP_CLKGATE_OPTION_ENABLE(var, REGDUMP)
#define   VPP_HDCP_AKSV_ADDRESS               (0xf7f64d00)
#define   VPP_HDCP_AKEY0_ADDRESS              (0xf7f64800)
#define   VPP_HDCP_STATUS_ADDRESS             (0xf7f64d44)

#define VPP_ENC_HDCPKEY_LEN                   (416)
#define VPP_HDCPKEY_LEN                       (288)

typedef struct PTS_T { /* 33-bit PTS */
    unsigned int hi_word; /* bit-32 */
    unsigned int lo_word; /* bit-31 ~ bit-0 */
} PTS;

typedef struct VPP_TILE_T {
    /*The width & height of the tile in pixels*/
    int width;
    int height;
    int bytesPerTile;   /*number of bytes per tile*/

    int nTileX; /*Number of tiles in X direction*/
    int nTileY; /*Number of tiles in Y direction*/
    int tileStrideFrac;   /*Number of extra bytes per Stride*/
    int tileStartFrac;    /*Number of extra bytes for StartX*/
}VPP_TILE, *PVPP_TILE;

typedef struct VMX_SETTINGS_T {
    UINT32 hres;
    UINT32 vres;
    UINT32 bg_embed_on;
    UINT32 watermark;
    UINT32 startx;
    UINT32 sizex;
    UINT32 starty;
    UINT32 sizey;
    UINT32 colno;
    UINT32 coldist;
    UINT32 lefthi;
    UINT32 leftmid;
    UINT32 leftlo;
    UINT32 righthi;
    UINT32 rightmid;
    UINT32 rightlo;
    UINT32 tophi;
    UINT32 topmid;
    UINT32 toplo;
    UINT32 direction;
    UINT32 strength;
} VMX_SETTINGS;

typedef struct VPP_WIN_T {
    int x;      /* x-coordination of a vpp window top-left corner in pixel, index starting from 0 */
    int y;      /* y-coordination of a vpp window top-left corner in pixle, index starting from 0 */
    int width;  /* width of a vpp window in pixel */
    int height; /* height of a vpp window in pixel */
} VPP_WIN;

typedef struct VPP_WIN_ATTR_T {
    int bgcolor;    /* background color of a vpp window */
    int alpha;      /* global alpha of a vpp window */
    ENUM_GLOBAL_ALPHA_FLAG globalAlphaFlag; /*1:enable plane/global alpha otherwise use per pixel alpha*/
} VPP_WIN_ATTR;

typedef struct VPP_BG_COLOR_T {
    int ColorFmt;   /*MV_PE_VPP_COLOR_FMT*/
    unsigned int Color;
} VPP_BG_COLOR;

typedef struct VPP_ZORDER_CTRL_T {
    int main;
    int pip;
    int gfx1;
} VPP_ZORDER_CTRL;

typedef struct VPP_PATGEN_DATA_T {
    int hmode;
    int vmode;
    int hPith;
    int vPith;
    int hColPith; // 0x00BBGGRR
    int vColPith; // 0x00BBGGRR
    int ColSeed1; // 0x00BBGGRR
    int ColSeed2; // 0x00BBGGRR
} VPP_PATGEN_DATA;

typedef struct VPP_PATGEN_TYPE_T {
    int type;
    int cadence;
    int rff;
} VPP_PATGEN_TYPE;

typedef struct VPP_WSS_DATA_T {
    char ARMode;
    char SourceMode;
    char SubinTT;
    char SubMode;
    char SoundMode;
    char Copyright;
    char Generation;
}VPP_WSS_DATA;

typedef struct VPP_CGMS_DATA_T {
    char ARMode;
    char CopyInfoTrans;
    char CopyInfoMode;
    char APSMode;
    char SourceMode;
}VPP_CGMS_DATA;

typedef struct VPP_CEC_RX_MSG_BUF_T{
    UINT8 buf[16];
    UINT8 len;
}VPP_CEC_RX_MSG_BUF;


/*DEINT DS */

typedef enum tagVPP_DEINTUSER_CAD_MODE
{
    VPP_DEINTUSER_CAD_MODE_1 = 0,
    VPP_DEINTUSER_CAD_MODE_2

}VPP_DEINTUSER_CAD_MODE;

typedef enum tagVPP_DEINTEXCL_PARAM_MODE
{
    VPP_DEINTEXCL_PARAM_MODE_NONE = 0,
    VPP_DEINTEXCL_PARAM_EXCLUSION_SEL,
    VPP_DEINTEXCL_PARAM_EXCLUSION_SEL_2ZONE

}VPP_DEINTEXCL_PARAM_MODE;

typedef struct tagVPP_DEINTUSER_CAD_PARAMS_NEW
{
    VPP_DEINTUSER_CAD_MODE Cadence;
    BOOL   Enable;
    UCHAR  Length;
    UINT32 Pattern;

} VPP_DEINTUSER_CAD_PARAMS_NEW, *PVPP_DEINTUSER_CAD_PARAMS_NEW;

typedef struct tagVPP_DEINTUSER_CAD_PARAMS
{
    BOOL   Enable;
    UCHAR  Length;
    UINT32 Pattern;

} VPP_DEINTUSER_CAD_PARAMS, *PVPP_DEINTUSER_CAD_PARAMS;

typedef struct tagVPP_DEINTEXCL_CTRL_PARAMS
{
    VPP_DEINTEXCL_PARAM_MODE ExclSel;
    UCHAR                    LinesFromTop;
    UCHAR                    LinesFromBottom;
    UCHAR                    PixelsFromLeft;
    UCHAR                    PixelsFromRight;

} VPP_DEINTEXCL_CTRL_PARAMS, *PVPP_DEINTEXCL_CTRL_PARAMS;

/*struct for Detail Control LPF settings.*/
typedef struct VPP_FE_DET_LPF_CTRL
{
    UINT8 Lpfmode;  //VPP_FE_DET_LPF_MODE
    BOOL    HlpfEnable;
    BOOL    VlpfEnable;
}VPP_FE_DET_LPF_CTRL,*PVPP_FE_DET_LPF_CTRL;

/*struct for LPF parameters (controls the sharpness).*/
typedef struct VPP_FE_DET_LPF_CTRL_PARAMS
{
    UINT8   DetailLpfHs;
    UINT8   DetailLpfVs;
}VPP_FE_DET_LPF_CTRL_PARAMS,*PVPP_FE_DET_LPF_CTRL_PARAMS;

typedef struct VPP_FE_DEINT_LPF_CTRL
{
    VPP_FE_DET_LPF_CTRL        LPFCtrl;
    VPP_FE_DET_LPF_CTRL_PARAMS    LPFParam;
}VPP_FE_DEINT_LPF_CTRL;

typedef enum tagVPP_DEINTVI_MODE
{
    VPP_DEINTMODE_OFF = 0,
    VPP_DEINTMODE_SAFE ,
    VPP_DEINTMODE_AGGRESSIVE

}VPP_DEINTVI_MODE;

typedef enum tagVPP_DEINTCHROMA_MOT_VID_MODE
{
    VPP_DEINTCHROMA_MOT_VID_SAME_AS_LUMA = 0,
    VPP_DEINTCHROMA_MOT_FIXED_2D,
    VPP_DEINTCHROMA_MOT_INDEP_CHROMA_MOT,
    VPP_DEINTCHROMA_MOT_MAX_OF_CHROMA_AND_LUMA_MOT

}VPP_DEINTCHROMA_MOT_VID_MODE;

/*
 * These Params are mainly based on the mode.
 * So they are combined with SetPresetMode().
 */
typedef struct tagVPP_DINT_PRESET_MODE_PARAMS
{
    VPP_DEINTVI_MODE               ViMode;
    VPP_DEINTCHROMA_MOT_VID_MODE   ChrMotVidMode;
    BOOL                           ChrMotFlmMode;
    BOOL                FieldPolarity;

}VPP_DINT_PRESET_MODE_PARAMS, *PVPP_DINT_PRESET_MODE_PARAMS;

typedef struct tagVPP_DEINT_PRESET_MODE
{
    int                DeintPresetMode;
    VPP_DINT_PRESET_MODE_PARAMS    DeintPresetModeParam;
}VPP_FE_DEINT_PRESET_MODE, *PVPP_FE_DEINT_PRESET_MODE;

typedef enum tagVPP_DEINT_SPATIO_TEMP_MIXER_CTRL_ENUM
{
    VPP_DEINT_MOTION_ADAPTIVE = 0,
    VPP_DEINT_FORCED_SPATIAL,
    VPP_DEINT_FORCED_TEMPORAL

}VPP_DEINT_SPATIO_TEMP_MIXER_CTRL_ENUM;

//Deint Saptio-Temporal Mixer Control Parameters
typedef struct tagVPP_DEINT_FORCED2D_CTRL
{
    VPP_DEINT_SPATIO_TEMP_MIXER_CTRL_ENUM       LumaCtrlForVideoPix;
    VPP_DEINT_SPATIO_TEMP_MIXER_CTRL_ENUM       ChromaCtrlForVideoPix;
    VPP_DEINT_SPATIO_TEMP_MIXER_CTRL_ENUM       LumaCtrlForFilmPix;
    VPP_DEINT_SPATIO_TEMP_MIXER_CTRL_ENUM       ChromaCtrlForFilmPix;

}VPP_DEINT_FORCED2D_CTRL, *PVPP_DEINT_FORCED2D_CTRL;

typedef enum tagVPP_DEINT_FILM_ENTRY_MODE
{
    VPP_DEINT_MODE_22=0,
    VPP_DEINT_MODE_32
}VPP_DEINT_FILM_ENTRY_MODE;

typedef struct tagVPP_DEINT_FILM_MODE_ENTRY_PARAM
{
    VPP_DEINT_FILM_ENTRY_MODE    EntryMode;
    UINT16    NofInHardBld;
    UINT16    NofInSoftBld;
}VPP_DEINT_FILM_MODE_ENTRY_PARAM, *PVPP_DEINT_FILM_MODE_ENTRY_PARAM;

typedef struct tagVPP_DEINTADV_MD_PARAMS
{
    BOOL      SmallMotFilmExit;         //SMFE
    BOOL      EnablePostWveErrMode32;
    BOOL      EnablePostWveErrMode22;
    BOOL      EnablePostWveErrModeOther;
    BOOL      EnableProgFlag;
    BOOL      Enable32SMFE;             //Not used

    BOOL      EnableIPodSplMode;        //special mode for Ipod
    BOOL      EnableTemporalSplMode;    //special mode for Temporal

} VPP_DEINTADV_MD_PARAMS, *PVPP_DEINTADV_MD_PARAMS;

typedef union tagVPP_VDO_DEINT_REG
{
    VPP_DEINTUSER_CAD_PARAMS_NEW DEINTUserCad;
    VPP_DEINTEXCL_CTRL_PARAMS    DEINTExclCtrl;
    VPP_FE_DEINT_LPF_CTRL         DEINTLPFCtrl;
    UINT16                        FilmCadence;
    VPP_FE_DEINT_PRESET_MODE    DEINTPreset;
    VPP_DEINT_FORCED2D_CTRL        DEINTF2DCtrl;
    VPP_DEINT_FILM_MODE_ENTRY_PARAM    DEINTFilmMode;
    UINT8                        DeintOSDCtrl;
    VPP_DEINTADV_MD_PARAMS        DeintADVCtrl;
}VPP_VDO_DEINT_REG;

/*Enum for FRC SCL unit number*/
typedef enum VPP_FRC_SCL_NUM
{
    VPP_FRC_SCL_MAIN = 0,
    VPP_FRC_SCL_PIP,
    VPP_FRC_SCL_OSD,
#if (BERLIN_CHIP_VERSION < BERLIN_VS680)
    VPP_FRC_SCL_VIP,
    VPP_FRC_SCL_OVP_Y,
    VPP_FRC_SCL_OVP_UV,
#else
    VPP_FRC_SCL_GFX2,
    VPP_FRC_SCL_VIP_Y,
    VPP_FRC_SCL_VIP_UV,
    VPP_FRC_SCL_OVP_Y,
    VPP_FRC_SCL_OVP_UV,
    VPP_FRC_SCL_DET,
#endif
    VPP_FRC_SCL_MAX
}VPP_FRC_SCL_NUM;

// ACE Param
/*-----------------------------------------------------------------------------
 * Enums, Structures and Unions
 *-----------------------------------------------------------------------------
 */

/* Ace Regions (Dark/Mid/High)*/
typedef enum tagVPP_CMU_ACE_REGION
{
    VPP_CMU_ACE_REGION1 = 0,
    VPP_CMU_ACE_REGION2,
    VPP_CMU_ACE_REGION3
} VPP_CMU_ACE_REGION;

/* Ace Thresholds */
typedef enum tagVPP_CMU_ACE_THOLD
{
    VPP_CMU_ACE_THOLD0 = 0, // Below this is Black crush
    VPP_CMU_ACE_THOLD1,
    VPP_CMU_ACE_THOLD2,
    VPP_CMU_ACE_THOLD3,
    VPP_CMU_ACE_THOLD4,
    VPP_CMU_ACE_THOLD5		// Above this is white pull up.
} VPP_CMU_ACE_THOLD;

/* Output luma value for below and above thresholds */
typedef enum tagVPP_CMU_ACE_OUTPUT
{
    VPP_CMU_ACE_OUTPUT0 = 0,	// White (below th0)
    VPP_CMU_ACE_OUTPUT5			// Black (above th5)
} VPP_CMU_ACE_OUTPUT;

/*
 *  Gaurd Bands between regions
 *  Pixels in this gaurd bands are not effected.
 *  (Same as input)
 */
typedef enum tagVPP_CMU_ACE_GUARDBAND
{
    VPP_CMU_ACE_GUARDBAND2 = 0,
    VPP_CMU_ACE_GUARDBAND3
} VPP_CMU_ACE_GUARDBAND;

/*
 * Maximu slope values for all the regions
 * Controls the level of Enhancement
 */
typedef enum tagVPP_CMU_ACE_MAXSLOPE
{
    VPP_CMU_ACE_MAXSLOPE1= 0,
    VPP_CMU_ACE_MAXSLOPE2,
    VPP_CMU_ACE_MAXSLOPE3
} VPP_CMU_ACE_MAXSLOPE;


/* defination of EE parameter cmd */
typedef enum {
    EE_CMD_INVALID = -1,
    VPP_CMD_EE_SET_DET_CTRL=0,
    VPP_CMD_EE_SET_DET_CTRL_PARAM,
    VPP_CMD_EE_SET_CTRL,
#if (BERLIN_CHIP_VERSION >= BERLIN_BG2_DTV_A0)
    VPP_CMD_CPCB_EE_PARAMS,
    VPP_CMD_CPCB_GDET_CTRL,
    VPP_CMD_CPCB_GDET_PARAMS,
    VPP_CMD_DETEE_GDET_CTRL,
    VPP_CMD_DETEE_GDET_PARAMS,
#endif
    MAX_NUM_EE_CMD

}VPP_CMD_EXT_EE;

typedef enum {
    ACE_CMD_INVALID = -1,
    VPP_CMD_ACE_SET_ENABLE=0,
    VPP_CMD_ACE_SET_MODE,
    VPP_CMD_ACE_SET_PARAM,
    VPP_CMD_ACE_SET_CLR_LEVEL,
    VPP_CMD_ACE_SET_WFACTOR_1,
    VPP_CMD_ACE_SET_WFACTOR_2,
    VPP_CMD_ACE_SET_WFACTOR_3,
    VPP_CMD_ACE_SET_THRSLD_0,
    VPP_CMD_ACE_SET_THRSLD_1,
    VPP_CMD_ACE_SET_THRSLD_2,
    VPP_CMD_ACE_SET_THRSLD_3,
    VPP_CMD_ACE_SET_THRSLD_4,
    VPP_CMD_ACE_SET_THRSLD_5,
    VPP_CMD_ACE_SET_OUTPUT_0,
    VPP_CMD_ACE_SET_OUTPUT_5,
    VPP_CMD_ACE_SET_GUARDBAND_2,
    VPP_CMD_ACE_SET_GUARDBAND_3,
    VPP_CMD_ACE_SET_MAX_SLOPE_1,
    VPP_CMD_ACE_SET_MAX_SLOPE_2,
    VPP_CMD_ACE_SET_MAX_SLOPE_3,
    MAX_NUM_ACE_CMD

}VPP_CMD_EXT_ACE;


/*
 * These modes are for setting the
 * cotrast in different levels. (Low to High)
 */
typedef enum tagVPP_CMU_ACE_MODE
{
    VPP_CMU_ACE_OFF = 0,
    VPP_CMU_ACE_MANUAL,
    VPP_CMU_ACE_LOW,
    VPP_CMU_ACE_MEDIUM,
    VPP_CMU_ACE_HIGH,
    VPP_CMU_ACE_MAX_MODE
} VPP_CMU_ACE_MODE;

/*
 * This range can either be taken from zero or from 16.
 */
typedef enum tagVPP_CMU_ACE_RANGE
{
    VPP_CMU_ACE_RANGE_0_255 = 0,
    VPP_CMU_ACE_RANGE_16_236,
    VPP_CMU_ACE_MAX_RANGE
} VPP_CMU_ACE_RANGE;



typedef struct tagVPP_CMU_ACE_PARAMS
{
     UCHAR   Enable;
     UCHAR   ColorCompensationLevel;
     UCHAR   Crush;
     UCHAR   WeightingFactor1;
     UCHAR   WeightingFactor2;
     UCHAR   WeightingFactor3;

     UINT16   ThresHold0;
     UINT16   ThresHold1;
     UINT16   ThresHold2;
     UINT16   ThresHold3;
     UINT16   ThresHold4;
     UINT16   ThresHold5;

     UINT16   OutPut0;  //White
     UINT16   OutPut5;  // Black

     UINT16  GuardBand2;
     UINT16  GuardBand3;

     UCHAR  MaximumSlope1;
     UCHAR  MaximumSlope2;
     UCHAR  MaximumSlope3;
#if (BERLIN_CHIP_VERSION >= BERLIN_BG2_DTV_A0)
     UCHAR Mode;
#endif

}VPP_CMU_ACE_PARAMS, *PVPP_CMU_ACE_PARAMS;

typedef struct VPP_VDO_ACE_REG_T
{
     VPP_CMU_ACE_PARAMS Params;
     VPP_CMU_ACE_RANGE Range; // for set ace mode
     VPP_CMU_ACE_MODE Mode; // for set ace mode
}VPP_VDO_ACE_REG;
// FTDC

/*definition of ACE mode*/
typedef enum {
    FIRST_ACE = 0,
    ACE_OFF = 0,
    ACE_MANUAL,
    ACE_LOW,
    ACE_MEDIUM,
    ACE_HIGH,
    MAX_ACE
}ENUM_ACE_MODE;

/*definition of BR level*/
typedef enum {
    FIRST_BR = 0,
    BR_LEVEL0 = 0,
    BR_LEVEL1,
    BR_LEVEL2,
    BR_LEVEL3,
    BR_LEVEL4,
    BR_LEVEL5,
    BR_LEVEL6,
    BR_LEVEL7,
    BR_LEVEL8,
    BR_LEVEL9,
    BR_LEVEL10,
    BR_LEVEL11,
    BR_LEVEL12,
    BR_LEVEL13,
    BR_LEVEL14,
    MAX_BR
}ENUM_BR_LEVEL;

typedef enum{
    FIRST_COLOR = 0,
    COLOR_RED = 0,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_CYAN,
    COLOR_YELLOW,
    COLOR_MAGENTA,
    COLOR_ALL,
    MAX_COLOR
}ENUM_COLOR_MODE;

/*definition of ICR mode*/
typedef enum{
    FIRST_ICR_MODE = 0,
    ICR_MODE_OFF = 0,
    ICR_MODE_SKY,
    ICR_MODE_GRASS,
    ICR_MODE_SKYGRASS,
    ICR_MODE_VIVID,
    MAX_ICR_MODE
}ENUM_ICR_MODE;

/*definition of FTDC mode*/
typedef enum {
    FIRST_FTDC = 0,
    FTDC_MODE1 = 0,
    FTDC_MODE2,
    FTDC_MODE3,
    FTDC_MODE4,
    FTDC_OFF,
    MAX_FTDC
}ENUM_FTDC_MODE;

typedef struct tagVPP_CMU_FTDC_PARAMS
{
    UCHAR    FtdcEnable; /* Main path enable */
#if (BERLIN_CHIP_VERSION >= BERLIN_BG2_DTV_A0)
    UCHAR    FtdcPipEnable;
#endif
#if (BERLIN_CHIP_VERSION < BERLIN_BG2_DTV_A0)
    UCHAR    ChromaLow;
    UINT16   ChromaMid;
    UINT16   ChromaHigh;
#elif (BERLIN_CHIP_VERSION >= BERLIN_BG2_DTV_A0)
    UINT16   OutChromaLow;
    UINT16   InChromaLow;
    UINT16   OutChromaHigh;
    UINT16   InChromaHigh;
#endif
    UINT16   HueInLow;
    UINT16   HueInHigh;
    UINT16   HueOutLow;
    UINT16   HueOutHigh;
    UCHAR    LumaLow;
    UCHAR    LumaHigh;
#if (BERLIN_CHIP_VERSION < BERLIN_BG2_DTV_A0)
    UCHAR    ICorrection;
    UCHAR    QCorrection;
#elif (BERLIN_CHIP_VERSION >= BERLIN_BG2_DTV_A0)
    UINT16   HueWidth;
    UINT16   ChromaWidth;
#endif
}VPP_CMU_FTDC_PARAMS, *PVPP_CMU_FTDC_PARAMS;


/*struct for FE detail EE control*/
typedef struct VPP_FE_DET_CTRL
{
    BOOL                    EeEnable;
    BOOL                    DetailEnable;
    BOOL                    DetailAdd;
    UINT8                   HfMode;  //VPP_FE_DET_HFMODE
    UINT8                   VfMode;  //VPP_FE_DET_HFMODE
    UINT8                   DetailLut[16];
}VPP_FE_DET_CTRL, *PVPP_FE_DET_CTRL;

/*struct for FE detail EE control parameters*/
typedef struct VPP_FE_DET_CTRL_PARAMS
{
     UINT8                  DetNosieTh1;
     UINT8                  DetNosieTh2;
     UINT8                  DetGain1;
     UINT8                  DetGain2;
     UINT8                  DetMaskGenMode;  //VPP_FE_DET_MASK_GEN_MODE
     UINT8                  DetThBase;
     UINT8                  DetThStep;
}VPP_FE_DET_CTRL_PARAMS, *PVPP_FE_DET_CTRL_PARAMS;

/*struct for CMU EE control*/
typedef struct VPP_CMU_EE_CTRL
{
    UINT8  UpsamplerEn;
#if (BERLIN_CHIP_VERSION >= BERLIN_BG2_DTV_A0)
    UINT8  AdaptiveSamplingEn;
#endif
    UINT8  LumaGainEn;
    UINT8  DetailEn;
    UINT8  ChromaGainEn;
#if (BERLIN_CHIP_VERSION >= BERLIN_BG2_DTV_A0)
    UINT8  FtLtiEn;
    UINT8  FtDetEn;
    UINT8  NewLtiAlgoSel;
    UINT8  NewCtiAlgoSel;

    /* color upsampler Controls */
    UINT8 ups_yshift;
    UINT8 ups_cshift;
    UINT8 ups_cswap;
#endif
} VPP_CMU_EE_CTRL, *PVPP_CMU_EE_CTRL;

/* definition of EE parameter profile */
typedef enum {
    EE_PARAM_INVALID        = -1,
    EE_TH_LOW               = 0, /* EE low threshold profile parameters */
    EE_TH_MEDIUM            = 1, /* EE medium threshold profile parameters */
    EE_TH_HIGH              = 2, /* EE low high profile parameters */
    EE_INTERNET_LOW_BR_SD   = 3, /* EE parameter profile for internet low quality SD content */
    EE_INTERNET_HIGH_BR_SD  = 4, /* EE parameter profile for internet high quality SD content */
    EE_INTERNET_HD          = 5, /* EE parameter profile for internet HD content */
    MAX_NUM_EE_PARAMS
}ENUM_EE_PARAMS;

#if (BERLIN_CHIP_VERSION >= BERLIN_BG2_DTV_A0)
typedef struct tagVPP_CMU_EE_PARAMS
{
    /* color upsampler */
    UINT16 ups_yblank;
    UINT16 ups_cblank;
    UINT8 ups_y_th;
    UINT8 ups_c_th;

    /* Skin Tone Region params */
    UINT16 FtdInlow;
    UINT16 FtdInhigh;
    UINT16 FtdOutlow;
    UINT16 FtdOuthigh;
    UINT8  FtdYlow;
    UINT8  FtdYhigh;
    UINT16 FtdOutchromaLow;
    UINT16 FtdInchromaLow;
    UINT16 FtdInchromaHigh;
    UINT16 FtdOutchromaHigh;
} VPP_CMU_EE_PARAMS, *PVPP_CMU_EE_PARAMS;

typedef struct tagVPP_CPCB_GDET_CTRL
{
    BOOL    gdet_switch;
    BOOL    gdet_en;
    BOOL    gdet_add;
    BOOL    ups_en;
} VPP_CPCB_GDET_CTRL, *PVPP_CPCB_GDET_CTRL;

typedef struct tagVPP_CPCB_GDET_PARAMS
{
    /* Control values */
    UINT8  win_r;
    UINT32 eps;
    UINT16 scale;
    UINT16 mid;
    UINT8  sigma_s1;
    UINT8  sigma_s2;

    /* Noise params */
    UINT16 noise_num;
    UINT8  noise_offset;
    UINT8  noise_cap;
    UINT8  noise_adj_s;
    UINT16 noise_act_cap;

    /* Skin prams */
    UINT16 scale_skin;
    UINT16 mid_skin;
    UINT8  sigma_s1_skin;
    UINT8  sigma_s2_skin;
    UINT16 noise_num_skin;
    UINT8  noise_offset_skin;
    UINT8  noise_cap_skin;
    UINT16 skin_cos2_h_low;
    UINT16 skin_cos2_h_high;
    UINT16 skin_s_low;
    UINT16 skin_s_high;
    UINT16 skin_v_low;
    UINT16 skin_v_high;
    UINT16 skin_y;
    UINT8  skin_rmv_th;
    UINT8  skin_set_th;

} VPP_CPCB_GDET_PARAMS, *PVPP_CPCB_GDET_PARAMS;
#endif


typedef struct tagVPP_DETEE_GDET_CTRL
{
    BOOL    gdet_switch;
    BOOL    gdet_en;
    BOOL    gdet_add;
    BOOL    ups_en;
} VPP_DETEE_GDET_CTRL, *PVPP_DETEE_GDET_CTRL;

typedef struct tagVPP_DETEE_GDET_PARAMS
{
    /* Control values */
    UINT8  win_r;
    UINT32 eps;
    UINT16 scale;
    UINT16 mid;
    UINT8  sigma_s1;
    UINT8  sigma_s2;

    /* Noise params */
    UINT16 noise_num;
    UINT8  noise_offset;
    UINT8  noise_cap;
    UINT8  noise_adj_s;
    UINT16 noise_act_cap;

    /* Skin prams */
    UINT16 scale_skin;
    UINT16 mid_skin;
    UINT8  sigma_s1_skin;
    UINT8  sigma_s2_skin;
    UINT16 noise_num_skin;
    UINT8  noise_offset_skin;
    UINT8  noise_cap_skin;
    UINT16 skin_cos2_h_low;
    UINT16 skin_cos2_h_high;
    UINT16 skin_s_low;
    UINT16 skin_s_high;
    UINT16 skin_v_low;
    UINT16 skin_v_high;
    UINT16 skin_y;
    UINT8  skin_rmv_th;
    UINT8  skin_set_th;

} VPP_DETEE_GDET_PARAMS, *PVPP_DETEE_GDET_PARAMS;

typedef union tagVPP_VDO_EE_REG
{
    VPP_FE_DET_CTRL             EEDetCtrl;
    VPP_FE_DET_CTRL_PARAMS      EEDetCtrlParam;
    VPP_CMU_EE_CTRL             EECtrl;
#if (BERLIN_CHIP_VERSION >= BERLIN_BG2_DTV_A0)
    VPP_CMU_EE_PARAMS       EEParam;
    VPP_CPCB_GDET_CTRL      EEGDetCtrl;
    VPP_CPCB_GDET_PARAMS    EEGDetParam;
#endif
}VPP_VDO_EE_REG;


/*definition of Gamma mode*/
typedef enum {
    FIRST_GM_MODE = 0,
    GM_1_8 = 0,
    GM_2_5,
    GM_SCURVE_LIGHT,
    GM_SCURVE_DARK,
    GM_OFF,
    GM_INVERSE,
    MAX_GM_MODE
}ENUM_GAMMA_MODE;

typedef enum tagVPP_CLOCK_GATE_BLOCK
{
    CLOCK_GATE_MIN = 0,
    MAIN_SCL_GATE = 0,
    PIP_SCL_GATE,
    GFX1_SCL_GATE,
    VIDEO_ENC_GATE,
    HDMI_TX_GATE,
    VP_TOP_GATE,    //DEINT+DMX
    VMX_TOP_GATE,
#if (BERLIN_CHIP_VERSION >= BERLIN_VS680)
    CMU_TOP_GATE,
#endif
    CLOCK_GATE_MAX,
}VPP_CLOCK_GATE_BLOCK;

/*Enums which be can be enabled for clk gate signature capture option
 *
 * format => 0xFFCCDDOO; Where FF:Future use,CC:Count, DD:Delayed trigger,OO:actual options
 */
typedef union __VPP_CLKGATE_OPT__ {
    UINT32 opt;
    struct {
        UINT32 clkGateOpt:8;
        UINT32 clkGateDelayedOpt:8;
        UINT32 clkGateDelayedCount:8;
        UINT32 clkGateOptFuture:8;
    };
}VPP_CLKGATE_OPT;

typedef enum __ENUM_VPP_CLKGATE_OPT__ {
    //This option should present at 1st byte from LSB
    VPP_CLKGATE_OPT_PRINT      = 0x01,
    VPP_CLKGATE_OPT_RECORD_RAW = 0x02,
    VPP_CLKGATE_OPT_REGDUMP    = 0x04,
    //This option should present at 2nd byte from LSB
    VPP_CLKGATE_DLY_OPT_REPEAT = 0x01,        //Repeat or don't Repeat delayed operation
    VPP_CLKGATE_DLY_OPT_SRCFMT = 0x02,        //Delayed print when scrfmt is changed
    VPP_CLKGATE_DLY_OPT_REF_DISP_WIN = 0x04,  //Delayed print when refwin/dispwin is changed
    //This option should present at 3rd byte from LSB
    VPP_CLKGATE_DLY_COUNT_MASK = 0x00FF0000,    //User defined delay in terms of interrupts, otherwise delay count will be used
}ENUM_VPP_CLKGATE_OPT;




#define VPP_REG_GROUP_ITEM_MAX 4
typedef struct VPP_REG_GROUP_T {
    UINT32  uiRegNum;           /**< Valid number of regitsters in the register group */
    UINT32  uiRegGroup[VPP_REG_GROUP_ITEM_MAX * 2];   /**< Array stored register group. */

} VPP_REG_GROUP;

typedef enum VPP_RES_CHANGE_STATUS_T {
    VPP_RES_CHANGE_START = 0,
    VPP_RES_CHANGE_DONE,
} VPP_RES_CHANGE_STATUS;

typedef VOID (*VPP_RES_NOTIFIER)(UINT32 uiData);
typedef struct VPP_RES_CALLBACK_T {
    VPP_RES_NOTIFIER ResNotifier;
    uintptr_t uiUserPara;
}VPP_RES_CALLBACK;

typedef struct VPP_RESOLUTION_DESCRIPTION_T {
        UINT32            uiActiveWidth;
        UINT32            uiActiveHeight;
        UINT32            uiWidth;
        UINT32            uiHeight;
        UINT32            uiRefreshRate;
        UINT32            uiIsInterlaced;
        UINT32            uiIsDDD;
}VPP_RESOLUTION_DESCRIPTION;

typedef struct VPP_INIT_PARM_T{
    INT iHDMIEnable;
    INT iVdacEnable;
#ifdef TZ_3_0
#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
    //Physical address of the shared memory - used for heap/SHM allocation
    UINT32 uiShmPA;
    //Size of the shared memory
    UINT32 uiShmSize;
    UINT32 phShm;
    UINT32 gMemhandle;
#endif
    UINT32 g_vpp;
#endif

}VPP_INIT_PARM;

typedef struct tag_VPP_STS_COUNTER {
     unsigned int value_low; /* real-time counter value: LSB 32bits */
     unsigned int value_high; /* real-time counter value: MSB 32bits */
} VPP_STS_COUNTER;

typedef struct target_display_config_s
{
    UINT16  bit_depth;                            /**<@brief pixel bit depth */
    UINT16  eotf_mode;                                  /**<@brief EOTF mode */
    UINT16  signal_range;                            /**<@brief signal range */
    UINT16  color_space;                              /**<@brief color space */
    UINT8   colorprimaries;
    UINT8   transferspec;
    UINT32  min;                                       /**<@brief signal min */
    UINT32  max;                                       /**<@brief signal max */
    UINT16  min_pq;                              /**<@brief signal min in PQ */
    UINT16  max_pq;                              /**<@brief signal max in PQ */
    UINT16  gamma;   /**<@brief signal gamma value if EOTF_MODE_BT1886 coded */
    UINT16  diag_size;                 /**<@brief display diagonal in inches */
    UINT8   wpExt; //internal or external white point
    INT32   V3Wp[3];    // the white point$
    INT32 CrossTalk;
    UINT8 ignore_mds_ccm;
    INT32 ccm[3];
    INT32 roll_off;
    /* bias */
    INT16  min_pq_bias;              /**<@brief minimum PQ bias */
    INT16  mid_pq_bias;                  /**<@brief mid PQ bias */
    INT16  max_pq_bias;              /**<@brief maximum PQ bias */
    INT16  trim_slope_bias;          /**<@brief trim slope bias */
    INT16  trim_offset_bias;        /**<@brief trim offset bias */
    INT16  trim_power_bias;          /**<@brief trim power bias */
    INT16  trim_slope_adj;
    INT16  trim_offset_adj;
    INT16  trim_power_adj;
    INT16  trim_chrmWt_adj;
    INT32  l2off;
    /* contrast and brightness */
    INT16  contrast;                                      /**<@brief contrast */
    INT16  brightness;                                  /**<@brief brightness */
     /* DM tuning parameters */
    UINT16 cross_talk;                                 /**<@ brief cross talk */
    UINT16 chroma_weight;                      /**<@brief brief chroma weight */
    UINT16 intensity_weight;
    INT16  chroma_weight_bias;  /**<@brief Helmoltz-Kohlrasush fxp: scaling: 64K - 1 */
    INT16  saturation_gain;                        /**<@brief saturation gain */
    UINT16 saturation_gain_bias;     /**<@brief saturation gain bias (4K - 1) */
    INT32 key_weight;
    /* the weight in case of DB_EDGE */
    UINT16  ms_weight;                           /**<@brief multi-scale weight */
    UINT16  ms_weight_bias;        /**<@brief multi-scale weight bias (4K - 1) */
    INT16   ms_edge_weight;                 /**<@brief multi-scale edge weight */
} target_display_config_t;

typedef struct source_signal_info_t_
{
    //User settings
    UINT8 m_bits_per_pixel;
    UINT8 m_color_fmt;
    INT32 m_eotf;
    UINT8 m_colorprimaries;
    UINT8 m_transferspec;
    UINT8 m_color_range;
    UINT16 Gamma;
    UINT16 MinPq, MaxPq;// sig min max in PQ
    UINT16 diagSize; //diagonal width
    UINT8 wpExt; //internal or external white point
    INT32 V3Wp[3];    // the white point$
    INT32 CrossTalk;
}source_signal_info_t;


typedef enum VPP_THDR_VITM_CMD_T
{
    VPP_THDR_VITM_CMD_MIN = 0,
    VPP_THDR_VITM_YUV_RANGE_IN = 1,
    VPP_THDR_VITM_YUV_RANGE_OUT,
    VPP_THDR_VITM_GAUSSIAN_FILTER_TYPE,
    VPP_THDR_VITM_Y_LUT_FILTER_TYPE,
    VPP_THDR_VITM_PQ_OUT_ENABLE,
    VPP_THDR_VITM_PQ_LMAX,
    VPP_THDR_VITM_IN_CLR_FMT,
    VPP_THDR_VITM_OUT_CLR_FMT,
    VPP_THDR_VITM_DEBANDING_ENABLE,
    VPP_THDR_VITM_DENOISING_ENABLE,
    VPP_THDR_VITM_CMD_MAX
}VPP_THDR_VITM_CMD;

#define VMX_FREQ_DIST_MAX 9
#define STRNGTH_THRESHOLD_MAX 12

typedef enum VPP_VMX_CMD_T{
    VPP_VMX_CMD_MIN = 0,
    VPP_VMX_ENABLE_BYPASS = 1,
    VPP_VMX_ENABLE_WATERMARK,
    VPP_VMX_ENABLE_BGWATERMARK,
    VPP_VMX_SET_STRENGTH,
    VPP_VMX_SET_DIRECTIONMAX,
    VPP_VMX_SET_LINEBUFFER,
    VPP_VMX_SET_FG_STRENGTH,
    VPP_VMX_SET_BG_STRENGTH,
    VPP_VMX_SET_FREQ_DIST,
    VPP_VMX_CMD_MAX
}VPP_VMX_CMD;

typedef struct tagVMX_COEFF
{
    UINT32 ColNum;
    UINT32 ColDist;
    INT Freq_Dist[VMX_FREQ_DIST_MAX];
    INT FgStrengthThr[STRNGTH_THRESHOLD_MAX];
    INT BgStrengthThr[STRNGTH_THRESHOLD_MAX];
    UINT32 DirectionMax;
    UINT32 StrengthFactor;
    UINT8 BgWaterMarkOn;
}VPP_VMX_COEFF, *PVPP_VMX_COEFF;

typedef struct VPP_CAPTURE_FRAME_SETTING_T
{
    UINT32           PlaneId;
    VPP_WIN          CapWin;
    UINT32           pFrmBuf;
    UINT32           FrmBufSize;
}VPP_CAPTURE_FRAME_SETTING;

typedef struct VPP_CAPTURE_FRAME_INFO_T
{
    UINT32           uSuccess;
    UINT32           uCapSize;
    UINT16           uiWidth;
    UINT16           uiHeight;
    ENUM_SRC_FMT     ColorFormat;
    ENUM_SRC_PRI     ColorPrimary;
    ENUM_SRC_ORDER   ColorOrder;
    ENUM_SRC_DEPTH   ColorDepth;
    ENUM_SRC_3D_VID_FMT  Video3DFormat;
}VPP_CAPTURE_FRAME_INFO;

typedef struct VPP_PLANE_VIDEO_INFO_T
{
    UINT16           uiWidth;
    UINT16           uiHeight;
    UINT16           uiMaxCapWidth;
    UINT16           uiMaxCapHeight;
    UINT8            ColorFormat;  //ENUM_SRC_FMT
    UINT8            ColorPrimary; //ENUM_SRC_PRI
    UINT8            ColorOrder;   //ENUM_SRC_ORDER
    UINT8            ColorDepth;   //ENUM_SRC_DEPTH
    UINT8            Video3DFormat; //ENUM_SRC_3D_VID_FMT
    UINT8            FrameRate;
}VPP_PLANE_VIDEO_INFO;

typedef void (*VPP_CAPTURE_CALLBACK)(VPP_CAPTURE_FRAME_INFO *info);

typedef struct VPP_VMX_INITPAR_T
{
    BOOL                vpp_vmx_init_flag; //watermark feature init flag
    UINT32              vmx_pass_shm;      //watermark feature pass share memory handle
    UINT32              vmx_pass_reorgnized_shm;      //watermark feature pass share memory handle
    UINT32              vmx_pass_shm_size;  //the size of vmx pass share memory
    VOID                *pass_reorganized_shm_vir;     //pass share memory virtual address
    VOID                *pass_reorganized_shm_phy;     //pass share memory physical address
    VOID                *pass_shm_vir;     //pass share memory virtual address
    VOID                *pass_shm_phy;     //pass share memory physical address
    UINT32              main_vmx_frame_desc_shm;//share memory handle for watermark vbuf info(frame desc)
    VOID                *main_vmx_frame_desc;   //the frame desc for main plane
    UINT32              main_bitmap_shm;        //the bitmap share memory handle for main plane
    UINT32              main_bitmap_shm_size;   //the size of bitmap share memory
    VOID                *main_bitmap_vir;       //the bitmap share memory virtual address
    VOID                *main_bitmap_phy;       //the bitmap share memory physical address
}VPP_VMX_INITPAR;
#if 0
typedef struct VPP_RESOLUTION_DESCRIPTION_T {
    UINT32            uiActiveWidth;
    UINT32            uiActiveHeight;
    UINT32            uiWidth;
    UINT32            uiHeight;
    UINT32            uiRefreshRate;
    UINT32            uiIsInterlaced;
    UINT32            uiIsDDD;
}VPP_RESOLUTION_DESCRIPTION;
#endif

#define VPP_DV_FRAME_DUMP_MAX_FRAMES               1000
#define VPP_DV_FRAME_DUMP_MAX_USED_BITS_PER_WORD   32
//1031/32 = 32 => 32*32 = 1024
#define VPP_DV_FRAME_DUMP_MAX_USED_BITS_WORDS      ((VPP_DV_FRAME_DUMP_MAX_FRAMES+VPP_DV_FRAME_DUMP_MAX_USED_BITS_PER_WORD-1)/VPP_DV_FRAME_DUMP_MAX_USED_BITS_PER_WORD)


typedef struct VPP_DV_FRAME_DUMP_T {
    //Configurable parameters
    //AMP_SHM_HANDLE handle;         //frame buffer SHM handle
    unsigned int phyAddr;            //frame buffer start address - PhyAddress
    unsigned int frameSize;          //size of one frame in the buffer
    unsigned int nFrames;            //#frames in frame buffer
    unsigned int skipFrames;         //Number of frames to be skipped before initiating framedump
    unsigned char overwriteFlag : 1; //Overwrite the frame buffer upon consuming all buffers
    unsigned char forcecapture  : 1; // force capture flag for enable builtin frame check for DV capture cases
    unsigned char reserve : 6;

    //Internal or output parameters
    unsigned int currentNdx;    //current read/write pointer of buffer, one behind the nextNdx
    unsigned int nextNdx;       //Next read/write pointer of buffer, update with every new frame recieved from inputq
    unsigned int usedFrames[VPP_DV_FRAME_DUMP_MAX_USED_BITS_WORDS];    //frame dump status bit (1st world LSB:0, 2nd word LSB:32
    unsigned int Errorstatus; //Flag for error handle during overlay screen capture
} VPP_DV_FRAME_DUMP;

//Support 128bit(6*(4*8) = 24*8 = 192) ClkGateSignature
typedef struct VPP_CLK_GATE_SIGNATURE_T {
  UINT32 signature0;
  UINT32 signature1;
  UINT32 signature2;
  UINT32 signature3;
  UINT32 signature4;
  UINT32 signature5;
} VPP_CLK_GATE_SIGNATURE;

typedef struct Mipi_Display_Config_T_
{
    UINT8 panelID;
    UINT8 panelResID;
    UINT8 panelType; //video mode or command mode
    UINT8 orientation;
    UINT8 lanes;
    UINT8 user_config_en;/*1: enable, 0: default config*/
}Mipi_Display_Config_t;

typedef enum
{
    DISPLAY_PANELID_BOE_540x960 = 0,
    DISPLAY_PANELID_LTK_1920x1200 = 1,
    DISPLAY_PANELID_BOE_800x1280 = 2,
    DISPLAY_PANELID_MAX
}Display_PanelID_t;

typedef struct VPP_DISP_OUT_PARAMS_T {
  ENUM_VOUT_ID uiDispId;
  int uiResId;
  UINT32 uiBitDepth;
  UINT32 uiColorFmt;
  INT32 iPixelRepeat;
} VPP_DISP_OUT_PARAMS;

#define VPP_CLKGATE_SIG_MAX_WORDS (sizeof(VPP_CLK_GATE_SIGNATURE)/sizeof(UINT32))

/************* VPP module external APIs *****************/
#ifdef VPP_ENABLE_FLOW_PRINT
/***********************************************************
* FUNCTION: initialize VPP API trustzone instance
* PARAMS: none
* RETURN: MV_VPP_OK - succeed
*         MV_VPP_EBADCALL - function called previously
**********************************************************/
INT MV_VPP_Early_Init(void);
#endif //VPP_ENABLE_FLOW_PRINT

#ifdef TZ_3_0
int MV_VPP_INITVPPS(int instance,UINT32 vpp_addr,UINT32 heap_handle);
#endif

/******************************************************
  * FUNCTION: AVIO Reset
  * INPUT: NONE
  * RETURN: NONE
  * NOTE : This function resets complete AVIO block. Need
  *         to config complete AVIO including DHUB block
  *******************************************************/
int MV_VPPOBJ_AVIO_Reset(void) ;

/*********************************************************
 * FUNCTION: initialize VPP module
 * PARAMS: vpp_init_parm - VPP init Parameter
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EBADCALL - function called previously
 ********************************************************/
int MV_VPPOBJ_Init(VPP_INIT_PARM *vpp_init_parm);

/***********************************************
 * FUNCTION: create a VPP object
 * PARAMS: base_addr - VPP object base address
 *         *handle - pointer to object handle
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EUNCONFIG - not initialized
 *         MV_VPP_ENODEV - no device
 *         MV_VPP_ENOMEM - no memory
 ***********************************************/
int MV_VPPOBJ_Create(int base_addr, int *handle);

/***********************************************
 * FUNCTION: destroy a VPP object
 * PARAMS: handle - VPP object handle
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EUNCONFIG - not initialized
 *         MV_VPP_ENODEV - no device
 *         MV_VPP_ENOMEM - no memory
 ***********************************************/
int MV_VPPOBJ_Destroy(int handle);

/***************************************
 * FUNCTION: VPP Suspend
 * INPUT: NONE
 * RETURN: NONE
 **************************************/
int MV_VPPOBJ_Suspend(int handle);

/***************************************
 * FUNCTION: VPP Resume
 * INPUT: NONE
 * RETURN: NONE
 **************************************/
int MV_VPPOBJ_Resume(int handle);

/***************************************
 * FUNCTION: VPP reset
 * INPUT: NONE
 * RETURN: NONE
 **************************************/
int MV_VPPOBJ_Reset(int handle);

/***********************************************
 * FUNCTION: Unregiste VPP interrupts
 * PARAMS: handle - VPP object handle
 * RETURN: MV_VPP_OK - succeed
 ***********************************************/
void VPP_ISR_Unregistration(int handle);

/********************************************************************
 * FUNCTION: Enable/Disable VPP video bypass mode
 * INPUT: flag: 1: enable; 0: disable
 * RETURN: NONE
 * NOTE: In video bypass mode, video format keeps in 4:2:2 format
 *       while going through VPP pipeline.
 *       This API needs to be called before MV_VPP_Config().
 ********************************************************************/
int MV_VPPOBJ_EnableBypassMode(int handle, int flag);
/***************************************
 * FUNCTION: VPP profile configuration
 * INPUT: NONE
 * RETURN: NONE
 **************************************/
int MV_VPPOBJ_Config(int handle, const int *pvinport_cfg, const int *pdv_cfg, const int *pzorder_cfg, const int *pvoutport_cfg);

// to be removed when VPP suspend/resume is ready
int MV_VPP_GetPlaneInfo(int handle, int planeID, uintptr_t *curr_frame, uintptr_t *curr_still_picture, int *mute);

/********************************************************************************
 * FUNCTION: Get VPP STS Counter Value
 * INPUT: handle  - VPP object handle
 *      : pSTSCounter  - pointer to STS counter
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_GetSTSCounter(int handle,VPP_STS_COUNTER *pSTSCounter);

/********************************************************************************
 * FUNCTION: Get VPP STS Counter
 * INPUT: handle  - VPP object handle
 *      : pSTSCounter  - pointer to VPP sts counter
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_GetSTSRunningCounter(int handle,  VPP_STS_COUNTER *pSTSCounter);

/******************************************************************************
 * FUNCTION: open a display window of a video/graphics plane for display.
 *           display window is defined in end display resolution
 * INPUT: planeID - id of a video/grahpics plane
 *        *win - pointer to a vpp window struct
 *        *attr - pointer to a vpp window attribute struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_OpenDispWindow(int handle, int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr);

/*********************************************************************************
 * FUNCTION: close a display window of a video/grahpics plane in displaying
 *    - discard all frames in frame queue;
 *    - setup plane in INACTIVE status.
 * INPUT: planeID - id of the plane
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_CloseDispWindow(int handle, int planeID);

/***************************************************************
 * FUNCTION: get plane status
 * INPUT: planeID - id of the plane
 *        *status - pointer to status variable
 * RETURN: STATUS_INACTIVE - inactive status
 *         STATUS_INACTIVE_PENDING - inactivate command issued
 *         STATUS_ACTIVE - active status, ready to accept frame
 *         STATUS_ACTIVE_PENDING - activate command issued
 *         STATUS_DISP_VIDEO - displaying video frames
 *         STATUS_DISP_LOGO - displaying logo frame
 **************************************************************/
int MV_VPPOBJ_GetPlaneStatus(int handle, int planeID, int *status);

/************************************************************************
 * FUNCTION: recycle plane frame descriptors
 * INPUT: planeID - id of the plane
 * RETURN: 0 - no frame descriptor for recycle or parameter error
 *         other - pointer to frame descriptor being recycled
 ************************************************************************/
uintptr_t MV_VPPOBJ_RecycleFrames(int handle, int planeID);

uintptr_t MV_VPPOBJ_GetFrameFromOvpScalr(int handle, int planeID);

/*************************************************************************
 * FUNCTION: set a new still picture for a plane
 *           old still picture will be returned if available and pold
 *           is non-NULL.
 * INPUT: planeID - id of the plane
 *        *pnew - new still picture frame descriptor
 *        **pold - old still picture frame descriptor
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 ************************************************************************/
int MV_VPPOBJ_SetStillPicture(int handle, int planeID, void *pnew, void **pold);

/*******************************************************************
 * FUNCTION: set display mode for a plane
 * INPUT: planeID - id of the plane
 *        mode - DISP_STILL_PIC: still picture, DISP_FRAME: frame
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int MV_VPPOBJ_SetDisplayMode(int handle, int planeID, int mode);

/*******************************************************************
 * FUNCTION: set display mode for a plane
 *           must be called from ISR because there is no race condition
 *           protection.
 *******************************************************************/
int MV_VPPOBJ_SetDisplayModeFromISR(int handle, int planeID, int mode);

/*******************************************************************
 * FUNCTION: Display a frame for a plane
 *    - push a frame into plane's frame queue;
 *    - set DISP_VIDEO status to display frame in frame queue.
 * INPUT: planeID - id of the plane
 *        *frame - frame descriptor
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int MV_VPPOBJ_DisplayFrame(int handle, int planeID, void *frame);

/*******************************************************************
 * FUNCTION: push frame for recycling
 * INPUT: planeID - id of the plane
 *        *frame - frame descriptor
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int MV_VPPOBJ_FillOutQueue(int handle, int planeID, void *frame);

/*******************************************************************
 * FUNCTION: get CPCB VCO frequency
 * INPUT: cpcbID - CPCB(for Berlin) or DV(for Galois) id
 *        *pvcofreq - pointer to vco frequency ID
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int MV_VPPOBJ_GetCPCBVCOFreq(int handle, int cpcbID, int *pvcofreq);

/*******************************************************************
 * FUNCTION: set CPCB or DV output resolution
 * INPUT: cpcbID - CPCB(for Berlin) or DV(for Galois) id
 *        resID - id of output resolution
 *        bit_depth - HDMI deep color bit depth
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 * Note: this function has to be called before enabling a plane
 *       which belongs to that CPCB or DV.
 *******************************************************************/
int MV_VPPOBJ_SetCPCBOutputResolution(int handle, int cpcbID, int resID, int bit_depth);

/*******************************************************************
 * FUNCTION: get CPCB or DV output resolution
 * INPUT: cpcbID - CPCB(for Berlin) or DV(for Galois) id
 *        *resID - pointer to resolution ID variable
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int MV_VPPOBJ_GetCPCBOutputResolution(int handle, int cpcbID, int *resID);

/*******************************************************************
 * FUNCTION: Set CPCB/Display output parameters
 * INPUT: cpcbID - CPCB(for Berlin) or DV(for Galois) id
 *        *pDispParams - Display output parameters
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int MV_VPPOBJ_SetFormat(INT handle, INT cpcbID,
                        VPP_DISP_OUT_PARAMS *pDispParams);

/*******************************************************************
 * FUNCTION: Get resoution description of resolution id ResId,
 * INPUT: ResID - the resolution index to get the description string
 *        pResDesc - the buffer to store the resolution description
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *******************************************************************/
int MV_VPPOBJ_GetResolutionDescription(int handle, int ResId,
                                       VPP_RESOLUTION_DESCRIPTION *pResDesc);

/*******************************************************************
 * FUNCTION: get CPCB input clock in unit of HZ
 * INPUT: cpcbID - cpcb ID
 *        *pclock - pointer to clock variable
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *******************************************************************/
int MV_VPPOBJ_GetCPCBInputClock(int handle, int cpcbID, unsigned int *pclock);

/*******************************************************************
 * FUNCTION: get the pixel clock by resolution
 * INPUT: cpcbID -resID - resolution ID
 *        *pixel_clock - pointer to pixel clock variable
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int MV_VPPOBJ_GetCPCBOutputPixelClock(int handle, int resID, int *pixel_clock);

/*******************************************************************
 * FUNCTION: get plane PTS
 * INPUT: planeID - plane id
 *        *lst_valid_PTS - contain last valid PTS
 *        *curr_PTS - contain current PTS
 *        *frame_cnt - contain frame count since last valid PTS
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *******************************************************************/
int MV_VPPOBJ_GetPTS(int handle, int planeID, PTS *lst_valid_PTS, PTS *curr_PTS, int *frame_cnt);

/***************************************************
 * FUNCTION: mute/un-mute a plane
 * PARAMS:  planeID - plane to mute/un-mute
 *          mute - 1: mute, 0: un-mute
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int MV_VPPOBJ_SetPlaneMute(int handle, int planeID, int mute);

/***************************************************
 * FUNCTION: adjust pq settings
 * PARAMS:  planeID - plane to adjust pq
 *          mode - the pq param to adjust
 *          value- value to set for mode
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int MV_VPPOBJ_SetPictureControl(INT handle, INT planeID, ENUM_PICTURE_CTRL mode, INT value);


/***********************************************
 * FUNCTION: change CPCB z-order dynamically
 * PARAMS:  cpcbID - CPCB to make change
 *          *pZorderCtrl - pointer to z-order for each layer
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int MV_VPPOBJ_ChangeZOrder(int handle, int cpcbID, VPP_ZORDER_CTRL *pZorderCtrl);

/***********************************************
 * FUNCTION: get CPCB z-order
 * PARAMS:  cpcbID - CPCB to make change
 *          *pZorderCtrl - pointer to z-order for each layer
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int MV_VPPOBJ_GetZOrder(int handle, int cpcbID, VPP_ZORDER_CTRL *pZorderCtrl);

/*******************************************************************
 * FUNCTION: VPP  log config set
 * INPUT: cmd - vpp log cmd
 *            Parm - command related
 *            module - VPP log module
 *            level - VPP log level
 * RETURN: MV_VPP_OK - SUCCEED
 *               MV_EBADPARAM - invalid parameters
 * Note: this function has to be called after the vpp log function initialized.
 *******************************************************************/
int MV_VPPOBJ_LogCtrl(unsigned int cmd, unsigned int parm, unsigned int module, unsigned int level);

/**************************************************************************
 * FUNCTION: get current field ID in display of a plane
 * PARAMS:  planeID - plane ID
 *          *field - field ID, 0: first field, 1: second field
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 **************************************************************************/
int MV_VPPOBJ_GetFieldID(int handle, int planeID, int *fieldID);

/**************************************************************************
 * FUNCTION: set alpha polarity of a plane
 * PARAMS:  planeID - plane ID
 *          src_pol - source alpha polarity
 *          bg_pol - border alpha polarity
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 **************************************************************************/
int MV_VPPOBJ_SetAlphaPolarity(int handle, int planeID, int src_pol, int bg_pol);

/********************************************************
 * FUNCTION: set pattern size for patgen in main channel
 * PARAMS:  pWin - size of pattern
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ********************************************************/
int MV_VPPOBJ_SetPatternSize(int handle, VPP_WIN *pWin);

/********************************************************
 * FUNCTION: set pattern frame type
 * PARAMS:  pType - frame type
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ********************************************************/
int MV_VPPOBJ_SetPatternFrameType(int handle, VPP_PATGEN_TYPE *pType);

/********************************************************
 * FUNCTION: set pattern data
 * PARAMS:  pData - pattern data
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ********************************************************/
int MV_VPPOBJ_SetPatternData(int handle, VPP_PATGEN_DATA *pData);

/*******************************************************************
 * FUNCTION: get CPCB VBI interrupt counter
 * INPUT: cpcbID - CPCB id
 *        *vbi_num - vbi interrupt count
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *******************************************************************/
int MV_VPPOBJ_GetCPCBInterruptCounter(int handle, int cpcbID, int *vbi_num);

/***************************************************************
 * FUNCTION: set the reference window for a plane
 * INPUT: planeID - id of the plane
 *        *win - pointer to the reference window struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetRefWindow(int handle, int planeID, VPP_WIN *win);

/***************************************************************
 * FUNCTION: set the reference window for a plane
 *           must be called from ISR because there is no race condition
 *           protection
 **************************************************************/
int MV_VPPOBJ_SetRefWindowFromISR(int handle, int planeID, VPP_WIN *win);

/******************************************************************************
 * FUNCTION: get reference window information of a video/graphics plane.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of a video/grahpics plane
 *        *win - pointer to a vpp window struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_GetRefWindow(int handle, int planeID, VPP_WIN *win);

/******************************************************************************
 * FUNCTION: get content window information of a video/graphics plane.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of a video/grahpics plane
 *        *win - pointer to a vpp window struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_GetContentWindow(int handle, int planeID, VPP_WIN *win);

/***************************************************************
 * FUNCTION: set the crop window for a plane
 * INPUT: planeID - id of the plane
 *        *win - pointer to the crop window struct
 * RETURN: MV_VPP_OK - SUCCEE
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetCropWindow(int handle, int planeID, VPP_WIN *win);

/******************************************************************************
 * FUNCTION: change a window of a video/graphics plane.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of a video/grahpics plane
 *        *win - pointer to a vpp window struct, NULL for no change
 *        *attr - pointer to a vpp window attribute struct, NULL for no change
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_ChangeDispWindow(int handle, int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr);

/******************************************************************************
 * FUNCTION: change a window of a video/graphics plane.
 *           must be called from ISR because there is no race condition
 *           protection
 ******************************************************************************/
int MV_VPPOBJ_ChangeDispWindowFromISR(int handle, int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr);

/******************************************************************************
 * FUNCTION: get window information of a video/graphics plane.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of a video/grahpics plane
 *        *win - pointer to a vpp window struct
 *        *attr - pointer to a vpp window attribute struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_GetDispWindow(int handle, int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr);

/*******************************************************************
 * FUNCTION: enable/disable non linear horizontal scaling
 * INPUT: planeID - id of the plane
 *        enable - 1: enable; 0: disable
 *        center - center value [0, 255]
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetScalerNonLinearControl(int handle, int planeID, int enable, int center_frac, int center_ratio);

/*******************************************************************
 * FUNCTION: set scaler horizontal and vertial phase DDA count ( sub pixel shifting)
 * INPUT: scalerId - scaler index
 *            hv - horizontal or vertical
 *            counter - DDA counter
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetScalerPhaseDda(int handle, VPP_FRC_SCL_NUM scalerId, int hv, int count);

/****************************************************************************
 * FUNCTION: set csc mode for gfx plane
 * INPUT: planeID - id of the GFX plane
 *        iCscMode - csc mode
 * RETURN: MV_VPP_OK - SUCCEED
 ***************************************************************************/
int MV_VPPOBJ_SetGfxCsc(int handle, int planeID, int iCscMode);

/********************************************************************************
 * FUNCTION: Set Deint Param
 * INPUT: deintCmd- deint cmd
   INPUT: pDeintRegs- Deint DS parameter depend upon cmd
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetDEINTParam(int handle, int deintCmd, VPP_VDO_DEINT_REG *pDeintRegs );

/********************************************************************************
 * FUNCTION: Get Deint Param
 * INPUT: deintCmd- deint cmd
   OUTPUT: pDeintRegs- Deint DS parameter depend upon cmd
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetDEINTParam(int handle, int deintCmd, VPP_VDO_DEINT_REG *pDeintRegs );

/****************************************************************
 * FUNCTION: set De-interlacer and VNR working mode
 * INPUT: deint_mode - de-interlacer mode
 *        vnr_mode - VNR mode
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 *****************************************************************/
int MV_VPPOBJ_SetDeintMode(int handle, int deint_mode);

/******************************************************************************
 * FUNCTION: set plane border alpha.
 * INPUT: planeID - id of a video/grahpics plane
 *        alpha - plane border alpha value[0, 255]
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_SetPlaneBorderAlpha(int handle, int planeID, int alpha);

/*******************************************************************
 * FUNCTION: set CPCB background color
 * INPUT: cpcbID - if of the CPCB
 *        pBgColor - pointer to the structure of background color
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int MV_VPPOBJ_SetCPCBBgColor(int handle, int cpcbID, VPP_BG_COLOR *pBgColor);

/********************************************************************************
 * FUNCTION: set the source of video output
 * INPUT: vout - id of the video output
 *            cpcbID - id of the CPCB
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetVoutSource(int handle, int vout, int cpcbID, int interlaced);

/********************************************************************************
 * FUNCTION: mute/un-mute a video output
 * INPUT: vout - id of the video output
 *            mute - 1:mute, 0:un-mute
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetVoutMute(int handle, int vout, int mute);

/********************************************************************************
 * FUNCTION: This function is extension of MV_VPPOBJ_SetVoutMute.
 *           Compared with MV_VPPOBJ_SetVoutMute, this function will
 *           mute Audio/Video or Both
 *           Notice, this function is only effect on HDMI port
 * INPUT: vout - id of the video output
 *        mute - 1:mute, 0:un-mute
 *        bVideo - 1:mute or un-mute video, 0:do nothing on video
 *        bAudio - 1:mute or un-mute audio, 0:do nothing on audio
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetVoutMute_Ext(INT handle, int vout, int mute, int bVideo, int bAudio);

/********************************************************************************
 * FUNCTION: power on/down an analog video output
 * INPUT: vdac - id of the video DAC
 *            enable - 1:power on, 0:power down
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetVdacPower(int handle, int vdac, int enable);

int MV_VPPOBJ_TurnOffCLK(int handle, int off);

int MV_VPPOBJ_SetHdmiPhyPower(int handle, int enable);

int MV_VPPOBJ_EnableHdmiAudioFmt(int handle, int enable);

int MV_VPPOBJ_SetHdmiTxControl(int handle, int enable);

/********************************************************************************
 * FUNCTION: Set Hdmi Video format
 * INPUT: color_fmt - color format (RGB, YCbCr 444, 422)
 *      : bit_depth - 8/10/12 bit color
 *      : pixel_rept - 1/2/4 repetitions of pixel
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiVideoFmt(int handle, int color_fmt, int bit_depth, int pixel_rept);

/********************************************************************************
 * FUNCTION: Set Hdmi TMDS Mux selection
 * INPUT:  MUX VALUE for Ch3Ch2Ch1Ch0 [Input value of Hex where each Nibble corresponds to each channel]
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiTmdsMuxSel(int handle, UINT32 mux_value);

/********************************************************************************
 * FUNCTION: Set Hdmi 3D structure
 * INPUT: structure - structure ID
 *      : ext_data - extra data for structure 8 or above
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmi3DStructure(int handle, int structure, int ext_data);

/********************************************************************************
 * FUNCTION: set Hdmi Audio format
 * INPUT: sampFreq - Audio sampling frequency
 *      : sampSize - Word length
 *      : mClkFactor - 128/256/384/512 times frequency
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiAudioFmt(int handle, int enable, VPP_HDMI_AUDIO_CFG *pAudioCfg);

/********************************************************************************
 * FUNCTION: set Hdmi audio VUC bits
 * INPUT: pVUCCfg        - pointer to VUC config structure
 *        loadOnAudioCfg - 1 for loading these data when audio
 *                       - is configured subsequently
 *                       - 0 for loading it on the next VBI
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiAudioVUCBits(int handle, VPP_HDMI_AUDIO_VUC_CFG *pVUCCfg, int loadOnAudioCfg);

/********************************************************************************
 * FUNCTION: set/clear Hdmi audio fifo reset
 * INPUT: bSet - 0/1 to set/clear audio fifo reset
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiAudioFifoReset(int handle, int bSet);

/********************************************************************************
 * FUNCTION: mute/un-mute hdmi audio output
 * INPUT: mute - 1:mute, 0:un-mute
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiAudioMute(int handle, int mute);

/********************************************************************************
 * FUNCTION: Set Hdmi output mode (Hdmi/Dvi)
 * INPUT: hdmiMode - 0:DVI, 1:HDMI
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiMode(int handle, int hdmiMode);

/********************************************************************************
 * FUNCTION: Set HDMI Video Information
 * INPUT: aspect ratio
 *      : scan info
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiVideoInfo(int handle, int aspRatio, int scanInfo);

/********************************************************************************
 * FUNCTION: Set HDMI Audio Information
 * INPUT: Speaker Map
 *      : Level Shift to denote attenuation (0 - 15)dB
 *      : Down mix inhibit flag
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiAudioInfo(int handle, VPP_HDMI_SPKR_ALLOC *pSpkrMap,
                               int lvlShiftVal, int downMixInhFlag);

/********************************************************************************
 * FUNCTION: Register for HDCP event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : callbackFn - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_RegisterHdcpCallback (int handle, UINT32 eventCode,
                                    MV_VPP_EVENT_CALLBACK callbackFn,
                                    void *contextParam);

/********************************************************************************
 * FUNCTION: Unregister for HDCP event notifications
 * INPUT: None
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_UnRegisterHdcpCallback (int handle);

/********************************************************************************
 * FUNCTION: Register for HDMI event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : callbackFn - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_RegisterHdmiCallback (int handle, UINT32 eventCode,
                                    MV_VPP_EVENT_CALLBACK callbackFn,
                                    void *contextParam);

/********************************************************************************
 * FUNCTION: Unregister for HDMI event notifications
 * INPUT: None
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_UnRegisterHdmiCallback (int handle);

/********************************************************************************
 * FUNCTION: Send HDMI specified auxillary packets
 * INPUT: pktType - Packet Type
 *      : pAuxPkt - Pointer to auxillary packet data
 *      : repeatPkt - Transmit packet once/repeat every VSync
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SendHdmiAuxPkt (int handle, VPP_HDMI_PKT_ID pktType,
                              VPP_HDMI_PKT *pAuxPkt, BOOL repeatPkt);

/********************************************************************************
 * FUNCTION: Stop HDMI auxillary packets
 * INPUT: pktType - Packet Type
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_StopHdmiAuxPkt (int handle, VPP_HDMI_PKT_ID pktType);

#ifdef XVYCC_SUPPORT
/********************************************************************************
 * FUNCTION: Enable\disable xvYCC
 * INPUT: Enable - 1: enable; 0:disable
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_xvYCCEnable (int handle, BOOL Enable);

/********************************************************************************
 * FUNCTION: Set xvYCC mode
 * INPUT: mode - 0: file mode;
                          1: hdmi in mode
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetxvYccMode (int handle, int mode);
#endif

/********************************************************************************
 * FUNCTION: Set SRM Data
 * INPUT: pSrmData - Pointer to SRM data and length (Input)
 *      : pDevSignData - Pointer to device signature to check content
 *      :              - validity (Input)
 *      : pVernNum - Pointer to return SRM info parsed and updated (output)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetSRM (int handle, VPP_HDCP_SRM_DATA *pSrmData,
                      VPP_DEV_SIGN_DATA *pDevSignData, VPP_HDCP_SRM_INFO *pSrmInfo);

/********************************************************************************
 * FUNCTION: Get SRM Version
 * INPUT: pHdcp22SrmVersion - HDCP 2.2 SRM File version (Output)
 *      : pHdcp22SrmVersion - HDCP 1.4 SRM File version (Output)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetSRMVersion(int handle, UINT16 *pHdcp22SrmVersion, UINT16 *pHdcp14SrmVersion);

/********************************************************************************
 * FUNCTION: Enable/Disable HDCP SRM check. This API controls if revoked list
 *         : check would be done
 * INPUT   : enable - Enable/Disable HDCP SRM Check
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_EnableSRMCheck(int handle, int enable);

/********************************************************************************
 * FUNCTION: Before loading HDCP Keys to the registers, check if the VPP status is ready
 * INPUT: handle    - VPP object handle
 *      : pHdcpKeys - Pointer to HDCP keys to be loaded
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - HDCP is enabled currently(Keys cannot be loaded
 *                         - with HDCP on)
 ********************************************************************************/
int MV_VPPOBJ_LoadHDCPKeys_PreCheck(int handle, UINT8 *pHdcpKeys);

/********************************************************************************
 * FUNCTION: Enable/Disable HDCP
 * INPUT: Enable - Control to turn HDCP on/off
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_EnableHDCP(int handle, int enable);

/********************************************************************************
 * FUNCTION: Read HDCP Version
 * OUTPUT:  - pHDCPVersion (value returned HDCP 1.x = 0 and HDCP2.x = 1)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDCPVersion(int handle, int *pHDCPVersion);

/********************************************************************************
 * FUNCTION: Read Current HDCP Version
 * OUTPUT:  - pHDCPVersion (value returned HDCP 1.x = 0 and HDCP2.2 = 1)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 *         MV_VPP_ENOSINKCNCTED - No sink connected
 *         MV_VPP_ENOHDCPENABLED - HDCP not enabled
 ********************************************************************************/
int MV_VPPOBJ_GetCurrentHDCPVersion(int handle, VPP_HDMI_HDCP_VERSION *pHDCPVersion);

/********************************************************************************
 * FUNCTION: Read Max HDCP Version capability of Sink
 * OUTPUT:  - pHDCPVersion (value returned HDCP 1.x = 0 and HDCP2.2 = 1)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 *         MV_VPP_ENOSINKCNCTED - No sink connected
 *         MV_VPP_ENOHDCPENABLED - HDCP not enabled
 ********************************************************************************/
int MV_VPPOBJ_GetSinkHDCPCapability(int handle, VPP_HDMI_HDCP_VERSION *pHDCPVersion);

/********************************************************************************
 * FUNCTION: Set HDCP Version
 * INPUT:  iHDCPVersion (value to be set for  HDCP 1.x = 0 and HDCP2.x = 1)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHDCPVersion(int handle, int iHDCPVersion);

/********************************************************************************
 * FUNCTION: Read HDCP Status
 * OUTPUT:  - pHDCPState (Authorization state)( HDCP_STATE)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDCPState(int handle, int *pHDCPState);

/********************************************************************************
 * FUNCTION: Get HDCP Ksv
 * INPUT: pHdcpKey - HDCP ksv data structure
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDCPKsv(int handle, VPP_HDCP_KSV_DATA *pHdcpKsv);

/********************************************************************************
 * FUNCTION: Set HDMI TX HDCP mode
 * INPUT:  - Mode(Repeater/Trasmitter mode)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_SetHDCPRepeaterMode(int handle, UINT32 Mode);

/********************************************************************************
 * FUNCTION: Get HPD status
 * INPUT: pHpdStatus
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHPDStatus(int handle, int *pHpdStatus);

/********************************************************************************
 * FUNCTION: Get HDR10 playback status
 * OUTPUT: pIsHdrPlaying (1-HDR playing, 0-HDR not playing)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 ********************************************************************************/
int MV_VPPOBJ_GetHDR10PlaybackStatus(int handle, BOOL *pIsHdrPlaying);

/********************************************************************************
 * FUNCTION: Get Hdmi Sink Capabilities
 * INPUT: pSinkCaps
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDMISinkCaps(int handle, VPP_HDMI_SINK_CAPS *pSinkCaps);

/********************************************************************************
 * FUNCTION: Get Hdmi Raw EDID
 * INPUT: pSinkCaps
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_GetHDMIRawEdid(int handle, VPP_HDMI_RAW_EDID *pRawEdid);

/*******************************************************************
 * FUNCTION: set Macrovision for HD/SD VOUT
 * INPUT:  vout - VOUT_HD or VOUT_SD
 *         mode - Macrovision mode
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetMacrovision(int handle, int vout, int mode);

/*******************************************************************
 * FUNCTION: set WSS Data
 * INPUT:  vout - VOUT_HD or VOUT_SD
 *         WssData - pointer to WW data struct
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetWSSData(int handle, int vout, VPP_WSS_DATA *WssData);

/*******************************************************************
 * FUNCTION: set CGMS Data
 * INPUT:  vout - VOUT_HD or VOUT_SD
 *         CgmsData - pointer to CGMS data struct
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetCGMSData(int handle, int vout, VPP_CGMS_DATA *CgmsData);

/*******************************************************************
 * FUNCTION: set CC Data
 * INPUT:   CCData - pointer to CC data.
 *         length - CC data length by 16-bit
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetCCData(int handle, unsigned short *CCData, int length);

/*******************************************************************
 * FUNCTION: set TT Data
 * INPUT:   TTData - pointer to TT data.
 *         length - TT data length by byte
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetTTData(int handle, unsigned char *TTData, int length, int TTCtrl);

/*******************************************************************
 * FUNCTION: set Luma Key
 * INPUT:  enable - enable control
 *         low_key - lower key (inclusive)
 *         up_key - upper key (inclusive)
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_SetLumaKey(int handle, int enable, int low_key, int high_key);

/****************************************************************************
 * FUNCTION: set GFX0/PG plane input mode
 * INPUT: planeID - id of the GFX plane
 *        mode - GFX_INPUT_MODE_NORMAL, GFX_INPUT_MODE_MATTE
 * RETURN: MV_VPP_OK - SUCCEED
 ***************************************************************************/
int MV_VPPOBJ_SetGfxInputMode(int handle, int planeID, int mode);

/********************************************************************************
 * FUNCTION: Configure the features that needs to be enabled for CEC (Feature
 *         : macros are defined in vpp_api_types.h)
 * INPUT:    handle            - VPP object handle
 *      :    featureSupportCfg - Config setting to indicate features to be
 *      :                      - enabled
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 *           MV_VPP_EBADCALL - CEC is already enabled, API can be called only
 *                           - if CEC is disabled
 ********************************************************************************/
int MV_VPPOBJ_CecConfigFeatureSupport (int handle, int featureSupportCfg);

/********************************************************************************
 * FUNCTION: Returns CEC feature configuration currently set
 * INPUT:    handle             - VPP object handle
 *      :    pFeatureSupportCfg - Pointer to return the feature configuration
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_CecGetFeatureSupportConfig (int handle, int *pFeatureSupportCfg);

/********************************************************************************
 * FUNCTION: Enable/Disable HDMI CEC feature
 * INPUT:    handle - VPP object handle
 *      :    enable - TRUE, to enable CEC; FALSE, to disable CEC
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 *           MV_EUNSUPPORT - channel not connected in configuration
 *           MV_VPP_EBADCALL - channel not connected to DV1
 ********************************************************************************/
int MV_VPPOBJ_CecEnable (int handle, int enable);

/********************************************************************************
 * FUNCTION: Set CEC device physical address - Setting to the invalid address
 *         : would put CEC in idle state
 * INPUT:    handle  - VPP object handle
 *      :    phyAddr - Physical address of the device
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 *           MV_EUNSUPPORT - channel not connected in configuration
 *           MV_VPP_EBADCALL - channel not connected to DV1
 ********************************************************************************/
int MV_VPPOBJ_CecSetPhysicalAddr (int handle, int phyAddr);

/********************************************************************************
 * FUNCTION: Allocate/Unallocate logical address for the given device type
 * INPUT:    handle          - VPP object handle
 *      :    alloc           - Allocate/Unallocate logical address
 *      :    deviceType      - Device type
 *      :    initialPollAddr - Initial polling address; -1, if default is to be
 *      :                      used
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 *           MV_EUNSUPPORT - channel not connected in configuration
 *           MV_VPP_EBADCALL - channel not connected to DV1
 ********************************************************************************/
int MV_VPPOBJ_CecAllocLogicalAddr (int handle, BOOL alloc,
                                   VPP_CEC_DEVICE_TYPE deviceType,
                                   int initialPollAddr);

/********************************************************************************
 * FUNCTION: Start transmission of given message on CEC line
 * INPUT:    handle  - VPP object handle
 *      :    pCecMsg   - Pointer to CEC message (Input)
 * RETURN:   MV_VPP_OK - SUCCEED
 *           MV_EBADPARAM - invalid parameters
 *           MV_EUNCONFIG - VPP not configured
 *           MV_VPP_ENODEV - no device
 *           MV_EUNSUPPORT - channel not connected in configuration
 *           MV_VPP_EBADCALL - channel not connected to DV1
 ********************************************************************************/
int MV_VPPOBJ_CecSendMessage (int handle, PVPP_CEC_MSG pCecMsg);

/********************************************************************************
 * FUNCTION: Register for CEC event notifications
 * INPUT: handle  - VPP object handle
 *      : eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_CecRegisterCallback (int handle, UINT32 eventCode,
                                   MV_VPP_EVENT_CALLBACK callbackFn,
                                   void *contextParam);

/********************************************************************************
 * FUNCTION: Unregister for CEC event notifications
 * INPUT: handle  - VPP object handle
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_CecUnregisterCallback (int handle);

int MV_VPPOBJ_GetCECEvtInfo(int handle, VPP_HDMI_CEC_EVT_INFO *pevtInfo);

#if defined(PE_3D_BD)
/**************************************************************************
 * FUNCTION: get current field ID in display of a plane
 * PARAMS:  planeID - plane ID
 *          *field - field ID, 0: first field, 1: second field
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 **************************************************************************/
int MV_VPPOBJ_GetViewID(int handle, int cpcbID, int *ViewID);

/***************************************************************
 * FUNCTION: set stereoscopic presentation type for the selected planes.
 * INPUT: PlaneMask - the mask to identify the planes which will be set.
 *         PresentTypes - present types for each selected plane. 1: BD mode; 0:BB mode
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetSSType(int handle, UINT32 PlaneMask, UINT32 PresentTypes);

/***************************************************************
 * FUNCTION: set plane default offset value for stereoscopic presentation.
 * INPUT: planeID - id of plane
 *         Offset - default offset value and direction. Bit 0-6: Offset value, up to 127 pixels.
 *             Bit 7: offset direction. 0: shift closer to the viewer; 1: shift far away from the viewer.
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetSSDefaultOffset(int handle, UINT32 planeID, UINT32 Offset);

/***************************************************************
 * FUNCTION: set plane default offset value for stereoscopic presentation.
 * INPUT: planeID - id of plane except main plane
 *         SeqIDRef - indentify offset id in video offset meta data.
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetSSOffsetSeqIDRef(int handle, UINT32 planeID, UINT32 SeqIDRef);

/***************************************************************
 * FUNCTION: set plane default offset value for stereoscopic presentation.
 * INPUT: planeID - id of plane except main plane
 *         SeqIDRef - indentify how to apply the offset of a plane
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_SetSSOffsetMode(int handle, UINT32 planeID, UINT32 OffsetMode);
#endif

/***************************************************************
 * FUNCTION: set hdmi CPCB0 inteertup callback function
 * INPUT: callbackFn - callback function pointer
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_RegisterHdmiCpcb0Callback (int handle, void *callbackFn);

/***************************************************************
 * FUNCTION: unregister hdmi CPCB0 inteertup callback function
 * INPUT:
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_UnRegisterHdmiCpcb0Callback (int handle);

/***************************************************************
 * FUNCTION: Dump all the HDMI TX Registers
 * INPUT: hdmiRegDump - pointer to register dump struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int MV_VPPOBJ_DumpHdmiRegisters(int handle, VPP_HDMITX_REG_DUMP *hdmiRegDump);

#ifdef PE_3D_FMT_CONVERT
/*******************************************************************
 * FUNCTION: set 3D video convert mode
 * INPUT:  mode - indicate 3D format convert mode (ENUM_SRC_3D_FMT)
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_Set3DConvertMode(int handle, int Mode);
/*******************************************************************
 * FUNCTION: Get 3D video convert mode
 * INPUT:  *pMode - indicate 3D format convert mode (ENUM_SRC_3D_FMT)
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_Get3DConvertMode(int handle, int* pMode);

/*******************************************************************
 * FUNCTION: set 3D video invert mode
 * INPUT:  mode - indicate 3D format invert mode
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int MV_VPPOBJ_Set3DInvertMode(int handle, BOOL Mode);
#endif
/********************************************************************************
 * FUNCTION: Get H/V SCL Coeff Mode
 ********************************************************************************/
int MV_VPPOBJ_GetSCLCoeffModes(int handle, INT UnitNum, INT* pHSCLCoeffMode, INT* pVSCLCoeffMode);
/*******************************************************************
 * FUNCTION: write register group
 *******************************************************************/
int MV_VPPOBJ_WriteRegGroup(int handle, VPP_REG_GROUP *pRegGroup);

/*******************************************************************
 * FUNCTION: read register group
 *******************************************************************/
int MV_VPPOBJ_ReadRegGroup(int handle, VPP_REG_GROUP *pRegGroup);

/*******************************************************************
 * FUNCTION: get the output frame rate
 *******************************************************************/
int MV_VPPOBJ_GetCPCBOutputFrameRate(int handle, int cpcbID, int *pts_per_cnt_4);

/********************************************************************************
 * FUNCTION: Register callback for resolution change notifications
 ********************************************************************************/
int MV_VPPOBJ_RegisterResCallback (int handle, VPP_RES_CALLBACK *pResCallback);

/********************************************************************************
 * FUNCTION: Unregister callback for resolution change notifications
 ********************************************************************************/
int MV_VPPOBJ_UnRegisterResCallback (int handle);

int MV_VPPOBJ_WatermarkGetInitPar(int handle, VPP_VMX_INITPAR **vmx_init_par);

int MV_VPPOBJ_SetVmxParams(int handle, int* param, int param_cnt);

/********************************************************************************
 * FUNCTION: Load SCL Customized Coeffs into cust_scl_coeffs table
 * INPUT:
 *              sclUnit - scalar unitNum
 *              coeffID - scalar coeffs mode
 *              pCoeffsTab - customized coeffs talbe's pointer
 *
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_PrepareSCLCustCoeffs(int handle, int sclUnit, int coeffID, UINT16* pCoeffsTab);

/********************************************************************************
 * FUNCTION: Load SCL Customized Coeffs
 * INPUT:
 *              coeffID - scalar coeffs mode
 *              hv      - indicating it is for horizontal(0) or vertical(1) scaler
 *              UnitNum - FRC SCL unit number the coeff is for
 *
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_LoadSCLCustCoeffs(int handle, int coeffID, int hv, INT UnitNum);

/********************************************************************************
 * FUNCTION: Reload SCL Default Coeffs
 * INPUT:
 *              coeffID - scalar coeffs mode
 *              hv      - indicating it is for horizontal(0) or vertical(1) scaler
 *              UnitNum - FRC SCL unit number the coeff is for
 *
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/

int MV_VPPOBJ_ReloadSCLDefCoeffs(int handle, int coeffID, int hv, INT UnitNum);

/********************************************************************************
 * FUNCTION: Register for Watermark event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : callbackFn - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_RegisterWatermarkCallback(int handle, unsigned int eventCode,
                        MV_VPP_EVENT_CALLBACK callbackFn, void *contextParam);

/********************************************************************************
 * FUNCTION: Unregister for Watermark event notifications
 * INPUT: None
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_UnRegisterWatermarkCallback(int handle, unsigned int eventCode);

/***********************************************************
 * FUNCTION: get video frame info
 * PARAMS: handle - the vpp handle
 *         pvinfo - return video info
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EUNCONFIG - not initialized
 *         MV_VPP_ENODEV - no device
 *         MV_VPP_ENOMEM - no memory
 *         MV_VPP_EBADCALL - not supported
 *         MV_VPP_EBADPARAM - wrong input params
 **********************************************************/
int MV_VPPOBJ_GetVideoInfo(int handle, UINT32 planeID, VPP_PLANE_VIDEO_INFO *pvinfo);

/***********************************************************
 * FUNCTION: capture video frame
 * PARAMS: handle - the vpp handle
 *         pCapFrmSetting - setting for video frame capture
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EUNCONFIG - not initialized
 *         MV_VPP_ENODEV - no device
 *         MV_VPP_ENOMEM - no memory
 *         MV_VPP_EBADCALL - not supported
 *         MV_VPP_EBADPARAM - wrong input params
 **********************************************************/
int MV_VPPOBJ_CaptureVideoFrame(int handle, VPP_CAPTURE_FRAME_SETTING *pCapFrmSetting);

/********************************************************************************
 * FUNCTION: Register for video frame capture call back
 * INPUT: callbackFn - call back function
 ********************************************************************************/
int MV_VPPOBJ_RegisterCaptureVideoCallback (int handle, VPP_CAPTURE_CALLBACK callbackFn);

/********************************************************************************
 * FUNCTION: frame capture task
 * INPUT: None
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_FrameCaptureTask(int handle);

/******************************************************************************
 * FUNCTION: Retrieve the current clock/SRAM gating signature of the AVIO/VPP
 * INPUT: VPP_CLKGATE_OPT, Various bit field option
 *   VPP_CLKGATE_OPT_PRINT      - Enable debug log
 *   VPP_CLKGATE_OPT_RECORD_RAW - Enable storing raw bit from the register into corresponding bit of IP block in signatrue
                Default behaiour: Enabling bit correspoing to IP block in signatrue only if clock gated/disabled
 * OUTPUT: pSignatureCount, The number of IP blocks which are clock gated/disabled
 *         pSignature, 128 bit signature (1 bit per IP block), ensure it points to array of 4 integers
 *
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_GetClkGateSignature(int handle, UINT32 *pSignatureCount,
        VPP_CLK_GATE_SIGNATURE *pSignature, VPP_CLKGATE_OPT options);
/******************************************************************************
 * FUNCTION: Retrieve the current clock/SRAM gating signature of the AVIO/VPP
 * INPUT: VPP_CLKGATE_OPT, Various bit field option
 *   VPP_CLKGATE_OPT_PRINT      - Enable debug log
 *   VPP_CLKGATE_OPT_RECORD_RAW - Enable storing raw bit from the register into corresponding bit of IP block in signatrue
                Default behaiour: Enabling bit correspoing to IP block in signatrue only if clock gated/disabled
 *         pSignatureValue, 128 bit signature value(1 bit per IP block), ensure it points to array of 4 integers
 *         pSignaturemASK, 128 bit signature mask(1 bit per IP block), ensure it points to array of 4 integers
 *
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int MV_VPPOBJ_SetClkGateSignature(int handle, VPP_CLK_GATE_SIGNATURE *pSignatureValue,
        VPP_CLK_GATE_SIGNATURE *pSignatureMask, VPP_CLKGATE_OPT options);


int MV_VPPOBJ_SetDacOversampling(int handle, int vout, int ovsmpl);
int MV_VPPOBJ_ReadHDMISinkCaps(int handle, VPP_HDMI_SINK_CAPS *pSinkCaps);
int MV_VPPOBJ_SetHDMIEnable(int handle, int vout, int Enable);

int MV_VPPOBJ_enClkGate(int handle, int enable);
int MV_VPPOBJ_RegWrite(int handle, unsigned int addr, unsigned int val);

int MV_VPPOBJ_SetRotationAngle(int handle, INT PlaneId, INT RotationAngle);
int MV_VPPOBJ_GetRotationAngle(int handle, INT PlaneId, INT *pRotationAngle);


int MV_VPPOBJ_SetHDMIEnable(int handle, int vout, int Enable);
int MV_VPPOBJ_ReadHDMISinkCaps(int handle, VPP_HDMI_SINK_CAPS *pSinkCaps);
int MV_VPPOBJ_SetDacOversampling(int handle, int vout, int ovsmpl);

/********************************************************************************
 * FUNCTION: Set HDR Enable
 * INPUT: enable - FALSE: To disable
                   TRUE : To Enable
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_HDREnable (INT handle, INT enable);

int MV_VPPOBJ_EnableEDR (INT handle, BOOL enable);

int MV_VPPOBJ_StartHdcp14(int handle, int index);

int MV_VPPOBJ_SetDVMode(int handle, INT Enable, INT Mode);

int MV_VPPOBJ_DV_EnableDolbyDump(int handle, int enable);
int MV_VPPOBJ_DV_InitFrameDump(int handle, INT Path, INT Enable, VPP_DV_FRAME_DUMP *pFrameDumpConfig);
int MV_VPPOBJ_DV_GetFrameDumpInfo(int handle, VPP_DV_FRAME_DUMP *pFrameDumpConfig);

int MV_VPPOBJ_SetDVDataDump(int handle, INT Path, INT Enable);

int MV_VPPOBJ_SetDVCmd(int handle, int module, int mode);

int MV_VPPOBJ_EnabletHDR(int handle, int PlaneId, int mode, int EnVitm);

int MV_VPPOBJ_EnableCrop(int handle, int planeID,
                         int rightcropy, int bottomcropy,
                         int rightcropc, int bottomcropc);

int MV_VPPOBJ_ClearDhubCommand(int handle, INT Enable);

int MV_VPPOBJ_SetHDMIClkCtrl (int handle, int cpcbDiv, int vopDiv);

int MV_VPPOBJ_EnDcm(int handle, int planeId, int Enable, int inMode, int OutMode);

int MV_VPPOBJ_GetResolutionDescription(int handle, int ResId,VPP_RESOLUTION_DESCRIPTION *pResDesc);

int MV_VPPOBJ_SetPriorityMode(int handle, int priority);
int MV_VPPOBJ_EnableLowLatency(int handle, int enable);
int MV_VPPOBJ_LoadVSDB(int handle, unsigned char *vsdb_buf);
int MV_VPPOBJ_SetDovi2HdrMap(int handle, int en);
//int MV_VPPOBJ_SetHdrFeature(int handle, VPP_HDR_FEATURE feature);
int MV_VPPOBJ_SetGfxInfo(int handle, int type, int gmin, int gmax);
int MV_VPPOBJ_SetHdmiOutputMode(int handle, int mode);

/********************************************************************************
  * FUNCTION: Set Hdmi Tx Phy DAMP settings
  * INPUT:  DAMP setting value array
  * RETURN: MV_VPP_OK - SUCCEED
  *         MV_EBADPARAM - invalid parameters
  *         MV_EUNCONFIG - VPP not configured
  *         MV_VPP_ENODEV - no device
  ********************************************************************************/
int MV_VPPOBJ_SetHdmiPhy(int handle,  unsigned int *PhyBuf, unsigned int PhyBufSize);

/********************************************************************************
* FUNCTION: Set the customer specific HDCP Settings
* INPUT:  TRUE to enable /FALSE to disable the feature
* RETURN: MV_VPP_OK - SUCCEED
*         MV_EBADPARAM - invalid parameters
*         MV_EUNCONFIG - VPP not configured
*         MV_VPP_ENODEV - no device
********************************************************************************/
int MV_VPPOBJ_SetHdmiCustHdcpSetting(int handle, int bEnable);

int MV_VPPOBJ_GetHDCPAksv(int handle, VPP_HDMI_HDCP_AKSV *pHdcpAksv);

int MV_VPPOBJ_ConfigInverseScan(int handle, int planeID,  unsigned int uiMode);

//void VPP_ISR_Handler(UINT32 msg_id, UINT32 msg_para);

int MV_VPPOBJ_SetThdrVitmParams(int handle, int iCmdId, int iValue);

int MV_VPPOBJ_SetDcmPictCtrl(int handle, int PlaneId, int Hue, int Saturation, int Lightness);

int MV_VPPOBJ_EarlyDisplayFrame(int handle, int planeID, void *frame);

int MV_VPPOBJ_SetHDMISignalSwap(int handle, int HdmiSignalSwap);

int MV_VPPOBJ_GetIsrTime(void);

/********************************************************************************
* FUNCTION: Get Hdmi Tx Phy power status
* OUTPUT:  - pPowerState (Phy power state)
* RETURN: MV_VPP_OK - SUCCEED
*         MV_EBADPARAM - invalid parameters
*         MV_EUNCONFIG - VPP not configured
*         MV_VPP_ENODEV - no device
********************************************************************************/
int MV_VPPOBJ_GetHdmiPowerStatus(int handle, int * pPowerState);

/********************************************************************************
* FUNCTION: Get VPP suspended status
* OUTPUT:  - pVppState (VPP object suspended status)
* RETURN: MV_VPP_OK - SUCCEED
*         MV_EBADPARAM - invalid parameters
*         MV_EUNCONFIG - VPP not configured
*         MV_VPP_ENODEV - no device
********************************************************************************/
int MV_VPPOBJ_GetVppStatus(int handle, int * pVppState);

/********************************************************************************
 * FUNCTION: Set Hdmi Config value
 * INPUT:   Hdmi config values and its size
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_SetHdmiConfig(int handle, unsigned int *PhyBuf, unsigned int PhyBufSize);

/********************************************************************************
* FUNCTION: Get BCHS programmed value
* INPUT: Plain ID [main/gfx], mode [B-Brightness,C-Contrast,S-Saturation,H-Hue]
* RETURN: MV_VPP_OK - SUCCEED
*         MV_EBADPARAM - invalid parameters
********************************************************************************/
int MV_VPPOBJ_GetGcsc(INT handle, UINT32 PlaneId, UINT32 mode, UINT32 *param_value);

int MV_VPPOBJ_ConfigAoiWin(int handle, int planeID, int Enable, VPP_WIN *win);

int MV_VPPOBJ_SetDolbyElThroughPip(int handle, int Enable);

int MV_VPPOBJ_SetNgptvParams(int handle, int iPlaneId, UINT32 *params, UINT32 param_cnt);

int MV_VPPOBJ_SetOfflineParams(int handle,int iPlaneId,int bpp,int srcfmt);

#ifdef __cplusplus
}
#endif

#endif

