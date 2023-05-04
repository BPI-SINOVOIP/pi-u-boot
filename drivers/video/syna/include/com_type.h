/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#ifndef __COMMON_TYPE_H__
#define __COMMON_TYPE_H__

typedef  unsigned char       uint8_t;
typedef  unsigned short      uint16_t;
typedef  unsigned int        uint32_t;
typedef  unsigned long long  uint64_t;
typedef  unsigned long       uintptr_t;
typedef  signed char         int8_t;
typedef  signed short        int16_t;
typedef  signed int          int32_t;
typedef  signed long         long int64_t;
typedef  signed long         intptr_t;
typedef  unsigned long       size_t;
typedef  unsigned char       UNSG8;
typedef  signed char         SIGN8;
typedef  unsigned short      UNSG16;
typedef  signed short        SIGN16;
typedef  unsigned int        UNSG32;
typedef  signed int          SIGN32;
typedef  unsigned long long  UNSG64;
typedef  signed long long    SIGN64;
typedef  float               REAL32;
typedef  double              REAL64;
#ifndef INLINE
    #define INLINE          inline
#endif
typedef unsigned char          UCHAR;
typedef char                   CHAR;
typedef UCHAR                  BOOL;
typedef UCHAR                  bool;
typedef UCHAR                  BOOLEAN;
typedef CHAR                   INT8;
typedef UCHAR                  UINT8;
typedef UCHAR                  BYTE;
typedef short                  INT16;
typedef unsigned short         UINT16;
typedef int                    INT32;
typedef unsigned int           UINT32;
typedef long long              INT64;
typedef unsigned long long     UINT64;
typedef unsigned int           UINT;
typedef int                    INT;
typedef signed int             HRESULT;
typedef long            LONG;
typedef unsigned long   ULONG;

/*---------------------------------------------------------------------------
 *    Multiple-word types
 *--------------------------------------------------------------------------*/
#ifndef Txxb
#define Txxb
typedef UINT8 T8b;
typedef UINT16 T16b;
typedef UINT32 T32b;
typedef UINT32 T64b[2];
typedef UINT32 T96b[3];
typedef UINT32 T128b[4];
typedef UINT32 T160b[5];
typedef UINT32 T192b[6];
typedef UINT32 T224b[7];
typedef UINT32 T256b[8];
#endif

typedef void VOID;
typedef void *PTR;
typedef void **PHANDLE;
typedef void *HANDLE;
typedef void *PVOID;
typedef long long loff_t;
typedef signed int hresult;
typedef void *SHM_HANDLE;

#define GaloisMemSet memset
#define GaloisFree(x) (void)0
#define GaloisMemcpy memcpy
#define GaloisMemClear(buf, n)  memset((buf), 0, (n))

#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

#ifndef True
#define True (1)
#endif
#ifndef False
#define False (0)
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#define E_GENERIC_BASE  (0x0000 << 16)

#define E_GEN_SUC(code) (E_SUC | E_GENERIC_BASE | (code&0x0000FFFF))
#define E_GEN_ERR(code) (E_ERR | E_GENERIC_BASE | (code&0x0000FFFF))

#define S_OK  E_GEN_SUC(0x0000)
#define S_FALSE  E_GEN_SUC(0x0001)

#define MAGIC_NUMBER (0xD2ADA3F1)
#define COMMAND_SIZE 5
#define _RESERVED_   0
#define IMAGE3_DDR_ADDR IMAGE3_DDR_ADDRESS
#define reg(addr) (*((int *)(addr)))

/* kernel interface return status definiton */
enum status {
	SUCCESS     = 0,
	FAIL        = -1,
};

#define AMP_SHM_STATIC 1
#define AMP_SHM_VIDEO_FB   2

#ifndef VIDEO_COLOUR_FORMAT_ABGR
#define VIDEO_COLOUR_FORMAT_ABGR 0
#endif //VIDEO_COLOUR_FORMAT_ABGR

#endif
