/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 *
 */

#ifndef _VDEC_COM_H_
#define _VDEC_COM_H_

#if !defined(OVP_IN_TRUST_ZONE)
#include "com_type.h"
#include "cmd_buf.h"
#include "avxbv_buf.h"
#include "ctypes.h"
#include "vpu_api.h"
#endif

#ifdef ENABLE_DEBUG
#define LOG_STRM_EVENT(x)   //printf x
#define LOG_REF_EVENT(x)    //printf x
#define LOG_FRM_EVENT(x)    //printf x
#define LOG_RENDERING(x)    printf x
#define LOG_VDEC_EVENT(x)   printf x
#define LOG_PTS_EVENT(x)    printf x
#else
#define LOG_STRM_EVENT(x)
#define LOG_REF_EVENT(x)
#define LOG_FRM_EVENT(x)
#define LOG_RENDERING(x)
#define LOG_VDEC_EVENT(x)
#define LOG_PTS_EVENT(x)
#endif

#ifdef VID_CUT_DOWN_MEM
  #define ALWAYS_FORCE_PROGRESSIVE_FRAME
  #define BG2_CLOUD_DISP_CONFIG
  #define VP8_USE_STREAM_IN
#else
  #define FORCE_STREAM_SWITCH
#endif
#define NEW_DIVX_DEC

#ifdef FORCE_STREAM_SWITCH
#define VDM_ERROR_RECOVER_ENABLE
//#define VDM_SHOW_RESET_SKIP_FRAME
//#define VDM_DBG_RD_DELAY_UPDATE
#define VDM_DYN_FORCE_PROGRESSIVE_FRAME
#endif

#define VDM_DYN_DISABLE_DEBLOCK

#if (BERLIN_CHIP_VERSION >= BERLIN_B_0)
#define KEEP_PULLDOWN_INTERLACE_SEQ
#endif

//---------------------------------------------------------------------------
// Macros
//---------------------------------------------------------------------------
#ifndef OFFSETOF
#define OFFSETOF(s_,m_)         (long)&((((s_ *)0)->m_))
#endif
#ifndef ARRAYSIZEOF
#define ARRAYSIZEOF(x_)         (sizeof(x_)/sizeof(x_[0]))
#endif

#define HAL_REVISION            0
#define STREAM_BUFFER_SIZE      16384
#define STREAM_PAD_SIZE         1024
#define STREAM_READ_THRESH      2048
#define CONTROL_INFO_INDEX_SIZE 10

#define ROUND2CACHE(a)          (a)

#define MTU                     128
#ifdef FORCE_STREAM_SWITCH
#define MTU_LOW_THRESH          (64*1024)
#else
#define MTU_LOW_THRESH          (1024*128)
//#define MTU_HIGH_THRESH       (1024*1024)
#endif

#define RAW_BUFFER_LOW_WATER_MARK   MTU_LOW_THRESH


#ifdef BERLIN_AV_ENGINE
#define MAX_MATCH_NUMBER        128
#else
#define MAX_MATCH_NUMBER        32
#endif

#ifdef BERLIN_VDEC_128M
// Just want to save some memory
#if VID_CUT_DOWN_MEM
#define MAX_VDEC_STREAM         1
#else
#define MAX_VDEC_STREAM         2
#endif
#else
#define MAX_VDEC_STREAM         8
#endif

#define V_MAX_REF_COUNT         16
#define V_MAX_DIS_COUNT         16
/* Display and Reference Buffer Pool Definition
 *
 * V_POOL_MAX_SIZE  : The maximum byte of display pool size (MUST be 1K alignment).
 * V_POOL_MIN_SIZE  : The minimum byte of display pool size reserved for decoder use (MUST be 1K alignment).
 * V_POOL_MAX_BUF_NUMBER  : The maximum number of display buffer.
 *
 * R_POOL_MAX_SIZE  : The maximum byte of reference pool size (MUST be 1K alignment).
 * R_POOL_MIN_SIZE  : The minimum byte of reference pool size reserved for decoder use (MUST be 1K alignment).
 * R_POOL_MAX_BUF_NUMBER  : The maximum number of reference buffer.
 *
 */

#if defined(SOC_ES1)
#define V_POOL_MAX_SIZE   (32*1024*1024)
#define V_POOL_MIN_SIZE   (16*1024*1024)
#define V_POOL_MAX_BUF_NUMBER        16
#define V_POOL_MIN_BUF_NUMBER        3

#define R_POOL_MAX_SIZE   (16*1024*1024)
#define R_POOL_MIN_SIZE              0
#define R_POOL_MAX_BUF_NUMBER        32
#define R_POOL_MIN_BUF_NUMBER        1
#else

#ifdef VID_CUT_DOWN_MEM
#define V_POOL_MAX_SIZE   (0x3FD000*9) // MAX((1280*720*2+4096)*18, (1920*1088*2+4096)*7) = 28MB
#define V_POOL_MIN_SIZE   (0x3FD000*4)
#define V_POOL_MAX_BUF_NUMBER      24
#define V_POOL_MIN_BUF_NUMBER      4
#else
/* FIXME
 * Change to 40MB to support H264@5.1
 */
#define V_POOL_MAX_SIZE   (0x3FD000*9) // MAX((1280*720*2+4096)*18, (1920*1088*2+4096)*9) = 36MB
#define V_POOL_MIN_SIZE   (0x3FD000*4)
#define V_POOL_MAX_BUF_NUMBER      24
#define V_POOL_MIN_BUF_NUMBER      4
#endif

#ifdef VID_CUT_DOWN_MEM
  #define R_POOL_MAX_SIZE   (0x3FD000*6) // (1920*1088*2 + 4096)*6 = 20MB
#elif defined(PE_ENLARGE_REFERENCE_BUF)
#define R_POOL_MAX_SIZE   (0x3FD000*9) // (1920*1088*2 + 4096)*9 = 36MB /*Need customer to afford 48M additional share memory*/
#else
#define R_POOL_MAX_SIZE   (0x3FD000*6) // (1920*1088*2 + 4096)*6 = 24MB
#endif
#define R_POOL_MIN_SIZE      (0x3FD000)
#define R_POOL_MAX_BUF_NUMBER      24
#define R_POOL_MIN_BUF_NUMBER      1
#endif

/* Definition specific for JPEG decoding,which only require display buffer */
#define J_POOL_MAX_SIZE   (32*1024*1024)
#define J_POOL_MIN_SIZE   (0)
#define J_POOL_MAX_BUF_NUMBER        16
#define J_POOL_MIN_BUF_NUMBER        0

#define MAX_EVENT_CTX_SIZE      1024
#define XBV_BUFFER_TYPE_VIDEO   0

#define DEF_HAL_SIZE            128

#define MAX_STREAM_SEGMENT      (128*4)
#define DEMUX_SEGMENT_SIZE      768
#define MAX_CONTROL_INFO_NUM    64
#define XBV_BUFFER_SIZE         2048

#define STRM_DESC_FLUSH         1
#define STRM_DESC_NORMAL        0

#define DEFAULT_DISP_CHANNEL    0       // main channel

#define MAX_USER_DATA_TYPE      10

#if !defined(OVP_IN_TRUST_ZONE) && !defined(CONFIG_FASTLOGO)
// video formats
typedef enum
{
    MV_AV_VIDEO_TYPE_DUMMY  = FCC_GEN('D', 'M', 'M', 'Y'),
    MV_AV_VIDEO_TYPE_H264   = FCC_GEN('H', '2', '6', '4'),
    MV_AV_VIDEO_TYPE_H263   = FCC_GEN('H', '2', '6', '3'),
    MV_AV_VIDEO_TYPE_VC1    = FCC_GEN('V', 'C', '-', '1'),
    MV_AV_VIDEO_TYPE_MPEG4  = FCC_GEN('M', 'P', 'G', '4'),
    MV_AV_VIDEO_TYPE_MPEG2  = FCC_GEN('M', 'P', 'G', '2'),
    MV_AV_VIDEO_TYPE_MPEG1  = FCC_GEN('M', 'P', 'G', '1'),
    MV_AV_VIDEO_TYPE_JPEG   = FCC_GEN('J', 'P', 'E', 'G'),
    MV_AV_VIDEO_TYPE_MJPG   = FCC_GEN('M', 'J', 'P', 'G'),
    MV_AV_VIDEO_TYPE_DIV3   = FCC_GEN('D', 'I', 'V', '3'),
    MV_AV_VIDEO_TYPE_DIV4   = FCC_GEN('D', 'I', 'V', '4'),
    MV_AV_VIDEO_TYPE_VC1M   = FCC_GEN('M', '1', 'C', 'V'),
    //MV_AV_VIDEO_TYPE_RV30   = FCC_GEN('R', 'V', '3', '0'),
    //MV_AV_VIDEO_TYPE_RV40   = FCC_GEN('R', 'V', '4', '0'),
    MV_AV_VIDEO_TYPE_RV30   = FCC_GEN('R', 'V', '8', '\0'),
    MV_AV_VIDEO_TYPE_RV40   = FCC_GEN('R', 'V', '9', '\0'),
    MV_AV_VIDEO_TYPE_VP6   = FCC_GEN('V', 'P', '-', '6'),
    MV_AV_VIDEO_TYPE_SORENSON   = FCC_GEN('S', 'R', 'S', 'N'),
    MV_AV_VIDEO_TYPE_VP8   = FCC_GEN('V', 'P', '8', '\0'),
} VDEC_FORMAT_ID;
#endif

#define VINFO_FLAG_MASK_PTS_VALID       0x01
#define VINFO_FLAG_MASK_DATA_ERR        0x02
#define VINFO_FLAG_MASK_PTS_DISCON      0x04
#define VINFO_FLAG_MASK_UPD_CLK         0x08
#define VINFO_FLAG_MASK_START_PTS       0x10
#define VINFO_FLAG_MASK_STOP_PTS        0x20
#define VINFO_FLAG_MASK_PAUSE_PTS       0x40
#define VINFO_FLAG_MASK_SLIDE_SHOW      0x80
#define VINFO_FLAG_MASK_REPEAT_FRAME    0x100
#define VINFO_FLAG_MASK_SCALING_FRAME   0x200
#define VINFO_FLAG_MASK_FIELD_FRAME     0x400
#define VINFO_FLAG_MASK_HALF_FIELD      0x800
#define VINFO_FLAG_MASK_FIELD_FIRST     0x1000
#define VINFO_FLAG_MASK_SEEK_SHOW       0x40000



// define the meaning for m_grp_alloc_flag field
#define VDES_INFO_GRP_ALLOC             0x1
#define VDES_DESC_GRP_ALLOC             0x2

#ifdef PE_EVENT_DRIVEN

#define VDM_EVENT_DRIVEN
#define VDM_INTRA_MSG_AVXBV_LOW         0x01
#define VDM_INTRA_MSG_AVXBV_HIGH        0x02

#define VDM_LOW_WATERMARK_RATIO         (0.2)
#endif

typedef struct xvYCC_data_t
{
    UINT16 m_format;    /* 0: for black/red/green/blue vertice; 1: for red/green/blue range */
    UINT16 m_precision; /* see GBD Color Precision */
    UINT16 m_space;     /* see GBD Color Space */
    UINT16 m_d0;        /* black-Y or min red */
    UINT16 m_d1;        /* black-Cb or max red */
    UINT16 m_d2;        /* black-Cr or min green */
    UINT16 m_d3;        /* red-Y or max green */
    UINT16 m_d4;        /* red-Cb or min blue */
    UINT16 m_d5;        /* red-Cr or max blue */
    UINT16 m_d6;        /* green-Y or not used */
    UINT16 m_d7;        /* green-Cb or not used */
    UINT16 m_d8;        /* green -Cr or not used */
    UINT16 m_d9;        /* blue-Y or not used */
    UINT16 m_d10;       /* blue-Cb or not used */
    UINT16 m_d11;       /* blue -Cr or not used */
}xvYCC_DATA;

typedef enum
{
    VDEC_BUF_3D_FMT_CBI = 0,/* checherboard interleaving */
    VDEC_BUF_3D_FMT_CI,     /* column interleaving */
    VDEC_BUF_3D_FMT_RI,     /* row interleaving */
    VDEC_BUF_3D_FMT_SBS,    /* side by side */
    VDEC_BUF_3D_FMT_TAB,    /* top and bottom */
    VDEC_BUF_3D_FMT_FS,     /* frame sequential */
    VDEC_BUF_3D_FMT_FP,     /* frame packing (for HDMI input frame) */
    VDEC_BUF_3D_FMT_MAX,
}VDEC_BUF_3D_FORMAT;

typedef enum
{
    SBS_HORIZONTAL_MODE0 = 0,   /* SBS horizontal subsampling: L/Odd, R/Odd */
    SBS_HORIZONTAL_MODE1 = 1,   /* SBS horizontal subsampling: L/Odd, R/Even */
    SBS_HORIZONTAL_MODE2 = 2,   /* SBS horizontal subsampling: L/Even, R/Odd */
    SBS_HORIZONTAL_MODE3 = 3,   /* SBS horizontal subsampling: L/Even, R/Even */
    SBS_QUINCUNX_MODE0 = 4,     /* SBS QUINCUNX subsampling: L/Odd, R/Odd */
    SBS_QUINCUNX_MODE1 = 5,     /* SBS QUINCUNX subsampling: L/Odd, R/Even */
    SBS_QUINCUNX_MODE2 = 6,     /* SBS QUINCUNX subsampling: L/Even, R/Odd */
    SBS_QUINCUNX_MODE3 = 7,     /* SBS QUINCUNX subsampling: L/Even, R/Even */
}SBS_SUBSAMP_MODE;

#ifdef CONFIG_FASTLOGO
//Bootloader is in 64bit; so pointer will take 8bytes instead of 4 bytes, So port pointer accordingly
typedef UINT32 VOID_PTR;
typedef UINT32 UINT32_PTR;
typedef UINT32 UINT8_PTR;
#else
typedef void* VOID_PTR;
typedef UINT32* UINT32_PTR;
typedef UINT8* UINT8_PTR;
#endif

typedef struct vbuf_info_dv_meta_t
{
      /* pointer to  rpu_ext_config_fixpt_main_t  structure for composer config.
         The pointer shall be Virtual address for Clear World or physical for TZ */
    VOID_PTR   m_dv_com_cfg;
        /*pointer to dm_metadata_t structure for display manager config
          The pointer shall be Virtual address for Clear World or physical for TZ */
    VOID_PTR   m_dv_dm_seq;
} DV_META_INFO;

    /** Technicolor configure definition for m_thdr_present_mode other bits are reserved*/
#define AMP_THDR            (1 << 0)
#define AMP_THDR_METADATA   (1 << 1)

typedef struct vbuf_info_thdr_meta_t
{
        /* pointer to tHDR metadatastructure.  */
    VOID_PTR  m_sl_hdr1_metadata;
}THDR_META_INFO;

    /* Dolby Vision configure definition for m_dv_present_mode other bits are reserved*/

 /* Bit mask for dolby vision configurature on Base layer present. it shall be on (1) when dolby vision present*/
#define AMP_DOLBY_VISION_BL  (1 << 0)
    /* Bit mask for dolby vision configurature on Enhance layer present (1) or not (0) */
#define AMP_DOLBY_VISION_EL  (1 << 1)
    /* Bit mask for dolby vision configurature on Meta data present. it shall be on (1) when dolby vision present */
#define AMP_DOLBY_VISION_RPU (1 << 2)

typedef struct vbuf_info_t
{
    //INT32 m_strmID;               // stream ID assigned for the buffer
    INT32   m_bufferID;             // buffer Id in buffer pool
    UINT32  m_grp_alloc_flag;       // is the discript in group alloc. bit 0: info grp alloc, bit 1: Descriptor grp alloc.
    VOID_PTR m_pbuf_start;          // base address of buffer;
    UINT32  m_buf_size;             // size in bytes;
    UINT32  m_allocate_type;        // 0, unallocated, 1, grp_alloc, 2: self-alloc
    UINT32  m_buf_type;             // 0, unkown 1: disp buffer, 2 reference
    UINT32  m_buf_use_state;        // 0, free, 1, decoding, 2, ready_disp,
                                    //  3, in_display, 4, ready to use(displayed)
    UINT32  m_flags;                // bit 0: 0, following pts invalid, 1, following pts valid.
                                    // bit 1: 0, no err,1, have error/no displayable.
                                    // bit 2, 0, no clk change,  1, clk change, pts is new clk
                                    //
    UINT32  m_pts_hi;               // bit 33 of pts.
    UINT32  m_pts_lo;               // bit 0-32 of pts
    // data format in frame buffer
    UINT32  m_srcfmt;               // ARGB, YCBCR, .. etc.
    UINT32  m_order;                // ARGB, ABGR, BGRA, ... etc.
    UINT32  m_bytes_per_pixel;      // number of bytes per pixel
    // active video window in reference resolution
    UINT    m_content_offset;       // offset (in bytes) of picture full content in frame buffer
    UINT32  m_content_width;        // picture full content width
    UINT32  m_content_height;       //picture full content height
    INT32   m_pan_left[4];          // pan scan top_left position(in 1/16 pixels) of the content;
    INT32   m_pan_top[4];           // pan scan top_left position(in 1/16 pixels) of the content;
    INT32   m_pan_width[4];         // pan scan window(in pixels) in the content;
    INT32   m_pan_height[4];        // pan scan window(in pixels) in the content;


    INT32   m_active_left;          // x-coordination (in pixels) of active window top left in reference window
    INT32   m_active_top;           // y-coordination (in pixels) of active window top left in reference window
    UINT32  m_active_width;         // with of active in pixels.
    UINT32  m_active_height;        // height of active data in pixels.
    INT32   m_disp_offset;          //Offset (in bytes) of active data to be displayed
                                    // in reference active window in frame buffer
    UINT32  m_buf_stride;           // line stride (in bytes) of frame buffer
    UINT32  m_is_frame_seq;         // non zero if it is frame-only sequence
    UINT32  m_is_top_field_first;   // only apply to interlaced frame, 1: top field first out
    UINT32  m_is_repeat_first_field;// only apply to interlaced frame, 1: repeat first field.
    UINT32  m_is_progressive_pic;   // non zero if this frame is a progressive picture.
    UINT32  m_pixel_aspect_ratio;   // pixel aspect ratio(PAR) 0: unknown, High[31:16] 16bits,Numerator(width), Low[15:0] 16bits, Denominator(heigh)
    UINT32  m_frame_rate_num;       // Numerator of frame rate if available
    UINT32  m_frame_rate_den;       // Denominator if frame rate if available

    UINT32  m_luma_left_ofst;      // MTR mode, luma left cropping offset
    UINT32  m_luma_top_ofst;       // MTR mode, luma top cropping offset
    UINT32  m_chroma_left_ofst;    // MTR mode, chroma left cropping offset
    UINT32  m_chroma_top_ofst;     //MTR mode, chroma top cropping offset

    UINT8_PTR m_user_data_block[MAX_USER_DATA_TYPE];

    // m_clut_... for color look-up table
    UINT32_PTR m_clut_ptr;         // color lookup table for CI8 (valid when element m_bytes_per_pixel==1)
    //UINT32    m_clut_order;      // use m_order and m_srcfmt elements to decide the type of the clut
    UINT32    m_clut_start_index;  // start index in IO map to copy the clut
    UINT32    m_clut_num_items;    // length of the clut (number of clut entries)

    UINT32     m_is_right_eye_view_valid;   // indicate whether right eye view content valid.
    VOID_PTR   m_right_eye_view_buf_start;  // the buffer start address for right view content.
    VOID_PTR   m_right_eye_view_descriptor; // the descriptor of right eye view buffer,
    UINT32     m_number_of_offset_sequences;// present how many offset data is valid.
    UINT8      m_offset_meta_data[32];      // the offset meta data.

    UINT32 m_is_virtual_frame;              // indicate whether this is a virtual frame
    VOID_PTR   m_sec_field_start;           // the buffer start address for the second field.
    VOID_PTR   m_first_org_descriptor;      // the original frame discriptor of first field
    VOID_PTR   m_second_org_descriptor;     // the second frame discritor of second field.
    UINT32  usage_count;                    // field used count for return the buffer.

    VOID_PTR  m_surface[2];

    UINT32        m_is_xvYCC_valid;
    xvYCC_DATA    m_xvYCC_data;

    UINT32     m_is_UHD_frame;
    VOID_PTR   m_cmpr_psample;
    VOID_PTR   m_decmpr_psample;
    UINT32  m_is_preroll_frame;             //preroll frame will not be displayed. 1: prerool frame;  0:normal frame
    UINT32  m_display_stc_high;             //high 32-bit of STC clock when the frame is displayed currently not used
    UINT32  m_display_stc_low;              //low 32-bit of STC clock when the frame is displayed

    UINT32 m_3D_source_fmt;
    UINT32 m_3D_interpretation_type;
    UINT32 m_3D_SBS_sampling_mode;
    UINT32 m_3D_convert_mode_from_SEI;

    UINT32 m_interlace_weave_mode;      //indicate whether weave mode used for interlace
    UINT32 m_is_progressive_stream;     //indicate whether it is a progressive stream, it's different with m_is_progressive_pic
                                        //because m_is_progressive_pic may be changed based on film detection
    UINT32 m_3D_SwitchLeftRightView;
    UINT32 m_hBD;
#if defined(VPP_TZ_ENABLE) || defined(OVP_TZ_ENABLE) || defined(OVP_IN_TRUST_ZONE) || defined(CONFIG_MV_AMP_TEE_ENABLE)
    UINT32 m_hDesc;
#endif
    UINT32 m_bits_per_pixel;
    UINT32 m_buf_pbuf_start_UV;         //Start Address of UV data in 420SP format
    UINT32 m_buf_stride_UV;             //Stride of UV data in 420SP format
    UINT32 m_colorprimaries;
    UINT32 m_OrgInputInterlaced;
    UINT32 m_pbuf_Out_start_0;
    UINT32 m_pbuf_Out_start_UV_0;
    UINT32 m_pbuf_Out_start_1;
    UINT32 m_pbuf_Out_start_UV_1;
    UINT32 m_Org_frameStatus;       //Whether frame is originally progressive or generated from OVP
    UINT32 m_Out_buf1_valid;        //Whether m_pbuf_Out_start_1 is valid or not
    UINT32 m_Out_buf0_valid;        //Whether m_pbuf_Out_start_1 is valid or not
    UINT32 m_FrameDesc_RefStat;     //Which frame is in use by any module/component or not.
    UINT32 m_hBuf;                  //For frame capture

    UINT32 m_is_compressed;

    //Tile&420SP Auto Mode support
    VOID_PTR   m_pbuf_start_1;          //Start Address of Y data in 420SP format
    UINT32 m_buf_stride_1;              //Stride of Y data in 420SP format
    UINT32 m_buf_pbuf_start_UV_1;       //Start Address of UV data in 420SP format
    UINT32 m_buf_stride_UV_1;           //Stride of UV data in 420SP format
    UINT32 m_v_scaling;                 // 2 indicate 1/2 420sp downscale,4 indicate 1/4

    /**
        * HDR information for HDMI spec
    **/
    UINT16 sei_display_primary_x[3];
    UINT16 sei_display_primary_y[3];
    UINT16 sei_white_point_x;
    UINT16 sei_white_point_y;
    UINT32 sei_max_display_mastering_lumi;
    UINT32 sei_min_display_mastering_lumi;
    UINT32 transfer_characteristics;
    UINT32 matrix_coeffs;
    UINT32 MaxCLL;
    UINT32 MaxFALL;
    UINT32 builtinFrame;
    UINT8 alt_transfer_characteristics;

    /**
     * Extra HDR information for HLG
     */
    UINT8       m_primaries;
    UINT16      m_bits_per_channel;
    UINT16      m_chroma_subsampling_horz;
    UINT16      m_Chroma_subsampling_vert;
    UINT16      m_cb_Subsampling_horz;
    UINT16      m_cb_subsampling_vert;
    UINT8       m_chroma_siting_horz;
    UINT8       m_chroma_siting_vert;
    UINT8  m_color_range; //for RGB & YUV

    //For Technicolor
    INT32 m_iDisplayOETF;
    INT32 m_iYuvRange;        //0 - limited range, 1 - full range
    INT32 m_iColorContainer;  //0 - 709, 1- BT 2020
    INT32 m_iHdrColorSpace;   //0 - 709, 1 - 2020, 2- P3
    INT32 m_iLdrColorSpace;
    INT32 m_iPeakLuminance;   //Peak Luminance
    UINT16  m_uiBa;         //Modulation factor.
    INT32 m_iProcessMode;

    //For V-ITM
    INT m_vitmyuvRangeIn;
    INT m_vitmyuvRangeOut;
    INT m_vitmGaussFilterType;
    INT m_vitmYLutFilterType;
    INT m_vitmPQOutEnable;
    INT m_vitmPQLMax;
    INT m_vitmEnDebanding;
    INT m_vitmEnDenoising;

    //Technicolor Present Mode
    UINT32 m_thdr_present_mode;
    THDR_META_INFO m_thdr_meta;

    /**
      * Whether the dolby vision is present and configuatations use bit-wise mask to specify the presence of each elements.
      * current valid configures are 0(no present), 0x7(BL+EL+M) and 0x5(BL+M)
      **/
    UINT32 m_dv_present_mode;

    /** The pointer to the enhance layer VBUF_INFO descriptor. if zero, means match failed and not present. **/
    VOID_PTR   m_dv_el_descriptor;

    /**
       * Exceptions flag bits for dolby vision only BL descriptor was used.
       * current definitions: bit 0: whether the meta data is invalid (error).
       * all other bits are reserved.
       **/
    UINT32 m_dv_status;

    /** DV_META_INFO for parsed meta data information, only BL diescriptor was used. **/
    DV_META_INFO  m_dv_meta;
    UINT32 m_sar_width;
    UINT32 m_sar_height;
}  VBUF_INFO;

#if !defined(OVP_IN_TRUST_ZONE)
#define VDEC_STRM_BUF_DESC  vdec_strm_buf_desc_t
#define VDEC_PIC_DESC       vdec_pic_desc_t
#define VDEC_VID_BUF_DESC   vdec_vid_buf_desc_t
#define VDEC_SEQ_DESC       vdec_seq_desc_t
#define VDEC_EVENT_DESC     vdec_strm_status_t
#define VDEC_COM_BUF_DESC   vdec_buf_desc_t

#define VDEC_STRM_CONFIG    dshow_vdec_strm_config_t

#define VDEC_STRM_STATUS    vdec_strm_status_t
#define VDEC_HAL_CONFIG     vdec_config_t
#define VDEC_HAL_INFO       vdec_const_t

typedef INT32 (* VDEC_EVENT_CALLBACK) (VOID *pStrmCtx, VDEC_EVENT_DESC *pEventDesc);


typedef struct vdec_buf_req_t
{
    UINT32 m_buf_size;                  // buffer size requested
    UINT32 m_num_frames;                // number of frames need
} VDEC_BUF_REQ;

// StreamCtx structure
#endif

#endif

