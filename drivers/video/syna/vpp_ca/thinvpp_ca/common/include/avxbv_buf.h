/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 *
 */

/*******************************************************************************
* File: avxbv_buf.h                                                            *
*                                                                              *
* Purpose:                                                                     *
*        header                                                                *
* Note:                                                                        *
*        definition for AV_XBV_BUFFER                                          *
*******************************************************************************/
#ifndef AV_XBV_BUFFER_H__
#define AV_XBV_BUFFER_H__

#include "cmd_buf.h"

#define		AV_BUF_FLUSHING	0x80000000

enum {
	XBV_AUDIO = 1,
	XBV_VIDEO = 2,
	XBV_COMMON = 3,
};

////////////// XBV buffer //////////////
typedef struct t_audio_buf_pts {
	UINT	m_PtsVal;
	INT		m_Counter;
	INT		m_PtsValid;
} AUDIO_BUF_PTS;

typedef struct t_audio_rd_node {
	INT		m_NodeInUsed;

	VOID*	m_pProcessor;
	VOID*	m_pBuffer;			// pointer point to AV_XBV_BUFFER, AUDIO_BUFFER or PCM_BUFFER

	INT		m_RdMode;			// used by AUDIO_BUFFER, refer to eAudioBufRdMode
	INT		m_RdOffset;
	INT		m_PtrWrapAround;	// wrap round indicator, if write wrap around, ++; if read wrap around --. semaphore.

	AUDIO_BUF_PTS m_RdPts;

	// add for processing control info position
	UINT64	m_ConsumedDataCount;
	INT		m_NeedReSyncPTS;

	// Control Info XBV buffer
	UINT	m_CtrlInfoReadIndex;
	UINT	m_LastCtrlInfoDataCount;
	UINT	m_CtrlInfoDataCountMask;
} AUDIO_RD_NODE;

typedef struct t_audio_rd_node_grp {
	INT		m_MaxAudioRdNode;
	INT		m_UsedAudioRdNode;
	AUDIO_RD_NODE	*m_RdNode;
} AUDIO_RD_NODE_GRP;

typedef struct t_AV_XBV_BUFFER {
#ifdef PE_EVENT_DRIVEN
    Buffer_Notifier_t m_BufNtf;
#endif

	INT		m_AvXbvType;		// buffer type: AUDIO BUFFER /VIDEO BUFFER etc.
	UINT*	m_AvXbvStart;
	INT		m_AvXbvWrOffset;	//in the unit of bytes

	union {
		AUDIO_RD_NODE_GRP	m_AvXbvRdPtr;
		INT		m_AvXbvRdOffset;
	};

	INT		m_AvXbvSize;		// in the size of bytes
	INT		m_AvXbvPadSize;		// padding size, (Bytes)

	CONTROL_INFO_XBV_BUF	m_ControlInfoBuf;

	INT		buf_flags;			// for demux purpose

#ifdef ENABLE_DEBUG
	UINT32 m_TotalWriteBytes;
	UINT32 m_TotalReadBytes;
#endif
} AV_XBV_BUFFER;

extern VOID AudioPtsReset(AUDIO_BUF_PTS *pPts);
extern VOID AudioRdNodeReset(AUDIO_RD_NODE *pNode);
extern VOID * AVXBV_BufferCreate(INT byte_size, INT pad_size, INT control_info_index_size, INT type, INT MaxRdNode);
extern INT AVXBV_BufferReset(AV_XBV_BUFFER *pbuf);
extern INT AVXBV_BufferFree(AV_XBV_BUFFER *pbuf);

extern INT AVXBV_BufferWrite(UCHAR *start, INT size, AV_XBV_BUFFER *pbuf);
extern INT AVXBV_BufferRead(UCHAR *dst, INT size, AV_XBV_BUFFER *pbuf);
extern INT AVXBV_BufferSkip(INT size, AV_XBV_BUFFER *pbuf);
extern INT AVXBV_BufferGetFullness(AV_XBV_BUFFER *pbuf);
extern INT AVXBV_BufferGetNodeFullness(AUDIO_RD_NODE *pNode);
extern INT AVXBV_BufferGetAvailSpace(AV_XBV_BUFFER *pbuf);
extern INT AVXBV_BufferGetNodeSpace(AUDIO_RD_NODE *pNode);
extern INT AVXBV_BufferFillPad(AV_XBV_BUFFER *pbuf);
extern INT AVXBV_BufferCopy(UCHAR *dst, INT size, AV_XBV_BUFFER *pbuf, INT offset);

extern void *AVXBV_BufferGetStartPtr(AV_XBV_BUFFER *pbuf);
extern void *AVXBV_BufferGetPadStartPtr(AV_XBV_BUFFER *pbuf);
extern void *AVXBV_BufferGetReadPtr(AV_XBV_BUFFER *pbuf);
extern void *AVXBV_BufferGetWritePtr(AV_XBV_BUFFER *pbuf);

extern INT AVXBV_BufferGetSize(AV_XBV_BUFFER *pbuf);
extern INT AVXBV_BufferGetPadSize(AV_XBV_BUFFER *pbuf);

extern INT AVXBV_BufferGetReadOffset(AV_XBV_BUFFER *pbuf);
extern INT AVXBV_BufferGetWriteOffset(AV_XBV_BUFFER *pbuf);

extern INT AVXBV_BufferSetWriteOffset(AV_XBV_BUFFER *pbuf, INT WriteOffset);
extern INT AVXBV_BufferSetReadOffset(AV_XBV_BUFFER *pbuf, INT ReadOffset);

extern INT AVXBV_BufferGetMaxReadNode(AV_XBV_BUFFER *pbuf);
extern INT AVXBV_BufferGetUsedNode(AV_XBV_BUFFER *pbuf);
extern INT AVXBV_BufferIncUsedNode(AV_XBV_BUFFER *pbuf);
extern INT AVXBV_BufferDecUsedNode(AV_XBV_BUFFER *pbuf);
extern AUDIO_RD_NODE *AVXBV_BufferGetNodeFromIndex(AV_XBV_BUFFER *pbuf, INT Index);

extern INT AVXBV_BufferControlInfoExists(AV_XBV_BUFFER *pbuf);

extern INT AVXBV_BufferControlInfoWrite(AV_XBV_BUFFER *pbuf, CONTROL_INFO_UNIT *pCtrlUnit);
extern INT AVXBV_BufferControlInfoFullness(AV_XBV_BUFFER *pbuf);
extern INT AVXBV_BufferControlInfoSpace(AV_XBV_BUFFER *pbuf);
extern BOOL AVXBV_BufferControlInfoIsFull(AV_XBV_BUFFER* pbuf);
extern INT AVXBV_BufferControlInfoRead(AV_XBV_BUFFER *pbuf, CONTROL_INFO_UNIT *unit);
extern INT AVXBV_BufferControlInfoPeek(AV_XBV_BUFFER *pbuf, CONTROL_INFO_UNIT *unit);
extern INT AVXBV_BufferControlInfoPeekExt(AV_XBV_BUFFER *pbuf, INT Index, CONTROL_INFO_UNIT *unit);
extern INT AVXBV_BufferControlInfoNodeRead(AV_XBV_BUFFER *pbuf, AUDIO_RD_NODE *pRdNode, CONTROL_INFO_UNIT *pCtrlInfoUnit);

extern INT XBV_ControlInfoReset(CONTROL_INFO_XBV_BUF *pXBVControlInfo);
extern INT XBV_ControlInfoWrite(CONTROL_INFO_XBV_BUF *pXBVControlInfo, CONTROL_INFO_UNIT *pCtrlUnit);
extern INT XBV_ControlInfoFullness(CONTROL_INFO_XBV_BUF *pXBVControlInfo);
extern INT XBV_ControlInfoSpace(CONTROL_INFO_XBV_BUF *pXBVControlInfo);
extern INT XBV_ControlInfoPeek(CONTROL_INFO_XBV_BUF *pXBVControlInfo, CONTROL_INFO_UNIT *pCtrlInfoUnit);
extern INT XBV_ControlInfoRead(CONTROL_INFO_XBV_BUF *pXBVControlInfo, CONTROL_INFO_UNIT *pCtrlInfoUnit);
extern VOID XBV_ControlInfoCoreDump(CONTROL_INFO_XBV_BUF *pControlInfoXbvBuf);
#endif


