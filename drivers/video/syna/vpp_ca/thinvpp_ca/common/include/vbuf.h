/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#ifndef _VBUF_H_
#define _VBUF_H_

#if !defined(VPP_IN_TRUST_ZONE)
#include "vdec_com.h"
#endif

//! \file Video Buffer Pool Interface
//!
//! This interface can be used to create video buffer pool and allocate
//! video frame buffer from the buffer pool.

//! \brief
//! This state indicates how the buffer is used.
//!
//! This enum should be used in VDEC_VID_BUF_DESC.user_data.m_buf_use_state.
//! [QUESTION: Is it OK to drop this type?]
typedef enum
{
    VM_BUF_UNALLOCATE = 0x0,
    VM_BUF_IN_DECODING,
    VM_BUF_READY_DISP,
    VM_BUF_IN_DISP,
    VM_BUF_RETIRE,
} VM_BUFFER_STATE;

//! \brief
//! This enum indicates the video buffer pool state.
typedef enum
{
    VBUF_POOL_STATE_EMPTY      = 0x0,  //! No water - no vbuf was allocated in the pool.
    VBUF_POOL_STATE_EXHAUSTED  = 0x1,  //! Sold out - no vbuf is  available in the pool.
    VBUF_POOL_STATE_NORMAL     = 0x2,  //! Have water and space, - vbuf is avaliable .

    VBUF_POOL_STATE_INVALID    = 0xFFFFFFFF, //! Unused state, just make 32-bit alignment.
}VBUF_POOL_STATE;

//! \brief
//! This enum indicates how the video frame buffer is allocated.
//!
//! This enum should be used in VBUF_INFO.m_allocate_type.
//! [QUESTION:
//! What's GRP_ALLOC and SELF_ALLOC? Who shall set this field and when
//! shall I set this field?]
typedef enum
{
    VM_MEM_UNALLOCATE = 0x0,
    VM_MEM_GRP_ALLOC,
    VM_MEM_SELF_ALLOC,
} VM_ALLOCATE_TYPE;

typedef enum
{
    VBUF_CTRL_TYPE_INVALID = 0,
    VBUF_CTRL_TYPE_START_PTS,
    VBUF_CTRL_TYPE_STOP_PTS,
    VBUF_CTRL_TYPE_PAUSE_PTS,
    VBUF_CTRL_TYPE_PTS_DISCON

}VBUF_CTRL_TYPE;

typedef struct _vbuf_ctrl_pts_info_
{
    UINT32  m_pts_hi;
    UINT32  m_pts_lo;
}VBUF_CTRL_PTS_INFO;

typedef union _vbuf_ctrl_info_union_
{
    VBUF_CTRL_PTS_INFO m_pts;

}VBUF_CTRL_INFO_UNION;

typedef struct _vbuf_control_info_
{
    VBUF_CTRL_TYPE 	m_CmdType;
    VOID*			m_CmdContext;
    UINT32          m_UserData;
    VBUF_CTRL_INFO_UNION m_Union;
#ifdef PTS_PLAYITEM_EXT
    MV_PlayItem_ID_t m_PID;
#endif
}VBUF_CONTROL_INFO;

/* user data block header */
typedef struct _user_data_block_header_
{
    UINT8   m_level  ;                              ///<: Reserved for VDM internal use.
    UINT8   m_sub_type;                             ///<: Reserved for PE internal use. Sub type of user data. so far only use for CC, 1-dvd cc 0-atsc/dvb/bd cc.
    UINT8   m_type;                                 ///<: type of user data, defined below. i.e., VIDEO_USER_DATA_TYPE_CC.
    UINT8   m_valid;                                ///<: TRUE: the user data is valid.
    UINT8   m_matched;                              ///<: Reserved for VDM internal user.
    UINT8   m_body_offset;                          ///<: The byte offset of user data body from the m_pStart.
    UINT16  m_length;                               ///<: The number of bytes of user data payload, starting from m_pStrat;
    UINT32  m_pos;                                  ///<: Reserved for VDM internal use.
    UINT8*  m_pStart;                               ///<: The user data start address, (including the header and body).
    UINT8   m_is_top_field_first;                   /// only apply to interlaced frame, 1: top field first out
    UINT8   m_is_repeat_first_field;                /// only apply to interlaced frame, 1: repeat first field.

}USER_DATA_BLOCK_HEADER;

/* official user data type */
#define VIDEO_USER_DATA_TYPE_CC         0x0         ///<: Closed Caption
#define VIDEO_USER_DATA_TYPE_AFD        0x1         ///<:
#define VIDEO_USER_DATA_TYPE_BAR        0x2         ///<: BAR
#define VIDEO_USER_DATA_TYPE_GSM        0x3         ///<: GOP Structure Map
#define VIDEO_USER_DATA_TYPE_ESM        0x4         ///<: EPS Structure Map
#define VIDEO_USER_DATA_TYPE_SEQ        0x5         ///<: Sequence Header
#ifdef PE_3D_BD
#define VIDEO_USER_DATA_TYPE_OMT        0x6         ///<: Offset metadata
#endif
#ifdef XVYCC_SUPPORT
#define VIDEO_USER_DATA_TYPE_YCC        0x7
#endif

//#define VIDEO_USER_DATA_TYPE_BD_CC    0x9

#ifdef PE_3D_FMT_CONVERT
#define VIDEO_USER_DATA_TYPE_FP         0x9
#endif

#if !defined(VPP_IN_TRUST_ZONE)
//! \brief
//! Handle of Video Buffer Pool
#define VBUF_HANDLE HANDLE

////////////////////////////////////////////////////////////////////////////////
//! \fn VBUF_HANDLE VBuf_PoolCreate(UINT32 uiMaxPoolSize, UINT32 uiMinPoolSize, UINT32 uiMaxBufNum, UINT32 uiMinBufNum);
//!	This function create a video buffer pool object. It also define how many buffer
//! number and how large the buffer size will be created initially, but it doesn't
//! allocate any video buffer for the pool yet.
//!
//! \param [IN]	UINT32 uiMaxPoolSize:\n
//! The maximum byte of memory this pool can used finally, incluing the buffer pre-allocated
//! before decoding the first frame and the self-allocated buffer at runtime. So the value of
//! this parameter should be larger than 0, or an error code will be returned.
//!
//! \param [IN]	UINT32 uiMinPoolSize:\n
//! The minumnm byte of memory will be created initially.
//! Even if user required a less size, a buffer with size equal to
//! uiMinPoolSize will still be allocated to reserve for future use,
//! which will reduce the number of malloc and free so as to improve
//! efficiency somehow.\n
//!
//! \param [IN]	UINT32 uiMaxBufNum:\n
//! The maximum buffer number in the pool, whose value also decide
//! the depth of buffer queue, So it is invalid to set this value to 0.\n
//!
//! \param [IN]	UINT32 uiMinBufNum:\n
//! The minumum buffer number in the pool that will be created initially.\n
//!
//! \return
//!	return a pointer as the handle of the buffer
//!	pool, if success. The pointer will points to NULL if fail, it
//!	only sets up the structure, and does not allocate memory for
//!	the buffer yet.\n
//!
//! \see
//!	VBuf_PoolRemove
////////////////////////////////////////////////////////////////////////////////
VBUF_HANDLE VBuf_PoolCreate(UINT32 uiMaxPoolSize, UINT32 uiMinPoolSize, UINT32 uiMinBufNum, UINT32 uiMaxBufNum);

////////////////////////////////////////////////////////////////////////////////
//! \fn HRESULT VBuf_PoolRemove(VBUF_HANDLE hVBufPool);
//!	Remove the buffer pool.
//!
//!	It will release everything inside the buffer pool. User should never
//!	access any resource, including video buffer and descriptors after this
//!	function is called.
//!
//! \param [IN] VBUF_HANDLE hVBufPool \n
//!	the handler of the buffer pool
//!
//! \return
//!	S_OK success; this function would never fail
//! \see
//!	VBuf_PoolCreate, VBuf_ItemFree
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_PoolRemove(VBUF_HANDLE hVBufPool);

////////////////////////////////////////////////////////////////////////////////
//! \fn HRESULT VBuf_PoolGetMemSize(VBUF_HANDLE hVBufPool,
//!                                 UINT32 *pMaxSize,
//!                                 UINT32 *pMinSize,
//!                                 UINT32 *pUsedSize,
//!                                 UINT32 *pAllocedSize)
//!	Get the VBuf Pool memory setting and usage information and return to relative
//! pointers. Just set pointer to NULL if you are not interested in the relative
//! information.\n
//!
//! \param [IN] VBUF_HANDLE hVBufPool
//!	the handler of the buffer pool\n
//! \param [OUT] UINT32 *pMaxSize.
//!	the pointer of address that store the value of Maxinum memory size of Pool.\n
//! \param [OUT] UINT32 *pMinSize
//!	the pointer of address that store the value of Minumum memory size of Pool.\n
//! \param [OUT] UINT32 *pUsedSize
//!	the pointer of address that store the value of How many memory size has been used.\n
//! \param [OUT] UINT32 *pAllocedSize
//!	the pointer of address that store the value of How many memory size has been allocated.\n
//!
//! \return
//!	S_OK success; Always return S_OK\n
//! \see
//!	VBuf_PoolCreate, VBuf_ItemFree
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_PoolGetMemSize(VBUF_HANDLE hVBufPool, UINT32 *pMaxSize, UINT32 *pMinSize, UINT32* pUsedSize, UINT32* pAllocedSize);

////////////////////////////////////////////////////////////////////////////////
//! \fn HRESULT VBUF_PoolGetBufNumbers(VBUF_HANDLE hVBufPool,
//!                                 UINT32 *pMaxBufNum,
//!                                 UINT32 *pMinBufNum,
//!                                 UINT32 *pAllocedNum)
//!	Get the VBuf Pool buffer number settings and usage and return to relative
//! pointers. Just set pointer to NULL if you are not interested in the information
//! of that field.\n
//!
//! \param [IN] VBUF_HANDLE hVBufPool
//!	the handler of the buffer pool\n
//! \param [OUT] UINT32 *pMaxBufNum.
//!	the pointer of address that store the value of Maxinum buffer number of Pool.\n
//! \param [OUT] UINT32 *pMinSize
//!	the pointer of address that store the value of Minumum buffer number of Pool.\n
//! \param [OUT] UINT32 *pUsedSize
//!	the pointer of address that store the value of How many buffer has been used.
//! including the buffer in using and buffer in retire but not be freed yet\n
//!
//! \return
//!	S_OK success; Always return S_OK\n
//! \see
//!	VBuf_PoolCreate, VBuf_ItemFree
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_PoolGetBufNumbers(VBUF_HANDLE hVBufPool, UINT32 *pMaxBufNum, UINT32 *pMinBufNum, UINT32 *pAllocedNum);

////////////////////////////////////////////////////////////////////////////////
//! \fn HRESULT VBuf_PoolReset(VBUF_HANDLE hVBufPool, INT32 mode);
//!	Reset the buffer pool in different way (mode)
//!
//! \param [IN] VBUF_HANDLE hVBufPool
//!	the handler of the buffer pool.
//! \param [IN] INT32 mode
//!	the way to reset \n
//!	0) It will return all preloaded buffers, clear the queues \n
//!	1) It only make all the allocated buffer to available queue.
//!	don't return buffer.
//!	[QUESTION: What does "return buffer" mean?]
//!	[QUESTION: Will it reset free queue and ready queue?]
//!
//! \return Return value: S_OK success; !=0, error with error code.
//!         Error code TBD.
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_PoolReset(VBUF_HANDLE hVBufPool, INT32 mode);

////////////////////////////////////////////////////////////////////////////////
//! \fn HRESULT VBuf_PoolPreAllocMem(VBUF_HANDLE hVBufPool, UINT32 uiSize);
//!	Pre-allocate memory for buffer pool
//!
//!	This function allocates memory for the buffer pool. It only defines
//!	an initial size of the buffer pool. The buffer pool size could be
//!	increased dynamically if more memory is required at runtime.
//!
//! \param [IN] VBUF_HANDLE hVBufPool
//!	it is the handle of buffer pool.
//! \param [IN] UINT32 size
//!	it is the total size (in bytes) for the pool.
//!
//! \return
//!	S_OK success; !=0, error with error code.
//!	Error code TBD.
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_PoolPreAllocMem(VBUF_HANDLE hVBufPool, UINT32 uiNum, UINT32 uiPerFrameSize);


////////////////////////////////////////////////////////////////////////////////
//! \fn HRESULT VBuf_ItemReq(VBUF_HANDLE hVBufPool, UINT32 uiReqType, UINT32 uiSize, VDEC_VID_BUF_DESC **pReqVidDesc);
//!	Request a buffer with the size requirement.
//!
//!	It may give a previously used buffer from the pool, or allocate
//!	it from the pre-allocated memory or allocate from the system memory.
//!	The behavior is control by ReqType.
//!	The buffer start address is 1K alignment.
//!
//! \param [IN] VBUF_HANDLE hVBufPool
//!	It is the handle of a valid buffer pool
//! \param [IN] UINT32 ReqType;
//!	0: pick up buffer from the free_index. if fail,
//!	allocate from preloaded buffer, still fail, allocate from system. \n
//!	1: allocate from preloaded buffer, if fail, allocate from system \n
//!	2: allocate from System \n
//!	3: Get one from ready_index only. \n
//!	4: Pick up from free_index only. \n
//!	[QUESTION: Does each item have identical size?]
//!	[QUESTION: When a buffer is created, how many items are
//!	in free queue and how many items are in ready queue?]
//!	[QUESTION: How can I get memory only from preloaded buffer?]
//!
//! \param [IN] UINT32 uiSize \n
//!	it is the total size (in bytes) for the buffer, the value only valid when ReqType is 0.
//!
//! \param [OUT] VDEC_VID_BUF_DESC **pReqVidDesc \n
//!	Return the descriptor pointer
//!	[QUESTION: may I allocate the buffer descriptor by my self or I must
//!	create it by VBuf_DesCreate?]
//!
//! \return
//!	S_OK success; !=0, error with error code.
//! \see
//!	VBuf_DesCreate
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_ItemReq(VBUF_HANDLE hVBufPool, UINT32 uiReqType,
                     UINT32 uiSize, VDEC_VID_BUF_DESC **pReqVidDesc);


////////////////////////////////////////////////////////////////////////////////
//! \fn HRESULT VBuf_ItemFree(VBUF_HANDLE hVBufPool, VDEC_VID_BUF_DESC *pVidBufDesc);
//! Free a item(Frame buffer) to Vbuf pool.
//!
//! \param [IN] VBUF_HANDLE hVBufPool
//!	it is the handle of buffer pool.
//! \param [IN] VDEC_VID_BUF_DESC *pVidBufDesc
//!	the descriptor of the item.
//!
//! \return
//!	S_OK success; !=0, error with error code.
//!	Error code TBD.
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_ItemFree(VBUF_HANDLE hVBufPool, VDEC_VID_BUF_DESC *pVidBufDesc);

//BOOL VBuf_ItemSearch(VBUF_HANDLE hVBufPool, VDEC_VID_BUF_DESC *pVidBufDesc);

////////////////////////////////////////////////////////////////////////////////
//! \fn HRESULT VBuf_DesCreate(VDEC_VID_BUF_DESC **pDesc, UINT32 BufID );
//!	Create a buffer descriptor
//!
//!	This function only allocates memory for the structure; do not really
//!	allocate the memory for the video buffer yet.
//!
//! \param [IN/OUT] VDEC_VID_BUF_DESC  **pDesc
//!	return a pointer as the handle of the buffer descriptor, if success.
//!	The pointer will points to NULL of fail, it only sets up the
//!	structure, and does not allocate memory for the buffer yet.
//!
//! \param [IN] UINT32 BufID
//!	assigned a buffer ID to the descriptor.
//!	[QUESTIN: how can I use the ID?]
//!
//! \return
//!	S_OK success; !=0, error with error code.
//!	Error code TBD.
//! \see
//!	VBuf_DesRemove
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_DesCreate(VDEC_VID_BUF_DESC **pDesc, UINT32 BufID );

////////////////////////////////////////////////////////////////////////////////
//! \fn HRESULT VBuf_DesRemove(VDEC_VID_BUF_DESC *pVidBufDesc);
//!	Remove the buffer descriptor. It will release everything,
//!	including the buffer it linked, inside the buffer descriptor.
//!	Be sure to recycle the buffer before call this function.
//!
//! \param [IN] VDEC_VID_BUF_DESC *pVidBufDesc
//!	the handler of the buffer descript.
//!
//! \return
//!	S_OK success; !=0, error with error code.
//!	Error code TBD.
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_DesRemove(VDEC_VID_BUF_DESC *pVidBufDesc);

////////////////////////////////////////////////////////////////////////////////
//! \fn HRESULT VBuf_DesSelfMemAlloc(VDEC_VID_BUF_DESC *pVidBufDesc, UINT32 uiSize);
//!	Alloc the buffer for the descriptor. It will self alloc memory instead from pool.
//!
//! \param [IN] VDEC_VID_BUF_DESC *pVidBufDesc
//!	the handler of the buffer descript.
//! \param [IN] UINT32 uiSize
//!  the size of the buffer.
//! \return
//!	S_OK success; !=0, error with error code.
//!	Error code TBD.
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_DesSelfMemAlloc(VDEC_VID_BUF_DESC *pVidBufDesc, UINT32 uiSize);

////////////////////////////////////////////////////////////////////////////////
//! \fn HRESULT VBuf_DesSelfAlloc(VDEC_VID_BUF_DESC *pVidBufDesc, UINT32 uiSize);
//!	Free the self allocated buffer for the descriptor.
//!
//! \param [IN] VDEC_VID_BUF_DESC *pVidBufDesc
//!	the handler of the buffer descript.
//! \return
//!	S_OK success; !=0, error with error code.
//!	Error code TBD.
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_DesSelfMemFree(VDEC_VID_BUF_DESC *pVidBufDesc);

////////////////////////////////////////////////////////////////////////////////
//! \fn HRESULT VBuf_PoolGetFullness(VBUF_HANDLE hVBufPool, UINT32*pReadyToDecode, UINT32 *pReadyToDisplay)
//!	Get fullness info for video buffer pool, both ReadyToDisp queue and ReadyToDecode queue
//!
//! \param [IN] VBUF_HANDLE hVBufPool
//!	it is the handle of buffer pool.
//! \param [OUT] UINT32* pReadyForDecode
//!	it is the fullness of queue, which indicte the number of  frames that are ready for decoding.
//! \param [OUT] UINT32* free_queue_fullness
//!	it is the fullness of queue, which indicte the number of  frames that are ready for display.
//!
//! \return
//!	S_OK success; !=0, error with error code.
//!	Error code TBD.
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_PoolGetFullness(VBUF_HANDLE hVBufPool, UINT32 *pReadyToDecode, UINT32 *pReadyToDisplay);

////////////////////////////////////////////////////////////////////////////////
//! \fn HRESULT VBuf_PushFrameReadyToDecode(VBUF_HANDLE hVBufPool, VDEC_VID_BUF_DESC *pVidBufDesc);
//!	Push a retired vbuf descriptor that ready for decoding into the vbuf pool's queue.
//!
//! \param [IN] VBUF_HANDLE hVBufPool
//!	it is the handle of buffer pool.
//! \param [IN] VDEC_VID_BUF_DESC *pVidBufDesc
//!	Video buffer descriptor.
//!
//! \return
//!	S_OK success; !=0, error with error code.
//!	Error code TBD.
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_PushFrameReadyToDecode(VBUF_HANDLE hVBufPool, VDEC_VID_BUF_DESC *pVidBufDesc);

#if defined(VDM_EVENT_DRIVEN)
////////////////////////////////////////////////////////////////////////////////
//! \fn HRESULT VBuf_PoolRegisterCallBack(VBUF_HANDLE hVBufPool, void *pCallBack, UINT32 Param);
//!	Register a callback function to notify vbuf pool producer.
//!
//! \param [IN] VBUF_HANDLE hVBufPool
//!	it is the handle of buffer pool.
//! \param [IN] pCallBack *pCallBack
//!	callback function pointer.
//! \param [IN] UINT32 Param
//!	parameter for the callback function.
//! \return
//!	S_OK success; !=0, error with error code.
//!	Error code TBD.
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_PoolRegisterCallBack(VBUF_HANDLE hVBufPool, void *pCallBack, UINT32 Param);
#endif

////////////////////////////////////////////////////////////////////////////////
//! \fn HRESULT VBuf_PushFrameReadyToDisp(VBUF_HANDLE hVBufPool, VDEC_VID_BUF_DESC *pVidBufDesc);
//!	Push a vbuf descriptor that ready for display into the vbuf pool's queue.
//!
//! \param [IN] VBUF_HANDLE hVBufPool
//!	it is the handle of buffer pool.
//! \param [IN] VDEC_VID_BUF_DESC *pVidBufDesc
//!	Video buffer descriptor.
//!
//! \return
//!	S_OK success; !=0, error with error code.
//!	Error code TBD.
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_PushFrameReadyToDisp(VBUF_HANDLE hVBufPool, VDEC_VID_BUF_DESC *pVidBufDesc);

////////////////////////////////////////////////////////////////////////////////
//! \fn VDEC_VID_BUF_DESC* VBuf_PopFrameReadyToDisp(VBUF_HANDLE hVBufPool, BOOL bAdvance);
//!	Pop a free vbuf descriptor that ready to display from the vbuf pool's queue.
//!
//! \param [IN] VBUF_HANDLE hVBufPool
//!	it is the handle of buffer pool.
//! \param [IN] BOOL bAdvance
//!	TRUE is to pop the frame from the queue; FALSE just peek a frame
//!	but not remove it from the queue.
//!
//! \return
//!	VDEC_VID_BUF_DESC *pVidBufDesc; Video buffer descriptor poped.
//!	NULL if no frame is available in queue.
////////////////////////////////////////////////////////////////////////////////
VDEC_VID_BUF_DESC* VBuf_PopFrameReadyToDisp(VBUF_HANDLE hVBufPool, BOOL bAdvance);

////////////////////////////////////////////////////////////////////////////////
//! \fn VDEC_VID_BUF_DESC* VBuf_PopFrameReadyToDecode(VBUF_HANDLE hVBufPool, BOOL bAdvance);
//!	Pop a vbuf descriptor that ready to decode from the vbuf pool's queue.
//!
//! \param [IN] VBUF_HANDLE hVBufPool
//!	it is the handle of buffer pool.
//! \param [IN] BOOL bAdvance
//!	TRUE is to pop the frame from the queue; FALSE just peek a frame
//!	but not remove it from the queue.
//!
//! \return
//!	VDEC_VID_BUF_DESC *pVidBufDesc; Video buffer descriptor poped.
//!	NULL if no frame is available in queue.
//!
////////////////////////////////////////////////////////////////////////////////
VDEC_VID_BUF_DESC* VBuf_PopFrameReadyToDecode(VBUF_HANDLE hVBufPool, BOOL bAdvance);

////////////////////////////////////////////////////////////////////////////////
//! \fn VBUF_INFO* VBuf_GetVBufInfo(VDEC_VID_BUF_DESC *pVidBufDesc);
//!	Get a vbuf info structure pointer from the vbuf descriptor.
//!
//! \param [IN] VDEC_VID_BUF_DESC *pVidBufDesc
//!	vbuf descriptor.
//!
//! \return
//!	VBUF_INFO *: VBuf Info structure pointer.
//!
////////////////////////////////////////////////////////////////////////////////
VBUF_INFO* VBuf_GetVBufInfo(VDEC_VID_BUF_DESC *pVidBufDesc);

////////////////////////////////////////////////////////////////////////////////
//! \fn VDEC_VID_BUF_DESC* VBuf_PushControlInfo(VBUF_HANDLE hVBufPool,VBUF_CONTROL_INFO* pInfo);
//!	Push a control tag into the VBuf control queue.
//!
//! \param [IN] VBUF_HANDLE hVBufPool it is the handle of buffer pool.
//! \param [IN] VBUF_CONTROL_INFO*    The pointer of control tag.
//!
//! \return
//! 	S_OK success; !=0, error with error code.
//!	    Error code TBD.
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT VBuf_PushControlInfo(VBUF_HANDLE hVBufPool, VBUF_CONTROL_INFO* pInfo);


HRESULT VBuf_PopControlInfo(VBUF_HANDLE hVBufPool, VDEC_VID_BUF_DESC *pVidBufDesc, VBUF_CONTROL_INFO* pInfo);


////////////////////////////////////////////////////////////////////////////////
//! \fn VBUF_POOL_STATE VBuf_PoolGetState(VBUF_HANDLE hVBufPool);
//!	Get a vbuf pool state.
//!
//! \param [IN] VBUF_HANDLE hVBufPool it is the handle of buffer pool.
//!
//! \return
//!	VBUF_POOL_STATE: see VBUF_POOL_STATE.
////////////////////////////////////////////////////////////////////////////////
VBUF_POOL_STATE VBuf_PoolGetState(VBUF_HANDLE hVBufPool);
#endif
#endif

