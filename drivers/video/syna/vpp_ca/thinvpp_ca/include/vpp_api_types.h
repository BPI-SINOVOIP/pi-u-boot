/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#ifndef __VPP_API_TYPES_H__
#define __VPP_API_TYPES_H__

//#include "com_type.h"
#if !defined(__LINUX_KERNEL__)
//#include <stdint.h>
#endif
/*-----------------------------------------------------------------------------
 * Macros and Constants
 *-----------------------------------------------------------------------------
 */

/*-----------------------------------------------------------------------------
* Watermark Notification Events
*-----------------------------------------------------------------------------
*/
#define MV_VPP_VMX_CALLBACK_ISR                 1
#define MV_VPP_VMX_CALLBACK_MAIN_INFO           2
#define MV_VPP_VMX_CALLBACK_PIP_INFO            3
#define MV_VPP_VMX_CALLBACK_4K_BYPASS_INFO      4

/*-----------------------------------------------------------------------------
 * HDMI Notification Events
 *-----------------------------------------------------------------------------
 */
#define MV_VPP_EVENT_HDMI_SINK_CONNECTED        0
#define MV_VPP_EVENT_HDMI_SINK_DISCONNECTED     1
#define MV_VPP_EVENT_HDMI_VIDEO_CFG_ERR         2
#define MV_VPP_EVENT_HDMI_AUDIO_CFG_ERR         3
#define MV_VPP_EVENT_HDMI_HDCP_ERR              4
#define MV_VPP_EVENT_HDMI_RESUME                5
#define MV_VPP_EVENT_HDMI_HDCP_AUTH_DONE        6
#define MV_VPP_EVENT_HDMI_EDID_READ_ERR         7
#define MV_VPP_EVENT_HDMI_HDR_PLAYBACK_STARTED  8
#define MV_VPP_EVENT_HDMI_HDR_PLAYBACK_STOPPED  9
#define MV_VPP_EVENT_HDMI_HDCP_VER_1_x          10
#define MV_VPP_EVENT_HDMI_HDCP_VER_2_x          11
#define MV_VPP_EVENT_HDMI_DV_PLAYBACK_START     12
#define MV_VPP_EVENT_HDMI_DV_PLAYBACK_STOP      13
#define MV_VPP_EVENT_HDMI_DV_ON                 14
#define MV_VPP_EVENT_HDMI_DV_OFF                15

/*-----------------------------------------------------------------------------
 * HDCP Notification Events
 *-----------------------------------------------------------------------------
 */
#define MV_VPP_EVENT_HDCP_DEV_NOT_RDY           0x10000000
#define MV_VPP_EVENT_HDCP_TXRX_ERR              0x20000000
#define MV_VPP_EVENT_HDCP_AUTH_ERR              0x30000000
#define MV_VPP_EVENT_HDCP_AUTH_SUCCESS          0x40000000
#define MV_VPP_EVENT_HDCP_DS_DEV_REPEATER       0x50000000
#define MV_VPP_EVENT_HDCP_DS_REAUTH_REQ         0x60000000
#define MV_VPP_EVENT_MASK_FLAG                  0xF0000000

#define MV_VPP_EVENT_HDCP_RI_ERR                1
#define MV_VPP_EVENT_HDCP_R0_ERR                2
#define MV_VPP_EVENT_HDCP_NO_KEYS_ERR           3
#define MV_VPP_EVENT_HDCP_RD_BKSV_ERR           4
#define MV_VPP_EVENT_HDCP_REP_VI_ERR            5
#define MV_VPP_EVENT_HDCP_REP_TOPOLOGY_ERR      6
#define MV_VPP_EVENT_HDCP_REP_DELAY_ERR         7
#define MV_VPP_EVENT_HDCP_KEY_REVOKED_ERR       8
#define MV_VPP_EVENT_HDCP_SRM_INTG_ERR          9
#define MV_VPP_EVENT_HDCP_AUTH_ERR_MAX_FLAG     0xF

/*-----------------------------------------------------------------------------
 * HDCP SRM Device signature size
 * Device key of 16 bytes and HMAC-SHA 256 - signature length is 32 bytes
 *-----------------------------------------------------------------------------
 */
#define MV_VPP_DEV_SIGNATURE_SIZE               32

/*-----------------------------------------------------------------------------
 * HDCP SRM ID
 *-----------------------------------------------------------------------------
 */
#define MV_VPP_HDCP_SRM_ID_1X                   8
#define MV_VPP_HDCP_SRM_ID_2X                   9

/*-----------------------------------------------------------------------------
 * HDMI CEC Notification Events
 *-----------------------------------------------------------------------------
 */
#define MV_VPP_EVENT_CEC_LOG_ADDR_STS           0
#define MV_VPP_EVENT_CEC_MSG_TX_STS             1
#define MV_VPP_EVENT_CEC_MSG_RX_STS             2

/*-----------------------------------------------------------------------------
 * HDMI CEC Feature Support Configuration
 *-----------------------------------------------------------------------------
 */
#define     VPP_CEC_FEATURE_ONE_TOUCH_PLAY              0x00000001
#define     VPP_CEC_FEATURE_ROUTING_CONTROL             0x00000002
#define     VPP_CEC_FEATURE_SYSTEM_STANDBY              0x00000004
#define     VPP_CEC_FEATURE_ONE_TOUCH_RECORD            0x00000008
#define     VPP_CEC_FEATURE_SYSTEM_INFO                 0x00000010
#define     VPP_CEC_FEATURE_DECK_CONTROL                0x00000020
#define     VPP_CEC_FEATURE_TUNER_CONTROL               0x00000040
#define     VPP_CEC_FEATURE_VENDOR_SPEC_CMDS            0x00000080
#define     VPP_CEC_FEATURE_OSD_STATUS_DISPLAY          0x00000100
#define     VPP_CEC_FEATURE_DEVICE_OSD_NAME_TX          0x00000200
#define     VPP_CEC_FEATURE_DEVICE_MENU_CONTROL         0x00000400
#define     VPP_CEC_FEATURE_REMOTE_CONTROL_PASS_THRU    0x00000800
#define     VPP_CEC_FEATURE_DEVICE_POWER_STATUS         0x00001000

#define     VPP_CEC_FEATURE_SYSTEM_AUDIO_CONTROL        0x00002000
#define     VPP_CEC_FEATURE_AUDIO_RATE_CONTROL          0x00004000
#define     VPP_CEC_FEATURE_TIMER_PROGRAMMING           0x00008000
#define     VPP_CEC_FEATURE_AUDIO_RETURN_CHANNEL        0x00010000

// Maximum message length
#define     VPP_CEC_MAX_MSG_LEN                         16

// Invalid physical address F.F.F.F
#define     VPP_CEC_INVALID_PHY_ADDR                    0xFFFF

/*-----------------------------------------------------------------------------
 * HDMI CEC Message Opcodes
 *-----------------------------------------------------------------------------
 */
#define VPP_CEC_MSG_OPCODE_UNDEFINED                    0xFFFF

// General Protocol messages
#define VPP_CEC_MSG_OPCODE_FEATURE_ABORT                0x00
#define VPP_CEC_MSG_OPCODE_ABORT                        0xFF

// One Touch Play
#define VPP_CEC_MSG_OPCODE_ACTIVE_SOURCE                0x82
#define VPP_CEC_MSG_OPCODE_IMAGE_VIEW_ON                0x04
#define VPP_CEC_MSG_OPCODE_TEXT_VIEW_ON                 0x0D

// Routing control
#define VPP_CEC_MSG_OPCODE_REQUEST_ACTIVE_SOURCE        0x85
#define VPP_CEC_MSG_OPCODE_SET_STREAM_PATH              0x86
#define VPP_CEC_MSG_OPCODE_ROUTING_CHANGE               0x80
#define VPP_CEC_MSG_OPCODE_ROUTING_INFO                 0x81
#define VPP_CEC_MSG_OPCODE_INACTIVE_SOURCE              0x9D

// Standby
#define VPP_CEC_MSG_OPCODE_STANDBY                      0x36

// One touch record
#define VPP_CEC_MSG_OPCODE_RECORD_OFF                   0x0B
#define VPP_CEC_MSG_OPCODE_RECORD_ON                    0x09
#define VPP_CEC_MSG_OPCODE_RECORD_STATUS                0x0A
#define VPP_CEC_MSG_OPCODE_RECORD_TV_SCREEN             0x0F

// System information
#define VPP_CEC_MSG_OPCODE_GET_MENU_LANG                0x91
#define VPP_CEC_MSG_OPCODE_GIVE_PHY_ADDR                0x83
#define VPP_CEC_MSG_OPCODE_REPORT_PHY_ADDR              0x84
#define VPP_CEC_MSG_OPCODE_SET_MENU_LANG                0x32
#define VPP_CEC_MSG_OPCODE_CEC_VERSION                  0x9E
#define VPP_CEC_MSG_OPCODE_GET_CEC_VERSION              0x9F

// Deck control
#define VPP_CEC_MSG_OPCODE_DECK_STATUS                  0x1B
#define VPP_CEC_MSG_OPCODE_GIVE_DECK_STATUS             0x1A
#define VPP_CEC_MSG_OPCODE_DECK_CONTROL                 0x42
#define VPP_CEC_MSG_OPCODE_PLAY                         0x41

// Tuner control
#define VPP_CEC_MSG_OPCODE_GIVE_TUNER_DEVICE_STATUS     0x08
#define VPP_CEC_MSG_OPCODE_SELECT_ANALOGUE_SERVICE      0x92
#define VPP_CEC_MSG_OPCODE_SELECT_DIGITAL_SERVICE       0x93
#define VPP_CEC_MSG_OPCODE_TUNER_STEP_DECREMENT         0x06
#define VPP_CEC_MSG_OPCODE_TUNER_STEP_INCREMENT         0x05
#define VPP_CEC_MSG_OPCODE_TUNER_DEVICE_STATUS          0x07

// Vendor specific commands
#define VPP_CEC_MSG_OPCODE_DEVICE_VENDOR_ID             0x87
#define VPP_CEC_MSG_OPCODE_GIVE_DEVICE_VENDOR_ID        0x8C
#define VPP_CEC_MSG_OPCODE_VENDOR_COMMAND               0x89
#define VPP_CEC_MSG_OPCODE_VENDOR_COMMAND_WITH_ID       0xA0
#define VPP_CEC_MSG_OPCODE_VENDOR_REMOTE_BTN_DOWN       0x8A
#define VPP_CEC_MSG_OPCODE_VENDOR_REMOTE_BTN_UP         0x8B

// OSD status display
#define VPP_CEC_MSG_OPCODE_SET_OSD_STRING               0x64

// Device OSD name transfer
#define VPP_CEC_MSG_OPCODE_GIVE_OSD_NAME                0x46
#define VPP_CEC_MSG_OPCODE_SET_OSD_NAME                 0x47

// Device menu control, Remote control pass-through
#define VPP_CEC_MSG_OPCODE_USER_CONTROL_PRESSED         0x44
#define VPP_CEC_MSG_OPCODE_USER_CONTROL_RELEASED        0x45
#define VPP_CEC_MSG_OPCODE_MENU_REQUEST                 0x8D
#define VPP_CEC_MSG_OPCODE_MENU_STATUS                  0x8E

// Device power status
#define VPP_CEC_MSG_OPCODE_GIVE_DEVICE_POWER_STATUS     0x8F
#define VPP_CEC_MSG_OPCODE_REPORT_POWER_STATUS          0x90

// System Audio Control
#define VPP_CEC_MSG_OPCODE_GIVE_AUDIO_STATUS            0x71
#define VPP_CEC_MSG_OPCODE_GIVE_SYS_AUDIO_MODE_STATUS   0x7D
#define VPP_CEC_MSG_OPCODE_REPORT_AUDIO_STATUS          0x7A
#define VPP_CEC_MSG_OPCODE_SET_SYS_AUDIO_MODE           0x72
#define VPP_CEC_MSG_OPCODE_SYS_AUDIO_MODE_REQUEST       0x70
#define VPP_CEC_MSG_OPCODE_SYS_AUDIO_MODE_STATUS        0x7E
#define VPP_CEC_MSG_OPCODE_REQUEST_SHORT_AUDIO_DESC     0xA4
#define VPP_CEC_MSG_OPCODE_REPORT_SHORT_AUDIO_DESC      0xA3

// Audio Rate Control
#define VPP_CEC_MSG_OPCODE_SET_AUDIO_RATE               0x9A

// Timer Programming
#define VPP_CEC_MSG_OPCODE_CLR_ANALOGUE_TIMER           0x33
#define VPP_CEC_MSG_OPCODE_CLR_DIGITAL_TIMER            0x99
#define VPP_CEC_MSG_OPCODE_CLR_EXTERNAL_TIMER           0xA1
#define VPP_CEC_MSG_OPCODE_SET_ANALOGUE_TIMER           0x34
#define VPP_CEC_MSG_OPCODE_SET_DIGITAL_TIMER            0x97
#define VPP_CEC_MSG_OPCODE_SET_EXTERNAL_TIMER           0xA2
#define VPP_CEC_MSG_OPCODE_SET_TIMER_PGM_TITLE          0x67
#define VPP_CEC_MSG_OPCODE_TIMER_CLEARED_STATUS         0x43
#define VPP_CEC_MSG_OPCODE_TIMER_STATUS                 0x35

// Audio Return Channel Feature
#define VPP_CEC_MSG_OPCODE_INITIATE_ARC                 0xC0
#define VPP_CEC_MSG_OPCODE_REPORT_ARC_INITIATED         0xC1
#define VPP_CEC_MSG_OPCODE_REPORT_ARC_TERMINATED        0xC2
#define VPP_CEC_MSG_OPCODE_REQUEST_ARC_INITIATION       0xC3
#define VPP_CEC_MSG_OPCODE_REQUEST_ARC_TERMINATION      0xC4
#define VPP_CEC_MSG_OPCODE_TERMINATE_ARC                0xC5
/*-----------------------------------------------------------------------------
 * Typedefs
 *-----------------------------------------------------------------------------
 */

/*-----------------------------------------------------------------------------
 * HDMI Aux packet enums, structures
 *-----------------------------------------------------------------------------
 */
typedef enum tagVPP_HDMI_PKT_ID
{
    VPP_HDMI_PKT_ID_ACP                 = 0x04,
    VPP_HDMI_PKT_ID_ISRC1               = 0x05,
    VPP_HDMI_PKT_ID_ISRC2               = 0x06,
    VPP_HDMI_PKT_ID_HDR_INFOFRAME       = 0x07,
    VPP_HDMI_PKT_ID_GAMUT_METADATA      = 0x0A,
    VPP_HDMI_PKT_ID_VENDOR_INFOFRAME    = 0x81,
    VPP_HDMI_PKT_ID_AVI_INFOFRAME       = 0x82,
    VPP_HDMI_PKT_ID_SPD_INFOFRAME       = 0x83,
    VPP_HDMI_PKT_ID_AUDIO_INFOFRAME     = 0x84,
    VPP_HDMI_PKT_ID_MPEG_SRC_INFOFRAME  = 0x85,
}VPP_HDMI_PKT_ID;

typedef enum tagDOLBYVISION_VSVDB_ERSION
{
    DOLBYVISION_VSVDB_VER0   = 0x00,
    DOLBYVISION_VSVDB_VER1_0 = 0x01,
    DOLBYVISION_VSVDB_VER1_1 = 0x02,
    DOLBYVISION_VSVDB_VER2   = 0x03,
}DOLBYVISION_VSVDB_VER;

/* Added for suspend and resume */
#define TOTAL_HDMI_PKT_ID 9
#define TOTAL_VPP_CMU_COLOR 7
/* End -- Added for suspend and resume */

// Gamut Meta data packet
typedef enum tagVPP_HDMI_GDB_PROFILE
{
    VPP_HDMI_GDB_PROFILE_P0 = 0x00,
    VPP_HDMI_GDB_PROFILE_P1,
    VPP_HDMI_GDB_PROFILE_P2,
    VPP_HDMI_GDB_PROFILE_P3
}VPP_HDMI_GDB_PROFILE;

typedef enum tagVPP_HDMI_PKT_GMD_SEQ_INFO
{
    VPP_HDMI_PKT_GMD_INTER_IN_SEQ   = 0x00,
    VPP_HDMI_PKT_GMD_FIRST_IN_SEQ,
    VPP_HDMI_PKT_GMD_LAST_IN_SEQ,
    VPP_HDMI_PKT_GMD_ONLY_IN_SEQ,
}VPP_HDMI_PKT_GMD_SEQ_INFO;

typedef struct tagVPP_HDMI_PKT_GMD
{
    BOOL                        nextField;
    BOOL                        noCurGBD;
    INT                         gdbProfile; // VPP_HDMI_GDB_PROFILE
    INT                         seqInfo; // VPP_HDMI_PKT_GMD_SEQ_INFO
    UINT8                       affectedGamutSeqNum;
    UINT8                       curGamutSeqNum;
    UINT8                       gbdData[28];
}VPP_HDMI_PKT_GMD;

// ACP packet
typedef enum tagVPP_HDMI_PKT_ACP_TYPE
{
    VPP_HDMI_PKT_ACP_GENERIC_AUDIO = 0x00,
    VPP_HDMI_PKT_ACP_IEC60958,
    VPP_HDMI_PKT_ACP_DVD_AUDIO,
    VPP_HDMI_PKT_ACP_SACD,
    VPP_HDMI_PKT_ACP_MAX,
}VPP_HDMI_PKT_ACP_TYPE;

typedef struct tagVPP_HDMI_PKT_ACP
{
    INT                     type; // VPP_HDMI_PKT_ACP_TYPE
    // Valid only for DVD Audio and SACD
    UINT8                   dataLen;
    UINT8                   dataBuf[16];
}VPP_HDMI_PKT_ACP;

// ISRC Packet
typedef enum tagVPP_HDMI_PKT_ISRC_STS
{
    VPP_HDMI_PKT_ISRC_STS_START_POS = 0x01,
    VPP_HDMI_PKT_ISRC_STS_INTER_POS = 0x02,
    VPP_HDMI_PKT_ISRC_STS_END_POS   = 0x04,
}VPP_HDMI_PKT_ISRC_STS;

typedef struct tagVPP_HDMI_PKT_ISRC1
{
    BOOL                    cont;
    INT                     sts; // VPP_HDMI_PKT_ISRC_STS
    BOOL                    valid;
    UINT8                   upc_ean_fld[16];
}VPP_HDMI_PKT_ISRC1;

typedef struct tagVPP_HDMI_PKT_ISRC2
{
    UINT8                   upc_ean_fld[16];
}VPP_HDMI_PKT_ISRC2;

// SPD InfoFrame
typedef enum tagVPP_HDMI_CEA_SRC_DEV_TYPE
{
    VPP_HDMI_CEA_SRC_DEV_UNKNOWN = 0x00,
    VPP_HDMI_CEA_SRC_DEV_DIG_STB,
    VPP_HDMI_CEA_SRC_DEV_DVD_PLAYER,
    VPP_HDMI_CEA_SRC_DEV_DVHS,
    VPP_HDMI_CEA_SRC_DEV_HDD_REC,
    VPP_HDMI_CEA_SRC_DEV_DVC,
    VPP_HDMI_CEA_SRC_DEV_DSC,
    VPP_HDMI_CEA_SRC_DEV_VIDEO_CD,
    VPP_HDMI_CEA_SRC_DEV_GAME,
    VPP_HDMI_CEA_SRC_DEV_PC,
    VPP_HDMI_CEA_SRC_DEV_BD_PLAYER,
    VPP_HDMI_CEA_SRC_DEV_SACD,
    VPP_HDMI_CEA_SRC_DEV_MAX
}VPP_HDMI_CEA_SRC_DEV_TYPE;

typedef struct tagVPP_HDMI_PKT_SPD_INFOFRM {
    UINT8                       vendorName[8];
    UINT8                       prodDescChar[16];
    INT   srcDev; // VPP_HDMI_CEA_SRC_DEV_TYPE
}VPP_HDMI_PKT_SPD_INFOFRM;

// MPEG source InfoFrame
typedef enum tagVPP_HDMI_CEA_MPG_FRM_TYPE
{
    VPP_HDMI_MPEG_FRM_UNKNOWN = 0x00,
    VPP_HDMI_MPEG_FRM_I_PICTURE,
    VPP_HDMI_MPEG_FRM_B_PICTURE,
    VPP_HDMI_MPEG_FRM_P_PICTURE,
    VPP_HDMI_MPEG_FRM_MAX,
}VPP_HDMI_CEA_MPG_FRM_TYPE;

typedef struct tagVPP_HDMI_PKT_MPEG_SRC_INFOFRM
{
    UINT32                      bitRate;
    INT                         mpegFrameType; // VPP_HDMI_CEA_MPG_FRM_TYPE
    BOOL                        repeatedField;
}VPP_HDMI_PKT_MPEG_SRC_INFOFRM;

// Vendor Specific InfoFrame
typedef enum tagVPP_HDMI_VIDEO_FORMAT
{
    VPP_HDMI_VIDEO_FMT_NO_INFO = 0x00,
    VPP_HDMI_VIDEO_FMT_EXTENDED_RES_FMT,
    VPP_HDMI_VIDEO_FMT_3D_FMT,
    VPP_HDMI_VIDEO_FMT_RESERVED,
}VPP_HDMI_VIDEO_FORMAT;

typedef enum tagVPP_HDMI_VIC
{
    VPP_HDMI_VIC_RESERVED1 = 0x00,
    VPP_HDMI_VIC_4K_2K_30,
    VPP_HDMI_VIC_4K_2K_25,
    VPP_HDMI_VIC_4K_2K_24,
    VPP_HDMI_VIC_4K_2K_24_SMPTE,
    VPP_HDMI_VIC_RESERVED2,
}VPP_HDMI_VIC;

typedef enum tagVPP_HDMI_3D_STRUCTURE
{
    VPP_HDMI_3D_STRUCT_FRAME_PACKING=0x00,
    VPP_HDMI_3D_STRUCT_FIELD_ALTERNATIVE=0x01,
    VPP_HDMI_3D_STRUCT_LINE_ALTERNATIVE=0x02,
    VPP_HDMI_3D_STRUCT_SIDE_BY_SIDE_FULL=0x03,
    VPP_HDMI_3D_STRUCT_L_DEPTH=0x04,
    VPP_HDMI_3D_STRUCT_L_DEPTH_GFX_GDEPTH=0x05,
    VPP_HDMI_3D_STRUCT_TOP_AND_BOTTOM=0x06,
    VPP_HDMI_3D_STRUCT_RESERVED2=0x07,
    VPP_HDMI_3D_STRUCT_SIDE_BY_SIDE_HALF=0x08,
    VPP_HDMI_3D_STRUCT_RESERVED3=0x09,
}VPP_HDMI_3D_STRUCTURE;

typedef enum tagVPP_HDMI_3D_METADATA_TYPE
{
    VPP_HDMI_3D_METADATA_TYPE_PARALLAX_INFO = 0x00,
    VPP_HDMI_3D_METADATA_TYPE_RESERVED = 0x01,
}VPP_HDMI_3D_METADATA_TYPE;

typedef enum tagVPP_HDMI_3D_EXT_DATA
{
    VPP_HDMI_HORZ_SUBSAMP_3D_EXT_DATA_OL_OR=0x00,
    VPP_HDMI_HORZ_SUBSAMP_3D_EXT_DATA_OL_ER,
    VPP_HDMI_HORZ_SUBSAMP_3D_EXT_DATA_EL_OR,
    VPP_HDMI_HORZ_SUBSAMP_3D_EXT_DATA_EL_ER,

    VPP_HDMI_QUIN_3D_EXT_DATA_OL_OR=0x04,
    VPP_HDMI_QUIN_3D_EXT_DATA_OL_ER,
    VPP_HDMI_QUIN_3D_EXT_DATA_EL_OR,
    VPP_HDMI_QUIN_3D_EXT_DATA_EL_ER,

    VPP_HDMI_3D_EXT_DATA_RESERVED = 0x08,

}VPP_HDMI_3D_EXT_DATA;

typedef struct tagVPP_HDMI_PKT_VNDRSPEC_INFOFRM
{
    UINT32    ieeeRegID;
    UINT8     PacketLength;
    UINT8     HdmiVideoFmt;//defines the structure of the extended video formats - //VPP_HDMI_EXTENDED_VIDEO_FORMAT
    UINT8     HDMI_VIC;//Video Identification Code - VPP_HDMI_VIC
    UINT8     Hdmi_3D_Structure;//Transmission format of 3D Video Data - VPP_HDMI_3D_STRUCTURE
    BOOL      Hdmi_3D_Meta_Present;//additional bytes of 3D Metadata are present in the Infoframe
    UINT8     Hdmi_3D_Ext_Data;//
    UINT8     Hdmi_3D_Metadata_Type;//Type of info of metadata whcich is used in the correct rendering of the stereoscopic video
    UINT8     Hdmi_3D_Metadata_Length;
    UINT8     Hdmi_3D_Metadata[16];//!WARN : Need to check
}VPP_HDMI_PKT_VNDRSPEC_INFOFRM;

typedef struct tagVPP_HDMI_PKT_HDRInfoFrame {

    UINT8     EOTF;
    UINT8     Static_metadata_Des_ID;
    UINT16    display_primaries_x[3];
    UINT16    display_primaries_y[3];
    UINT16    white_point_x;
    UINT16    white_point_y;
    UINT16    max_display_mastering_lumi;
    UINT16    min_display_mastering_lumi;
    UINT16    Maximum_Content_Light_Lvl;
    UINT16    Maximum_Frame_Average_Light_Lvl;

}VPP_HDMI_PKT_HDR_INFO_FRAME, *PVPP_HDMI_PKT_HDR_INFO_FRAME;

// Aux packet structure
typedef union tagVPP_HDMI_PKT
{
    VPP_HDMI_PKT_ACP                acpPkt;
    VPP_HDMI_PKT_GMD                gmdPkt;
    VPP_HDMI_PKT_ISRC1              isrc1Pkt;
    VPP_HDMI_PKT_ISRC2              isrc2Pkt;
    VPP_HDMI_PKT_SPD_INFOFRM        spdInfoFrm;
    VPP_HDMI_PKT_MPEG_SRC_INFOFRM   mpegSrcInfoFrm;
    VPP_HDMI_PKT_VNDRSPEC_INFOFRM   vndrSpecInfoFrm;
    VPP_HDMI_PKT_HDR_INFO_FRAME     hdrInfoFrm;
}VPP_HDMI_PKT;

/*-----------------------------------------------------------------------------
 * HDMI Audio configuration structure
 *-----------------------------------------------------------------------------
 */
typedef struct VPP_HDMI_AUDIO_CFG_T {
    int     numChannels;
    int     portNum;
    int     sampFreq;
    int     sampSize;
    int     mClkFactor;
    int     audioFmt; /* VPP_HDMI_AUDIO_FMT_T */
    int     hbrAudio; /* True/False for controlling HBR audio */
}VPP_HDMI_AUDIO_CFG;

typedef enum tagVPP_HDMI_AUDIO_VUC_CFG_TYPE
{
    VPP_HDMI_VUC_CFG_SET_DEFAULT = 0x00,
    VPP_HDMI_VUC_CFG_UPDATE_WITH_GIVEN_CFG,
    VPP_HDMI_VUC_CFG_NO_UPDATE_TO_CURRENT_CFG,
    VPP_HDMI_VUC_CFG_MAX
}VPP_HDMI_AUDIO_VUC_CFG_TYPE;

typedef struct VPP_HDMI_AUDIO_VUC_CFG_T {
    // VPP_HDMI_AUDIO_VUC_CFG_TYPE
    unsigned char vBitCfg;
    unsigned char vBit;

    // VPP_HDMI_AUDIO_VUC_CFG_TYPE
    unsigned char uBitsCfg;
    unsigned char uBits[14];

    // VPP_HDMI_AUDIO_VUC_CFG_TYPE
    unsigned char cBitsCfg;
    unsigned char cBits[5];
}VPP_HDMI_AUDIO_VUC_CFG;

/*-----------------------------------------------------------------------------
 * HDMI Sink Capabilities structures
 *-----------------------------------------------------------------------------
 */
typedef enum VPP_HDMI_AUDIO_FMT_T
{
    VPP_HDMI_AUDIO_FMT_UNDEF = 0x00,
    VPP_HDMI_AUDIO_FMT_PCM   = 0x01,
    VPP_HDMI_AUDIO_FMT_AC3,
    VPP_HDMI_AUDIO_FMT_MPEG1,
    VPP_HDMI_AUDIO_FMT_MP3,
    VPP_HDMI_AUDIO_FMT_MPEG2,
    VPP_HDMI_AUDIO_FMT_AAC,
    VPP_HDMI_AUDIO_FMT_DTS,
    VPP_HDMI_AUDIO_FMT_ATRAC,
    VPP_HDMI_AUDIO_FMT_ONE_BIT_AUDIO,
    VPP_HDMI_AUDIO_FMT_DOLBY_DIGITAL_PLUS,
    VPP_HDMI_AUDIO_FMT_DTS_HD,
    VPP_HDMI_AUDIO_FMT_MAT,
    VPP_HDMI_AUDIO_FMT_DST,
    VPP_HDMI_AUDIO_FMT_WMA_PRO,
}VPP_HDMI_AUDIO_FMT;

typedef struct VPP_HDMI_RES_INFO_T {
    int     hActive;
    int     vActive;
    // Refresh rate in Hz, -1 if refresh rate is
    // undefined in the descriptor
    int     refreshRate;
    // 0 = progressive, 1 = interlaced,  2 = undefined
    int     interlaced;
    // 1 = 4:3, 2 = 16:9
    int     AspRatio;
} VPP_HDMI_RES_INFO;

typedef struct VPP_HDMI_AUDIO_FREQ_SPRT_T {
    unsigned char   Res         : 1;
    unsigned char   Fs32KHz     : 1;
    unsigned char   Fs44_1KHz   : 1;
    unsigned char   Fs48KHz     : 1;
    unsigned char   Fs88_2KHz   : 1;
    unsigned char   Fs96KHz     : 1;
    unsigned char   Fs176_4KHz  : 1;
    unsigned char   Fs192KHz    : 1;
} VPP_HDMI_AUDIO_FREQ_SPRT;

typedef struct VPP_HDMI_AUDIO_WDLEN_SPRT_T {
    unsigned char   Res1    : 1;
    unsigned char   WdLen16 : 1;
    unsigned char   WdLen20 : 1;
    unsigned char   WdLen24 : 1;
    unsigned char   Res2    : 4;
} VPP_HDMI_AUDIO_WDLEN_SPRT;

typedef struct VPP_HDMI_AUDIO_INFO_T {
    INT                         audioFmt; // VPP_HDMI_AUDIO_FMT
    VPP_HDMI_AUDIO_FREQ_SPRT    freqSprt;
    // Field is valid only for compressed audio formats
    UINT32                      maxBitRate; // in KHz
    // Field is valid only for LPCM
    VPP_HDMI_AUDIO_WDLEN_SPRT   wdLenSprt;
    unsigned char               maxNumChnls;
} VPP_HDMI_AUDIO_INFO;

typedef struct VPP_HDMI_SPKR_ALLOC_T {
    unsigned char   FlFr  : 1; // FrontLeft/Front Rear
    unsigned char   Lfe   : 1; // Low Frequency Effect
    unsigned char   Fc    : 1; // Front Center
    unsigned char   RlRr  : 1; // Rear Left/Rear Right
    unsigned char   Rc    : 1; // Rear Center
    unsigned char   FlcFrc: 1; // Front Left Center/Front Right Center
    unsigned char   RlcRrc: 1; // Rear Left Center /Rear Right Center
    unsigned char   Res   : 1;
} VPP_HDMI_SPKR_ALLOC;

// Calorimetry support
typedef struct tagVPP_HDMI_CALORIMETRY_INFO
{
    unsigned short  xvYCC601    : 1;
    unsigned short  xvYCC709    : 1;
    unsigned short  sYCC601     : 1;
    unsigned short  AdobeYCC601 : 1;
    unsigned short  AdobeRGB    : 1;
    unsigned short  BT2020RGB   : 1;
    unsigned short  BT2020YCC   : 1;
    unsigned short  BT2020cYCC  : 1;
    unsigned short  MD0         : 1;
    unsigned short  MD1         : 1;
    unsigned short  MD2         : 1;
    unsigned short  MD3         : 1;
    unsigned short  res         : 4;
}VPP_HDMI_CALORIMETRY_INFO;

typedef struct tagVPP_HDMI_CAPABILITY_INFO
{
    unsigned char   QS         : 1;
    unsigned char   QY         : 1;
    unsigned char   res         : 6;
}VPP_HDMI_CAPABILITY_INFO;

typedef struct tagVPP_HDMI_YCbCr420_CAP_INFO
{
    UINT8    VideoIdCode[16]; //Check max VICs at that support only 420
}VPP_HDMI_YCbCr420_CAP_INFO, *PVPP_HDMI_YCbCr420_CAP_INFOO;

typedef struct tagVPP_HDMI_YCbCr420_CAP_MAP_INFO
{
    UINT8    VideoIdCode[31];
}VPP_HDMI_YCbCr420_CAP_MAP_INFO, *PVPP_HDMI_YCbCr420_CAP_MAP_INFO;

//HDR Statci Metadata block
typedef struct tagVPP_HDMI_HDR_STATIC_METADATA
{
    UINT8  EOTF_ET_0    : 1;
    UINT8  EOTF_ET_1    : 1;
    UINT8  EOTF_ET_2    : 1;
    UINT8  EOTF_ET_3    : 1;

    /*ET4 to ET5 are reserved for future use*/
    UINT8  EOTF_ET_4    : 1;
    UINT8  EOTF_ET_5    : 1;
    UINT8  rsvd1        : 1;
    UINT8  rsvd2        : 1;

    UINT8  EOTF_SM_0    : 1;
    /*SM1 to SM7 are reserved for future use*/
    UINT8  EOTF_SM_1    : 1;
    UINT8  EOTF_SM_2    : 1;
    UINT8  EOTF_SM_3    : 1;
    UINT8  EOTF_SM_4    : 1;
    UINT8  EOTF_SM_5    : 1;
    UINT8  EOTF_SM_6    : 1;
    UINT8  EOTF_SM_7    : 1;

    UINT32  DesCntMaxLuminanceData;
    UINT32  DesCntMaxFrmAvgLuminanceData;
    UINT32  DesCntMinLuminanceData;

}VPP_HDMI_HDR_METADATA_INFO, *PVPP_HDMI_HDR_METADATA_INFO;

typedef struct tagHDMI_DOLBYVISION_VSVDB0
{
    UINT32 IEEE_OUI_Identifier;
    unsigned char Supports_YUV422_12Bit   :1;
    unsigned char Supports_2160p_60       :1;
    unsigned char Supports_Global_Dimming :1;
    unsigned char reserved                :2;
    unsigned char VSVDB_Version           :3;
    unsigned char Rxy_lsb                 :8;
    unsigned char Rx_msb                  :8;
    unsigned char Ry_msb                  :8;
    unsigned char Gxy_lsb                 :8;
    unsigned char Gx_msb                  :8;
    unsigned char Gy_msb                  :8;
    unsigned char Bxy_lsb                 :8;
    unsigned char Bx_msb                  :8;
    unsigned char By_msb                  :8;
    unsigned char Wxy_lsb                 :8;
    unsigned char Wx_msb                  :8;
    unsigned char Wy_msb                  :8;
    unsigned char PQ_lsb                  :8;
    unsigned char PQ_min                  :8;
    unsigned char PQ_max                  :8;
    unsigned char DM_minor_version       :4;
    unsigned char DM_major_version       :4;
}HDMI_DOLBYVISION_VSVDB0;

typedef struct tagHDMI_DOLBYVISION_VSVDB1
{
    UINT32 IEEE_OUI_Identifier;
    unsigned char Supports_YUV422_12Bit  :1 ;
    unsigned char Supports_2160p_60      :1 ;
    unsigned char DM_Version             :3 ;
    unsigned char VSVDB_Version          :3 ;
    unsigned char Supports_Global_Dimming:1 ;
    unsigned char Target_max_lum         :7 ;
    unsigned char colorimetry            :1 ;
    unsigned char Target_min_lum         :7 ;
    unsigned char R_x                    :8 ;
    unsigned char R_y                    :8 ;
    unsigned char G_x                    :8 ;
    unsigned char G_y                    :8 ;
    unsigned char B_x                    :8 ;
    unsigned char B_y                    :8 ;
}HDMI_DOLBYVISION_VSVDB1;

typedef union tagHDMI_DOLBYVISION_VSVDB
{
    HDMI_DOLBYVISION_VSVDB0 dolby_vsvdb_v0;
    HDMI_DOLBYVISION_VSVDB1 dolby_vsvdb_v1;
}HDMI_DOLBYVISION_VSVDB, *PHDMI_DOLBYVISION_VSVDB;

typedef struct tagVPP_DOLBYVISION_VSVDB
{
    UINT32 uiVersion;
    HDMI_DOLBYVISION_VSVDB dolby_vsvdb;
}VPP_HDMI_DOLBYVISION_VSVDB,*PVPP_HDMI_DOLBYVISION_VSVDB;


//Hdmi 2.0 VSDB Structures
typedef struct tagVPP_EDID_CEA_HFVSDB_FEATURE_SUPPORT {
    UINT8 Hdmi_3D_OSD_Disparity :1;
    UINT8 Dual_View             :1;
    UINT8 Independent_view      :1;
    UINT8 LTE_340Mcsc_scramble  :1;
    UINT8 Reserved              :2;
    UINT8 RR_Capable            :1;
    UINT8 SCDC_Present          :1;
}VPP_EDID_CEA_HFVSDB_FEATURE_SUPPORT , *PVPP_EDID_CEA_HFVSDB_FEATURES_SUPPORT;

typedef struct tagVPP_EDID_CEA_HFVSDB_DEEPCOL_SUPPORT{
    UINT8 DC_30bit_420          :1;
    UINT8 DC_36bit_420          :1;
    UINT8 DC_48bit_420          :1;
    UINT8 Reserved              :5;
}VPP_EDID_CEA_HFVSDB_DEEPCOL_SUPPORT , *PVPP_EDID_CEA_HFVSDB_DEEPCOL_SUPPORT;

typedef struct tagVPP_HDMI_DOLBYVISION_CAP
{
    UINT8 isDolbyVisionSupported  :1;
    UINT8 is4Kp60Supported        :1;
    UINT8 is422_12bitSupported    :1;
    UINT8 dv_version              :2;
    UINT8 latency                 :2;
    UINT8 Reserved                :1;
    UINT32 MaxLum;
    UINT32 MinLum;
}VPP_HDMI_DOLBYVISION_CAP, *PVPP_HDMI_DOLBYVISION_CAP;

typedef struct tagVPP_HDMI_HDR_CAPABILITY
{
    UINT8 isHDR10Supported    :1;
    UINT8 isDVSupported       :1;
    UINT8 isHLGSupported      :1;
    UINT8 Reserved            :5;
    VPP_HDMI_HDR_METADATA_INFO  hdr10Info;
    VPP_HDMI_DOLBYVISION_CAP  dolbyvisionvsvdb;
}VPP_HDMI_HDR_CAPABILITY,*PVPP_HDMI_HDR_CAPABILITY;

typedef struct tagVPP_EDID_CEA_HFVSDB {

    UINT8 VendorSpecificTagCode;
    UINT8 Length;
    UINT8 IEEE_OUI_ThirdOctet;
    UINT8 IEEE_OUI_SecondOctet;
    UINT8 IEEE_OUI_FirstOctet;
    UINT8 Version;
    UINT8 MAX_TMDS_Character_Rate;
    VPP_EDID_CEA_HFVSDB_FEATURE_SUPPORT HfvsdbFeatures;
    VPP_EDID_CEA_HFVSDB_DEEPCOL_SUPPORT HfVsdbDeepCol;
}VPP_EDID_CEA_HFVSDB , *PVPP_EDID_CEA_HFVSDB;

// Pixel repetition info
typedef struct tagVPP_HDMI_PIXEL_REPT_INFO
{
    unsigned int    resMask     : 26;
    unsigned int    prSupport   : 6;
}VPP_HDMI_PIXEL_REPT_INFO;

// EDID Latency info
typedef enum VPP_EDID_LATENCY_STS_T
{
    VPP_EDID_LATENCY_STS_NOT_PRESENT = 0,
    VPP_EDID_LATENCY_STS_INVALID,
    VPP_EDID_LATENCY_STS_UNSUPP_AV,
    VPP_EDID_LATENCY_STS_VALID
}VPP_EDID_LATENCY_STS;

typedef struct VPP_EDID_LATENCY_INFO_T
{
    // Video Latency for Progressive Video Formats - value in ms
    VPP_EDID_LATENCY_STS prgVidLatencySts;
    UINT32               prgVidLatencyValue;

    // Audio Latency for Progressive Video Formats - value in ms
    VPP_EDID_LATENCY_STS prgAudLatencySts;
    UINT32               prgAudLatencyValue;

    // Video Latency for Interlaced Video Formats - value in ms
    VPP_EDID_LATENCY_STS intVidLatencySts;
    UINT32               intVidLatencyValue;

    // Audio Latency for Interlaced Video Formats - value in ms
    VPP_EDID_LATENCY_STS intAudLatencySts;
    UINT32               intAudLatencyValue;
}VPP_EDID_LATENCY_INFO;

// HDCP KSV DS
typedef struct VPP_HDCP_BSTATUS_T{
    UINT16        Depth;
    UINT16        MaxDeviceExceeded;
    UINT16        MaxCascadeExceeded;
}VPP_HDCP_BSTATUS;

typedef struct VPP_HDCP_KSV_DATA_T{
    UINT8                  KsvBuf[640];// size will be maxnumberdev*5+5
    UINT32                 KsvLen;//(RxBStatusReg.DeviceCount * 5) + 5
    VPP_HDCP_BSTATUS       BStatus;
}VPP_HDCP_KSV_DATA;

// Device Signature
typedef enum tagVPP_DEV_SIGN_STS {
    VPP_DEV_SIGN_UNSUPPORTED,
    VPP_DEV_SIGN_NONE,
    VPP_DEV_SIGN_UNVERIFIED,
    VPP_DEV_SIGN_VALID,
    VPP_DEV_SIGN_INVALID,
}VPP_DEV_SIGN_STS;

typedef struct VPP_DEV_SIGN_DATA_T {
    UINT8   signature[32];
    UINT32  signSts; //VPP_DEV_SIGN_STS
}VPP_DEV_SIGN_DATA;

// HDCP SRM Data
typedef struct VPP_HDCP_SRM_DATA_T{
    UINT8   *pBuf;
    UINT32  bufLen;
    BOOL    bForceUpdate;
}VPP_HDCP_SRM_DATA;

typedef struct VPP_HDCP_SRM_INFO_T {
    UINT8   valid;
    UINT8   ID;
    UINT16  version;
    UINT16  generation;
    UINT32  updated;
}VPP_HDCP_SRM_INFO;

#define VPP_SIZE_OF_EDID_BLK   128
#define VPP_EDID_EXTBLK_FLAG_ADR   0x7E
#define VPP_EDID_CHCKSUM_ADDR       0x7F

typedef struct VPP_HDMI_RAW_EDID_T{
    UINT8    databuf[128];
    UINT8    databuf_ext[128];
    UINT8    IsValid;
}VPP_HDMI_RAW_EDID;

typedef struct VPP_HDMI_SINK_CAPS_T {
    // EDID Valid
    BOOL    validEdid;

    // Product Info
    unsigned char   monitorName[14];// Monitor Name in ASCII format
    unsigned char   mnfrName[4];
    unsigned char   productCode[5];
    unsigned char   mnfrWeek;
    unsigned short  mnfrYear;
    unsigned int    serialNum;

    // Monitor Limits
    unsigned char   minVRate;       // Minimum Vertical Rate in Hz
    unsigned char   maxVRate;       // Maximum Vertical Rate in Hz
    unsigned char   minHRate;       // Minimum Horizontal Rate in KHz
    unsigned char   maxHRate;       // Maximum Horizontal Rate in KHz
    unsigned int    maxPixelClock;  // Maximum supported pixel clock rate in MHz

    // Maximum image size
    unsigned short  maxHSize;
    unsigned short  maxVSize;

    // Gamma Factor
    int   gamma;

    // Resolution information
    unsigned int      resCnt;
    VPP_HDMI_RES_INFO resInfo[64];

    // Indicates if the receiver is HDMI capable
    BOOL              hdmiMode;

    // Video resolutions supported both by transmitter and receiver
    // (Common ones that can be set for hdmi output display)
    unsigned int       sprtedDispRes;

    BOOL              support_3D;    // indicates if the receiver supports 3D formats
    unsigned int      sprted3DDispRes; // bit[x]: 1 - support, 0 - not support. x means for 3D resolution 3D_FIRST_3D+x
    unsigned int      sprted3DStructures[/*MAX_NUM_RES_3D-FIRST_RES_3D+1*/32]; // sprted3DStructures[x]: x means for 3D resolution 3D_FIRST_3D+x
                                                                         // bit[15~0]: bit[x] set to 1 means 3D structure x is supported, otherwise is not supported.
                                                                         // bit[31~16]: extra parameter for structure 8 or above.

     unsigned int     hdmi_VIC_Len;                                     //Indicates the total length from HDMi_VIC_1 to HDMI_VIC_M
     unsigned int     sprtedHdmiVICRes;

    // Preferred video resolution
    VPP_HDMI_RES_INFO           prefResInfo;

    // Sink feature support
    unsigned char               YCbCr444Sprt;
    unsigned char               YCbCr422Sprt;
    unsigned char               DC30bitSprt;
    unsigned char               DC36bitSprt;
    unsigned char               YCbCr444AtDC;
    unsigned char               AISprt;

    // Pixel repetition support
    VPP_HDMI_PIXEL_REPT_INFO    prInfo[4];

    // Colorimtery information
    VPP_HDMI_CALORIMETRY_INFO   calInfo;
    //Add flag for QS,QY range info. Verify audio after enabling this
    VPP_HDMI_CAPABILITY_INFO    capInfo;
    VPP_HDMI_YCbCr420_CAP_INFO  YCbCr420_capInfo;

    // Supported audio format
    VPP_HDMI_SPKR_ALLOC         spkrAlloc;
    VPP_HDMI_AUDIO_INFO         audioInfo[15];

    // CEC Physical address
    unsigned short              cecPhyAddr;
    unsigned short              maxTMDSClock;
    VPP_EDID_CEA_HFVSDB         CeaHdmiVSDB;
    VPP_HDMI_YCbCr420_CAP_MAP_INFO   YCbCr420_capmapInfo;

    //SVD block parsing - maximum of 31 vics can exist here
    unsigned short              svdblocklen;
    unsigned short              svd_vics[31];

    VPP_EDID_LATENCY_INFO       latencyInfo;
    VPP_HDMI_HDR_CAPABILITY    HdrCapInfo;
} VPP_HDMI_SINK_CAPS;
/*-----------------------------------------------------------------------------
 * HDMI CEC Message enums and structures
 *-----------------------------------------------------------------------------
 */
// Logical device types
typedef enum tagVPP_CEC_DEVICE_TYPE
{
    VPP_CEC_DEVICE_FIRST = 0,
    VPP_CEC_DEVICE_TV   = 0,
    VPP_CEC_DEVICE_REC,
    VPP_CEC_DEVICE_RES,
    VPP_CEC_DEVICE_TUNER,
    VPP_CEC_DEVICE_PB,
    VPP_CEC_DEVICE_AUDIO_SYS,
    VPP_CEC_DEVICE_MAX
}VPP_CEC_DEVICE_TYPE, *PVPP_CEC_DEVICE_TYPE;

// Status information
typedef enum tagVPP_CEC_STATUS_TYPE
{
    VPP_CEC_STATUS_SUCCESS = 0x00,
    VPP_CEC_STATUS_INVALID_PARAM,
    VPP_CEC_STATUS_ERR_INVALID_MODE,
    VPP_CEC_STATUS_ERR_MSG_UNSUPPORTED,
    VPP_CEC_STATUS_ERR_INVALID_FRAME_LEN,
    VPP_CEC_STATUS_ERR_MSG_IGNORED,
    VPP_CEC_STATUS_TX_ERR_COMM_FAIL,
    VPP_CEC_STATUS_RX_ERR_COMM_FAIL,
    VPP_CEC_STATUS_TX_ERR_WAIT_FAIL,
    VPP_CEC_STATUS_TX_ERR_RETRY_FAIL,
    VPP_CEC_STATUS_INFO_MAX
}VPP_CEC_STATUS_TYPE, *PVPP_CEC_STATUS_TYPE;

// Feature abort reason
typedef enum tagVPP_CEC_ABORT_REASON_TYPE
{
    VPP_CEC_ABORT_REASON_UNREC_OPCODE = 0x00,
    VPP_CEC_ABORT_REASON_INV_MODE_TO_RESPOND,
    VPP_CEC_ABORT_REASON_CANNOT_PROVIDE_SRC,
    VPP_CEC_ABORT_REASON_INV_OPCODE,
    VPP_CEC_ABORT_REASON_REFUSED,
    VPP_CEC_ABORT_REASON_MAX
}VPP_CEC_ABORT_REASON_TYPE;

// Menu request type
typedef enum tagVPP_CEC_MENU_REQUEST_TYPE
{
    VPP_CEC_MENU_REQUEST_FIRST = 0x00,
    VPP_CEC_MENU_REQUEST_ACTIVATE = 0x00,
    VPP_CEC_MENU_REQUEST_DEACTIVATE,
    VPP_CEC_MENU_REQUEST_QUERY,
    VPP_CEC_MENU_REQUEST_MAX
}VPP_CEC_MENU_REQUEST_TYPE;

// Menu States
typedef enum tagVPP_CEC_MENU_STATE_TYPE
{
    VPP_CEC_MENU_STATE_FIRST = 0x00,
    VPP_CEC_MENU_STATE_ACTIVATED = 0x00,
    VPP_CEC_MENU_STATE_DEACTIVATED,
    VPP_CEC_MENU_STATE_MAX
}VPP_CEC_MENU_STATE_TYPE;

// User Control Code
typedef enum tagVPP_CEC_USER_CONTROL_CODE
{
    VPP_CEC_UC_SELECT       = 0x00,
    VPP_CEC_UC_UP           = 0x01,
    VPP_CEC_UC_DOWN         = 0x02,
    VPP_CEC_UC_LEFT         = 0x03,
    VPP_CEC_UC_RIGHT        = 0x04,
    VPP_CEC_UC_RIGHT_UP     = 0x05,
    VPP_CEC_UC_RIGHT_DOWN   = 0x06,
    VPP_CEC_UC_LEFT_UP      = 0x07,
    VPP_CEC_UC_LEFT_DOWN    = 0x08,
    VPP_CEC_UC_ROOT_MENU    = 0x09,
    VPP_CEC_UC_SETUP_MENU   = 0x0A,
    VPP_CEC_UC_CONTENTS_MENU= 0x0B,
    VPP_CEC_UC_FAV_MENU     = 0x0C,
    VPP_CEC_UC_EXIT         = 0x0D,
    VPP_CEC_UC_MEDIA_TOP_MENU=0x10,
    VPP_CEC_UC_MEDIA_CONTEXT_SENSITIVE_MENU=0x11,
    VPP_CEC_UC_NUM_ENTRY_MODE=0x1D,
    VPP_CEC_UC_NUM_11        = 0x1E,
    VPP_CEC_UC_NUM_12        = 0x1F,
    VPP_CEC_UC_NUM_00       = 0x20,
    VPP_CEC_UC_NUM_01       = 0x21,
    VPP_CEC_UC_NUM_02       = 0x22,
    VPP_CEC_UC_NUM_03       = 0x23,
    VPP_CEC_UC_NUM_04       = 0x24,
    VPP_CEC_UC_NUM_05       = 0x25,
    VPP_CEC_UC_NUM_06       = 0x26,
    VPP_CEC_UC_NUM_07       = 0x27,
    VPP_CEC_UC_NUM_08       = 0x28,
    VPP_CEC_UC_NUM_09       = 0x29,
    VPP_CEC_UC_DOT          = 0x2A,
    VPP_CEC_UC_ENTER        = 0x2B,
    VPP_CEC_UC_CLEAR        = 0x2C,
    VPP_CEC_UC_NEXT_FAV     = 0x2F,
    VPP_CEC_UC_CH_UP        = 0x30,
    VPP_CEC_UC_CH_DOWN      = 0x31,
    VPP_CEC_UC_PREV_CH      = 0x32,
    VPP_CEC_UC_SOUND_SEL    = 0x33,
    VPP_CEC_UC_INPUT_SEL    = 0x34,
    VPP_CEC_UC_DISP_INFO    = 0x35,
    VPP_CEC_UC_HELP         = 0x36,
    VPP_CEC_UC_PAGE_UP      = 0x37,
    VPP_CEC_UC_PAGE_DOWN    = 0x38,
    VPP_CEC_UC_POWER        = 0x40,
    VPP_CEC_UC_VOL_UP       = 0x41,
    VPP_CEC_UC_VOL_DOWN     = 0x42,
    VPP_CEC_UC_MUTE         = 0x43,
    VPP_CEC_UC_PLAY         = 0x44,
    VPP_CEC_UC_STOP         = 0x45,
    VPP_CEC_UC_PAUSE        = 0x46,
    VPP_CEC_UC_RECORD       = 0x47,
    VPP_CEC_UC_REWIND       = 0x48,
    VPP_CEC_UC_FAST_FORWARD = 0x49,
    VPP_CEC_UC_EJECT        = 0x4A,
    VPP_CEC_UC_FORWARD      = 0x4B,
    VPP_CEC_UC_BACKWARD     = 0x4C,
    VPP_CEC_UC_STOP_REC     = 0x4D,
    VPP_CEC_UC_PAUSE_REC    = 0x4E,
    VPP_CEC_UC_ANGLE        = 0x50,
    VPP_CEC_UC_SUB_PICT     = 0x51,
    VPP_CEC_UC_VIDEO_ON_DEMAND      = 0x52,
    VPP_CEC_UC_EPG                  = 0x53,
    VPP_CEC_UC_TIMER_PGM            = 0x54,
    VPP_CEC_UC_INIT_CFG             = 0x55,
    VPP_CEC_UC_SEL_BRD_TYPE_FN        = 0x56,
    VPP_CEC_UC_SEL_SOUND_PPT_FN        = 0x57,
    VPP_CEC_UC_PLAY_FN              = 0x60,
    VPP_CEC_UC_PAUSE_PLAY_FN        = 0x61,
    VPP_CEC_UC_RECORD_FN            = 0x62,
    VPP_CEC_UC_PAUSE_RECORD_FN      = 0x63,
    VPP_CEC_UC_STOP_FN              = 0x64,
    VPP_CEC_UC_MUTE_FN              = 0x65,
    VPP_CEC_UC_RESTORE_VOL_FN       = 0x66,
    VPP_CEC_UC_TUNE_FN              = 0x67,
    VPP_CEC_UC_SEL_DISK_FN          = 0x68,
    VPP_CEC_UC_SEL_AV_INPUT_FN      = 0x69,
    VPP_CEC_UC_SEL_AUDIO_INPUT_FN   = 0x6A,
    VPP_CEC_UC_POWER_TOGGLE_FN      = 0x6B,
    VPP_CEC_UC_POWER_OFF_FN         = 0x6C,
    VPP_CEC_UC_POWER_ON_FN          = 0x6D,
    VPP_CEC_UC_F1_BLUE              = 0x71,
    VPP_CEC_UC_F2_RED               = 0x72,
    VPP_CEC_UC_F3_GREEN             = 0x73,
    VPP_CEC_UC_F4_YELLOW            = 0x74,
    VPP_CEC_UC_F5                   = 0x75,
    VPP_CEC_UC_DATA            = 0x76,
    VPP_CEC_UC_MAX
}VPP_CEC_USER_CONTROL_CODE;

// Recording Source
typedef enum tagVPP_CEC_REC_SRC_TYPE
{
    VPP_CEC_REC_SRC_OWN_SRC = 0x01,
    VPP_CEC_REC_SRC_DIG_SRV = 0x02,
    VPP_CEC_REC_SRC_ANA_SRV = 0x03,
    VPP_CEC_REC_SRC_EXT_PLUG = 0x04,
    VPP_CEC_REC_SRC_EXT_PHY_ADDR = 0x05,
}VPP_CEC_REC_SRC_TYPE;

// Recording status
typedef enum tagVPP_CEC_REC_STS_INFO
{
    VPP_CEC_REC_STS_FIRST                           = 0x01,
    VPP_CEC_REC_STS_RECORDING_OWN_SRC               = 0x01,
    VPP_CEC_REC_STS_RECORDING_DIG_SRV               = 0x02,
    VPP_CEC_REC_STS_RECORDING_ANA_SRV               = 0x03,
    VPP_CEC_REC_STS_RECORDING_EXT_INP               = 0x04,
    VPP_CEC_REC_STS_NO_REC_UNABLE_TO_REC_DIG_SRV    = 0x05,
    VPP_CEC_REC_STS_NO_REC_UNABLE_TO_REC_ANA_SRV    = 0x06,
    VPP_CEC_REC_STS_NO_REC_UNABLE_TO_SEL_SRV        = 0x07,
    VPP_CEC_REC_STS_NO_REC_INV_EXT_PLUG_NUM         = 0x09,
    VPP_CEC_REC_STS_NO_REC_INV_EXT_PHY_ADDR         = 0x0A,
    VPP_CEC_REC_STS_NO_REC_CA_SYS_UNSUPP            = 0x0B,
    VPP_CEC_REC_STS_NO_REC_NO_CA_ENTITLEMENT        = 0x0C,
    VPP_CEC_REC_STS_NO_REC_SRC_COPY_NOT_ALLOWED     = 0x0D,
    VPP_CEC_REC_STS_NO_REC_NO_MORE_COPIES_ALLOWED   = 0x0E,
    VPP_CEC_REC_STS_NO_REC_NO_MEDIA                 = 0x10,
    VPP_CEC_REC_STS_NO_REC_PLAYING                  = 0x11,
    VPP_CEC_REC_STS_NO_REC_ALREADY_RECORDING        = 0x12,
    VPP_CEC_REC_STS_NO_REC_MEDIA_PROTECTED          = 0x13,
    VPP_CEC_REC_STS_NO_REC_NO_SOURCE                = 0x14,
    VPP_CEC_REC_STS_NO_REC_MEDIA_PROBLEM            = 0x15,
    VPP_CEC_REC_STS_NO_REC_NO_SPACE                 = 0x16,
    VPP_CEC_REC_STS_NO_REC_PAR_LOCK                 = 0x17,
    VPP_CEC_REC_STS_REC_TERMINATED_NORMALLY         = 0x1A,
    VPP_CEC_REC_STS_REC_ALREADY_TERMINATED          = 0x1B,
    VPP_CEC_REC_STS_NO_REC_OTHER_REASON             = 0x1F
}VPP_CEC_REC_STS_INFO;

// Digital broadcast system
typedef enum tagVPP_CEC_DIG_BROADCAST_SYS
{
    VPP_CEC_DIG_BROADCAST_SYS_ARIB_GEN = 0x00,
    VPP_CEC_DIG_BROADCAST_SYS_ATSC_GEN = 0x01,
    VPP_CEC_DIG_BROADCAST_SYS_DVB_GEN  = 0x02,
    VPP_CEC_DIG_BROADCAST_SYS_ARIB_BS  = 0x08,
    VPP_CEC_DIG_BROADCAST_SYS_ARIB_CS  = 0x09,
    VPP_CEC_DIG_BROADCAST_SYS_ARIB_T   = 0x0A,
    VPP_CEC_DIG_BROADCAST_SYS_ATSC_CAB  = 0x10,
    VPP_CEC_DIG_BROADCAST_SYS_ATSC_SAT  = 0x11,
    VPP_CEC_DIG_BROADCAST_SYS_ATSC_TER  = 0x12,
    VPP_CEC_DIG_BROADCAST_SYS_DVB_C     = 0x18,
    VPP_CEC_DIG_BROADCAST_SYS_DVB_S     = 0x19,
    VPP_CEC_DIG_BROADCAST_SYS_DVB_S2    = 0x1A,
    VPP_CEC_DIG_BROADCAST_SYS_DVB_T     = 0x1B,
}VPP_CEC_DIG_BROADCAST_SYS;

// Service Identification Method
typedef enum tagVPP_CEC_SRV_IDEN_TYPE
{
    VPP_CEC_SRV_IDEN_DIG_ID = 0x00,
    VPP_CEC_SRV_IDEN_CHN    = 0x01,
}VPP_CEC_SRV_IDEN_TYPE;

// Deck control mode
typedef enum VPP_CEC_DECK_CTRL_MODE
{
    VPP_CEC_DECK_CTRL_MODE_SKIPFWD_WINDFWD = 0x01,
    VPP_CEC_DECK_CTRL_MODE_SKIPBWD_REWIND  = 0x02,
    VPP_CEC_DECK_CTRL_MODE_STOP            = 0x03,
    VPP_CEC_DECK_CTRL_MODE_EJECT           = 0x04,
    VPP_CEC_DECK_CTRL_MODE_MAX
}VPP_CEC_DECK_CONTROL_MODE;

// Deck Info
typedef enum VPP_CEC_DECK_INFO
{
    VPP_CEC_DECK_INFO_PLAY              = 0x11,
    VPP_CEC_DECK_INFO_RECORD            = 0x12,
    VPP_CEC_DECK_INFO_PLAY_REV          = 0x13,
    VPP_CEC_DECK_INFO_STILL             = 0x14,
    VPP_CEC_DECK_INFO_SLOW              = 0x15,
    VPP_CEC_DECK_INFO_SLOW_REV          = 0x16,
    VPP_CEC_DECK_INFO_FAST_FWD          = 0x17,
    VPP_CEC_DECK_INFO_FAST_REV          = 0x18,
    VPP_CEC_DECK_INFO_NO_MEDIA          = 0x19,
    VPP_CEC_DECK_INFO_STOP              = 0x1A,
    VPP_CEC_DECK_INFO_WIND              = 0x1B,
    VPP_CEC_DECK_INFO_REWIND            = 0x1C,
    VPP_CEC_DECK_INFO_INDEX_SEARCH_FWD  = 0x1D,
    VPP_CEC_DECK_INFO_INDEX_SEARCH_REV  = 0x1E,
    VPP_CEC_DECK_INFO_OTHER_STS         = 0x1F,
    VPP_CEC_DECK_INFO_MAX,
}VPP_CEC_DECK_INFO;

// Status Request
typedef enum tagVPP_CEC_STATUS_REQUEST
{
    VPP_CEC_STATUS_REQUEST_ON   = 0x01,
    VPP_CEC_STATUS_REQUEST_OFF  = 0x02,
    VPP_CEC_STATUS_REQUEST_ONCE = 0x03,
    VPP_CEC_STATUS_REQUEST_MAX
}VPP_CEC_STATUS_REQUEST;

// Play Mode
typedef enum tagVPP_CEC_PLAY_MODE
{
    VPP_CEC_PLAY_MODE_FORWARD               = 0x24,
    VPP_CEC_PLAY_MODE_REVERSE               = 0x20,
    VPP_CEC_PLAY_MODE_STILL                 = 0x25,
    VPP_CEC_PLAY_MODE_FAST_FWD_MIN_SPEED    = 0x05,
    VPP_CEC_PLAY_MODE_FAST_FWD_MED_SPEED    = 0x06,
    VPP_CEC_PLAY_MODE_FAST_FWD_MAX_SPEED    = 0x07,
    VPP_CEC_PLAY_MODE_FAST_REV_MIN_SPEED    = 0x09,
    VPP_CEC_PLAY_MODE_FAST_REV_MED_SPEED    = 0x0A,
    VPP_CEC_PLAY_MODE_FAST_REV_MAX_SPEED    = 0x0B,
    VPP_CEC_PLAY_MODE_SLOW_FWD_MIN_SPEED    = 0x15,
    VPP_CEC_PLAY_MODE_SLOW_FWD_MED_SPEED    = 0x16,
    VPP_CEC_PLAY_MODE_SLOW_FWD_MAX_SPEED    = 0x17,
    VPP_CEC_PLAY_MODE_SLOW_REV_MIN_SPEED    = 0x19,
    VPP_CEC_PLAY_MODE_SLOW_REV_MED_SPEED    = 0x1A,
    VPP_CEC_PLAY_MODE_SLOW_REV_MAX_SPEED    = 0x1B,
    VPP_CEC_PLAY_MODE_MAX
}VPP_CEC_PLAY_MODE;

// Display Control
typedef enum tagVPP_CEC_DISPLAY_CONTROL
{
    VPP_CEC_DISPCTRL_DISP_FOR_DEF_TIME   = 0x00,
    VPP_CEC_DISPCTRL_DISP_UNTIL_CLEARED  = 0x40,
    VPP_CEC_DISPCTRL_CLR_PREV_MSG        = 0x80,
    VPP_CEC_DISPCTRL_RESERVED            = 0xC0
}VPP_CEC_DISPLAY_CONTROL;

// Power Status
typedef enum tagVPP_CEC_PWR_STS
{
    VPP_CEC_PWR_STS_ON   = 0x00,
    VPP_CEC_PWR_STS_STANDBY,
    VPP_CEC_PWR_STS_IN_TRANS_STANDBY_TO_ON,
    VPP_CEC_PWR_STS_IN_TRANS_ON_TO_STANDBY
}VPP_CEC_PWR_STS;

// Tuner Display Info
typedef enum tagVPP_CEC_TUNER_DISPLAY_INFO
{
    VPP_CEC_TUNER_DISPLAY_INFO_DIG_TUNER_DISPLAYED = 0x00,
    VPP_CEC_TUNER_DISPLAY_INFO_TUNER_NOT_DISPLAYED,
    VPP_CEC_TUNER_DISPLAY_INFO_ANA_TUNER_DISPLAYED,
    VPP_CEC_TUNER_DISPLAY_INFO_MAX
}VPP_CEC_TUNER_DISPLAY_INFO;

// Recording Flag
typedef enum tagVPP_CEC_REC_FLAG
{
    VPP_CEC_REC_FLAG_TUNER_NOT_USED = 0x00,
    VPP_CEC_REC_FLAG_TUNER_USED
}VPP_CEC_REC_FLAG;

// Recording Sequence
typedef enum tagVPP_CEC_REC_SEQUENCE
{
    VPP_CEC_REC_SEQUENCE_ONCE_ONLY  = 0,
    VPP_CEC_REC_SEQUENCE_SUN        = 1,
    VPP_CEC_REC_SEQUENCE_MON        = 2,
    VPP_CEC_REC_SEQUENCE_TUES       = 4,
    VPP_CEC_REC_SEQUENCE_WED        = 8,
    VPP_CEC_REC_SEQUENCE_THUR       = 16,
    VPP_CEC_REC_SEQUENCE_FRI        = 32,
    VPP_CEC_REC_SEQUENCE_SAT        = 64,
}VPP_CEC_REC_SEQUENCE;

// Analogue Broadcast Type
typedef enum tagVPP_CEC_ANALOGUE_BROADCAST_TYPE
{
    VPP_CEC_ANALOGUE_BROADCAST_CABLE       = 0x00,
    VPP_CEC_ANALOGUE_BROADCAST_SATELLITE   = 0x01,
    VPP_CEC_ANALOGUE_BROADCAST_TERRESTRIAL = 0x02,
}VPP_CEC_ANALOGUE_BROADCAST_TYPE;

// Broadcast system
typedef enum tagVPP_CEC_BROADCAST_SYSTEM
{
    VPP_CEC_BROADCAST_SYSTEM_PAL_BG      = 0x00,
    VPP_CEC_BROADCAST_SYSTEM_SECAM_LDASH = 0x01,
    VPP_CEC_BROADCAST_SYSTEM_PAL_M       = 0x02,
    VPP_CEC_BROADCAST_SYSTEM_NTSC_M      = 0x03,
    VPP_CEC_BROADCAST_SYSTEM_PAL_I       = 0x04,
    VPP_CEC_BROADCAST_SYSTEM_SECAM_DK    = 0x05,
    VPP_CEC_BROADCAST_SYSTEM_SECAM_BG    = 0x06,
    VPP_CEC_BROADCAST_SYSTEM_SECAM_L     = 0x07,
    VPP_CEC_BROADCAST_SYSTEM_PAL_DK      = 0x08,
    VPP_CEC_BROADCAST_SYSTEM_OTHER_SYS   = 0x1F,
}VPP_CEC_BROADCAST_SYSTEM;

// External source specifier
typedef enum tagVPP_CEC_EXT_SRC_SPECIFIER
{
    VPP_CEC_EXT_SRC_PLUG     = 0x04,
    VPP_CEC_EXT_SRC_PHY_ADDR = 0x05,
}VPP_CEC_EXT_SRC_SPECIFIER;

// Timer Cleared Status Data
typedef enum tagVPP_CEC_TIMER_CLRED_STATUS_DATA
{
    VPP_CEC_TIMER_STS_NOT_CLRED_REC         = 0x00,
    VPP_CEC_TIMER_STS_NOT_CLRED_NO_MATCHING = 0x01,
    VPP_CEC_TIMER_STS_NOT_CLRED_NO_INFO     = 0x02,
    VPP_CEC_TIMER_STS_CLRED                 = 0x80,
}VPP_CEC_TIMER_CLRED_STATUS_DATA;

// Timer Overlap Warning
typedef enum tagVPP_CEC_TIMER_OVERLAP_WARNING_TYPE
{
    VPP_CEC_TIMER_OVERLAP_WARNING_NO_OVERLAP     = 0x00,
    VPP_CEC_TIMER_OVERLAP_WARNING_BLOCKS_OVERLAP = 0x01,
}VPP_CEC_TIMER_OVERLAP_WARNING_TYPE;

// Media Info
typedef enum tagVPP_CEC_MEDIA_INFO
{
    VPP_CEC_MEDIA_INFO_PRESENT_NOT_PROT = 0x00,
    VPP_CEC_MEDIA_INFO_PRESENT_PROT     = 0x01,
    VPP_CEC_MEDIA_INFO_NOT_PRESENT      = 0x02,
    VPP_CEC_MEDIA_INFO_FUTURE_USE       = 0x03
}VPP_CEC_MEDIA_INFO;

// Timer Programmed Indicator
typedef enum tagVPP_CEC_TIMER_PGM_INDICATOR
{
    VPP_CEC_TIMER_PGM_IND_NOT_PROGRAMMED = 0x00,
    VPP_CEC_TIMER_PGM_IND_PROGRAMMED     = 0x01
}VPP_CEC_TIMER_PGM_INDICATOR;

// Timer Programmed Info
typedef enum tagVPP_CEC_TIMER_PGMED_INFO
{
    VPP_CEC_TIMER_PGMED_INFO_SPACE_FOR_REC    = 0x08,
    VPP_CEC_TIMER_PGMED_INFO_NO_SPACE_FOR_REC = 0x09,
    VPP_CEC_TIMER_PGMED_INFO_NO_MEDIA_INFO    = 0x0A,
    VPP_CEC_TIMER_PGMED_INFO_MAYNOT_SPACE_FOR_REC = 0x0B,
}VPP_CEC_TIMER_PGMED_INFO;

// Timer not programmed error info
typedef enum tagVPP_CEC_TIMER_NOT_PGMED_ERR_INFO
{
    VPP_CEC_TIMER_NOT_PGMED_ERR_NO_FREE_TIMER       = 0x01,
    VPP_CEC_TIMER_NOT_PGMED_ERR_DATE_OUT_OF_RANGE   = 0x02,
    VPP_CEC_TIMER_NOT_PGMED_ERR_REC_SEQ_ERR         = 0x03,
    VPP_CEC_TIMER_NOT_PGMED_ERR_INV_EXT_PLUG_NUM    = 0x04,
    VPP_CEC_TIMER_NOT_PGMED_ERR_INV_EXT_PHY_ADDR    = 0x05,
    VPP_CEC_TIMER_NOT_PGMED_ERR_CA_SYS_UNSUPPORT    = 0x06,
    VPP_CEC_TIMER_NOT_PGMED_ERR_CA_ENTITLEMENTS     = 0x07,
    VPP_CEC_TIMER_NOT_PGMED_ERR_RES_UNSUPPORT       = 0x08,
    VPP_CEC_TIMER_NOT_PGMED_ERR_PAR_LOCK_ON         = 0x09,
    VPP_CEC_TIMER_NOT_PGMED_ERR_CLK_FAILURE         = 0x0A,
    VPP_CEC_TIMER_NOT_PGMED_ERR_ALREADY_PGMED       = 0x0E
}VPP_CEC_TIMER_NOT_PGMED_ERR_INFO;

typedef enum tagVPP_CEC_AUDIO_RATE_CONTROL
{
    VPP_CEC_AUDIO_RATE_CTRL_OFF                 = 0x00,
    VPP_CEC_AUDIO_RATE_CTRL_WIDERANGE_STD_RATE  = 0x01,
    VPP_CEC_AUDIO_RATE_CTRL_WIDERANGE_FAST_RATE = 0x02,
    VPP_CEC_AUDIO_RATE_CTRL_WIDERANGE_SLOW_RATE = 0x03,
    VPP_CEC_AUDIO_RATE_CTRL_NARROWRANGE_STD_RATE= 0x04,
    VPP_CEC_AUDIO_RATE_CTRL_NARROWRANGE_FAST_RATE=0x05,
    VPP_CEC_AUDIO_RATE_CTRL_NARROWRANGE_SLOW_RATE=0x06
}VPP_CEC_AUDIO_RATE_CONTROL;

// CEC Version
typedef enum tagVPP_CEC_VERSION_TYPE
{
    VPP_CEC_VERSION_1_1  = 0x00,
    VPP_CEC_VERSION_1_2  = 0x01,
    VPP_CEC_VERSION_1_2a = 0x02,
    VPP_CEC_VERSION_1_3  = 0x03,
    VPP_CEC_VERSION_1_3a = 0x04,
    VPP_CEC_VERSION_1_4 = 0x05,
    VPP_CEC_VERSION_1_4a = 0x05
}VPP_CEC_VERSION_TYPE;

// Feature abort message operand
typedef struct tagVPP_CEC_OPERAND_FEATURE_ABORT
{
    UINT8                       featureOpcode;
    INT                         abortReason; //VPP_CEC_ABORT_REASON_TYPE
}VPP_CEC_OPERAND_FEATURE_ABORT;

// Report physical address message operand
typedef struct tagVPP_CEC_OPERAND_REPORT_PHY_ADDR
{
    UINT16                      phyAddr;
    INT                         deviceType; // VPP_CEC_DEVICE_TYPE
}VPP_CEC_OPERAND_REPORT_PHY_ADDR;

// ARIB Data
typedef struct tagVPP_CEC_OPERAND_ARIB_DATA
{
    UINT16  txStreamID;
    UINT16  content_srvID;
    UINT16  origNetworkID;
}VPP_CEC_OPERAND_ARIB_DATA;

// ATSC Data
typedef struct tagVPP_CEC_OPERAND_ATSC_DATA
{
    UINT16  txStreamID;
    UINT16  content_srvID;
}VPP_CEC_OPERAND_ATSC_DATA;

// DVB Data
typedef struct tagVPP_CEC_OPERAND_DVB_DATA
{
    UINT16  txStreamID;
    UINT16  content_srvID;
    UINT16  origNetworkID;
}VPP_CEC_OPERAND_DVB_DATA;

// Channel Data
typedef struct tagVPP_CEC_OPERAND_CHN_IDEN
{
    UINT32  chnNumFormat    : 6;
    UINT32  majChnNum       : 10;
    UINT32  minChnNum       : 16;
}VPP_CEC_OPERAND_CHN_IDEN;

// Digital service identification
typedef struct tagVPP_CEC_OPERAND_DIG_SRV_ID
{
    INT     srvIdenType;  // VPP_CEC_SRV_IDEN_TYPE
    INT     broadcastSys; // VPP_CEC_DIG_BROADCAST_SYS
    union
    {
        VPP_CEC_OPERAND_ARIB_DATA   aribData;
        VPP_CEC_OPERAND_ATSC_DATA   atscData;
        VPP_CEC_OPERAND_DVB_DATA    dvbData;
        VPP_CEC_OPERAND_CHN_IDEN    chnData;
    }srvIden;
}VPP_CEC_OPERAND_DIG_SRV_ID;

// Analogue Service Identification
typedef struct tagVPP_CEC_OPERAND_ANALOGUE_SRV_ID
{
    INT     broadcastType; // VPP_CEC_ANALOGUE_BROADCAST_TYPE
    // 62.5 * frequency for the actual value
    UINT16  frequency;
    INT     broadcastSystem; // VPP_CEC_BROADCAST_SYSTEM
}VPP_CEC_OPERAND_ANALOGUE_SRV_ID;

// Record source
typedef struct tagVPP_CEC_OPERAND_REC_SRC
{
    INT recSrc; // VPP_CEC_REC_SRC_TYPE
    union
    {
        VPP_CEC_OPERAND_DIG_SRV_ID      digSrvIden;
        VPP_CEC_OPERAND_ANALOGUE_SRV_ID anaSrvIden;
        UINT8                           extPlugNum;
        UINT16                          extPhyAddr;
    }recSrcIden;
}VPP_CEC_OPERAND_REC_SRC;

// OSD String
typedef struct tagVPP_CEC_OPERAND_OSD_STRING
{
    INT                         dispCtrl; // VPP_CEC_DISPLAY_CONTROL
    UINT8                       osdStringLen;
    UINT8                       osdString[13];
}VPP_CEC_OPERAND_OSD_STRING;

// OSD Name
typedef struct tagVPP_CEC_OPERAND_OSD_NAME
{
    UINT8   osdNameLen;
    UINT8   osdName[14];
}VPP_CEC_OPERAND_OSD_NAME;

// Vendor Specific Data
typedef struct tagVPP_CEC_OPERAND_VENDOR_SPEC_DATA
{
    UINT8       dataLength;
    // Will be used only for vendor command with ID message
    UINT32      vendorID;
    UINT8       vendorData[14]; // max 14 bytes
}VPP_CEC_OPERAND_VENDOR_SPEC_DATA;

// Tuner Device Info
typedef struct tagVPP_CEC_OPERAND_TUNER_DEVICE_INFO
{
    INT recordingFlag; // VPP_CEC_REC_FLAG
    INT tunerDispInfo; // VPP_CEC_TUNER_DISPLAY_INFO
    union
    {
        VPP_CEC_OPERAND_ANALOGUE_SRV_ID analogueSrvIden;
        VPP_CEC_OPERAND_DIG_SRV_ID      digSrvIden;
    }srvIden;
}VPP_CEC_OPERAND_TUNER_DEVICE_INFO;

// Analogue timer information
typedef struct tagVPP_CEC_OPERAND_ANALOGUE_TIMER_INFO
{
    UINT8                           dayOfMonth;
    UINT8                           monthOfYear;
    UINT8                           startTimeHour;
    UINT8                           startTimeMin;
    UINT8                           durationHour;
    UINT8                           durationMin;
    INT                             recSequence; // VPP_CEC_REC_SEQUENCE
    INT                             broadcastType; // VPP_CEC_ANALOGUE_BROADCAST_TYPE
    // 62.5 * frequency for the actual value
    UINT16                          frequency;
    INT                             broadcastSystem; // VPP_CEC_BROADCAST_SYSTEM
}VPP_CEC_OPERAND_ANALOGUE_TIMER_INFO;

// Digital timer information
typedef struct tagVPP_CEC_OPERAND_DIGITAL_TIMER_INFO
{
    UINT8                           dayOfMonth;
    UINT8                           monthOfYear;
    UINT8                           startTimeHour;
    UINT8                           startTimeMin;
    UINT8                           durationHour;
    UINT8                           durationMin;
    INT                             recSequence; // VPP_CEC_REC_SEQUENCE
    VPP_CEC_OPERAND_DIG_SRV_ID      digSrvID;
}VPP_CEC_OPERAND_DIGITAL_TIMER_INFO;

// External timer information
typedef struct tagVPP_CEC_OPERAND_EXT_TIMER_INFO
{
    UINT8                           dayOfMonth;
    UINT8                           monthOfYear;
    UINT8                           startTimeHour;
    UINT8                           startTimeMin;
    UINT8                           durationHour;
    UINT8                           durationMin;
    INT                             recSequence; // VPP_CEC_REC_SEQUENCE
    INT                             extSrcSpec; // VPP_CEC_EXT_SRC_SPECIFIER
    UINT8                           extPlugNum;
    UINT16                          extPhyAddr;
}VPP_CEC_OPERAND_EXT_TIMER_INFO;

// Timer Status Data
typedef struct tagVPP_CEC_OPERAND_TIMER_STATUS_DATA
{
    INT                             timerOverLapWarning; // VPP_CEC_TIMER_OVERLAP_WARNING_TYPE
    INT                             mediaInfo; // VPP_CEC_MEDIA_INFO
    INT                             pgmedIndicator; // VPP_CEC_TIMER_PGM_INDICATOR
    INT                             pgmedInfo; // VPP_CEC_TIMER_PGMED_INFO
    INT                             notPgmedErrInfo; // VPP_CEC_TIMER_NOT_PGMED_ERR_INFO
    // Indicates if duration is available to be loaded
    UINT8                           durationAvailable;
    UINT8                           availableDurHour;
    UINT8                           availableDurMin;
}VPP_CEC_OPERAND_TIMER_STATUS_DATA;
//audio req mode
typedef struct tagVPP_CEC_OPERAND_AUDIO_MODE_REQUEST
{
        BOOL audioMode; // TRUE -> ON , FALSE -> OFF
         UINT16        phyAddr;
}VPP_CEC_OPERAND_AUDIO_MODE_REQUEST;

// Audio Status
typedef struct tagVPP_CEC_OPERAND_AUDIO_STATUS
{
    BOOL        audioMuteStatus;
    // Value between 0x00 - 0x64, 0x7F for volume unknown
    UINT8       audioVolStatus;
}VPP_CEC_OPERAND_AUDIO_STATUS;

typedef struct tagVPP_CEC_AUDI0_FMT_WITH_ID
{
    UINT8    audioFmtID;  // 6-7 bit
    UINT8    audioFmtCode; // 0-5 bit
}VPP_CEC_AUDI0_FMT_WITH_ID;

typedef struct tagVPP_CEC_OPERAND_AUDIO_FMT_WITH_ID
{
    INT num;
    VPP_CEC_AUDI0_FMT_WITH_ID audioFmt[4];
}VPP_CEC_OPERAND_AUDIO_FMT_WITH_ID;

typedef struct tagVPP_CEC_OPERAND_AUDIO_SHORT_DESC
{
    INT num;
    UINT8 audioShortDesc[4][3];
}VPP_CEC_OPERAND_AUDIO_SHORT_DESC;
// Routing Change
typedef struct tagVPP_CEC_OPERAND_ROUTING_CHANGE
{
    UINT16      origPhyAddr;
    UINT16      newPhyAddr;
}VPP_CEC_OPERAND_ROUTING_CHANGE;

// String
typedef struct tagVPP_CEC_OPERAND_STRING
{
    UINT8   stringLen;
    UINT8   stringName[14];
}VPP_CEC_OPERAND_STRING;

// UI command
typedef struct tagVPP_CEC_OPERAND_UI_COMMAND
{
    INT  userControlCode; // VPP_CEC_USER_CONTROL_CODE
    INT  fnOperandPresent;
    union
    {
        INT                         playMode;       // VPP_CEC_PLAY_MODE
        VPP_CEC_OPERAND_CHN_IDEN    chnIden;
        UINT8                       uiFnMedia;
        UINT8                       uiFnSelAVIn;
        UINT8                       uiFnSelAudIn;
        UINT8                       uiFnSelSoundCtrl; // sound ppt ctrl
        UINT8                       uiFnSelBrdCstIn;
    }fnOperand;
}VPP_CEC_OPERAND_UI_COMMAND;

// Union of message operands
typedef union tagVPP_CEC_MSG_OPERAND
{
    UINT16                                  opPhyAddr;
    UINT8                                   opLanguage[3];
    VPP_CEC_OPERAND_FEATURE_ABORT           opFeatureAbort;
    VPP_CEC_OPERAND_REPORT_PHY_ADDR         opRepPhyAddr;
    INT                                     opMenuRequest; // VPP_CEC_MENU_REQUEST_TYPE
    INT                                     opMenuState; // VPP_CEC_MENU_STATE_TYPE
    VPP_CEC_OPERAND_UI_COMMAND              opUICommand;
    VPP_CEC_OPERAND_REC_SRC                 opRecSrc;
    INT                                     opRecStsInfo; // VPP_CEC_REC_STS_INFO
    INT                                     opDeckControlMode; // VPP_CEC_DECK_CONTROL_MODE

    INT                                     opDeckInfo; // VPP_CEC_DECK_INFO
    INT                                     opStatusRequest; // VPP_CEC_STATUS_REQUEST
    INT                                     opPlayMode; // VPP_CEC_PLAY_MODE
    VPP_CEC_OPERAND_OSD_STRING              opOsdString;
    VPP_CEC_OPERAND_OSD_NAME                opDevOsdName;
    INT                                     opPwrSts; // VPP_CEC_PWR_STS
    UINT32                                  opVendorID;
    UINT8                                   opVendorRCCode;
    VPP_CEC_OPERAND_VENDOR_SPEC_DATA        opVendorSpecData;
    VPP_CEC_OPERAND_DIG_SRV_ID              opDigSrvIden;
    VPP_CEC_OPERAND_ANALOGUE_SRV_ID         opAnalogueSrvIden;
    VPP_CEC_OPERAND_TUNER_DEVICE_INFO       opTunerDeviceInfo;

    VPP_CEC_OPERAND_ANALOGUE_TIMER_INFO     opAnalogueTimerInfo;
    VPP_CEC_OPERAND_DIGITAL_TIMER_INFO      opDigitalTimerInfo;
    VPP_CEC_OPERAND_EXT_TIMER_INFO          opExtTimerInfo;
    INT                                     opTimerClearedStsData;//VPP_CEC_TIMER_CLRED_STATUS_DATA
    VPP_CEC_OPERAND_TIMER_STATUS_DATA       opTimerStatusData;
    VPP_CEC_OPERAND_STRING                  opPgmTitleString;
    VPP_CEC_OPERAND_AUDIO_MODE_REQUEST      opSysAudioModeRequest; // added for system audio mode request
    INT                                     opAudioRateControl; // VPP_CEC_AUDIO_RATE_CONTROL
    VPP_CEC_OPERAND_AUDIO_STATUS            opAudioStatus;
    BOOL                                    opSysAudioStatus;
    VPP_CEC_OPERAND_AUDIO_FMT_WITH_ID        opAudioFormatWithID;
    VPP_CEC_OPERAND_AUDIO_SHORT_DESC        opAudioShortDesc;
    INT                                     opVersion; // VPP_CEC_VERSION_TYPE

    VPP_CEC_OPERAND_ROUTING_CHANGE          opRoutingChange;
}VPP_CEC_MSG_OPERAND, *PVPP_CEC_MSG_DATA;

// Structure of CEC message passed to/from application
typedef struct tagVPP_CEC_MSG
{
    UINT8                   srcAddr;
    UINT8                   destAddr;
    UINT16                  opcode;
    VPP_CEC_MSG_OPERAND     operand;
}VPP_CEC_MSG, *PVPP_CEC_MSG;

/*-----------------------------------------------------------------------------
 * HDMI CEC Event Notification Data
 *-----------------------------------------------------------------------------
 */
typedef struct VPP_HDMI_CEC_EVT_INFO_T {
    int     infoID;
    union
    {
        struct {
            int deviceType;
            int logAddrAllocSts;
            int logAddr;
        } logAddrStsEvtInfo;
        struct {
            int  txMsgOpcode;
            int  txStatus; // VPP_CEC_STATUS_TYPE
        } txStatusEvtInfo;
        struct {
            int  rxStatus; // VPP_CEC_STATUS_TYPE
            VPP_CEC_MSG cecMsg;
        } rxStatusEvtInfo;
    } evtInfo;
} VPP_HDMI_CEC_EVT_INFO;

typedef struct VPP_AUX_FRAME_INFO_T{
    UINT32     uSuccess;    /**< indicate that whether the capturing successes.
                                                SUCCESS means success,
                                                otherwise fail */
    UINT32     uSize;           /**< The memory size of captured video frame used*/
    INT          viewID;         /**< 0: left view  1:right view*/
}VPP_AUX_FRAME_INFO;

/* Data Structure to Get HDCP Aksv*/
typedef struct VPP_HDMI_HDCP_AKSV_T{
    UINT8       AksvBuf[5];
}VPP_HDMI_HDCP_AKSV;

#endif // __VPP_API_TYPES_H__
