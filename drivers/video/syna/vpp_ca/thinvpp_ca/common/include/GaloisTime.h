/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 *
 */

////////////////////////////////////////////////////////////////////////////////
//! \file GaloisTime.h
//! \brief
//!
//! Purpose:
//!
//!
//!	Version, Date and Author :
//!		V 1.00,	7/19/2007,	Fang Bao
//!		V 1.01,	10/08/2007,	Fang Bao	Add the API - MV_TimePTS_SetbyPTS, MV_TimePTS_AddbyUINT64
//!		V 1.02,	1/23/2008,	Yufeng Zheng, macro to access raw timer(AKA performance timer.)
//!		V 1.03,	1/31/2008,	Fang Bao, Add the MV_Time_GetPFTIMER()
//!		V 1.04,	4/17/2007,	Fang Bao	New APIs - MV_Time_Init(), MV_Time_Exit() for MV_Time_GetPFTIMER()
//!										New API - MV_Time_GetSysTime() Windows and Linux are available.
//!		V 1.05,	4/21/2007,	Fang Bao	Add SYSTEM_TIMER_VALUE,SYSTEM_TIMER_RATIO,system_timer_jiffies for MV_Time_GetSysTime() in eCos OS
//!		V 1.06,	5/07/2007,	Fang Bao	Bugfix - timespec defined in WIN32, time_t is not same as Linux. So long is the better type.
//!		V 1.09,	6/05/2008,	Fang Bao	Remove old MV_Time_GetSysTimeMS(), add new MV_Time_GetSysTimeMS() and MV_Time_GetOSTimeMS()
//!		V 1.10,	7/22/2007,	Fang Bao	Bugfix - MV_Time_GetSysTime() and MV_Time_GetSysTimeMS() will get the error value from TIMER6
//!										Add MV_Time_GetTIMER6() and MV_Time_GetTIMER7() which is supported by module of REGMAP in Linux Kernel
//!		V 1.11,	7/23/2007,	Fang Bao	code merge - MV_TimePTS_Diff() added by yongchun
//!		V 1.12,	6/2/2009,	Fang Bao	add MV_Time_System_Timer_Clock to store cfgClk setting.
//!		V 1.13,	6/3/2009,	Fang Bao	add _CYGNUM_HAL_RTC_PERIOD_value to replace CYGNUM_HAL_RTC_PERIOD in driver file "galois_misc.c"
//!		V 1.14,	6/11/2009,	Fang Bao	modify MV_TimePlay_TimeStr_t to remove a building warnings
//!		V 1.14,	7/10/2009,	Fang Bao	add CMD_ICCEXTRA_GET_CLOCK for new MV_Time_GetSysTime() and ICCEXTRA module
//!		V 1.20,	7/14/2009,	Fang Bao	add new APIs - MV_Time_GetSysTime_NEW & MV_Time_GetSysTimeMS_NEW
//!		V 1.21,	7/28/2009,	Fang Bao	add CMD_ICCEXTRA_GET_CLOCK_ALL for MV_Time_GetSysTime_NEW & MV_Time_GetSysTimeMS_NEW
//!		V 1.22,	2/22/2010,	Fang Bao	add Android support
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef _GALOISTIME_H_
#define _GALOISTIME_H_

#if defined( __LINUX__ )
#include "time.h"
#elif defined( __ECOS__ )
#include <sys/time.h>
#endif

#include "com_type.h"
#include "ErrorCode.h"

#ifdef __cplusplus
extern "C"  {
#endif

////////////////////////////////////////////////////////////////////////////////
//! \brief Access raw timer(performance timer) for test and/or debug.
//! [ioctl commands] for Linux
//!
////////////////////////////////////////////////////////////////////////////////
#if !defined(WIN32) && !defined(SoC_PCSim)

extern unsigned long *ptr_system_timer_jiffies;
extern unsigned long *ptr_system_clock_timer;
extern unsigned long *ptr_system_clock_second;
extern unsigned int MV_Time_System_Timer_Clock;			// GaloisTime.c
extern unsigned long _CYGNUM_HAL_RTC_PERIOD_value;		// galois_misc.c

#define CMD_PFTIMER_GET_VALUE 				(0x1000)

#define CMD_ICCEXTRA_GET_SYSTIME			(0x1F07)
#define CMD_ICCEXTRA_TEST_01				(0x1F08)
#define CMD_ICCEXTRA_GET_CLOCK				(0x1F09)
#define CMD_ICCEXTRA_GET_CLOCK_ALL			(0x1F10)

#define MV_Time_FLOAT_TO_FIXED(x, fract_bits)	((INT32)((x) * (0x01LL << fract_bits)))
#define MV_Time_TIMER_Diff(x_new, y_old)	( (x_new < y_old) ? (y_old - x_new): ~(x_new - y_old))

typedef struct _system_clock_data {
         unsigned long   	m_Clock_Second;
         unsigned long     	m_Clock_Timer;
         unsigned long		m_Hardware_Timer;
} system_clock_data_t;

#endif

#if defined(WIN32) || defined(__UBOOT__)
struct timespec {
         long   	tv_sec;         		/* seconds */
         long     	tv_nsec;        		/* nanoseconds */
};

#endif


typedef struct timespec MV_TimeSpec_t, *pMV_TimeSpec_t;
typedef struct timeval 	MV_TimeVal_t, *pMV_TimeVal_t;


////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MV_HANDLE_Time_t | Handle to a time object
//!
////////////////////////////////////////////////////////////////////////////////
typedef PVOID 	MV_HANDLE_Time_t;
typedef PVOID 	*pMV_HANDLE_Time_t;


////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MV_HANDLE_TimePlay_t | Handle to a time object for playing
//!
////////////////////////////////////////////////////////////////////////////////
typedef PVOID 	MV_HANDLE_TimePlay_t;
typedef PVOID 	*pMV_HANDLE_TimePlay_t;

typedef struct {
            UINT16 is_pi;	    /* if 1: pi_id is playitem id; don¡¯t care sub_pi_id and sync_pi_id
							    if 0: sub_pi_id is sub playitem id; if sync_pi_id == 0xffff, sub playitem is
							    not synced with playitem; otherwise sync_pi_id is the playitem id synced with the sub playitem id */
            UINT16 pi_id;       /* 0xffff: invalid; otherwise playitem id */
            UINT16 sub_pi_id;   /* 0xffff: invalid; otherwise sub playitem id */
            UINT16 sync_pi_id;  /* 0xffff: invalid; otherwise playitem id synced with the sub playitem id */
} MV_PlayItem_ID_t;

////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MV_TimePTS_t | PTS time stamp
//!
////////////////////////////////////////////////////////////////////////////////
typedef struct _MV_TimePTS
{						// PTS is a 33bit unsigned integer
	UINT32 m_high;		// Only the lowest bit is available for calculating, MSB has some special meanings
	UINT32 m_low;		// 32bit is available
  MV_PlayItem_ID_t PI;
} MV_TimePTS_t, *pMV_TimePTS_t;

#define MV_TIMEPTS_INITVALUE        {0, 0, }
#define MV_TIMEPTS_FREQUENCY		(90000)
#define MV_TIMEPTS_HIGHMASK			(0x00000001)
#define MV_TIMEPTS_VALIDMASK		(0x80000000)
#define MV_TIMEPTS_PTSPERHOUR		(MV_TIMEPTS_FREQUENCY * 60 * 60)
#define MV_TIMEPTS_PTSPERMINUTE		(MV_TIMEPTS_FREQUENCY * 60)
#define MV_TIMEPTS_PTSPERSEC		(MV_TIMEPTS_FREQUENCY)

#define	RESET_PTS(x)		        do{(x).m_high = 0; (x).m_low=0;}while(0)
#define	IS_PTS_VALID(x)		        ((x).m_high & MV_TIMEPTS_VALIDMASK)
#define	SET_PTS_VALID(x)	        do{(x).m_high |= MV_TIMEPTS_VALIDMASK;}while(0)


////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MV_TimePTSCounter_t | PTS time Counter
//!
////////////////////////////////////////////////////////////////////////////////
typedef struct _MV_TimePTSCounter
{
	MV_TimePTS_t	m_Counter;
	INT				m_Counter_Residua_Numerator;

	INT				m_Counter_Step_Integer;
	INT				m_Counter_Step_Numerator;
	UINT			m_Counter_Step_Denominator;

} MV_TimePTSCounter_t, *pMV_TimePTSCounter_t;








////////////////////////////////////////////////////////////////////////////////
//----------------------------------TimePlay------------------------------------
////////////////////////////////////////////////////////////////////////////////


typedef enum _MV_TimePlay_FrameType
{
	MV_TimePlay_FrameType_50 = 50,
	MV_TimePlay_FrameType_60 = 60,
	MV_TimePlay_FrameType_75 = 75

} MV_TimePlay_FrameType_t;


typedef enum _MV_TimePlay_TimeType
{
	MV_TimePlay_TimeType_Str = 0,
	MV_TimePlay_TimeType_Num = 1,
	MV_TimePlay_TimeType_BCD = 2

} MV_TimePlay_TimeType_t;


typedef struct _MV_TimePlay_TimeCommon
{
	MV_TimePlay_TimeType_t m_TimeType;
	MV_TimePlay_FrameType_t m_FrameType;

} MV_TimePlay_TimeCommon_t, *pMV_TimePlay_TimeCommon_t;


typedef struct _MV_TimePlay_TimeStr
{
	MV_TimePlay_TimeType_t m_TimeType;
	MV_TimePlay_FrameType_t m_FrameType;

	CHAR m_Time[12];		// HH:MM:SS:FF\0

} MV_TimePlay_TimeStr_t, *pMV_TimePlay_TimeStr_t;


typedef struct _MV_TimePlay_TimeNum
{
	MV_TimePlay_TimeType_t m_TimeType;
	MV_TimePlay_FrameType_t m_FrameType;

	UCHAR m_HH;				// HH
	UCHAR m_MM;				// MM
	UCHAR m_SS;				// SS
	UCHAR m_FF;				// FF

} MV_TimePlay_TimeNum_t, *pMV_TimePlay_TimeNum_t;


typedef struct _MV_TimePlay_TimeBCD		// BCD = 8421
{
	MV_TimePlay_TimeType_t m_TimeType;
	MV_TimePlay_FrameType_t m_FrameType;

	UCHAR m_HH_BCD;			// HH = 4:4
	UCHAR m_MM_BCD;			// HH = 3:4		1bit pading
	UCHAR m_SS_BCD;			// HH = 3:4
	UCHAR m_FF_BCD;			// HH = 3:4

} MV_TimePlay_TimeBCD_t, *pMV_TimePlay_TimeBCD_t;




////////////////////////////////////////////////////////////////////////////////
//----------------------------------TimePTS-------------------------------------
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTS_Equal
//!
//! Description: whether a and b is equal
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimePTS_t
//! \param b 	(IN):	The pointer to the b which type is MV_TimePTS_t
//!
//! \return Return:		>0	= Equal
//!						0	= Not Equal
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimePTS_Equal(MV_TimePTS_t *a, MV_TimePTS_t *b);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTS_Compare
//!
//! Description: compare a with b
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimePTS_t
//! \param b 	(IN):	The pointer to the b which type is MV_TimePTS_t
//!
//! \return Return:		>0	= a > b
//!						0	= a = b
//!						<0	= a < b
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_TimePTS_Compare(MV_TimePTS_t *a, MV_TimePTS_t *b);



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTS_Set
//!
//! Description: Set a number to the PTS time value;
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimePTS_t
//! \param b 	(IN):	The vaule of a PTS time
//!
//! \return Return:		S_OK
//! 					E_OUTOFRANGE
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimePTS_Set(MV_TimePTS_t *a, UINT64 b);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTS_Get
//!
//! Description: Get a number from the PTS time value;
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimePTS_t
//! \param b 	(IN):	The vaule of a PTS time
//!
//! \return Return:		S_OK
//! 					E_OUTOFRANGE
//!
//!
////////////////////////////////////////////////////////////////////////////////
UINT64  MV_TimePTS_Get(MV_TimePTS_t *a);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTS_Add
//!
//! Description: Add a to b, if the result greater than 33bit number, the exceeding bit will be ommited.
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimePTS_t
//! \param b 	(IN):	The pointer to the b which type is MV_TimePTS_t
//!
//! \return Return:		result of (a+b)
//!
//!
////////////////////////////////////////////////////////////////////////////////
MV_TimePTS_t MV_TimePTS_Add(MV_TimePTS_t *a, MV_TimePTS_t *b);



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTS_Sub
//!
//! Description: Subtract b from a
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimePTS_t
//! \param b 	(IN):	The pointer to the b which type is MV_TimePTS_t
//!
//! \return Return:		result of (a-b), but a signed integer
//!
//!
////////////////////////////////////////////////////////////////////////////////
INT64  MV_TimePTS_Sub(MV_TimePTS_t *a, MV_TimePTS_t *b);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTS_Diff
//!
//! Description: the PTS difference between a and b,
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimePTS_t
//! \param b 	(IN):	The pointer to the b which type is MV_TimePTS_t
//!
//! \param pts_diff	(OUT):	Give the diff value, NOTE UINT32;
//! \return Return:		with wrapper around compare between the two PTS,
//!						>0  A later than B
//!						0, same time or B later than A, from the diff.
//!						<0, Invid compare
//!
////////////////////////////////////////////////////////////////////////////////
INT32  MV_TimePTS_Diff(MV_TimePTS_t *a, MV_TimePTS_t *b, UINT32 *pts_diff);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTS_SetbyPTS
//!
//! Description: Set a PTS to the PTS time value;
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimePTS_t
//!
//! \return Return:		the result of PTS
//!
//!
////////////////////////////////////////////////////////////////////////////////
MV_TimePTS_t  MV_TimePTS_SetbyPTS(MV_TimePTS_t *a);



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTS_AddbyUINT64
//!
//! Description: Add a to b, if the result greater than 33bit number, the exceeding bit will be ommited.
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimePTS_t
//! \param b 	(IN):	The vaule of a PTS time
//!
//! \return Return:		result of (a+b)
//!
//!
////////////////////////////////////////////////////////////////////////////////
MV_TimePTS_t MV_TimePTS_AddbyUINT64(MV_TimePTS_t *a, UINT64 b);







////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTS_ToTimeVal
//!
//! Description: convert the time which type is MV_TimePTS_t to the time which type is MV_TimeVal_t
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimePTS_t
//!
//! \return Return:		the time which type is MV_TimeVal_t
//!
//!
////////////////////////////////////////////////////////////////////////////////
MV_TimeVal_t  MV_TimePTS_ToTimeVal(MV_TimePTS_t *a);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTS_ToTimeSpec
//!
//! Description: convert the time which type is MV_TimePTS_t to the time which type is MV_TimeSpec_t
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimePTS_t
//!
//! \return Return:		the time which type is MV_TimeSpec_t
//!
//!
////////////////////////////////////////////////////////////////////////////////
MV_TimeSpec_t  MV_TimePTS_ToTimeSpec(MV_TimePTS_t *a);




////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTS_ToTimePlay
//!
//! Description: convert the time which type is MV_TimePTS_t to the time which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimePTS_t
//! \param b 	(OUT):	The pointer to the b which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_TimePTS_ToTimePlay(MV_TimePTS_t *a, MV_HANDLE_TimePlay_t b);




////////////////////////////////////////////////////////////////////////////////
//----------------------------------TimePTSConter-------------------------------------
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTSCounter_Set
//!
//! Description: Set the PTS Counter by PTS value
//!
//! \param pCounter	(IN/OUT):	The pointer to the counter
//! \param a 			(IN):	The pointer to the a which type is MV_TimePTS_t
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimePTSCounter_Set(pMV_TimePTSCounter_t pCounter, MV_TimePTS_t *a);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTSCounter_SetStep
//!
//! Description: Set the Step value of PTS Countet
//!
//! \param pCounter	(IN/OUT):	The pointer to the counter
//! \param Integer		(IN):	The Integer of step ( unit = PTS )
//! \param Numerator	(IN):	The Numerator of step ( unit = PTS )
//! \param Denominator	(IN):	The Denominator of step ( unit = PTS )
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimePTSCounter_SetStep(pMV_TimePTSCounter_t pCounter,
									INT 				Integer,
									INT 				Numerator,
									UINT 				Denominator);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTSCounter_Get
//!
//! Description: Get the PTS Counter by PTS value
//!
//! \param pCounter	(IN/OUT):	The pointer to the counter
//! \param a 			(IN):	The pointer to the a which type is MV_TimePTS_t
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimePTSCounter_Get(pMV_TimePTSCounter_t pCounter, MV_TimePTS_t *a);



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTSCounter_GetStep
//!
//! Description: Get the step value of PTS Countet
//!
//! \param pCounter		(IN):	The pointer to the counter
//! \param Integer		(OUT):	The Integer of step ( unit = PTS )
//! \param Numerator	(OUT):	The Numerator of step ( unit = PTS )
//! \param Denominator	(OUT):	The Denominator of step ( unit = PTS )
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimePTSCounter_GetStep(pMV_TimePTSCounter_t pCounter,
									INT 				*Integer,
									INT 				*Numerator,
									UINT 				*Denominator);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTSCounter_Step
//!
//! Description: add a step
//!
//! \param pCounter		(IN):	The pointer to the counter
//!
//! \return Return:		S_OK
//!						E_OUTOFRANGE
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimePTSCounter_Step(pMV_TimePTSCounter_t pCounter);



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTSCounter_MultiStep
//!
//! Description: add several steps
//!
//! \param pCounter		(IN):	The pointer to the counter
//! \param Multiple		(IN):	The Multiple of the steps
//!
//! \return Return:		S_OK
//!						E_OUTOFRANGE
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimePTSCounter_MultiStep(pMV_TimePTSCounter_t pCounter, UINT Multiple);



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTSCounter_Add
//!
//! Description: add a PTS value to the counter
//!
//! \param pCounter	(IN/OUT):	The pointer to the counter
//! \param a		(IN):	The value of the PTS time
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimePTSCounter_Add(pMV_TimePTSCounter_t pCounter, MV_TimePTS_t *a);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTSCounter_Sub
//!
//! Description: sub a PTS value from the counter (if result is less than zero, result is zero and returns E_OUTOFRANGE.)
//!
//! \param pCounter	(IN/OUT):	The pointer to the counter
//! \param a		(IN):	The value of the PTS time
//!
//! \return Return:		S_OK
//!						E_OUTOFRANGE
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimePTSCounter_Sub(pMV_TimePTSCounter_t pCounter, MV_TimePTS_t *a);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTSCounter_Equal
//!
//! Description: test if counter is equal to the PTS (the delta is less than mini unit of PTS)
//!
//! \param pCounter	(IN/OUT):	The pointer to the counter
//! \param a		(IN):	The value of the PTS time
//!
//! \return Return:		>0	= Equal
//!						0	= Not Equal
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimePTSCounter_Equal(pMV_TimePTSCounter_t pCounter, MV_TimePTS_t *a);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTSCounter_Compare
//!
//! Description: compare the counter to the PTS time (return 0 only when they are exactly same)
//!
//! \param pCounter	(IN/OUT):	The pointer to the counter
//! \param a		(IN):	The value of the PTS time
//!
//! \return Return:		>0	= Counter > a
//!						0	= Counter = a
//!						<0	= Counter < a
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimePTSCounter_Compare(pMV_TimePTSCounter_t pCounter, MV_TimePTS_t *a);





////////////////////////////////////////////////////////////////////////////////
//----------------------------------TimeSpec-------------------------------------
////////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimeSpec_Equal
//!
//! Description: whether a and b is equal
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimeSpec_t
//! \param b 	(IN):	The pointer to the b which type is MV_TimeSpec_t
//!
//! \return Return:		>0	= Equal
//!						0	= Not Equal
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimeSpec_Equal(MV_TimeSpec_t *a, MV_TimeSpec_t *b);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimeSpec_Compare
//!
//! Description: compare a with b
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimeSpec_t
//! \param b 	(IN):	The pointer to the b which type is MV_TimeSpec_t
//!
//! \return Return:		>0	= a > b
//!						0	= a = b
//!						<0	= a < b
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimeSpec_Compare(MV_TimeSpec_t *a, MV_TimeSpec_t *b);





////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimeSpec_Add
//!
//! Description: Add a to b
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimeSpec_t
//! \param b 	(IN):	The pointer to the b which type is MV_TimeSpec_t
//!
//! \return Return:		result of (a+b)
//!
//!
////////////////////////////////////////////////////////////////////////////////
MV_TimeSpec_t  MV_TimeSpec_Add(MV_TimeSpec_t *a, MV_TimeSpec_t *b);



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimeSpec_Sub
//!
//! Description: Subtract b from a
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimeSpec_t
//! \param b 	(IN):	The pointer to the b which type is MV_TimeSpec_t
//!
//! \return Return:		result of (a-b)
//!
//!
////////////////////////////////////////////////////////////////////////////////
MV_TimeSpec_t  MV_TimeSpec_Sub(MV_TimeSpec_t *a, MV_TimeSpec_t *b);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimeSpec_ToTimePTS
//!
//! Description: convert the time which type is MV_TimeSpec_t to the time which type is MV_TimePTS_t
//! warning:	the time should be greater than zero.
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimePTS_t
//!
//! \return Return:		the time which type is MV_TimePTS_t
//!
//!
////////////////////////////////////////////////////////////////////////////////
MV_TimePTS_t  MV_TimeSpec_ToTimePTS(MV_TimeSpec_t *a);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimeSpec_ToTimePlay
//!
//! Description: convert the time which type is MV_TimeSpec_t to the time which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//! warning:	the time should be greater than zero.
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimeSpec_t
//! \param b 	(OUT):	The pointer to the b which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimeSpec_ToTimePlay(MV_TimeSpec_t *a, MV_HANDLE_TimePlay_t b);



////////////////////////////////////////////////////////////////////////////////
//----------------------------------TimeVal-------------------------------------
////////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimeVal_Equal
//!
//! Description: whether a and b is equal
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimeVal_t
//! \param b 	(IN):	The pointer to the b which type is MV_TimeVal_t
//!
//! \return Return:		>0	= Equal
//!						0	= Not Equal
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimeVal_Equal(MV_TimeVal_t *a, MV_TimeVal_t *b);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimeVal_Compare
//!
//! Description: compare a with b
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimeVal_t
//! \param b 	(IN):	The pointer to the b which type is MV_TimeVal_t
//!
//! \return Return:		>0	= a > b
//!						0	= a = b
//!						<0	= a < b
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimeVal_Compare(MV_TimeVal_t *a, MV_TimeVal_t *b);



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimeVal_Add
//!
//! Description: Add a to b
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimeVal_t
//! \param b 	(IN):	The pointer to the b which type is MV_TimeVal_t
//!
//! \return Return:		result of (a+b)
//!
//!
////////////////////////////////////////////////////////////////////////////////
MV_TimeVal_t  MV_TimeVal_Add(MV_TimeVal_t *a, MV_TimeVal_t *b);



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimeVal_Sub
//!
//! Description: Subtract b from a
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimeVal_t
//! \param b 	(IN):	The pointer to the b which type is MV_TimeVal_t
//!
//! \return Return:		result of (a-b)
//!
//!
////////////////////////////////////////////////////////////////////////////////
MV_TimeVal_t  MV_TimeVal_Sub(MV_TimeVal_t *a, MV_TimeVal_t *b);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimeVal_ToTimePTS
//!
//! Description: convert the time which type is MV_TimeVal_t to the time which type is MV_TimePTS_t
//! warning:	the time should be greater than zero.
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimePTS_t
//!
//! \return Return:		the time which type is MV_TimePTS_t
//!
//!
////////////////////////////////////////////////////////////////////////////////
MV_TimePTS_t  MV_TimeVal_ToTimePTS(MV_TimeVal_t *a);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimeVal_ToTimePlay
//!
//! Description: convert the time which type is MV_TimeVal_t to the time which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//!
//! \param a 	(IN):	The pointer to the a which type is MV_TimeVal_t
//! \param b 	(OUT):	The pointer to the b which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimeVal_ToTimePlay(MV_TimeVal_t *a, MV_HANDLE_TimePlay_t b);




////////////////////////////////////////////////////////////////////////////////
//----------------------------------TimePlay-------------------------------------
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePlay_ToTimePTS
//!
//! Description: convert the time which type is MV_HANDLE_TimePlay_t to the time which type is MV_TimePTS_t
//!
//! \param a 	(IN):	The pointer to the a which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//! \param b 	(OUT):	The pointer to the b which type is MV_TimePTS_t
//!
//! \return Return:		the time which type is MV_TimePTS_t
//!
//!
////////////////////////////////////////////////////////////////////////////////
MV_TimePTS_t MV_TimePlay_ToTimePTS(MV_HANDLE_TimePlay_t a);



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePlay_SetByPTS
//!
//! Description: Set a MV_HANDLE_TimePlay_t, if b is null, set by 0;
//!
//! \param a 		(IN/OUT):	The pointer to the a which type is MV_HANDLE_TimePlay_t which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//! \param FrameType	(IN):	The frame type to the MV_HANDLE_TimePlay_t
//! \param TimeType		(IN):	The time type to the MV_HANDLE_TimePlay_t
//! \param b 			(IN):	The pointer to the b which type is MV_TimePTS_t
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_TimePlay_SetByPTS(MV_HANDLE_TimePlay_t a, MV_TimePlay_FrameType_t FrameType, MV_TimePlay_TimeType_t TimeType, MV_TimePTS_t *b);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePlay_Equal
//!
//! Description: whether a and b is equal
//!
//! \param a 	(IN):	The pointer to the a which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//! \param b 	(IN):	The pointer to the b which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//!
//! \return Return:		>0	= Equal
//!						0	= Not Equal
//!						<0	= Error
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimePlay_Equal(MV_HANDLE_TimePlay_t a, MV_HANDLE_TimePlay_t b);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePlay_Compare
//!
//! Description: compare a with b
//!
//! \param a 	(IN):	The pointer to the a which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//! \param b 	(IN):	The pointer to the b which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//!
//! \return Return:		1	= a > b
//!						0	= a = b
//!						-1	= a < b
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimePlay_Compare(MV_HANDLE_TimePlay_t a, MV_HANDLE_TimePlay_t b);



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePlay_Add
//!
//! Description: Add a to b, return the result to a
//!
//! \param a(IN/OUT):	The pointer to the a which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//! \param b 	(IN):	The pointer to the b which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//!
//! \return Return:		S_OK (a = a + b)
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_TimePlay_Add(MV_HANDLE_TimePlay_t a, MV_HANDLE_TimePlay_t b);



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePlay_Sub
//!
//! Description: Subtract b from a, return the result to a
//!
//! \param a(IN/OUT):	The pointer to the a which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//! \param b 	(IN):	The pointer to the b which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//!
//! \return Return:		>0 (a = a - b)
//! 					=0 (a = 0)
//!						<0 (a = b - a)
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_TimePlay_Sub(MV_HANDLE_TimePlay_t a, MV_HANDLE_TimePlay_t b);



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePlay_ToTimeVal
//!
//! Description: convert the time which type is MV_HANDLE_TimePlay_t to the time which type is MV_TimeVal_t
//!
//! \param a 	(IN):	The pointer to the a which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//! \param b 	(OUT):	The pointer to the b which type is MV_TimeVal_t
//!
//! \return Return:		the time which type is MV_TimeVal_t
//!
//!
////////////////////////////////////////////////////////////////////////////////
MV_TimeVal_t  MV_TimePlay_ToTimeVal(MV_HANDLE_TimePlay_t a);



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePlay_ToTimeSpec
//!
//! Description: convert the time which type is MV_HANDLE_TimePlay_t to the time which type is MV_TimeSpec_t
//!
//! \param a 	(IN):	The pointer to the a which type is one of (MV_TimePlay_TimeStr_t, MV_TimePlay_TimeNum_t, MV_TimePlay_TimeBCD_t)
//! \param b 	(OUT):	The pointer to the b which type is MV_TimeSpec_t
//!
//! \return Return:		the time which type is MV_TimeSpec_t
//!
//!
////////////////////////////////////////////////////////////////////////////////
MV_TimeSpec_t  MV_TimePlay_ToTimeSpec(MV_HANDLE_TimePlay_t a);




////////////////////////////////////////////////////////////////////////////////
//----------------------------------Timing--------------------------------------
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimeSpec_Delay
//!
//! Description: yield the procesor for time(MV_TimeSpec_t) period
//!
//! \param pTime  (IN):	The pointer to the time value to delay
//!
//! \return Return:		S_OK
//!						E_FAIL
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_TimeSpec_Delay(MV_TimeSpec_t *pTime);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_TimePTS_Delay
//!
//! Description: yield the procesor for time(MV_TimePTS_t) period
//!
//! \param pTime  (IN):	The pointer to the time value to delay
//!
//! \return Return:		S_OK
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_TimePTS_Delay(MV_TimePTS_t *pTime);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Time_GetOSTimeMS
//!
//! Description: Get the time(milisecond) from OS (The time is not same in different CPU)
//!
//!
//! \return Return:		the time value by ms
//!
//!
////////////////////////////////////////////////////////////////////////////////
UINT32 MV_Time_GetOSTimeMS( void );


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Time_Init
//!
//! Description: initialize for MV_Time_GetPFTIMER in Linux OS
//!
//!
//! \return Return:		S_OK
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_Time_Init(void);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Time_Exit
//!
//! Description: exit for MV_Time_GetPFTIMER in Linux OS
//!
//!
//! \return Return:		S_OK
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_Time_Exit(void);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Time_GetPFTIMER
//!
//! Description: Get the time from hardware
//!
//!
//! \return Return:		the time value by system tick
//!
//!
////////////////////////////////////////////////////////////////////////////////
UINT32 MV_Time_GetPFTIMER( void );


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Time_GetSysTime
//!
//! Description: Get the system time
//!
//! OS on Platform			The time source of the MV_Time_GetSysTime()
//!
//! eCos on CPU1 of ES2		Timer 6 and time Interrupt, a 32bit + 32bit time count (Hardware)
//! Linux on CPU0 of ES2	Linux API gettimeofday()
//! Linux on PC				Linux API gettimeofday()
//! Windows on PC			Windows API timeGetTime()
//!
//! \param pTimeSpec  (OUT):	The pointer to the time value
//!
//! \return Return:		S_OK
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_Time_GetSysTime(MV_TimeSpec_t *pTimeSpec);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Time_GetSysTimeMS
//!
//! Description: Get the system time(milisecond). It is same as MV_Time_GetSysTime()
//!
//!
//! \return Return:		the time value by ms
//!
//!
////////////////////////////////////////////////////////////////////////////////
UINT32 MV_Time_GetSysTimeMS( void );


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    Output_MV_TimePTS
//!
//! Description: print TimePTS value for trace (Debug)
//!
//! \param MV_TimePTS_t  (IN):	The pointer to the time value
//!
//! \return Return:		S_OK
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT Output_MV_TimePTS(MV_TimePTS_t *a);

#ifndef BERLIN_SINGLE_CPU
UINT32 MV_Time_GetTIMER6(void);
#else
#define MV_Time_GetTIMER6   MV_Time_GetTIMER7
#endif

UINT32 MV_Time_GetTIMER7(void);

INT64 MV_Get_System_Time_Tick( void );

void  MV_TimeSpec_Normaized(MV_TimeSpec_t *a, long sec, long nsec);

#ifdef __cplusplus
}
#endif


#endif
