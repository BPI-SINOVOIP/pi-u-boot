/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#include "vpp_api.h"
#include "vpp_ca_wrap.h"
#include "vdec_com.h"
#include "tee_ca_common.h"
#include "common.h"

#define MAX_NUM_GLOBAL_VBUF_DESC 4

static VBUF_INFO *gDescArray[MAX_NUM_GLOBAL_VBUF_DESC];

static UINT32 gDescIndex=0;


HRESULT get_vbuf_info(VBUF_INFO **pVBInfo)
{
   gDescIndex++;
   if (gDescIndex == MAX_NUM_GLOBAL_VBUF_DESC) {
	   gDescIndex = 0;
   }
   *pVBInfo = gDescArray[gDescIndex];
	return SUCCESS;
}

void build_frames( VBUF_INFO *vbufinfo, INT32 srcfmt,
        INT32 bit_depth, INT32 x, INT32 y, INT32 width, INT32 height, INT32 progressive, INT32 pattern_type,bool IsPatt)
{
    UINT32 datasize = 1;
    INT32 bit_frac = 1;

    bit_frac = 1;
    /* set other fields of logo frame descriptor */
    if(srcfmt==SRCFMT_YUV422)
    {
        vbufinfo->m_bytes_per_pixel = 2;
        vbufinfo->m_bits_per_pixel=16;
        vbufinfo->m_srcfmt =SRCFMT_YUV422;
        vbufinfo->m_order = ORDER_UYVY;
    }
    else
    {
        vbufinfo->m_bytes_per_pixel = 4;
        vbufinfo->m_bits_per_pixel=32;
    #if 0
        //same as bootloader
        vbufinfo->m_srcfmt =SRCFMT_ARGB32;
        vbufinfo->m_order = ORDER_BGRA;
    #else
        //same as recovery
        vbufinfo->m_srcfmt = SRCFMT_XRGB32;
        vbufinfo->m_order = ORDER_RGBA;
    #endif
        //vbufinfo->m_order = 2;
    }
    datasize = bit_frac * width * height * vbufinfo->m_bytes_per_pixel;

    vbufinfo->m_content_width = width;
    vbufinfo->m_content_height = height;

    vbufinfo->m_buf_stride = vbufinfo->m_content_width * vbufinfo->m_bytes_per_pixel;
    vbufinfo->m_buf_pbuf_start_UV = 0;
    vbufinfo->m_buf_size = (INT32)datasize;
    vbufinfo->m_allocate_type = 0x02;
    vbufinfo->m_buf_type = 2;
    vbufinfo->m_buf_use_state = 0x02;
    vbufinfo->m_flags = 1; /* PTS valid */
    vbufinfo->m_disp_offset = 0;
    vbufinfo->m_buf_stride = vbufinfo->m_content_width * vbufinfo->m_bytes_per_pixel;
    vbufinfo->m_is_frame_seq = progressive; // progressive
    vbufinfo->m_frame_rate_num = 60000;
    vbufinfo->m_frame_rate_den = 1000; //1001; //60000/1001=59.94
    vbufinfo->m_active_width = width;
    vbufinfo->m_active_height = height;
    vbufinfo->m_active_left = x;
    vbufinfo->m_active_top = y;
    vbufinfo->m_content_offset=0 ;
    vbufinfo->m_is_progressive_pic = 0;
    vbufinfo->m_is_repeat_first_field = 0;
    vbufinfo->m_is_top_field_first = 0;
    vbufinfo->m_is_virtual_frame = 0;
    vbufinfo->m_is_right_eye_view_valid = 0;
    vbufinfo->m_right_eye_view_buf_start = (UINT32)(uintptr_t)NULL;
    vbufinfo->m_right_eye_view_descriptor = (UINT32)(uintptr_t)NULL;
    vbufinfo->m_number_of_offset_sequences = 0;
    vbufinfo->m_interlace_weave_mode = 0;

}

HRESULT create_vbuf_desc(VBUF_INFO **pDesc, UINT32 uiBufID)
{
    VBUF_INFO *pVBufInfo = NULL;
    HRESULT Ret = SUCCESS;

#ifdef CONFIG_TRUSTZONE
    pVBufInfo = (VBUF_INFO *) (uintptr_t)VPP_TZ_ALLOC(sizeof(VBUF_INFO));
#else
    pVBufInfo = (VBUF_INFO *) GaloisMalloc(sizeof(VBUF_INFO));
#endif

    if (!pVBufInfo) {
        printf("GaloisMalloc fails");
        Ret = MV_VPP_ENOMEM;
        goto create_vbuf_desc_exit;
    }

    GaloisMemClear(pVBufInfo, sizeof(VBUF_INFO));

    pVBufInfo->m_bufferID = uiBufID;
    pVBufInfo->m_grp_alloc_flag = 0;
    pVBufInfo->m_pbuf_start = 0;
    pVBufInfo->m_buf_size = 0;
    pVBufInfo->m_allocate_type = 0;
    pVBufInfo->m_buf_type = 0;
    pVBufInfo->m_flags = 0;
    pVBufInfo->m_frame_rate_num = 0xFF;
    pVBufInfo->m_frame_rate_den = 0xFF;
    pVBufInfo->m_content_width  = 0xFFFF;
    pVBufInfo->m_content_height = 0xFFFF;
    pVBufInfo->m_pixel_aspect_ratio = 0xDDDD;

    *pDesc = pVBufInfo;

    return SUCCESS;

create_vbuf_desc_exit:
    *pDesc = NULL;

    return Ret;
}

HRESULT destroy_vbuf_desc(VBUF_INFO *pVidBufDesc)
{
    return SUCCESS;
}

HRESULT create_global_desc_array(int src_fmt, int bitdepth,int width, int height)
{
    UINT32 uiCnt;
    HRESULT Ret = SUCCESS;
    for (uiCnt = 0; uiCnt < MAX_NUM_GLOBAL_VBUF_DESC; uiCnt++) {
        Ret = create_vbuf_desc(&(gDescArray[uiCnt]), -1);
        if (Ret != SUCCESS) {
           printf("create_global_desc_array fails");
           goto create_global_desc_array_exit;
        }
        build_frames( (gDescArray[uiCnt]), src_fmt, bitdepth, 0,0, width,height,1, 0,0);
    }
    return SUCCESS;

create_global_desc_array_exit:
	{
        for (uiCnt = 0; uiCnt < MAX_NUM_GLOBAL_VBUF_DESC; uiCnt++) {
            if (gDescArray[uiCnt]) {
                destroy_vbuf_desc(gDescArray[uiCnt]);
            }
        }
 }
    return Ret;

}

