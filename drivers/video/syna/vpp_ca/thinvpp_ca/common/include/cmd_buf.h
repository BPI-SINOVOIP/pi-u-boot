/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 *
 */

/*    cmd_buf.h

    This file defines two kinds of command buffers and associated cmd types
    1. Stream control info buffer: control info coming from the stream.
    2. Input control tags: tags given by application.
*/

#ifndef __CMD_BUF_H__
#define __CMD_BUF_H__
#include "com_type.h"
#include "GaloisTime.h"

#if defined (ARM) && defined (__ARMCC_VERSION)
#pragma anon_unions
#endif


//////////////individual control info definition /////////////

typedef struct t_mv_timestamp {
    UINT    m_StampHigh;    // general time stamp high bits, highest bit set to 1 to indicate time stamp valid.
    UINT    m_StampLow;     // Time stamp low bits, in units of 90KHz clock
} MV_TIMESTAMP;

#define IS_VALID_TIMESTAMP(pTimeStamp)      (((pTimeStamp)->m_StampHigh & 0x80000000) >> 31)
#define CLEAR_TIMESTAMP_VALID(pTimeStamp)   ((pTimeStamp)->m_StampHigh &= ~0x80000000)
#define SET_TIMESTAMP_VALID(pTimeStamp)     ((pTimeStamp)->m_StampHigh |= 0x80000000)

typedef struct t_unit_start {
    MV_TIMESTAMP    m_PtsValue;
    MV_TIMESTAMP    m_DtsValue;
} CONTROL_UNITSTART;

typedef struct t_unit_end {
    UINT    m_Rsvd;
} CONTROL_UNITEND;

typedef struct t_transport_error {
    UINT    m_Rsvd;
    UINT    m_DataSize;
} CONTROL_TRANSPORTERROR;

typedef struct t_continuity_error {
    UINT    m_Rsvd;
} CONTROL_CONTINUITYERROR;

typedef struct t_crc_error {
    UINT    m_Rsvd;
} CONTROL_CRCERROR;

typedef struct t_scrambled_ts {
    UINT    m_NewMode;
} CONTROL_SCRAMBLE_CHG;

typedef struct t_pes_priority {
    UINT    m_Rsvd;
} CONTROL_PES_PRIORITY;

typedef struct t_copyright {
    UINT    m_CopyRight;
    UINT    m_Original_Copy;
    UINT    m_AdditionalCopyInfo;
} CONTROL_COPYRIGHT;

typedef struct t_trick_mode {
    UINT    m_TrickModeCtrl;
    UINT    m_TrickModeData;
} CONTROL_TRICKMODE;

typedef struct t_escr_esrate {
    MV_TIMESTAMP    m_EscrValue;
    UINT    m_EsRate;
} CONTROL_ESCR_ESRATE;

typedef struct t_pcrinfo {
    MV_TIMESTAMP    m_PcrValue;
    MV_TIMESTAMP    m_StcValue;
} CONTROL_PCRINFO;

typedef struct t_pes_priv_data{
    UCHAR    m_PesPrivData[16];
} CONTROL_PES_PRIV_DATA;

typedef struct t_discont_indicate {
    //! following fields only for BD,
    //! in the case of PCR discontinuity within a single AV stream file
    UINT    m_PcrPid;
    UINT    m_StartPts;
    UINT    m_EndPts;
    MV_PlayItem_ID_t m_PI;
} CONTROL_DISCONT_INDICATE;

//! Note.1:
//! About CONTROL_DISCONT_INDICATE control tag:
//!This control tag indicates there is a PCR discontinuity. It could be from the following sources:
//!a)    it¡¯s announced PCR discontinuity that indicated within the TS stream. It means the time
//!         info (PCR/PTS/DTS etc) after this point is on a new time base.
//!b)    It¡¯s a PCR discontinuity notified by application, that could be source stream change, or
//!         discontinuity within one single av stream file. It means that all time info after this point
//!         is on a new time base.
//!c)    it¡¯s un-announced PCR discontinuity, detected by DMX module (in the dtv case). In this case,
//!         the eBS_DiscontIndicate control tag is issued when a new steady time base is established. At
//!         least 3 consecutive PCR is needed to establish a new time base. So the position(data count)
//!         indicated in the control tag could be up to several hundred milisecond behind the real change
//!         position. If you need more precise info on the time base change, you can use the following two
//!         new control tags.

typedef struct t_pcr_jump {
    UINT    m_Rsvd;
} CONTROL_PCR_JUMP;

typedef struct t_timebase_notif {
    UINT    m_IsNewTimeBase;
} CONTROL_TIMEBASE_NOTIF;
//! Note.2:
//! About CONTROL_PCR_JUMP & CONTROL_TIMEBASE_NOTIF control tags:
//! eBS_PcrJump will be issued as soon as a PCR out of proper range is received. It could be caused by
//! a time base change, but also could be input data error. DMX need to monitor the several PCR after
//! the first error one. After a steady time base is established again, eBS_TimeBaseNotif control tag
//! will be output to every output channel. If the new established time base is a different time base
//! from the original one, m_IsNewTimeBase in the control tag will be set to 1. Otherwise, m_IsNewTimeBase=0.


typedef struct t_random_access {
    UINT    m_Rsvd;
} CONTROL_RAMDOM_ACCESS;

typedef struct t_es_priority {
    UINT    m_Rsvd;
} CONTROL_ES_PRIORITY;

typedef struct t_splice_countdown {
    UINT    m_CountdownVal;
} CONTROL_SPLICE_CNTDWN;

typedef struct t_splice_info {
    UINT    m_SpliceType;
    MV_TIMESTAMP    m_DtsNextAU;
} CONTROL_SPLICE_INFO;

typedef struct t_last_gop_notify {
    UINT    m_SourceID;     // source index
    UINT    m_EntryID;      // stream entry index in the source
} CONTROL_LAST_GOP_NOTIFY;


//////control info for DVD/HDDVD//////
typedef struct t_SCR {
    MV_TIMESTAMP    m_ScrValue;
} CONTROL_SCR;

typedef struct t_muxrate {
    UINT    m_MuxRate;
} CONTROL_MUXRATE;

typedef struct t_system_header {
    UINT    m_RateBound         :22;
    UINT    m_AudioBound        :6;
    UINT    m_Rsv1              :4;

    UINT    m_RateRestrictFlg   :1;
    UINT    m_SysVLkFlg         :1;
    UINT    m_SysALkFlg         :1;
    UINT    m_CSPSFlg           :1;
    UINT    m_FixFlg            :1;
    UINT    m_Rsv2              :27;

    UINT    m_VideoBound;
} CONTROL_SYSTEM_HEADER;

typedef struct t_PSTD_buffer_bound {
    UINT    m_StreamID;
    UINT    m_BufferBoundScale;
    UINT    m_BufferSizeBound;
} CONTROL_PSTD_BUFFER_BOUND;

typedef struct t_PSTD_buffer_size {
    UINT    m_Scale;
    UINT    m_Size;
} CONTROL_PSTD_BUFFER_SIZE;

#define MV_ES_FMT_VC1           0x41
#define MV_ES_FMT_WMV3          0x42
#define MV_ES_FMT_MPEG2         0x42
#define MV_ES_FMT_MKV_H264      0x43

typedef struct t_frame_info {
//  UINT    m_NumberOfFrameHeader;
//  UINT    m_FirstAUPointer;
    UINT    m_EsFormatType      :8  ;
    UINT    m_PatternSize       :4  ;
    UINT    m_Rsv               :20 ;

    UINT    m_EsFrameSize;
    UINT    m_Pattern;
    UINT    m_SkipHeader;

} CONTROL_FRAME_INFO;

typedef struct t_lpcm_info {
    UINT    m_AudioEmphasisflg  :1 ;
    UINT    m_AudioMuteFlg      :1 ;
    UINT    m_QuanWordLen       :2 ;
    UINT    m_SampFreq          :3 ;
    UINT    m_Rsv1              :25;

    UINT    m_DynamicRgCtrl     :8 ;
    UINT    m_AudioChanNum      :4 ;
    UINT    m_Rsv2              :20;

    UINT    m_DwMxCodeVld       :1 ;
    UINT    m_DwMxCode          :4 ;
    UINT    m_ChanAsign         :5 ;
    UINT    m_Rsv3              :22;

} CONTROL_LPCM_INFO;

typedef struct t_ra_info {
    UINT    m_BlockSize;
    UINT    m_BlockFlag;
    UINT    m_TimeStamp;
} CONTROL_RA_INFO;

typedef struct t_input_ctrl_stop_pts {
    MV_TIMESTAMP    m_PtsValue;
    UINT    m_IsBroadcast;
    //! if m_IsBroadcast != 0, broadcast this tag to all output
    //! if m_IsBroadcast = 0, only output this control tag to the entry m_StreamEntryID
    UINT    m_StreamEntryID;
    MV_PlayItem_ID_t m_PI;
} CONTROL_INPUT_STOP_PTS;

typedef struct t_input_ctrl_start_pts {
    MV_TIMESTAMP    m_PtsValue;
    UINT    m_IsBroadcast;
    //! if m_IsBroadcast != 0, broadcast this tag to all output
    //! if m_IsBroadcast = 0, only output this control tag to the entry m_StreamEntryID
    UINT    m_StreamEntryID;
    MV_PlayItem_ID_t m_PI;
} CONTROL_INPUT_START_PTS;

typedef struct t_input_ctrl_pause_pts {
    MV_TIMESTAMP    m_PtsValue;
    UINT    m_IsBroadcast;
    //! if m_IsBroadcast != 0, broadcast this tag to all output
    //! if m_IsBroadcast = 0, only output this control tag to the entry m_StreamEntryID
    UINT    m_StreamEntryID;
    MV_PlayItem_ID_t m_PI;
} CONTROL_INPUT_PAUSE_PTS;

typedef struct t_input_ctrl_startstoppts_clear_pts {
    UINT    m_DataCount;
    UINT    m_Rsvd1;
} CONTROL_INPUT_START_STOP_PTS_CLEAR;

typedef struct t_input_buffer_flush {
    UINT    m_DataCount;
    UINT    m_Rsvd1;

} CONTROL_INPUT_BUFFER_FLUSH;

typedef struct t_source_change {
    UINT    m_DataCount;
    UINT    m_Rsvd1;

} CONTROL_SOURCE_CHANGE;

typedef struct t_aacs_key_change {
    UINT    m_DataCount;
    UINT    m_Rsvd1;
    UINT    m_TKIdx;    //Title Key Index
}CONTROL_INPUT_AACS_KEY_CHANGE;
typedef struct t_aacs_title_key_backdoor{
    UCHAR    uTK[40];
}CONTROL_INPUT_AACS_TITLE_KEY_HACK;


//! general key change control info
typedef struct t_general_key_change {
    UINT    m_DataCount;
    UINT    m_BlockSize;
    UINT    m_TKIdx;    //Key Index
}CONTROL_INPUT_GEN_KEY_CHANGE;

typedef struct t_CPRM_DCICCI_change {
    UINT8    m_DCICCI[8];
    UINT    m_DCICCISize;
    UINT8    m_DCICCIVer[8];
    UINT    m_DCICCIVerSize;
}CONTROL_INPUT_CPRM_DCICCI_CHANGE;


typedef struct t_input_ctrl_stream_switch {
    UINT    m_DataCount;
    UINT    m_Rsvd1;
    UINT    m_StreamEntry;
} CONTROL_INPUT_STREAM_SWITCH;

typedef struct t_input_ctrl_stream_change {
    UINT32    m_DataCount;
    UINT32    m_Rsvd1;
    UINT32    m_StreamEntry;
    UINT16    m_IDType;
    UINT16    m_ID;
    // Obsolete code, will be removed later. WY, 2010.6.30
    /*
    union{
        MV_PE_VIDEO_ATTRIB    VideoAttrib;
        MV_PE_AUDIO_ATTRIB    AudioAttrib;
    }m_StreamAttrib;
    */
} CONTROL_INPUT_STREAM_CHANGE;

//! DesiredIFrame tag is only for "Special-I-Mode" for VDM
typedef struct t_desired_i_frame {
    UINT    m_Rsvd;
} CONTROL_DESIRED_I_FRAME;

//! dmx will pad data to vbv to video decoder threshold
typedef struct t_slide_show_vid_end{
    UINT32    m_VidStreamEntryID;
    UINT32    m_OpMode;
} CONTROL_SLIDE_SHOW_VIDSEQ_END;

typedef struct t_enter_discard_mode{
    UINT    m_Rsvd;
} CONTROL_ENTER_DISCARD_MODE;

typedef struct t_enter_take_mode{
    UINT    m_Rsvd;
} CONTROL_ENTER_TAKE_MODE;

//! Control info only used by audio
typedef struct t_aud_frame_size {
    UINT    m_nFrmSize;
} CONTROL_AUD_FRAME_SIZE;

typedef struct t_aud_frame_samprate {
    UINT    m_nSampRate;
    // in order to distinguish who produce the control tag
    UINT    m_nProducerType;
} CONTROL_AUD_FRAME_SAMPRATE;

typedef struct t_aud_frame_chmask {
    INT     m_nType;
    UINT    m_nChNum;
    UINT    m_nChMask[8];
} CONTROL_AUD_FRAME_CHMASK;

typedef struct t_aud_drc_info {
    UINT    m_Rsvd;
    // TODO
} CONTROL_AUD_DRC_INFO;

typedef struct t_aud_mix_info {
    INT     m_nType;
    INT     m_nStreamId;
    VOID    *m_pMixMetaData;
} CONTROL_AUD_MIX_INFO;

typedef struct t_aud_dmx_info {
    INT     m_nType;
    VOID    *m_pDmxMetaData;
} CONTROL_AUD_DMX_INFO;

typedef struct t_aud_hdmi_info {
    UINT    m_nHDMIFlag;
} CONTROL_AUD_HDMI_INFO;

typedef struct t_aud_scms_info {
    UINT16  m_nCopyRight;
    UINT16  m_nOrginal;
} CONTROL_AUD_SCMS_INFO;

#if defined(BERLIN_AV_ENGINE)
typedef struct t_aud_frame_end {
    VOID    *m_pDecompressedSample;
    UINT    m_Rsvd;
} CONTROL_AUD_FRM_END;
#endif

typedef struct t_aud_property_info {
    UINT    m_nAudioType;
} CONTROL_AUD_PROPERTY_INFO;

typedef struct t_cdda_consume_datapos {
    UINT    m_LBA;
} CDDA_CONSUME_DATAPOS;

typedef struct t_dummy_info {
    UINT    m_Rsvd;
} CONTROL_DUMMY_INFO;

typedef struct t_clip_info {
    UINT32  m_ClipNum;
    UINT32  m_ClipOffsetHigh;
    UINT32  m_ClipOffsetLow;
} CONTROL_CLIP_INFO;

typedef struct t_media_trans_info {
    UINT8   m_SP[16];
    UINT32  m_SP_ID;
    UINT8   m_FM_ID[8];
} CONTROL_MEDIA_TRANS_INFO;


typedef struct t_stop_pts_detect {
    MV_TIMESTAMP    m_PtsValue;
} CONTROL_STOPPTS_DETECT;

typedef struct t_input_ctrl_video_frame_scaling {
    UINT    m_content_pos_x;         // source offset x
    UINT    m_content_pos_y;         // source offset y
    UINT    m_content_width;         // source picture width, if 0 use source nature size
    UINT    m_content_height;        // source picture height if 0 use source nature size
    UINT    m_disp_pos_x;            // display offset x
    UINT    m_disp_pos_y;            // display offset y
    UINT    m_disp_width;            // display  width,   if 0 use display full screen size
    UINT    m_disp_height;           // display  height,  if 0, use dislay full screen size
    UINT    operation_mode;          // reserved, shall be 0 now.
} CONTROL_VIDEO_FRAME_SCALING;

#ifdef BERLIN_AV_ENGINE
typedef struct t_psample {
    void*   m_pCompressedSample;
    void*   m_pDecompressedSample;
    UINT    m_DecompressedSampleCount;
    UINT    m_Length;
} CONTROL_PSAMPLE;
#endif

typedef struct t_aud_stream_stc_info {
    UINT    m_nStreamId;
    VOID    *m_pCallback;
    VOID    *m_pCallbackCtx;
} CONTROL_AUD_STC_INFO;

typedef struct t_aud_stream_spdif_info {
    UINT    m_nStreamId;
    UINT    m_nSpdifFreq;
    UINT    m_nDigitalData;
    UINT    m_nSpdifMode;
} CONTROL_AUD_SPDIF_INFO;


typedef struct t_wm_payload_ctrl
{
    UINT32  m_IsEncrypted;
    UINT32  m_PayloadSize   :32;
    UINT32  m_Seed[2];
}CONTROL_WM_PAYLOAD_INFO;
typedef struct t_playready_payload_ctrl
{
    UINT32  m_IsEncrypted;
    UINT32  m_PayloadSize;
    UINT64  SampleID;
    UINT64  BlockOffset;
    UINT16  ByteOffset;
}CONTROL_PLAYREADY_PAYLOAD_INFO;

typedef struct t_marlin_payload_ctrl
{
    UINT32  m_IsEncrypted;
    UINT32  m_PayloadSize;
    UINT8   m_IVKey[16];
}CONTROL_MARLIN_PAYLOAD_INFO;

typedef struct t_hls_payload_ctrl
{
    UINT32  m_IsEncrypted;
    UINT32  m_PayloadSize;
    UINT8   m_IVKey[16];
}CONTROL_HLS_PAYLOAD_INFO;

typedef struct t_widevine_payload_ctrl
{
    UINT32  m_IsEncrypted;
    UINT32  m_PayloadSize;
    UINT8   m_IVKey[16];
}CONTROL_WIDEVINE_PAYLOAD_INFO;

typedef struct t_hdcp_payload_ctrl
{
    UINT32  m_IsEncrypted;
    UINT32  m_PayloadSize;
    UINT32  m_StreamCtr;
    UINT32  m_InputCtr[2];
    UINT32  m_Padding0;
}CONTROL_HDCP_PAYLOAD_INFO;

typedef struct t_dtcpip_payload_ctrl
{
    UINT32  m_IsEncrypted;
    UINT32  m_PayloadSize;
    UINT64  Nc;
    UINT32  EmiMode;
}CONTROL_DTCPIP_PAYLOAD_INFO;
typedef struct t_reenctypt_payload_ctrl
{
    UINT32  m_PayloadSize;
}CONTROL_REENCRYPT_PAYLOAD_INFO;

typedef struct t_audio_output_aligned
{
    UINT32  m_AlignType;
    UINT32  m_IsAligned;
}CONTROL_AUDIO_OUTPUT_ALIGNED;

typedef struct t_aud_stream_mix_mask
{
    UINT32  m_nStreamMixMask;
}CONTROL_AUD_STREAM_MIX_MASK;

typedef union t_control_info_audio_misc_union
{
    CONTROL_AUD_STREAM_MIX_MASK m_AudioStreamMixMask;
}CONTROL_INFO_AUDIO_MISC_UNION;

typedef struct t_audio_misc_info
{
    UINT32 m_MiscInfoType;
    CONTROL_INFO_AUDIO_MISC_UNION m_MiscInfo;
}CONTROL_AUDIO_MISC_INFO;

typedef    union t_control_info_union {
    //! TS
    CONTROL_DUMMY_INFO      m_DummyInfo;
    CONTROL_UNITSTART       m_UnitStart;
    CONTROL_UNITEND         m_UnitEnd;
    CONTROL_TRANSPORTERROR  m_TransportError;
    CONTROL_CONTINUITYERROR m_ContinuityError;
    CONTROL_CRCERROR        m_CrcError;

    CONTROL_PES_PRIORITY    m_PESPriority;
    CONTROL_COPYRIGHT       m_CopyRightInfo;
    CONTROL_TRICKMODE       m_TrickMode;
    CONTROL_ESCR_ESRATE     m_ESRateInfo;

    CONTROL_PCRINFO         m_PCRInfo;
    CONTROL_PES_PRIV_DATA   m_PESPrivData;

    //! PCR discontinuity related control tags,
    //! see Note.1 and Note.2
    CONTROL_DISCONT_INDICATE    m_DiscontIndicate;
    CONTROL_PCR_JUMP            m_PCRJump;
    CONTROL_TIMEBASE_NOTIF      m_TimeBaseNotif;

    CONTROL_SCRAMBLE_CHG        m_ScramChg;

    CONTROL_RAMDOM_ACCESS       m_RamdomAccess;
    CONTROL_ES_PRIORITY         m_ESPriority;
    CONTROL_SPLICE_CNTDWN       m_SpliceCountdown;
    CONTROL_SPLICE_INFO         m_SpliceInfo;

    CONTROL_LAST_GOP_NOTIFY     m_LastGopNotify;

    //! PS
    CONTROL_SCR                 m_SCR;
    CONTROL_MUXRATE             m_MuxRate;
    CONTROL_SYSTEM_HEADER       m_SysHeader;
    CONTROL_PSTD_BUFFER_BOUND   m_PSTDBufBound;
    CONTROL_PSTD_BUFFER_SIZE    m_PSTDBufSize;
    CONTROL_FRAME_INFO          m_FrameInfo;
    CONTROL_LPCM_INFO           m_LPCMInfo;
    CONTROL_RA_INFO             m_RAInfo;

    //! Control input
    CONTROL_INPUT_STOP_PTS      m_StopPts;
    CONTROL_INPUT_START_PTS     m_StartPts;
    CONTROL_INPUT_PAUSE_PTS     m_PausePts;
    CONTROL_INPUT_START_STOP_PTS_CLEAR    m_StartStopPtsClear;

    CONTROL_INPUT_BUFFER_FLUSH       m_BufferFlush;
    CONTROL_INPUT_AACS_KEY_CHANGE    m_AACSKeyChange;
    CONTROL_INPUT_CPRM_DCICCI_CHANGE m_CPRMDCICCIChange;
    //CONTROL_INPUT_AACS_TITLE_KEY_HACK m_AACSkeyhack;
    CONTROL_INPUT_GEN_KEY_CHANGE    m_KeyChange;
    //* Removed because its' size >32bytes. WY.2010.6.30
    CONTROL_INPUT_STREAM_CHANGE     m_StreamChange;
    CONTROL_INPUT_STREAM_SWITCH     m_StreamSwitch;
    CONTROL_SOURCE_CHANGE           m_SourceChange;
    CONTROL_DESIRED_I_FRAME         m_DesiredIFrame;
    CONTROL_SLIDE_SHOW_VIDSEQ_END   m_SlideShowVidEnd;
    CONTROL_ENTER_DISCARD_MODE      m_DiscardMode;
    CONTROL_ENTER_TAKE_MODE         m_TakeMode;
    CONTROL_CLIP_INFO               m_ClipInfo;
    CONTROL_MEDIA_TRANS_INFO        m_MediaTransInfo;
    CONTROL_STOPPTS_DETECT          m_StopPtsDetect;
    //! Audio
    CONTROL_AUD_FRAME_SIZE      m_AudioFrmSize;
    CONTROL_AUD_FRAME_SAMPRATE  m_AudioFrmSampRate;
    CONTROL_AUD_FRAME_CHMASK    m_AudioFrmChMask;
    CONTROL_AUD_DRC_INFO        m_AudioDrcInfo;
    CONTROL_AUD_MIX_INFO        m_AudioMixInfo;
    CONTROL_AUD_DMX_INFO        m_AudioDmxInfo;
    CONTROL_AUD_HDMI_INFO       m_AudioHDMIInfo;
    CONTROL_AUDIO_MISC_INFO     m_AudioMiscInfo;
    CONTROL_AUD_SCMS_INFO       m_AudioSCMSInfo;
    CONTROL_AUD_PROPERTY_INFO   m_AudioPropertyInfo;
    CDDA_CONSUME_DATAPOS        m_CDDAConsumeDataPos;
    CONTROL_VIDEO_FRAME_SCALING m_VideoFrameScaling;
    CONTROL_AUD_STC_INFO        m_AudioSTCInfo;
    CONTROL_AUD_SPDIF_INFO      m_AudioSpdifInfo;
    //...
    CONTROL_WM_PAYLOAD_INFO         m_WmPayloadInfo;
    CONTROL_PLAYREADY_PAYLOAD_INFO  m_PrPayloadInfo;
    CONTROL_MARLIN_PAYLOAD_INFO     m_MlPayloadInfo;
    CONTROL_HLS_PAYLOAD_INFO        m_HLSPayloadInfo;
    CONTROL_WIDEVINE_PAYLOAD_INFO   m_WvPayloadInfo;
    CONTROL_HDCP_PAYLOAD_INFO       m_HdcpPayloadInfo;
    CONTROL_DTCPIP_PAYLOAD_INFO     m_DtcpipPayloadInfo;
    CONTROL_REENCRYPT_PAYLOAD_INFO  m_REPayloadInfo;
#ifdef BERLIN_AV_ENGINE
    CONTROL_PSAMPLE                 m_pSample;
    CONTROL_AUD_FRM_END             m_AudioFrmEnd;
#endif
    CONTROL_AUDIO_OUTPUT_ALIGNED    m_AudioOutputAlign;

} CONTROL_INFO_UNION;

#define        CONTROL_INFO_INVALID_USERDATA        0xffffffff

typedef struct t_control_info_unit {
    UINT        m_ControlInfoType   :8;
    UINT        m_Rsvd              :24;
    UINT        m_DataCount;
    UINT        m_PrivateData;
    UINT        m_UserData;             //set to 0xFFFFFFFF means invalid user data
    CONTROL_INFO_UNION    m_Union;
} CONTROL_INFO_UNIT;

typedef struct t_control_info_buf {
    CONTROL_INFO_UNIT    *m_ControlInfoStart;        // buffer pointer
    INT        m_IndexSize;    // Number of records in the buffer
    INT        m_RdIndex;    // read index
    INT        m_WrIndex;    // write index
} CONTROL_INFO_XBV_BUF;

//! defines the min space the control buffer should have
//! before the producer continue to output control information.
#define        AVXBV_CTRL_BUF_MIN_SPACE_SIZE        128

///////////////////// AV ACTION COMMAND BUFFER /////////////////////////////
typedef struct t_audio_cmd_data {
    INT m_Parameter_1;
    INT m_Parameter_2;
} AUDIO_CMD_DATA;

typedef    union t_audio_cmd_data_union {
//  AUDIO_INFO m_audio_info;
    AUDIO_CMD_DATA m_cmd_data;
} AUDIO_CMD_DATA_UNION;

typedef struct t_audio_cmd_cell {
    INT m_SeqId; // sequence ID coming from the upper level
    INT m_UserParam;
    INT m_CmdType;
    INT m_StreamId; // Input stream
    INT m_PathId; // output path
    INT m_Asap;    // 1: ASAP (will ignore m_Effect_PTS), 0: not ASAP
    UINT m_EffectPts; // EffectPts and EffectCntr will composed the precisely control.
    UINT m_EffectCntr;
    AUDIO_CMD_DATA_UNION m_Union;
    INT m_CmdStatus;
    VOID (*m_CallBackFunction)(VOID *pInfo);
} AUDIO_CMD_CELL;

typedef struct t_vidio_cmd_data {
    INT m_Parameter_1;
    INT m_Parameter_2;
    INT m_Parameter_3;
} VIDEO_CMD_DATA;

typedef    union t_video_cmd_data_union {
    VIDEO_CMD_DATA m_cmd_data;
} VIDEO_CMD_DATA_UNION;

typedef struct t_video_cmd_cell {
    INT m_CmdType;
    VIDEO_CMD_DATA_UNION m_Union;
} VIDEO_CMD_CELL;

typedef struct t_av_cmd_ring_buf {
    union {
        AUDIO_CMD_CELL *m_AudioCmdStart;
        VIDEO_CMD_CELL *m_VideoCmdStart;
    };
    INT m_CmdRdIndex;
    INT m_CmdWrIndex;
    INT m_CmdSize;
} AV_CMD_RING_BUF;

typedef struct t_av_cmd_que_list {
    union {
        AUDIO_CMD_CELL    m_AudioCmdQueCell;
        VIDEO_CMD_CELL    m_VideoCmdQueCell;
    };
    struct t_av_cmd_que_list *m_Next;
    struct t_av_cmd_que_list *m_Prev;
} AV_CMD_QUE_LIST;

typedef struct t_AV_cmd_que_buffer {
    AV_CMD_QUE_LIST    *m_FreeList;
    AV_CMD_QUE_LIST    *m_BusyList;
    AV_CMD_QUE_LIST    *m_CmdListHead; // Assuming that we allocate the sequencial buffer at once.
    UINT            m_QueSize;
    UINT            m_cmdque_format;
} AV_CMD_QUE_BUFFER;

//////////////// Command Que Format ////////////////////////////
enum {
    eCmdQueAudio    = 1,
    eCmdQueVideo,
};

/////////////////// command type ////////////////////////
enum {
    eDRM_WMPayloadCtrl=0x10,       //TspEventType_Define_WMEV_PAYLOAD_START
    eDRM_PlayReadyPayloadCtrl=0x11,
    eDRM_MarlinPayloadCtrl=0x12,
    eDRM_WidevinePayloadCtrl=0x13,
    eDRM_HLSPayloadCtrl=0x14,
    eDRM_ReencryptPayloadCtrl=0x15,
    eDRM_HDCPPayloadCtrl=0x16,
    eDRM_DTCPIPPayloadCtrl=0x17,
    //control info from bitstream
    eBS_UnitStart = 0xa0,
    eBS_UnitEnd = 0xa1,
    eBS_TransportError = 0xa2,
    eBS_ContinuityError = 0xa3,
    eBS_ScrambledTS = 0xa4,
    eBS_ScrambledPES = 0xa5,
    eBS_PesPriority = 0xa6,
    eBS_CopyRight = 0xa7,
    eBS_TrickMode = 0xa8,
    eBS_EscrEsRate = 0xa9,
    eBS_PcrInfo = 0xaa,
    eBS_DiscontIndicate = 0xab,
    eBS_RamdomAccess = 0xac,
    eBS_EsPriority = 0xad,
    eBS_SpliceCountdown = 0xae,
    eBS_SpliceInfo = 0xaf,


    eBS_SCR = 0xb0,
    eBS_MuxRate = 0xb1,
    eBS_SysHeader = 0xb2,
    eBS_PSTDBufBound = 0xb3,
    eBS_PSTDBufSize = 0xb4,
    eBS_FrameInfo = 0xb5,
    eBS_LPCMInfo = 0xb6,
    eBS_RAInfo = 0xb7,

    eBS_TsScramChg = 0xc0,
    eBS_PesScramChg = 0xc1,

    eBS_CrcError = 0xc2,

    //! for precise info of un-announced pcr discontinuity in dtv
    eBS_PcrJump = 0xc3,
    eBS_TimeBaseNotif = 0xc4,

    eBS_PesPrivData = 0xc5,

    //! for notification to dmx the last video gop is followed.
    eBS_LastGopNotify = 0xc6,

    eBS_BufferFull = 0xc7,

    //demux input control info
#define    MIN_INPUT_CTRL_SOURCE    0xd0
#define    MAX_INPUT_CTRL_SOURCE    0xdf

    eIN_StartPts = 0xd0,
    eIN_StopPts = 0xd1,
    eIN_BufferFlush = 0xd2,
    eIN_AACSKeyChange = 0xd3,
    eIN_GenKeyChange = 0xd3,
    eIN_CPRMDCICCIChange = 0xd3,
    eIN_StopCmd    = 0xd4,        //currently only used inside figo
    eIN_DesiredIFrame = 0xd5,
    eIN_SlideShowVidEnd = 0xd6,
    eIN_DiscardMode = 0xd7,
    eIN_TakeMode = 0xd8,
    eIN_PausePts = 0xd9,
    eIN_StartStopPtsClear = 0xda,
    eIN_ClipInfo = 0xdb,
    eIN_MediaTransInfo = 0xdc,
    eIN_StopPTSDetect = 0xdd,

#define    MIN_INPUT_CTRL_STRM        0xe0
#define    MAX_INPUT_CTRL_STRM        0xe8

    eIN_StreamChange = 0xe0,
    eIN_StreamEnable = 0xe1,
    eIN_StreamDisable = 0xe2,

#ifdef BERLIN_AV_ENGINE
    eIN_StreampSample = 0xe3,
#endif

#define MIN_VIDEO_CTRL_INFO        0xe9
#define MAX_VIDEO_CTRL_INFO        0xef
    eVID_PerFrameScaling = 0xe9,

#define MIN_AUDIO_CTRL_INFO        0xf0
#define MAX_AUDIO_CTRL_INFO        0xff

    eAUD_StreamFrmSize        = 0xf0,
    eAUD_StreamFrmSampRate    = 0xf1,
    eAUD_StreamFrmChMask    = 0xf2,
    eAUD_StreamDrcInfo        = 0xf3,
    eAUD_StreamMixInfo        = 0xf4,
    eAUD_StreamDmxInfo        = 0xf5,
    eAUD_StreamHDMIInfo     = 0xf6,
    eAUD_MiscInfo            = 0xf7,
    eIN_CDDAConsumePos = 0xf8,
    eAUD_StreamSCMSInfo        = 0xf9,
    eAUD_LineInActiveState        = 0xfa,
#if defined(BERLIN_AV_ENGINE)
    eAUD_StreamFrmEnd        = 0xfb,
#endif
    eAUD_StreamProperty        = 0xfc,
    eAUD_StreamSTCInfo        = 0xfd,
    eAUD_StreamSpdifInfo        = 0xfe,
    eAUD_OutputALign            = 0xff,

 };

#endif //__CMD_BUF_H__



/////////////////////////// AV Command Queue Buffer ////////////////////////////////////
/**************************************************************************************
 * Function:    AVCmdQueCreate
 *
 * Description: Create CmdQue Buffer.
 *
 * Inputs:      cmd_que_size --- item size of the command Queue
 *              cmd_que_format-- specify the format of the command queue. for audio/video
 * Outputs:     none
 *
 * Return:      return the handler of cmmand queue buffer
 **************************************************************************************/
VOID *AVCmdQueCreate(INT cmd_que_size, INT cmd_que_format);

/**************************************************************************************
 * Function:    AVCmdQueFree
 *
 * Description: Free CmdQue Buffer.
 *
 * Inputs:      pHandler --- handler of cmmand queue buffer.
 *
 * Outputs:     pHandler --- handler will be NULL.
 *
 * Return:      0 means success, none 0 means command queue is empty.
 **************************************************************************************/
INT AVCmdQueFree(AV_CMD_QUE_BUFFER **pHandler);

/**************************************************************************************
 * Function:    GetCmdQueFreeCell
 *
 * Description: Get a free CmdQueCell from Free List.
 *
 * Inputs:      pCmdList --- Command queue buffer handler.
 *
 * Outputs:     none
 *
 * Return:      pointer of the free command queue cell.
 **************************************************************************************/
VOID *GetAVCmdQueFreeCell(AV_CMD_QUE_BUFFER *pCmdList);

/**************************************************************************************
 * Function:    AddCmdQueToBusyList
 *
 * Description: Add the cmdQue into Busy List. For audio, the command will be reorganized
 *              In order according to the PTS associated with the command.
 *
 * Inputs:      pCmdList --- Command queue buffer handler.
 *              pList-- command queue cell handler
 * Outputs:     none
 *
 * Return:      none
 **************************************************************************************/
VOID AddCmdQueToBusyList(AV_CMD_QUE_BUFFER *pCmdList, AV_CMD_QUE_LIST *pList);

/**************************************************************************************
 * Function:    PutCmdQueCellIntoFreeList
 *
 * Description: Put the command Que cell into free list
 *
 * Inputs:      pCmdList --- Command queue buffer handler.
 *              pList-- command queue cell handler
 * Outputs:     none
 *
 * Return:      none
 **************************************************************************************/
VOID PutCmdQueCellIntoFreeList(AV_CMD_QUE_BUFFER *pCmdList, AV_CMD_QUE_LIST *pList);

/**************************************************************************************
 * Function:    RemoveCmdQueFromBusyList
 *
 * Description: Search for the pList in CMD_QUE_BUFFER, remove the pList from the
 *              CMD_QUE_BUFF busy_list.
 *
 * Inputs:      pCmdList --- Command queue buffer handler.
 *              pList-- command queue cell handler
 * Outputs:     none
 *
 * Return:      none
 **************************************************************************************/
VOID RemoveCmdQueFromBusyList(AV_CMD_QUE_BUFFER *pCmdList, AV_CMD_QUE_LIST **pList);
