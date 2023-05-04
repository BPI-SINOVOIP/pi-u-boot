/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 *
 */

//---------------------------------------------------------------------------
//! \file
//! \brief      VPU API
//! \author     Hongjie Guan
//---------------------------------------------------------------------------

#ifndef __VPU_API_H__
#define __VPU_API_H__

#ifdef __cplusplus
extern "C"
{
#endif

//---------------------------------------------------------------------------
//              Macros
//---------------------------------------------------------------------------
#ifndef TYPEDEF_STRUCT
#define TYPEDEF_STRUCT(x_)      typedef struct x_ x_; struct x_
#endif


enum VPU_HW_ID
{
    VPU_VMETA = 0,              // vmeta/vmeta_lite
    VPU_V2G   = 1,
    VPU_G2    = 2,
    VPU_G1    = 3,
    VPU_H1_0  = 4,
    VPU_H1_1  = 5,
    VPU_HW_IP_NUM,
};

//---------------------------------------------------------------------------
////! \brief      Basic information about VPU's IP info
////---------------------------------------------------------------------------
TYPEDEF_STRUCT(vpu_hw_info_t)
{
    unsigned int hw_version;                 //!< VPU H/W IP version supported by this library
    unsigned int rsvd;                       //!< Reserved.
    char        *version_string;             //!< Version string of the library.
    unsigned int vmeta_enable;
    unsigned int v2g_enable;
    unsigned int g2_enable;
    unsigned int g1_enable;
    unsigned int h1_0_enable;
    unsigned int h1_1_enable;
    unsigned int rsvd0;
    unsigned int rsvd1;
};


//---------------------------------------------------------------------------
//! \brief      Basic Constant Parameters of VPU HW IP (read-only)
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vpu_const_t)
{
    unsigned int    hw_version;                 //!< VPU H/W IP version supported by this library
    unsigned int    rsvd;                       //!< Reserved.
    char           *version_string;             //!< Version string of the library.
    unsigned int    vpu_context_size;           //!< Size (in bytes) of the context for software.
    unsigned int    vpu_hw_context_size;        //!< Size (in bytes) of the context for hardware.
    unsigned int    vdec_strm_context_size;     //!< Size (in bytes) of the decoder stream context for software.
    unsigned int    vdec_strm_hw_context_size;  //!< Size (in bytes) of the decoder stream context for hardware.
    unsigned int    venc_strm_context_size;     //!< Size (in bytes) of the encoder stream context.
    unsigned int    venc_strm_hw_context_size;  //!< Size (in bytes) of the encoder stream context for hardware.
};


//---------------------------------------------------------------------------
//! \brief      Basic Constant Parameters of Video Decoder (read-only)  //Obsolete, for backward compatibity only
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vdec_const_t)
{
    unsigned int    version;                    //!< Reserved.
    unsigned int    build;                      //!< Reserved.
    char           *version_string;             //!< Version string of the video decoder library.
    unsigned int    vdec_context_size;          //!< Size (in bytes) of the video decoder context for software.
    unsigned int    vdec_hw_context_size;       //!< Size (in bytes) of the video decoder context for hardware.
    unsigned int    strm_context_size;          //!< Size (in bytes) of the stream context for software.
    unsigned int    strm_hw_context_size;       //!< Size (in bytes) of the stream context for hardware.
    unsigned int    rsvd0;                      //!< Reserved.
    unsigned int    rsvd1;                      //!< Reserved.
};

//---------------------------------------------------------------------------
//! \brief      Basic Constant Parameters of Video Encoder (read-only)  //Obsolete, for backward compatibity only
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(venc_const_t)
{
    unsigned int    version;                    //!< Reserved.
    unsigned int    build;                      //!< Reserved.
    char           *version_string;             //!< Version string of the video encoder library.
    unsigned int    venc_context_size;          //!< Size (in bytes) of the video encoder context.
    unsigned int    venc_hw_context_size;       //!< Size (in bytes) of the video decoder context for hardware.
    unsigned int    rsvd0;                      //!< Reserved.
    unsigned int    rsvd1;                      //!< Reserved.
    unsigned int    strm_context_size;          //!< Size (in bytes) of the stream context.
    unsigned int    strm_hw_context_size;       //!< Size (in bytes) of the stream context for hardware.
};

//---------------------------------------------------------------------------
//! \brief      The Virtual external DMA's interface callback routines;
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(dma_drv_cfg_t)
{
    void           *dma_init;                //!< Function pointer for external dma init;
    void           *dma_config;              //!< Function pointer for external dma config;
    void           *dma_clear;               //!< Function pointer for external dma clear;
    void           *dma_push_cmd;            //!< Function pointer for push a dma cmd;
    void           *dma_get_cmdq_space;      //!< Function pointer for get cmdQ space and wptr;
    void           *dma_get_cmdq_count;      //!< Function pointer for get cmdQ counter and rptr;
    void           *dma_enable_empty_intr;   //!< Function pointer for enable empty interrupt;
    void           *dma_disable_empty_intr;  //!< Function pointer for disable empty interrupt;
    void           *dma_check_empty_intr;    //!< Function pointer to check DMA empty interrupt status;
};

//---------------------------------------------------------------------------
//! \brief        Linear Memory Structure
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(g1dec_linear_mem)
{
    unsigned int    *virtualAddress;
    unsigned int    busAddress;
    unsigned int    size;
};

//---------------------------------------------------------------------------
//! \brief      Stream-independent VPU Settings
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vpu_config_t)
{
    void            *user_data;           //!< User-defined decoder context data, not used by the video decoder.
    void            *internal_data;       //!< Reserved. For video decoder internal use only.
    unsigned int    io_base;              //!< Base address of the video decoder I/O space.
    unsigned int    fast_io_base;         //!< Reserved. For video decoder internal use only.
    unsigned int    hw_context_addr;      //!< Base address (4KB-aligned) of the video decoder context buffer for hardware.
    void            *hw_context_vaddr;    //!< Virtual address (4KB-aligned) of the video decoder context buffer for hardware.
    void            *io_rd32;             //!< Function pointer for 32-bit I/O read.
    void            *io_wr32;             //!< Function pointer for 32-bit I/O write.

    void            *external_clock_disable;    //!< Function pointer for external_clock_disable 1: disable; 0: enable.
    void            *reg32_read;          //!< Function pointer to access 32-bit register space.  //Reserved for future improvements
    void            *reg32_write;         //!< Function pointer to access 32-bit register space.  //Reserved for future improvements
    void            *reg32_fill;          //!< Function pointer to access 32-bit register space.  //Reserved for future improvements
    void            *reg32_copy;          //!< Function pointer to access 32-bit register space.  //Reserved for future improvements
    void            *mem32_read;          //!< Function pointer to access 32-bit physical memory. //Reserved for future improvements
    void            *mem32_write;         //!< Function pointer to access 32-bit physical memory. //for cabac opt
    void            *mem32_fill;          //!< Function pointer to access 32-bit physical memory. //Reserved for future improvements
    void            *mem32_copy;          //!< Function pointer to access 32-bit physical memory. //Reserved for future improvements

    void            *sl2_buf_drain;       //!< Function pointer to drain SL2 store buffer;
    void            *before_asic_start;   //!< Function pointer for do sth. befor asic start

    void            *dbg_write_file;      //!< Function pointer to write internal buffer data to file;
    void            *dbg_write_file_2d;   //!< Function pointer to write internal buffer data to file;
    dma_drv_cfg_t   dma_drv_cfg;          //!< Function pointer sets for external fake DMA driver implementation;
    void            *dbg_panic;		      //!< Reserved. For video decoder internal use only
    void            *dbg_printf;		  //!< Reserved. For video decoder internal use only
    void            *dbg_ahblog;		  //!< Reserved. For video decoder internal use only
    void            *dbg_dump_log;		  //!< Reserved. For video decoder internal use only
    unsigned int    hw_id;                //!< HW IP instance's ID

    void            *mem_malloc;         //!< Function pointer for memory allocate
    void            *mem_free;           //!< Function pointer for memory free
    void            *event_callback;     //!< Function Pointer to interrupt polling mode callback

    void            *low_v2g_clock;
    void            *high_v2g_clock;
	
	// for ahb dump
	void			*mem_p2v;
    unsigned int    mem_base_s;
    unsigned int    mem_base_d;
    unsigned int    mem_base_r;
    unsigned int    mem_base_t;
	unsigned int    mem_base_c;
	unsigned int    mem_base_m;
	unsigned int	mem_base_z;
};
typedef vpu_config_t vdec_config_t;
typedef vpu_config_t venc_config_t;


//---------------------------------------------------------------------------
//! \brief      VPU Return Codes
//---------------------------------------------------------------------------
//!             All API functions return non-negative code on succuess
//!             and negative error code on failure.
//---------------------------------------------------------------------------

#define VPU_RC_OK                          0
#define VPU_RC_PIC_HRDS_ERROR           30007
#define VPU_RC_SLICE_MISS               30006  // Not a fatal error; slice missing for non-ASO streams.
#define VPU_RC_UNSUPPORTED_SYNTAX       30005  // Not a fatal error; Informational
#define VPU_RC_INTERNAL_ERROR           -30001
#define VPU_RC_INVALID_ARGUMENT         -30002
#define VPU_RC_INVALID_CONFIG           -30003
#define VPU_RC_UNSUPPORTED_FORMAT       -30004
#define VPU_RC_UNSUPPORTED_RESOLUTION   -30005
#define VPU_RC_UNSUPPORTED_BITDEPTH     -30006
#define VPU_RC_HW_TIMEOUT               -30007
#define VPU_NON_EXIST_HW_IP             -30008
#define VPU_NON_IMPLEMENTED_ROUTINE     -30009

#define VDEC_RC_OK                      VPU_RC_OK
#define VDEC_RC_PIC_HRDS_ERROR          VPU_RC_PIC_HRDS_ERROR
#define VDEC_RC_SLICE_MISS              VPU_RC_SLICE_MISS
#define VDEC_RC_UNSUPPORTED_SYNTAX      VPU_RC_UNSUPPORTED_SYNTAX     //Obsolete, for backward compatibity only
#define VDEC_RC_INTERNAL_ERROR          VPU_RC_INTERNAL_ERROR         //Obsolete, for backward compatibity only
#define VDEC_RC_INVALID_ARGUMENT        VPU_RC_INVALID_ARGUMENT       //Obsolete, for backward compatibity only
#define VDEC_RC_INVALID_CONFIG          VPU_RC_INVALID_CONFIG         //Obsolete, for backward compatibity only
#define VDEC_RC_UNSUPPORTED_FORMAT      VPU_RC_UNSUPPORTED_FORMAT     //Obsolete, for backward compatibity only
#define VDEC_RC_UNSUPPORTED_RESOLUTION  VPU_RC_UNSUPPORTED_RESOLUTION //Obsolete, for backward compatibity only
#define VDEC_RC_UNSUPPORTED_BITDEPTH    VPU_RC_UNSUPPORTED_BITDEPTH   //Obsolete, for backward compatibity only
#define VDEC_RC_HW_TIMEOUT              VPU_RC_HW_TIMEOUT

#define VENC_RC_OK                      VPU_RC_OK
#define VENC_RC_INTERNAL_ERROR          VPU_RC_INTERNAL_ERROR         //Obsolete, for backward compatibity only
#define VENC_RC_INVALID_ARGUMENT        VPU_RC_INVALID_ARGUMENT       //Obsolete, for backward compatibity only
#define VENC_RC_INVALID_CONFIG          VPU_RC_INVALID_CONFIG         //Obsolete, for backward compatibity only
#define VENC_RC_UNSUPPORTED_FORMAT      VPU_RC_UNSUPPORTED_FORMAT     //Obsolete, for backward compatibity only
#define VENC_RC_HW_TIMEOUT              VPU_RC_HW_TIMEOUT


#define VXG_FLAG_USE_FOR_REF  (0x1)
#define VXG_FLAG_USE_FOR_DIS  (0x2)


//---------------------------------------------------------------------------
//              Video Decoder Data Structures
//---------------------------------------------------------------------------
enum
{
	// output format
    VDEC_OUTPUT_UYVY = 0,
    VDEC_OUTPUT_VYUY,
    VDEC_OUTPUT_YUYV,
    VDEC_OUTPUT_YVYU,
    VDEC_OUTPUT_SPUV,
    VDEC_OUTPUT_SPVU,
    VDEC_OUTPUT_YUV,
    VDEC_OUTPUT_YVU,
    VDEC_OUTPUT_TILE,
    VDEC_OUTPUT_NONE,
	VDEC_OUTPUT_AUTO,
	// kind of decoding mode instead of output format
	VDEC_OUTPUT_TILE_420SP_AUTO,
	VDEC_OUTPUT_MULTI_CHANNEL,
	VDEC_OUTPUT_FORMAT_NUM
};

enum
{
	VDEC_DISP_CH0 = 0,
	VDEC_DISP_CH1,
	VDEC_DISP_CH_NUM
};

enum
{
    VC1_PROFILE_SIMPLE = 0,     /** Simple profile */
    VC1_PROFILE_MAIN = 1,       /** Main profile */
    VC1_PROFILE_ADVANCED = 3,   /** Advanced profile */
};

enum
{
    H264_PROFILE_BASE       = 66,
    H264_PROFILE_MAIN       = 77,
    H264_PROFILE_HIGH       = 100,
};

enum
{
    MPEG2_PROFILE_SIMPLE    = 5,
    MPEG2_PROFILE_MAIN      = 4,
    MPEG2_PROFILE_HIGH      = 1,
};

enum
{
    MPEG2_LEVEL_LOW     = 10,
    MPEG2_LEVEL_MAIN    = 8,
    MPEG2_LEVEL_HIGH    = 4,
};

enum
{
    MPEG4_SP_L0 = 0x08,             /* Mpeg4 Simple profile level 0 */
    MPEG4_SP_L1 = 0x01,
    MPEG4_SP_L2 = 0x02,
    MPEG4_SP_L3 = 0x03,

    MPEG4_ASP_L0 = 0xf0,
    MPEG4_ASP_L1 = 0xf1,
    MPEG4_ASP_L2 = 0xf2,
    MPEG4_ASP_L3 = 0xf3,
    MPEG4_ASP_L4 = 0xf4,
    MPEG4_ASP_L5 = 0xf5,
};

enum
{
    SEI_PIC_STRUCT_FRAME = 0,
    SEI_PIC_STRUCT_TOP_FIELD,
    SEI_PIC_STRUCT_BOTTOM_FIELD,
    SEI_PIC_STRUCT_TOP_BOTTOM,
    SEI_PIC_STRUCT_BOTTOM_TOP,
    SEI_PIC_STRUCT_TOP_BOTTOM_TOP,
    SEI_PIC_STRUCT_BOTTOM_TOP_BOTTOM,
    SEI_PIC_STRUCT_FRAME_DOUBLE,
    SEI_PIC_STRUCT_FRAME_TRIPLE,
    SEI_PIC_STRUCT_RESERVED
};

// for display rotaion option
enum
{
    VDIS_ROTATION_NONE      = 0,
    VDIS_ROTATION_RIGHT_90  = 1,
    VDIS_ROTATION_180,
    VDIS_ROTATION_LEFT_90,
    VDIS_ROTATION_HOR_FLIP,
    VDIS_ROTATION_VER_FLIP,
};

enum
{
    TILE_8BIT_MODE_AUTO  = 0,
    TILE_8BIT_MODE_H8V4  = 1,
    TILE_8BIT_MODE_V4H8  = 2,
};

// mmu feature
enum
{
	VXG_MMU_DIS_FEATURE	 = 0x0,			// disable all mmu feature
	VXG_MMU_VM_ENABLE	 = 0x1,			// enable mmu virtual address to physical address mapping
	VXG_MMU_BM_ENABLE	 = 0x2,			// enable mmu bank mapping
	VXG_MMU_PM_ENABLE	 = 0x4,			// enable mmu page mapping
	VXG_MMU_SH_ENABLE	 = 0x8,			// enable mmu address shuffle

	VXG_MMU_PM_PH_4		 = 0x00000000, 	// MMU PAGE MAPPING PAGE HEIGHT = 4
	VXG_MMU_PM_PH_8      = 0x00010000, 	// MMU PAGE MAPPING PAGE HEIGHT = 8
	VXG_MMU_PM_PH_16     = 0x00020000, 	// MMU PAGE MAPPING PAGE HEIGHT = 16
	VXG_MMU_PM_PH_32     = 0x00030000, 	// MMU PAGE MAPPING PAGE HEIGHT = 32
	VXG_MMU_PM_PH_64     = 0x00040000, 	// MMU PAGE MAPPING PAGE HEIGHT = 64
	VXG_MMU_PM_PH_128    = 0x00050000, 	// MMU PAGE MAPPING PAGE HEIGHT = 128
		
	VXG_MMU_PM_PW_64     = 0x00000000, 	// MMU PAGE MAPPING PAGE WIDTH = 64
	VXG_MMU_PM_PW_128    = 0x00100000, 	// MMP PAGE MAPPING PAGE WIDTH = 128
};

//---------------------------------------------------------------------------
//! \brief      Per-stream Decoder Settings
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vdec_strm_config_t)
{
    void           *user_data;              //!< User-defined stream context data, not used by the video decoder.
    void           *internal_data;          //!< Reserved. For video decoder internal use only.
    void           *vdec_cntxt;             //!< Pointer to the video decoder context.
    void           *base_strm;              //!< Pointer to the stream context of the base stream. Only used in H.264 MVC.
    unsigned int    format;                 //!< Stream format ID.

    unsigned int    hw_context_addr;        //!< Base address (4KB-aligned) of the stream context buffer for hardware.
    void           *hw_context_vaddr;       //!< Virtual adress (4KB-aligned) of the stream context buffer for hardware.
    unsigned int    strm_pool_addr;         //!< Base address of the stream buffer pool.
    unsigned int    strm_pool_size;         //!< Size (in bytes) of the stream buffer pool.
    unsigned int    mon_width;              //!< Preferred display buffer/monitor width in pixels. 0 = auto.
    unsigned int    mon_height;             //!< Preferred display buffer/monitor height in pixels. 0 = auto.
    unsigned int    fmtc_in_stride;         //!< format convert input stride.
    unsigned int    min_strm_size;          //!< Reserved. For video decoder internal use only.
    unsigned int    no_reordering;          //!< Nonzero to disable the display buffer reordering.
    unsigned int    ref_padding_mode;       //!< Padding mode for reference picture (MPEG4 only).
                                            //      0: auto, 1: aligned pic boundary, 2: actual pic boundary
    unsigned int    pp_scan_mode;           //!< Source scan mode (for post-processing only):
                                            //      0: auto, 1: force progressive, 2: force interlaced
    unsigned int    pp_filter_mode;         //!< Up-sampling filter mode (for post-processing only):
                                            //      0: auto, 1: line repeat, 2: multi-tap filter
    unsigned int    pp_hscale;              //!< Horizontal down-scaling ratio (vMeta JPEG only), obsolete
                                            //      0 or 1: no scaling, 2: 1/2, 4: 1/4, 8: 1/8
    unsigned int    pp_vscale;              //!< Vertical down-sampling ratio
                                            //      0 or 1: no scaling, N: 1/N: 2: 1/2, 3: 1/3, 4: 1/4, up to 1/16
    unsigned int    pp_hscale_numer;        //!< Horizontal down-scaling ratio of numerator (support from BG4CDP-A0)
    unsigned int    pp_hscale_denom;        //!< Horizontal down-scaling ratio of denominator(support from BG4CDP-A0)

    unsigned int    dblk_mode;              //!< Advanced setting for deblocking filter
                                            //      0=auto; 1=normal; 2=ref-only; 3=I-only; 4=Disable;
    unsigned int    mvc_mode;               //!< Advanced setting for H.264 MVC support
                                            //      0=auto; 1=disabled; 2=enabled;
    unsigned int    aso_mode;               //!< Advanced setting for H.264 ASO support
                                            //      0=auto; 1=disabled; 2=enabled;
    unsigned int    slc_opt_flag;           //!< 1: apply multi-slc opt for H264;   0: else
    unsigned int    encry_flag;             //!< 0: if clear ES in DDR;  else: encrypted ES in DDR;
    unsigned int    disable_mem32_read;     //!< 0: use mem32_read ;  1: disable mem32_read;
    unsigned int    disable_vld_dcg;        //!< Flag to  disable VLD module's dynamic clock gating: temporary flag for V2.2

    unsigned int    disable_gmc23;          //!< for MPEG4 GMC, 1: reset pointnum = 0 if pointnum >= 2
    unsigned int    enable_sei_3d;          //

    void           *dma_obj;                //!< The external DMA object;

    unsigned int    roi_x;                  //!< ROI (region of interest) start location in horizontal direction (after down-scaling, unit in pixel) (JPEG only)
    unsigned int    roi_y;                  //!< ROI start location in vertical direction (after down-scaling, unit in pixel) (JPEG only)
    unsigned int    roi_w;                  //!< ROI width  (after down-scaling, unit in pixel) (JPEG only)
    unsigned int    roi_h;                  //!< ROI height (after down-scaling, unit in pixel) (JPEG only)

    unsigned int    slc_row;
    unsigned int    frame_in_mode;          //!< 0: stream-in mode; 1: frame-in mode; 2: frame-in turbo mode, remove 32B align

    unsigned int    p_frame_seek;           //!< setting for P Frame seek (all intra mb),h263/mpeg4
                                            //      0=disable; 1=enable
    unsigned int    idr_seek;               //!< seek only from IDR, for h264 only
    unsigned int    idr_err_skip;           //!< if error in IDR (e.g.slice miss), skip frames untill next I frame.

    unsigned int    post_deblocking;        //!< flag for post deblocking (now for mpeg4 only)
                                            //      0=disable; 1=enable
    unsigned int    user_dpb_size;          // for external dpb setting; 0 = disable;

    unsigned int    rvcombo_flag;           // indicate that it is an rv combo stream;
    unsigned int    output_mode;            // VDEC_OUTPUT_UYVY...
	unsigned int	output_mode_ch[VDEC_DISP_CH_NUM];
											// two display channel, possible value : VDEC_OUTPUT_UYVY...VDEC_OUTPUT_NONE
											// currently ch[0] should be always set as the same with reference_mode (VDEC_OUTPUT_TILE) or
											// VDEC_OUTPUT_NONE (do not need ch[0]'s output) or VDEC_OUTPUT_AUTO (vpu decide)!!!
											// otherwise ch[0] will be modified as the same with reference_mode
											// VDEC_OUTPUT_UYVY ... VDEC_OUTPUT_TILE, what you set is what you get
											// VDEC_OUTPUT_AUTO, vpu decide
											// VDEC_OUTPUT_NONE, no output
    unsigned int    tile_ref_flag;          // 0:  no separate reference buffer, ref format is same as output_mode;  1:  use tile reference
    unsigned int    tile_8bmode;            // 0:  auto   1: TILE_H8V4   2: TILE_V4H8
    unsigned int    no_res_change;          // 0:resolution change allowd, 1: resolution change forbidden

    unsigned int    force_ref_out;          // 1: output reference buffer even for non-refence picture;
    unsigned int    disable_dis_out;        // 1: don't output display buffer;

    unsigned int    vxrd_paddr;             // only for BG3 with two vMeta instances;
    unsigned int    p3wr_paddr;             // only for BG3 with two vMeta instances;

    unsigned int    rotation_allow;         // 0: NOT rotate, 1: maybe rotate for some pics
    unsigned int    rotate_mode;            // 0: original, 1: 90 degree, 2: 180 degree, 3: 270 degree, 4: horizontal flip, 5, vertical flip

    // for vmeta only
    unsigned int    disable_min_strm_size;  // decoding would kick off 0: min_strm_size = 4k; 1: (min_strm_size = 4k) || (sbuf_load - sbuf_free) >= 8
    unsigned int    dpb_force_flush;        // 1: force flush dpb@sps header for error handling.

    // hantro g1 field
    unsigned int    skipNonReference;       /* Flag to enable decoder skip non-reference * frames to reduce processor load */
    unsigned int    dither_enable;          // 0: diable dithering, if the stream is 10 bit, the display is 10bit;
    unsigned int    dither_bit;             // for 10bit stream: 0: 8b  1: 10b  2: 16bit

    unsigned int    dbg_log_enable;

    unsigned int    ref_stride;             //if set, will use this to calculate buffer size and configure VCU and PFU, it is of byte unit: address diff between two continous TILE row;
    unsigned int    dis_stride;             //if set, will use this to calculate buffer size and configure PFU;
    unsigned int    mtuTile;                //relative with PFU display burst length, set 0 as default;
    unsigned int    mtuDisp;                //relative with PFU TILE burst length, set 0 as default;
    unsigned int    mtuDispType;            //output mode for display: raster, 8x4, 6x4 or DWA mode. set 0 (raster) as default.
    unsigned int    fetch_collect;          //if set, will use this to configure VCU: default is 0, can try 3;
    unsigned int    max_support_layer;      //[H265 only]: the supported maximum temporal layers: starting from 1.
    unsigned int    buf_combined_mode;      //0: separate dis & ref buffer, 1: combine dis & ref buffer
    unsigned int    enable_deinterlace;     //0: disable 1: enable deinterlace for interlaced stream (only for g1 H264)
	unsigned int    vp8_strm_mode;          //!< VP8 input stream mode: 0: RAW Stream 1: ivf file
    unsigned int    dis_offset_uv;          // UV offset in display buffer of 420SP mode; or U offset in display buffer of 420P mode;
    unsigned int    dis_offset_uv1;         //  V offset in display buffer of 420P mode;
	unsigned int    mtr_enable;             //  MTR can only be enabled from BG4CDPA0;
	unsigned int    mtr_x_align_mode;       //  0:  align to coded frame boundary in x direction;   1: align to deblocking boundary in x direction;
    unsigned int    mtr_meta_base;          // meta data buffer base address, should be 4k align, size = 1M (from bg6ct)
	unsigned int    qos_w_high;             // high QoS for write
	unsigned int    qos_w_low;              // low Qos for write
	unsigned int    qos_r_high;             // high Qos for read
	unsigned int    qos_r_low;              // low Qos for read
	
	unsigned int    h265_tile_opt;          // flag for h265 multi-tile opt
	unsigned int    h264_low_latency_output;// flag for h264 progressive stream, output display buffer immediately after end of pic
	
	unsigned int	mmu_enable;				// refer to enum {VXG_MMU_xxx, }
	unsigned char	mmu_page_size;			// 0~5: 4K | 16K | 64K | 256K | 1M | 2M
};

//---------------------------------------------------------------------------
//! \brief      Sequence Descriptor
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vdec_seq_desc_t)
{
    void           *user_data;              //!< User-defined data, not used by the video decoder.
    void           *internal_data;          //!< Reserved. For video decoder internal use only.
    unsigned int    max_width;              //!< Maximum coded picture width (in pixels) of the whole sequence.
    unsigned int    max_height;             //!< Maximum coded picture height (in pixels) of the whole sequence.
    unsigned int    is_new_seq;             //!< Nonzero if it is a new sequence (not a repeated sequence header).
    unsigned int    is_intl_seq;            //!< Nonzero if it is an interlaced sequence. Not always reliable.
    unsigned int    dis_buf_size;           //!< Size (in bytes) of the display frame buffer.
	unsigned int	dis_size_y;				//!< Size (in bytes) of the display frame buffer for LUMA.
	unsigned int	dis_size_uv;			//!< Size (in bytes) of the display frame buffer for CHROMA.
    unsigned int    ref_buf_size;           //!< Size (in bytes) of the reference frame buffer. in g1 it's the total buffer usage in dpb
    unsigned int    ref_size_y;             //!< Size (in bytes) of the reference frame buffer for LUMA.
    unsigned int    ref_size_uv;            //!< Size (in bytes) of the reference frame buffer for CHROMA.
    unsigned int    tctx_buf_size;          //!< Size (in bytes) of the tctx buffer. (from bg6ct)
    unsigned int    meta_buf_size;          //!< Size (in bytes) of the meta data buffer if enable compression. (from bg6ct)
	unsigned int	separate_pix_buf;		//!< whether to separate pixel buffer
    unsigned int    num_dis_bufs;           //!< Informative. Number of display frame buffers required by reordering.
    unsigned int    num_ref_bufs;           //!< Informative. Number of reference frame buffers required by decoding.
    unsigned int    frame_rate_num;         //!< Numerator of the frame rate, if available.
    unsigned int    frame_rate_den;         //!< Denominator of the frame rate, if available.
    unsigned int    bit_rate;               //!< bit rate, if available
    unsigned int    sar_width;              //!< Horizontal size of the sample aspect ratio, if available.
    unsigned int    sar_height;             //!< Vertical size of the sample aspect ratio, if available.
    unsigned int    src_video_format;       //!< Source video format, if available.
                                            //      0: unknown, 'COMP': component,
                                            //      'PAL': PAL, 'NTSC': NTSC, 'SECA': SECAM, 'MAC': MAC
    unsigned short  dis_width;              //!< Width (in pixels) of the display window.
    unsigned short  dis_height;             //!< Height (in pixels) of the display window.
    short           dis_left;               //!< Horizontal offset (in pixels) of the display window relative to the coded picture.
    short           dis_top;                //!< Vertical offset (in pixels) of the display window relative to the coded picture.

    unsigned int    level_idc;
    unsigned int    profile_idc;
    unsigned int    max_reorder_frames;     //!< max of the reorder frames.
    unsigned int    poc_type;               //!< pic_order_cnt_type

	// for jpeg only
    unsigned int    mcu_width;              // if mcu_width = 8, do not apply pp_hscale = 8;
    unsigned int    is_progressive_jpeg;    // 0: baseline jpeg, 1: progressive jpeg
    unsigned int    unsupported_dct_type;   // 0: baseline DCT supported, >0 :unsupported DCT type
    unsigned int    number_of_images;       // number of images.
    unsigned int    image_size[4];          //
    unsigned int    image_offset[4];

    // hantro g1 field
    unsigned int	total_mbs_in_pic;     //!< mb number in a picture
    unsigned int	outputFormat;         /* format of the output picture */

    // for h265 only
    unsigned int    bit_depth;              //!< decoded bit depth of the stream 8bit or 10bit etc.
    unsigned int    max_temporal_layer;     //!< maximum temporal scaling layer
    unsigned int    color_primaries;        //!< hevc SEI information
};


//---------------------------------------------------------------------------
//! \brief      Common Physcical DRAM Memory Descriptor
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vdec_buf_desc_t)
{
    void           *user_data;              //!< User-defined data, not used by the video decoder.
    void           *internal_data;          //!< Reserved. For video decoder internal use only.
    unsigned int    type;                   //!< Type of the buffer.
    unsigned int    paddr;                  //!< Base physical address of the buffer.
    void           *vaddr;                  //!< Base virtual address of the buffer.
    unsigned int    data_size;              //!< Size (in bytes) of the valid data.
    unsigned int    buf_size;               //!< Size (in bytes) of the buffer.
    unsigned int    id;                     //!< Buffer ID assigned by the buffer allocator, not used by the video decoder.
    unsigned int    sn;                     //!< Reserved. For video decoder internal use only.
    unsigned int    pos;                    //!< Reserved. For video decoder internal use only.
    void           *side_info;              //!< Side information to further describe the buffer content.
};

//---------------------------------------------------------------------------
//! \brief      Decoder Picture Descriptor
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vdec_pic_desc_t)
{
    void           *user_data;              //!< User-defined data, not used by the video decoder.
    void           *internal_data;          //!< Reserved. For video decoder internal use only.
    unsigned short  coded_width;            //!< Width (in pixels) of the coded frame.
    unsigned short  coded_height;           //!< Height (in pixels) of the coded frame.
    unsigned short  ds_width;               //!< Width (in pixels) of the downscale frame.
    unsigned short  ds_height;              //!< Height (in pixels) of the downscale frame.
    unsigned short  h_scaling;              //!< Horizontal up-scaling ratio of the coded frame. 0: no scaling, obsolete.
    unsigned short  v_scaling;              //!< Vertical up-scaling ratio the coded frame. 0: no scaling, obsolete.
    unsigned short  dis_width;              //!< Width (in pixels) of the display window.
    unsigned short  dis_height;             //!< Height (in pixels) of the display window.
    short           dis_left;               //!< Horizontal offset (in pixels) of the display window relative to the coded picture.
    short           dis_top;                //!< Vertical offset (in pixels) of the display window relative to the coded picture.
	unsigned short  valid_dis_width;		//!< Width (in pixels) of the valid display window, only applicable for VP9 now, for the other format the value should be the same with dis_width.
	unsigned short  valid_dis_height;		//!< Height (in pixels) of the valid display window, only applicable for VP9 now, for the other format the value should be the same with dis_width.
    unsigned int    total_mbs_in_pic;       //!< Total number of macroblcoks in the picture.
    unsigned int    error_mbs_in_pic;       //!< Total number of error macroblcoks in the picture.
    unsigned int    slice_miss;             //!< 0:no slice miss; 1:slice miss
    unsigned int    slice_qp;               //!<
    unsigned int    total_bits_in_pic;      //!< bit lenghth of the picture.

    unsigned char   is_there_idr;           //!< Nonzero if there is no idr frame for current frame.
    unsigned char   is_valid;               //!< Nonzero if it is a valid picture.
    unsigned char   is_skipped;             //!< Nonzero if it the picture is skipped by user.
    unsigned char   is_aborted;             //!< Nonzero if it is aborted by decoder, for pictures which can(should) not be decoded totally.
    unsigned char   is_idr;                 //!< Nonzero if it is an IDR picture.
    unsigned char   is_new_sps;             //!< Nonzero if new sps detected.

    unsigned char   is_ref;                 //!< Nonzero if it is a reference picture.
    unsigned char   is_fld;                 //!< Nonzero if it is a field picture.
    unsigned char   is_btm;                 //!< Nonzero if it is a bottom field picture.
    unsigned char   is_intl;                //!< Nonzero if it is interlaced (field or interlaced frame picture).
    unsigned char   is_coded;               //!< Nonzero if it is a coded picture (not a skip picture).
    unsigned char   is_open_gop_b;          //!< Nonzero if it is a B picture before I pic (in display order) at the start of an open gop.
    unsigned char   sei_pic_struct;         //!< SEI pic struct for display
    unsigned char   pd_top_field_first;     //!< Pull-down parameter. Nonzero if the top field needs to be output first.
    unsigned char   pd_repeat_first_field;  //!< Pull-down parameter. Nonzero if the first field needs to be repeated.
    unsigned char   pd_repeat_frame_count;  //!< Pull-down parameter. Number of times the whole frame needs to be repeated.
    unsigned char   reserved1;              //!< Reserved.
    unsigned char   reserved2;              //!< Reserved.

    unsigned int    ps_num_vectors;         //!< Number of available pan-scan vectors.
    unsigned short  ps_width[4];            //!< Width (in pixels) of the pan-scan window.
    unsigned short  ps_height[4];           //!< Height (in pixels) of the pan-scan window.
    int             ps_left[4];             //!< Horizontal offset (in 1/16 pixels) of the pan-scan window relative to the diplay window.
    int             ps_top[4];              //!< Vertical offset (in 1/16 pixels) of the pan-scan window relative to the diplay window.
    unsigned int    pic_type;               //!< Unified picture type ('I', 'P', 'B').
    unsigned int    native_pic_type;        //!< Native picture type defined by the video standard.
    int             poc;                    //!< Picture order count of the current picture.
    int             fld_poc[2];             //!< [0]: top poc, [1]: bottom poc, only valid for interlaced frame picture.
    unsigned int    stream_pos;             //!< 29-bit offset (in bytes) of the picture start code in the input stream.
    unsigned int    coded_pic_idx;          //!< Picture index in decoding order.
    unsigned int    output_delay;           //!< Reserved. For video decoder internal use only.
    unsigned int    chksum_cnt;             //!< Reserved. For video decoder internal use only.
    unsigned int    chksum_data[10];        //!< Reserved. For video decoder internal use only.
    unsigned int    perf_cntr[4];           //!< Reserved. For video decoder internal use only.
    unsigned int    internal_flags;         //!< Reserved. For video decoder internal use only.
    unsigned int    rotate_mode;            // 0: original, 1: 90 degree, 2: 180 degree, 3: 270 degree, 4: horizontal flip, 5, vertical flip
    unsigned int    dis_offset_r;           //!< Offset (in bytes) of valid data in a display frame buffer for rotation
    unsigned int    dis_stride_r;           //!< Stride (in bytes) of the display frame bufferfor rotation
    unsigned int    temporal_layer;         //!< [H265 only] temporal layer of this picture, starting from 1;

    // for channel 0
    unsigned int    is_compressed;          //whether the current buffer is in compressed format
    unsigned int    output_mode;            // VDEC_OUTPUT_UYVY...
    unsigned int    dis_offset;             //!< Offset (in bytes) of valid data in a display frame buffer.
    unsigned int    dis_stride;             //!< Stride (in bytes) of the display frame buffer.
    unsigned int    dis_offset_uv;          //!< Offset (in bytes) of chroma(420sp | 420p u) in a display frame buffer.
    unsigned int    dis_stride_uv;          //!< Stride (in bytes) of chroma in the display frame buffer.
    unsigned int    dis_offset_uv1;         //!< Offset (in bytes) of chroma(420p v) in a display frame buffer.

    unsigned int    luma_left_ofst;			//!< the left offset from base to real coded of luma
    unsigned int    luma_top_ofst;			//!< the top offset from base to real coded of luma
    unsigned int    chroma_left_ofst;		//!< the left offset from base to real coded of chroma
    unsigned int    chroma_top_ofst;		//!< the top offset from base to real coded of chroma

	// for channel 1
    unsigned int    ds_output_mode;         // VDEC_OUTPUT_UYVY..., output mode of downscale frame
	unsigned int	ds_bitdepth;
    unsigned int    ds_offset;              //!< offset of downscale display frame in (tile reference + downscale display) buffer.
    unsigned int    ds_stride;              //!< Stride (in bytes) of the downscale display frame buffer.
    unsigned int    ds_offset_uv;           //!< Offset (in bytes) of chroma in downscale display frame buffer.
    unsigned int    ds_stride_uv;           //!< Stride (in bytes) of chroma in downscale display frame buffer.

    unsigned short  sei_display_primary_x[3];
    unsigned short  sei_display_primary_y[3];
    unsigned short  sei_white_point_x;
    unsigned short  sei_white_point_y;
    unsigned int    sei_max_display_mastering_lumi;
    unsigned int    sei_min_display_mastering_lumi;

    unsigned int    transfer_characteristics;
    unsigned int    matrix_coeffs;

    unsigned int    mtr_chksum_cnt[4];      //!< check sum counter for MTR.
    unsigned int    mtr_chksum_data[4][10];  //!< check sum date for MTR.

    unsigned int    mtr_meta_width;         //!< Number of meta data columns that are valid in the meta data buffer. Unit: meta packet. (from bg6ct)
    unsigned int    mtr_meta_height;        //!< Number of meta data rows that are valid in the meta data buffer. Unit: meta packet.    (from bg6ct)
    unsigned int    mtr_meta_stride;        //!< meta data buffer stride for luma   (from bg6ct)
    unsigned int    mtr_meta_width_uv;      //!< Number of meta data columns that are valid in the meta data buffer. Unit: meta packet. (from bg6ct)
    unsigned int    mtr_meta_height_uv;     //!< Number of meta data rows that are valid in the meta data buffer. Unit: meta packet.    (from bg6ct)
    unsigned int    mtr_meta_stride_uv;     //!< meta data buffer stride for chroma (from bg6ct)
    unsigned int    mtr_meta_offset_uv;     //!< meta data buffer uv offset			(from bg6ct)
};

//---------------------------------------------------------------------------
//! \brief      Side Information for Video Display Buffer
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vdec_dbuf_side_info_t)
{
    vdec_pic_desc_t pic[2];                 //!< Picture descriptor pair. [0]: frame/top, [1]: bottom.
};

//---------------------------------------------------------------------------
//! \brief      Decoder Video Buffer Descriptor
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vdec_vid_buf_desc_t)
{
    void           *user_data;              //!< User-defined data, not used by the video decoder.
    void           *internal_data;          //!< Reserved. For video decoder internal use only.
    unsigned int    rsvd1;                  //!< Reserved. For video decoder internal use only.
    unsigned int    base_addr;              //!< Base address of the frame buffer. (Physical address)
    void           *base_addr_virtual;      //!< Base address of the frame buffer. (Virtual Address)
    unsigned int    size;                   //!< buffer size
    unsigned int    rsvd4;                  //!< Reserved. For video decoder internal use only.
    unsigned int    id;                     //!< Buffer ID assigned by the buffer allocator, not used by the video decoder.
    unsigned int    sn;                     //!< Reserved. For video decoder internal use only.
    unsigned int    rsvd5;                  //!< Reserved. For video decoder internal use only.
    void           *rsvd6;                  //!< Reserved. For video decoder internal use only.
    unsigned int    used_flag;              //!< (FLAG_USE_FOR_REF = 0x1, FLAG_USE_FOR_DIS = 0x2)
	unsigned int    base_addr_uv;			//!< Base address of the uv frame buffer. (Physical address)
    void           *base_addr_uv_virtual;	//!< Base address of the uv frame buffer. (Virtual Address)
    unsigned int    size_uv;				//!< uv buffer size
    unsigned int    tctx_addr;              //!< Base address of tctx buffer address                    (from bg6ct)
    unsigned int    tctx_buf_size;          //!< size of tctx buffer                                    (from bg6ct)
    unsigned int    meta_addr;              //!< Base address of meta data address, offset from base    (from bg6ct)
    unsigned int    meta_buf_size;          //!< size of meta data                                      (from bg6ct)
	unsigned int    base_addr_ch1;			//!< Base address of the frame buffer for channel 1. (Physical address)
    void           *base_addr_virtual_ch1;	//!< Base address of the frame buffer for channel 1. (Virtual Address)
    unsigned int    size_ch1;				//!< buffer size for channel 1
	unsigned int    base_addr_uv_ch1;		//!< Base address of the uv frame buffer for channel 1. (Physical address)
    void           *base_addr_uv_virtual_ch1;//!< Base address of the uv frame buffer for channel 1. (Virtual Address)
    unsigned int    size_uv_ch1;			//!< uv buffer size for channel 1
    vdec_pic_desc_t pic[2];                 //!< Picture descriptor pair. [0]: frame/top, [1]: bottom.
};

//---------------------------------------------------------------------------
//! \brief      Decoder Stream Buffer Descriptor    ps. these two desc should be used in queue structure
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vdec_strm_buf_desc_t)
{
    void           *user_data;              //!< User-defined stream context data, not used by the video decoder.
    void           *internal_data;          //!< Reserved. For video decoder internal use only.

    unsigned int    pre_padding_bytes;      //!< for frame_in_mode 2.pre_padding buffer size
    unsigned int    addr;                   //!< Base address of the stream buffer.
    void           *addr_virtual;           //!< Reserved. For video decoder internal use only.
    unsigned int    size;                   //!< Size (in bytes) of the stream buffer.
    unsigned int    rsvd3;                  //!< Reserved. For video decoder internal use only.
    unsigned int    id;                     //!< Buffer ID assigned by the buffer allocator, not used by the video decoder.
    unsigned int    sn;                     //!< Reserved. For video decoder internal use only.
    unsigned int    pos;                    //!< Reserved. For video decoder internal use only.
    void           *rsvd4;                  //!< Reserved. For video decoder internal use only.
    unsigned int    flags;                  //!< Reserved. For video decoder internal use only.

   // Output
    unsigned int    curr_addr;
    void           *curr_addr_virtual;
    unsigned int    dataLeft;
    unsigned int    curr_size;

    // Output
    unsigned int    aligned_addr;           // address aligned at 32B
    unsigned int    aligned_size;           // size aligned at 32B
};

//---------------------------------------------------------------------------
//! \brief      MPEG4 GMC Frame Level Parameters
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vdec_gmc_param_t)
{
    short           gmc_warp_point_x[3];
    short           gmc_warp_point_y[3];

    unsigned char   sprite_warp_point_num;
    unsigned char   sprite_warp_accuracy;
    unsigned char   sprite_bright_change;
    unsigned char   rounding_ctrl;

    vdec_buf_desc_t *cur_pic_ref_buf;
    vdec_buf_desc_t *cur_pic_dis_buf;
    vdec_buf_desc_t *ref_pic_ref_buf;
    vdec_buf_desc_t *gmc_buf;
};

//---------------------------------------------------------------------------
//! \brief      Per-stream Decoder Status and Event Information
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(vdec_strm_status_t)
{
    void            *user_data;         //!< User-defined stream context data, not used by the video decoder.
    void            *internal_data;     //!< Reserved. For video decoder internal use only.
    int              event_id;          //!< Event ID.
    unsigned int     event_args[2];     //!< Event arguments.
    unsigned int     wait_for_int;      //!< Nonezero if decoder is waiting for interrupt.
    unsigned int     safe_for_switch;   //!< Reserved. For video decoder internal use only.
    unsigned int     strm_pos;          //!< 29-bit stream position (in bytes) in which the event occurs, if applicable.
    vdec_seq_desc_t *seq_desc;          //!< Pointer to a vdec_seq_desc_t structure.
    vdec_pic_desc_t *pic_desc;          //!< Pointer to a vdec_pic_desc_t structure.
    vdec_gmc_param_t	*gmc_param;     //!< Pointer to a vdec_gmc_param_t structure.
    vdec_strm_buf_desc_t *sbuf_desc;    //!< Pointer to a sbuf_desc structure.

    //Parameters for VDEC_EVENT_SYNCWORD
    int             hdr_idx;            //!< Index of sequence level headers; Negative if not a sequence level header. <300 if H.264; <8 if other codecs.
    unsigned int    hdr_len;            //!< Length (in bytes) of the sequence level header.
};

//---------------------------------------------------------------------------
//! \brief      Decoder Event IDs
//---------------------------------------------------------------------------
enum
{
    // No event
    VDEC_EVENT_NONE = 0,

    // Stream events

    VDEC_EVENT_START_OF_STREAM,
    VDEC_EVENT_END_OF_STREAM,
    VDEC_EVENT_START_OF_SEQUENCE,
    VDEC_EVENT_END_OF_SEQUENCE,
    VDEC_EVENT_START_OF_GOP,
    VDEC_EVENT_END_OF_GOP,
    VDEC_EVENT_START_OF_FRAME,
    VDEC_EVENT_END_OF_FRAME,
    VDEC_EVENT_START_OF_PICTURE,
    VDEC_EVENT_END_OF_PICTURE,
    VDEC_EVENT_START_OF_SLICE,          // for JPEG only
    VDEC_EVENT_END_OF_SLICE,            // for JPEG only
    VDEC_EVENT_SYNCWORD,
    VDEC_EVENT_SWITCH_POINT,

    // Buffer management events

    VDEC_EVENT_NEED_BUF,            //arg0: buffer type; arg1: buffer size
    VDEC_EVENT_RELEASE_BUF,         //arg0: buffer type

    VDEC_EVENT_ALLOC_ALL,
    VDEC_EVENT_FREE_ALL,
    VDEC_EVENT_ALLOC_STR_BUF,
    VDEC_EVENT_ALLOC_REF_BUF,
    VDEC_EVENT_ALLOC_DIS_BUF,
    VDEC_EVENT_CHECK_STR_BUF,
    VDEC_EVENT_CHECK_REF_BUF,
    VDEC_EVENT_CHECK_DIS_BUF,

    VDEC_EVENT_FLUSH_DCACHE,

    VDEC_EVENT_INTR_DEC,    // hantro G1 special added
    VDEC_EVENT_INTR_PP,     // hantro G1 special added
    // End of event definitions

    VDEC_EVENT_RESERVED
};

//---------------------------------------------------------------------------
//// VPU Common API
////---------------------------------------------------------------------------
const   vpu_hw_info_t*vpu_get_hw_info(void);
const   vpu_const_t *vpu_get_const(unsigned int hwid);
int     vpu_open                  (void *vpu_cntxt, vpu_config_t *vpu_config);
int     vpu_close                 (void *vpu_cntxt);

//---------------------------------------------------------------------------
// Video Decoder API
//---------------------------------------------------------------------------

const vdec_const_t *vdec_get_const  (unsigned int hwid);                                //Obsolete, for backward compatibity only
int     vdec_open                   (void *vdec_cntxt, vdec_config_t *vdec_config);     //Obsolete, for backward compatibity only
int     vdec_configure              (void *vdec_cntxt, vdec_config_t *vdec_config);     //Obsolete, for backward compatibity only
int     vdec_close                  (void *vdec_cntxt);                                 //Obsolete, for backward compatibity only
int     vdec_switch_stream          (void *strm_cntxt, void *next_strm_cntxt);          //Obsolete, for backward compatibity only
int     vdec_stream_switch_out      (void *strm_cntxt);
int     vdec_stream_switch_in       (void *strm_cntxt);

int     vdec_open_stream            (void *strm_cntxt, vdec_strm_config_t *strm_config);
int     vdec_configure_stream       (void *strm_cntxt, vdec_strm_config_t *strm_config);
int     vdec_close_stream           (void *strm_cntxt);
int     vdec_push_buffer            (void *strm_cntxt, unsigned int type, vdec_buf_desc_t *desc);
int     vdec_pop_buffer             (void *strm_cntxt, unsigned int type, vdec_buf_desc_t **desc);

int     vdec_push_stream_buffer     (void *strm_cntxt, vdec_strm_buf_desc_t *desc);
int     vdec_pop_stream_buffer      (void *strm_cntxt, vdec_strm_buf_desc_t **desc);
int     vdec_push_display_buffer    (void *strm_cntxt, vdec_vid_buf_desc_t *desc);
int     vdec_pop_display_buffer     (void *strm_cntxt, vdec_vid_buf_desc_t **desc);
int     vdec_push_reference_buffer  (void *strm_cntxt, vdec_vid_buf_desc_t *desc);
int     vdec_pop_reference_buffer   (void *strm_cntxt, vdec_vid_buf_desc_t **desc);

int     vdec_decode_stream          (void *strm_cntxt, vdec_strm_status_t **strm_status);
int     vdec_skip                   (void *strm_cntxt, int to);
int     vdec_save_context           (void *strm_cntxt, void *backup_strm_cntxt);
int     vdec_load_context           (void *strm_cntxt, void *backup_strm_cntxt);

int     vdec_flush_stream           (void *strm_cntxt);
int     vdec_switch_flush           (void *strm_cntxt);  // for switch at wait_for_input_buffer.
int     vdec_config_rotate_mode     (void *strm_cntxt, vdec_strm_config_t *strm_config);
int     vdec_int_timeout            (void *strm_cntxt);

//---------------------------------------------------------------------------
//              Video Encoder Data Structures
//---------------------------------------------------------------------------
enum
{
    ORIG_CH = 0,
    HALF_CH,
    QUAT_CH,
    VENC_MAX_CH,
};

//---------------------------------------------------------------------------
//! \brief      Per-stream Encoder Settings
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(venc_strm_config_t)
{
    void           *user_data;              //!< User-defined stream context data, not used by the video encoder.
    void           *internal_data;          //!< Reserved. For video encoder internal use only.
    void           *venc_cntxt;             //!< Pointer to the video encoder context
    unsigned int    format;                 //!< Stream format ID.
    unsigned int    hw_context_addr;        //!< Base address (4KB-aligned) of the stream context buffer for hardware.
    void           *hw_context_vaddr;       //!< Virtual adress (4KB-aligned) of the stream context buffer for hardware.

    // per-stream parameters necessary for encoding intialization
    unsigned int    src_yuv_format;         //!< YUV format of input video source, e.g. YUV422
    unsigned int    use_1stpass;            //!< Whether enable first-pass encoding, i.e. 1/4 x 1/4 mostion search
    unsigned int    frm_width;              //!< Number of pels, actual encoding width
    unsigned int    frm_height;             //!< Number of lines, actual encoding height

    unsigned int    field_flag;             //!< To code the frames (never mind whether it is interlaced or not) in frame or field.
    unsigned int    interlaced_frame;       //!< If field_flag=1, all frames are coded in fields and treated as interlaced (in displaying),
                                            //   else use this flag to indicate whether the coded frames are interlaced (in displaying).
    unsigned int    mbaff_enable;           //!< Enable mbaff
    unsigned int    disable_deblocking;     //!< Disable deblocking flag
    unsigned int    poc_type;               //!< Setting picture order count type: 0,2 supportted 1 unsupportted
    unsigned int    entropy_mode;           //!< 0:cavlc, 1:cabac
    unsigned int    transform_8x8_flag;     //!< Enable 8x8 transform flag
    unsigned int    ref_num_in_buf;         //!< Reference buffer count. For H1, 2~4.
    unsigned int    sar_width;              //!< Horizontal size of the sample aspect ratio
    unsigned int    sar_height;             //!< Vertical size of the sample aspect ratio

    unsigned int    profile_idc;            //!< H.264/mpeg2/mpeg4/vc1 profile
    unsigned int    level_idc;              //!< H.264 level
    unsigned int    yuv_downsample;         //!< 0: orig; 1: x direction 1/2 ds; 2: y direction 1/2 ds; 3: x,y direction 1/2 ds
    unsigned int    vp8_strm_mode;          //!< VP8 output stream mode: 0: RAW Stream 1: ivf file
    unsigned int    vp8_mvprob_mode;        //!< 0: don't update vp8 mv prob table; 1: update mv prob table. BG2-CT ECO version support this feature
    unsigned int    vp8_multi_channel;      //!< VP8 multi channel mode: 0: normal mode 1: multi channel, output 3 bitstream: 1, 1/2, 1/4

    // for roi encoding and yuv downscale only.
    unsigned int    dis_offset;             //!< Offset (in bytes) of valid data in a display frame buffer, guarantee the offset in chroma is interger.
    unsigned int    dis_stride;             //!< Stride (in bytes) of the display frame buffer
    unsigned int    dis_buf_size;           //!< Size (in bytes) of the display frame buffer including Y and UV components.

    unsigned int    frame_rate_num;         //!< Numerator of the frame rate, if available.
    unsigned int    frame_rate_den;         //!< Denominator of the frame rate, if available.
    unsigned int    src_video_format;       //!< Source video format, if available.
                                            //      0: unknown, 'COMP': component,
                                            //      'PAL': PAL, 'NTSC': NTSC, 'SECA': SECAM, 'MAC': MAC

    unsigned int    enc_mode;               //!< Encoding mode, see 'ENC_MODE_XXX'

    unsigned int    b_hybrid;               //<! allow express mode when (enc_mode==ENC_MODE_TURBO)
    unsigned int    express_top;            //<! The top MB-lines would be encoded in express mode.
    unsigned int    express_bottom;         //<! The bottom MB-lines would be encoded in express mode.
    unsigned int    express_left;           //<! The left MB-lines would be encoded in express mode.
    unsigned int    express_right;          //<! The right MB-lines would be encoded in express mode.
    unsigned int    turbo_rate;             //<! In range [0,100], would overwrite the previous 4 parameters.

    unsigned int    gop_type;               //!< GOP structure, see '__RCGOP_xxx'
    unsigned int    gop_size;               //!< Max frame count in a GOP
    unsigned int    open_gop;               //!< If open-GOP allowed
    unsigned int    switch_gop;             //!< Enable adaptive sub-GOP switching
    unsigned int    scene_change;           //!< Enable adaptive scene change
    unsigned int    init_qp;                //!< initial (constant) qp index if bit rate control is disabled
                                            //! for jpeg encoder init_qp = quality factor [0,100].
    unsigned int    vbv_size;               //!< VBV buffer size (bit), 0 to disable vbv control
    unsigned int    hrd_rate;               //!< HRD (Hypothetical Reference Decoder) filling speed (bps)
    unsigned int    min_rate;               //!< Minimum target bit rate (bps)
    unsigned int    max_rate;               //!< Maximum target bit rate (bps), CBR if (min_rate == max_rate && min_rate == target_rate)
    unsigned int    dri_mb_rows;            //!< (JPEG only) Number of MB rows in the restart interval (slice).
                                            //          0 (default) - disable restart interval
                                            //          1~(65535/mbs_in_row)     - MB rows in one restart interval (slice)

    int             rc_sel;                 //!< Rate-control strategy
                                            //.....>0: junior mode
                                            //          vMeta fully control the rate control process
                                            //      9: fixed qp mode(fixed with init_qp)
                                            //      5: new rate control algorihm (!!!support Frame & Field)
                                            //      0 thru 4: select from the preset rate-control parameter sets (!!!only support Frame)
                                            //                0 to minimize the quality fluctuation
                                            //                4 to minimize the bit-rate fluctuation
                                            //.....<0:
                                            //     -1: senior mode, disable the preset parameter set and configure the individual parameters.
                                            //          vMeta config picture type, maintain yuv reorder
                                            //          user could insert I(in P position) & PSKIP, config qp
                                            //     -2: expert mode
                                            //          user config picture type & qp, maintain yuv reoder


    // internal rate-control parameters, need not to be set
    unsigned int    min_qp_idx;             //!< minimum qp index
    unsigned int    max_qp_idx;             //!< maximum qp index
    unsigned int    tracking_window;        //!< Rate control tracking window size in 1/10 second (default is 50 i.e. 5 seconds)
    unsigned int    short_term_min_rate;    //!< upper-bound (bps) for short-term bit-rate
    unsigned int    short_term_max_rate;    //!< lower-bound (bps) for short-term bit-rate
    unsigned int    short_term_window;      //!< short-term window size (1/10-sec)

    unsigned int    par;                    //!< pixel aspect ratio:
                                            // 0000: forbidden
                                            // 0001: 1:1(square)
                                            // 0010: 12:11(cif for 4:3 pictures)
                                            // 0011: 10:11(525-type for 4:3 picture)
                                            // 0100: 16:11 (CIF stretched for 16:9 picture)
                                            // 0101: 40:33 (525-type stretched for 16:9 picture)
                                            // 0110-1110: Reserved
                                            // 1111: Extended PAR
    unsigned int    avc_on;                 // If on, just output glolal sps/pps.
    unsigned int    aud_on;                 // If on, output access unit delimiter every frame.


    /** The following option is used for HANTRO encoder (H1) only */
    unsigned int    src_width;
    unsigned int    src_height;
    unsigned int    x_offset;
    unsigned int    y_offset;

    /**
     *  The  picture  scaler  component  receives  the  same  picture  as  the  encoder  and  performs  a
     *  down-scaling operation for the picture. The down-scaled picture is then written to external
     *  memory where it can be accessed for lower resolution encoding or preview. The resolution
     *  of  the  down-scaled  picture  is  specified  at  the  initialization  phase  to  allocate  a  buffer  of
     *  correct  size.  The  picture  format  of  the  down-scaled  picture  is  always  interleaved  YCbCr
     *  4:2:2.
     */
    unsigned int    ds_width;               //!< [16, width] must be multiple of 4. would be overwrite if yuv_downsaple is set on.
    unsigned int    ds_height;              //!< [16, height] must be multiple of 2.would be overwrite if yuv_downsaple is set on.

    unsigned int    rotation;

    unsigned int    quarter_mv;             //!< 0=OFF, 1=Adaptive (Disable 1/4 MVs for >720P), 2=ON [1]

    unsigned int    pictureRc;              //!< Adjust QP between pictures, [0,1] */
    unsigned int    mbRc;                   //!< Adjust QP inside picture, [0,1] */
    unsigned int    pictureSkip;            //!< Allow rate control to skip pictures, [0,1] */
    unsigned int    qpHdr;                  //!< QP for next encoded picture, [-1..51]
                                            //!< -1 = Let rate control calculate initial QP
                                            //!< This QP is used for all pictures if
                                            //!< HRD and pictureRc and mbRc are disabled
                                            //!< If HRD is enabled it may override this QP

    unsigned int    qpMin;                  //!< Minimum QP for any picture, [0..51] */
    unsigned int    qpMax;                  //!< Maximum QP for any picture, [0..51] */
    unsigned int    bitPerSecond;           //!< Target bitrate in bits/second, this is
                                            //!< needed if pictureRc, mbRc, pictureSkip or
                                            //!< hrd is enabled [10000..60000000]

    unsigned int    hrd;                    //!< Hypothetical Reference Decoder model, [0,1]
                                            //!< restricts the instantaneous bitrate and
                                            //!< total bit amount of every coded picture.
                                            //!< Enabling HRD will cause tight constrains
                                            //!< on the operation of the rate control

    unsigned int    hrdCpbSize;             //!< Size of Coded Picture Buffer in HRD (bits) */
    unsigned int    gopLen;                 //!< Length for Group of Pictures, indicates
                                            //!< the distance of two intra pictures,
                                            //!< including first intra [1..300]

    int             intraQpDelta;           //!< Intra QP delta. intraQP = QP + intraQpDelta
                                            //!< This can be used to change the relative quality
                                            //!< of the Intra pictures or to lower the size
                                            //!< of Intra pictures. [-12..12]

    unsigned int    fixedIntraQp;           //!< Fixed QP value for all Intra pictures, [0..51]
                                            //!< 0 = Rate control calculates intra QP.

    int             mbQpAdjustment;         //!< Encoder uses MAD thresholding to recognize
                                            //!< macroblocks with least details. This value is
                                            //!< used to adjust the QP of these macroblocks
                                            //!< increasing the subjective quality. [-8..7]
};

//---------------------------------------------------------------------------
//! \brief      Per-stream Encoder information input for first pass
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(venc_1stpass_param_t)
{
    unsigned int  pic_type;   //!< picture type, I, P, B, P SKIP, we need to think how to use B type
    unsigned int  pic_sn;     //!< picture serial number, handle re-order when B frame is used
    unsigned int  pic_qp;
};

//---------------------------------------------------------------------------
//! \brief      Per-stream Encoder status: output of first pass
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(venc_1stpass_stat_t)
{
    unsigned int  sum_sad;       //!< Sum of all down-sampled macro-blocks SAD
    unsigned int  sum_sd;        //!< Sum of all down-sampled macro-blocks SD
    unsigned int  num_intra_mbs; //!< Number of intra MBs in the 1st pass encode.
    unsigned int  pic_sn;        //!< picutre serial number
    unsigned int  pic_type;      //!< picture type, I, P, B, P SKIP
};

//---------------------------------------------------------------------------
//! \brief      Per-stream Encoder information: input for second pass
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(venc_2ndpass_param_t)
{
    unsigned int  pic_type;    //!< picture type
    unsigned int  pic_sn;      //!< picutre serial number
    unsigned int  pic_qp;      //!< QP index for the picture
};

//---------------------------------------------------------------------------
//! \brief      Per-stream Encoder status: output of second pass
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(venc_2ndpass_stat_t)
{
    unsigned int  strm_len;       //!< encoded bit stream length
    unsigned int  num_intra_mbs;  //!< Number of Intra MBs after 2nd pass encode
    unsigned int  sum_sad;        //!< This SAD is sad_fullres.
    unsigned int  pic_sn;         //!< picutre serial number
    unsigned int  pic_type;       //!< picture type, I, P, B, P SKIP
};

//---------------------------------------------------------------------------
//! \brief      Encoder Sequence Descriptor
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(venc_seq_desc_t)
{
    void           *user_data;              //!< User-defined data, not used by the video decoder.
    void           *internal_data;          //!< Reserved. For video decoder internal use only.
    unsigned int    is_intl_seq;            //!< Nonzero if it is an interlaced sequence. Not always reliable.
    unsigned int    src_yuv_format;         //!< Source YUV format (0: 420 tile, 1: UYVY 422 ...)
    unsigned int    width;                  //!< Image width for this sequence
    unsigned int    height;                 //!< Image height for this sequence
    unsigned int    vid_buf_size;           //!< Size (in bytes) of original 420 tiled video buffer size
    unsigned int    vid422_buf_size;        //!< Size (in bytes) of original 422 raster video buffer size
    unsigned int    ref_buf_size;           //!< Size (in bytes) of the reference buffer size (including reference, down sample video, temporal context)
    unsigned int    ds_buf_size;            //!< Size (in bytes) of the downsampled buffer size
    unsigned int    num_ref_bufs;           //!< Informative. Number of reference frame buffers required by decoding.2:IP, 3:IPB,IPBB, IPBBB
    unsigned int    mch_ref_buf_size[VENC_MAX_CH];  //!< Size (in bytes) of the reference buffer size (multi-channels)
    unsigned int    mch_vid_buf_size[VENC_MAX_CH];  //!< Size (in bytes) of original 420 tiled video buffer size (multi-channels)
};


//---------------------------------------------------------------------------
//! \brief      Encoder Picture Descriptor
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(venc_pic_desc_t)
{
    void           *user_data;              //!< User-defined data, not used by the video decoder.
    void           *internal_data;          //!< Reserved. For video decoder internal use only.
    unsigned short  coded_width;            //!< Coded picture width (in pixels).
    unsigned short  coded_height;           //!< Coded picture height (in pixels).
    unsigned char   is_idr;                 //!< Nonzero if it is an IDR picture.
    unsigned char   is_ref;                 //!< Nonzero if it is a reference picture.
    unsigned char   is_fld;                 //!< Nonzero if it is a field picture.
    unsigned char   is_btm;                 //!< Nonzero if it is a bottom field picture.
    unsigned int    pic_type;               //!< Picture coding type ('I', 'P', 'B').
    int             poc;                    //!< Picture order count of the current picture.
    int             fld_poc[2];             //!< [0]: top poc, [1]: bottom poc, only valid for interlaced frame picture.
    unsigned int    stream_pos;             //!< 29-bit offset (in bytes) of the picture start code in the input stream.
    unsigned int    coded_pic_bits;         //!< Size (in bits) of the coded picture.
    unsigned int    seq_hdr_bits;           //!< Size (in bits) of the sequence header (if any) in coded picture.
    unsigned int    pic_hdr_bits;           //!< Size (in bits) of the picture header (if any) in coded picture.
    unsigned int    coded_pic_idx;          //!< Picture index in decoding order.
    unsigned int    chksum_data[8];         //!< Reserved. For video decoder internal use only.
    unsigned int    perf_cntr[4];           //!< Reserved. For video decoder internal use only.
};


//---------------------------------------------------------------------------
//! \brief      Encoder Video Buffer Descriptor
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(venc_vid_buf_desc_t)
{
    void           *user_data;              //!< User-defined buffer context data
    void           *internal_data;          //!< Reserved. For video decoder internal use only.
    unsigned int    base_addr;              //!< Base address of the frame buffer in vMeta internal tiled format
    unsigned int    id;                     //!< Buffer ID assigned by the buffer allocator
    unsigned int    sn;                     //!< Reserved. For video decoder internal use only.
    unsigned int    tctx_addr;              //!< Reserved. For video encoder internal use only.
    venc_pic_desc_t pic[2];                 //!< Picture descriptor pair. [0]: frame/top, [1]: bottom.
};


//---------------------------------------------------------------------------
//! \brief      Encoder Stream Buffer Descriptor
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(venc_strm_buf_desc_t)
{
    void           *user_data;              //!< User-defined buffer context data
    void           *internal_data;          //!< Reserved. For video decoder internal use only.
    unsigned int    addr;                   //!< Base physical address of the stream buffer.
    void           *vaddr;                  //!< Base virtual address of the stream buffer.
    unsigned int    size;                   //!< Size (in bytes) of the stream buffer.
    unsigned int    valid_data;             //!< Size (in bytes) of the valid bitstream
    unsigned int    flags;                  //!< Bit 0 is set if last stream buffer of picture. Bit 1 is set if last stream buffer of slice (bit 1 for JPEG only).
    unsigned int    id;                     //!< Buffer ID assigned by the buffer allocator
    unsigned int    sn;                     //!< Reserved. For video decoder internal use only.
    unsigned int    pos;                    //!< Reserved. For video decoder internal use only.
    unsigned int    ch_id;          //!< Reserved. For VP8, indicate the channel id (multi-channel)
    unsigned int    part;           //!< Reserved. For VP8, indicate header(0) or coefficient part(1). For other codec part(2) indicate sequence header(pps/sps.etc)
    unsigned int    key;            //!< Reserved. For VP8, indicate if key frame.
};


//---------------------------------------------------------------------------
//! \brief      Per-stream Encoder Status and Event Information
//---------------------------------------------------------------------------
TYPEDEF_STRUCT(venc_strm_status_t)
{
    void                  *user_data;       //!< User-defined stream context data
    void                  *internal_data;   //!< reserved for internal use
    int                    event_id;        //!< Event ID.
    unsigned int           wait_for_int;    //!< None zero if encoder is waiting for interrupt.
    unsigned int           safe_for_switch; //!< Nonezero if decoder is in a safe state for context switch.
    unsigned int           strm_pos;        //!< 29-bit stream position (in bytes) in which the event occurs, if applicable.
    venc_seq_desc_t        *seq_desc;       //!< Pointer to a venc_seq_desc_t structure.
    venc_pic_desc_t        *pic_desc;       //!< Pointer to a venc_pic_desc_t structure.
    venc_1stpass_param_t   *param_1stpass;  //!< Pointer to the first pass parameters  (input).
    venc_1stpass_stat_t    *stat_1stpass;   //!< Pointer to the first pass statistics  (output).
    venc_2ndpass_param_t   *param_2ndpass;  //!< Pointer to the second pass parameters (input).
    venc_2ndpass_stat_t    *stat_2ndpass;   //!< Pointer to the second pass statistics (output).
};


//---------------------------------------------------------------------------
//! \brief      Encoder Event IDs
//---------------------------------------------------------------------------
enum
{
    // No event

    VENC_EVENT_NONE = 0,

    // Stream events

    VENC_EVENT_START_OF_STREAM,
    VENC_EVENT_END_OF_STREAM,

    //Picture events
    VENC_EVENT_INPUT_REFORMAT_END,
    VENC_EVENT_END_OF_FRAME,
    VENC_EVENT_END_OF_PICTURE,
    VENC_EVENT_FIRST_PASS_START,
    VENC_EVENT_FIRST_PASS_END,
    VENC_EVENT_SECOND_PASS_START,
    VENC_EVENT_SECOND_PASS_END,

    // Buffer management events

    VENC_EVENT_ALLOC_ALL,
    VENC_EVENT_FREE_ALL,
    VENC_EVENT_ALLOC_STR_BUF,
    VENC_EVENT_ALLOC_REF_BUF,
    VENC_EVENT_ALLOC_VID_BUF,
    VENC_EVENT_ALLOC_VID422_BUF,
    VENC_EVENT_ALLOC_DS_BUF,
    VENC_EVENT_CHECK_STR_BUF,
    VENC_EVENT_CHECK_REF_BUF,
    VENC_EVENT_CHECK_VID_BUF,
    VENC_EVENT_CHECK_VID422_BUF,
    VENC_EVENT_CHECK_DS_BUF,
    VENC_EVENT_SWITCH_POINT,

    // Buffer management events for vp8 multi-channels support
    VENC_EVENT_ALLOC_VID_H_BUF,
    VENC_EVENT_ALLOC_REF_H_BUF,
    VENC_EVENT_CHECK_VID_H_BUF,
    VENC_EVENT_CHECK_REF_H_BUF,
    VENC_EVENT_ALLOC_VID_Q_BUF,
    VENC_EVENT_ALLOC_REF_Q_BUF,
    VENC_EVENT_CHECK_VID_Q_BUF,
    VENC_EVENT_CHECK_REF_Q_BUF,

    // ASIC event
    VENC_EVENT_BEFORE_ASIC_START,

    // Trouble event
    VENC_STATE_WAIT_FOR_ENCODING_TIMEOUT,

    // End of event definitions
    VENC_EVENT_INTR_ENC,    // hantro H1 special added

    VENC_EVENT_RESERVED
};

enum
{
    VENC_YUV420_TILE = 0,
    VENC_YUV422_UYVY,
    VENC_YUV422_YUYV,
    VENC_YUV420_PLANE,
    VENC_YUV422_PLANE,
    VENC_YUV420_SP,
    VENC_YUV420_SPA,
    VENC_YUV420_YV12,
    VENC_RGBA,
    VENC_INPUT_UNSUPPORTED
};

//Rate Control Parameters
#define __RCGOP_I       0       // I... only
#define __RCGOP_IP      1       // IP... only
#define __RCGOP_IBP     2       // IBP... with adaptive {BP}->{PP} sub-GOP decision
#define __RCGOP_IBBP    3       // IBBP... with adaptive {BBP}->{PPP} sub-GOP decision
#define __RCGOP_IBBBP   4       // IBBBP... with adaptive {BBBP}->{BP/PP,BP/PP} sub-GOP decision
#define __RCGOP_IBbbP   5       // IBbbP... with adaptive {BbBP}->{BP/PP,BP/PP} sub-GOP decision

enum
{
    VENC_PIC_TYPE_I = 0,        // Intra only
    VENC_PIC_TYPE_P,            // Forward prediction
    VENC_PIC_TYPE_PSKIP,        // Forward prediction, SKIP all the MB, in Panic mode
    VENC_PIC_TYPE_B,            // Bi Prediction
    VENC_PIC_TYPE_BREF,         // Bi Prediction and reconstruct as reference
    VENC_PIC_TYPE_FORCE_IDR,    // force IDR
    VENC_PIC_TYPE_NA,           // non-supported
};

enum
{
    VENC_MODE_EXPRESS = 0,
    VENC_MODE_TURBO,
    VENC_MODE_EXPERT,
    VENC_MODE_UNSUPPORTED
};

enum {
    VENC_PROFILE_BASE       = 66,       //* Only I,P, CAVLC, no transform8x8, level<=3.0
    VENC_PROFILE_MAIN       = 77,       //* Only I,P and B, CAVLC/CABAC, no transform, level>=3.0
    VENC_PROFILE_EXTENDED   = 88,       //* direct8x8=1, CAVLC, no transform8x8
    VENC_PROFILE_HIGH       = 100,      //* I,P and B, SI, SP, CAVLC/CABAC, transform
};

enum {
    VENC_LEVEL_1_0 = 10,                //* MaxMBs:99   (qcif),     MaxBR=64kbps
    VENC_LEVEL_1_1,                     //* MaxMBs:396  (cif),      MaxBR=192kbps
    VENC_LEVEL_1_2,                     //* MaxMBs:396  (cif),      MaxBR=384kbps
    VENC_LEVEL_1_3,                     //* MaxMBs:396  (cif),      MaxBR=768kbps
    VENC_LEVEL_2_0 = 20,                //* MaxMBs:396  (cif),      MaxBR=2000kbps
    VENC_LEVEL_2_1,                     //* MaxMBs:792  (scif),     MaxBR=4000kbps
    VENC_LEVEL_2_2,                     //* MaxMBs:1620 (D1,y576),  MaxBR=4000kbps
    VENC_LEVEL_3_0 = 30,                //* MaxMBs:1620 (D1,y576),  MaxBR=10000kbps
    VENC_LEVEL_3_1,                     //* MaxMBs:3600 (720p),     MaxBR=14000kbps
    VENC_LEVEL_3_2,                     //* MaxMBs:5120 (720p+),    MaxBR=20000kbps
    VENC_LEVEL_4_0 = 40,                //* MaxMBs:8192 (HD),       MaxBR=20000kbps
    VENC_LEVEL_4_1,                     //* MaxMBs:8192 (HD),       MaxBR=50000kbps
    VENC_LEVEL_4_2,                     //* MaxMBs:8704 (HD),       MaxBR=50000kbps
    VENC_LEVEL_5_0 = 50,                //* MaxMBs:22080 (2kx2k),   MaxBR=135000kbps
    VENC_LEVEL_5_1,                     //* MaxMBs:36864 (4kx2k),   MaxBR=240000kbps
};

enum
{
    VENC_VP8_RAW = 0,
    VENC_VP8_IVF = 1,
};

#define SIZEOF_COMMON_PACKET_HEADER (16)

//---------------------------------------------------------------------------
// Video Encoder API
//---------------------------------------------------------------------------

const   venc_const_t *venc_get_const(unsigned int hwid);                                //Obsolete, for backward compatibity only
int     venc_open                   (void *venc_cntxt, venc_config_t *venc_config);     //Obsolete, for backward compatibity only
int     venc_configure              (void *vdec_cntxt, venc_config_t *venc_config);     //Obsolete, for backward compatibity only
int     venc_close                  (void *venc_cntxt);                                 //Obsolete, for backward compatibity only
int     venc_switch_stream          (void *strm_cntxt, void *next_strm_cntxt);          //Obsolete, for backward compatibity only
int     venc_stream_switch_out      (void *strm_cntxt);
int     venc_stream_switch_in       (void *strm_cntxt);

int     venc_open_stream            (void *strm_cntxt, venc_strm_config_t *strm_config);
int     venc_configure_stream       (void *strm_cntxt, venc_strm_config_t *strm_config);                //Reserved for future improvements
int     venc_close_stream           (void *strm_cntxt);
int     venc_push_buffer            (void *strm_cntxt, unsigned int type, vdec_buf_desc_t *desc);       //Reserved for future improvements
int     venc_pop_buffer             (void *strm_cntxt, unsigned int type, vdec_buf_desc_t **desc);      //Reserved for future improvements

int     venc_push_video_buffer      (void *strm_cntxt, venc_vid_buf_desc_t *desc);
int     venc_pop_video_buffer       (void *strm_cntxt, venc_vid_buf_desc_t **desc);
int     venc_push_video422_buffer   (void *strm_cntxt, venc_vid_buf_desc_t *desc);
int     venc_pop_video422_buffer    (void *strm_cntxt, venc_vid_buf_desc_t **desc);
int     venc_push_stream_buffer     (void *strm_cntxt, venc_strm_buf_desc_t *desc);
int     venc_pop_stream_buffer      (void *strm_cntxt, venc_strm_buf_desc_t **desc);
int     venc_push_reference_buffer  (void *strm_cntxt, venc_vid_buf_desc_t *desc);
int     venc_pop_reference_buffer   (void *strm_cntxt, venc_vid_buf_desc_t **desc);
int     venc_push_ds_buffer         (void *strm_cntxt, venc_vid_buf_desc_t *desc);
int     venc_pop_ds_buffer          (void *strm_cntxt, venc_vid_buf_desc_t **desc);

int     venc_mch_push_reference_buffer  (void *strm_cntxt, venc_vid_buf_desc_t *desc, unsigned int ch_idx);     // buffer management for vp8 multi-channels
int     venc_mch_pop_reference_buffer   (void *strm_cntxt, venc_vid_buf_desc_t **desc, unsigned int ch_idx);    // buffer management for vp8 multi-channels
int     venc_mch_push_video_buffer      (void *strm_cntxt, venc_vid_buf_desc_t *desc, unsigned int ch_idx);     // buffer management for vp8 multi-channels
int     venc_mch_pop_video_buffer       (void *strm_cntxt, venc_vid_buf_desc_t **desc, unsigned int ch_idx);    // buffer management for vp8 multi-channels

int     venc_encode_stream          (void *strm_cntxt, venc_strm_status_t **strm_status);
int     venc_curr_framenum          (void *strm_cntxt);
int     venc_curr_dispnum           (void *strm_cntxt);


#ifdef __cplusplus
}
#endif

#endif

