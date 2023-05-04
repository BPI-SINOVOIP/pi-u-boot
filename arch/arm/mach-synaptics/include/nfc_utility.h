/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */

#ifndef _NFC_UTILITY_
#define _NFC_UTILITY_

/**** extern ****/
#define memzero(addr, size)                 memset(addr,0,size)
#define cache_clean(start_addr, size)       {CleanDCacheRegion((unsigned int*)start_addr, size); arm_ISB();}
#define cache_invalidate(start_addr, size)  {InvalidateDCacheRegion((unsigned int*)start_addr, size); arm_ISB();}
//#define mem_cpy(d,s,l)                      memcpy(d,s,l)     //has to be 32bit access
//#define mem_cmp(s,d,l)                      memcmp(s,d,l)
//#define PRINT(...)                          dbg_printf(PRN_RES, __VA_ARGS__)
//#define PRINT_INFO(...)                     dbg_printf(PRN_INFO, __VA_ARGS__)

/**** includes ****/
#include "Galois_memmap.h"          // for base addr
#include "global.h"                 // for nfc reset

/**** hw def ****/
#define NFC_REG_BASE                MEMMAP_NAND_AHB_REG_BASE
#define NFC_SDMA_PORT_ADDR          MEMMAP_NAND_FLASH_REG_BASE
#define NFC_SDMA_PORT_SIZE          MEMMAP_NAND_FLASH_REG_SIZE
#define IRQ_NFC                     IRQ_nanfIntr

#define MAX_SNG_TRF_CNT             0x100

#define MAX_TRDNUM                  7

/**** user defines ****/
#define DEFAULT_TIMEOUT_COUNT       0
#define DEFAULT_DELAY_INTERVAL      100
#define DEFAULT_RBN_STABLE_CNT      20

#define NFC_DESC_BUFF_ADDR          0xB00000
#define NFC_SDMA_BUFF_ADDR          0xB80000

#define WORK_MODE_DMA               0
#define WORK_MODE_PIO               1
#define WORK_MODE_GEN               2

#define RESET_TYPE_ASYNC            0
#define RESET_TYPE_LUN              1

#define NUM_SUPPORTED_DEVICES       8

#define INT_TYPE_SDMA_TRIGG         0
#define INT_TYPE_TRD_COMP           1
#define INT_TYPE_OTHER              2
#define INT_TYPE_MAX                3

#define ADDR_PATT_XOR_CONST         0xffca5300

/**** register r/w macro ****/
//#define NFC_REG_REMAP(offset)           (((offset) & 0x1000) ? ((offset) ^ 0x1900) : ((offset) & 0x2000) ? ((offset) ^ 0x2a00) : (offset))
#define NFC_REG(offset)                 (*(volatile unsigned int *)(unsigned long)((NFC_REG_BASE) + (offset)))
#define NFC_RD_REG(ra, v)               (v) = NFC_REG(ra)
#define NFC_WR_REG(ra, v)               NFC_REG(ra) = (v)

#define NFC_RD_REG_NAME(regname, v)     *(v) = NFC_REG(RA__##regname)
#define NFC_WR_REG_NAME(regname, v)     NFC_REG(RA__##regname) = *(v)

#define NFC_RD_FLD_NAME(fieldname, v)   {\
    register unsigned int t9d8fu289139d;\
    t9d8fu289139d = NFC_REG(BA__##fieldname);\
    v = (t9d8fu289139d >> LSb__##fieldname) & MSK__##fieldname;}

#define NFC_WR_FLD_NAME(fieldname, v)   {\
    register unsigned int t9d8fu289139d;\
    t9d8fu289139d = NFC_REG(BA__##fieldname);\
    t9d8fu289139d &= ~(MSK__##fieldname << LSb__##fieldname);\
    t9d8fu289139d |= ((v) << LSb__##fieldname);\
    NFC_REG(BA__##fieldname) = (t9d8fu289139d);}

#define THREAD_IS_BUSY(trd) ((NFC_REG(RA__TRD_STATUS) >> (trd)) & 0x1)

#define LA2RA(logical_addr) ((logical_addr) / (i_curr_dev.page_size))

/**** interrupt masks ****/
// same bit position for enable and status registers, except for gloabl enable bit
#define INT_GLOBAL          (1 << LSb__INTR_ENABLE__intr_en)
#define INT_SDMA_ERR        (1 << LSb__INTR_ENABLE__sdma_err_en)
#define INT_SDMA_TRIGG      (1 << LSb__INTR_ENABLE__sdma_trigg_en)
#define INT_CMD_IGNORED     (1 << LSb__INTR_ENABLE__cmd_ignored_en)
#define INT_DDMA_TERR       (1 << LSb__INTR_ENABLE__ddma_terr_en)
#define INT_CDMA_TERR       (1 << LSb__INTR_ENABLE__cdma_terr_en)
#define INT_CDMA_IDLE       (1 << LSb__INTR_ENABLE__cdma_idle_en)

#define INT_ALL             (INT_GLOBAL\
                            |INT_SDMA_ERR\
                            |INT_SDMA_TRIGG\
                            |INT_CMD_IGNORED\
                            |INT_DDMA_TERR\
                            |INT_CDMA_TERR\
                            |INT_CDMA_IDLE)

#define THREAD_ALL          (0xff)

/**** error status masks ****/
// for command operation status check
#define ERR_DESC            (1 << 0)
#define ERR_CMD             (1 << 0)
#define ERR_ECC             (1 << 1)
#define ERR_MAX             (0xff << 2)
#define ERR_DEV             (1 << 12)
#define ERR_FAIL            (1 << 14)
#define ERR_BUS             (1 << 16)

#define ERR_ALL             (ERR_DESC\
                            |ERR_ECC\
                            |ERR_MAX\
                            |ERR_DEV\
                            |ERR_FAIL\
                            |ERR_BUS)

#define ERR_ALL_GEN         (ERR_DESC\
                            |ERR_ECC\
                            |ERR_MAX\
                            |ERR_FAIL\
                            |ERR_BUS)

/**** timings  ****/
#define TMSK_BUILD(regfieldname)        (BA__##regfieldname | (MSK__##regfieldname << 19) | (LSb__##regfieldname << 14))
#define TMSK_RH                         TMSK_BUILD(ASYNC_TOGGLE_TIMINGS__tRH)
#define TMSK_RP                         TMSK_BUILD(ASYNC_TOGGLE_TIMINGS__tRP)
#define TMSK_WH                         TMSK_BUILD(ASYNC_TOGGLE_TIMINGS__tWH)
#define TMSK_WP                         TMSK_BUILD(ASYNC_TOGGLE_TIMINGS__tWP)
#define TMSK_ADL                        TMSK_BUILD(TIMINGS0__tADL)
#define TMSK_CCS                        TMSK_BUILD(TIMINGS0__tCCS)
#define TMSK_WHR                        TMSK_BUILD(TIMINGS0__tWHR)
#define TMSK_RHW                        TMSK_BUILD(TIMINGS0__tRHW)
#define TMSK_RHZ                        TMSK_BUILD(TIMINGS1__tRHZ)
#define TMSK_WB                         TMSK_BUILD(TIMINGS1__tWB)
#define TMSK_CWAW                       TMSK_BUILD(TIMINGS1__tCWAW)
#define TMSK_VDLY                       TMSK_BUILD(TIMINGS1__tVDLY)
#define TMSK_FEAT                       TMSK_BUILD(TIMINGS2__tFEAT)
#define TMSK_CEH                        TMSK_BUILD(TIMINGS2__CS_hold_time)
#define TMSK_CS                         TMSK_BUILD(TIMINGS2__CS_setup_time)
#define TMSK_RDQS                       TMSK_BUILD(PHY_CTRL_REG__phony_dqs_timing)

#define TMSK_VAL(name, val)             {TMSK_##name, val}
#define TNAME_MSK(name)                 {#name, TMSK_##name}

#define TVAL2REG(regfieldname, val)     (((val) & MSK__##regfieldname) << LSb__##regfieldname)

/**** command type ****/
//tbl 2.4, 2.23, 2.34
#define CT_SET_FEATURE                  (0x0100)
#define CT_THREAD_RESET(type)           (0x0200 | ((type) & 0xff))
#define CT_BLOCK_ERASE(blk_cnt)         (0x1000 | (((blk_cnt) - 1) & 0xff))
#define CT_ASYNC_RESET                  (0x1100)
#define CT_LUN_RESET                    (0x1102)
#define CT_COPYBACK(page_cnt)           (0x1200 | (((page_cnt) - 1) & 0xff))
#define CT_PROGRAM(page_cnt)            (0x2100 | (((page_cnt) - 1) & 0xff))
#define CT_READ(page_cnt)               (0x2200 | (((page_cnt) - 1) & 0xff))
#define CT_NOP                          (0xFFFF)

#define IS_COPYBACK_CMD(command_type)   (((command_type) >> 8) == 0x12 ? 1 : 0)
#define CT_INS_CNT(command_type, cnt)   (((command_type) & 0xff00) | (((cnt) - 1) & 0xff))

/**** instruction type (tbl 2.31) ****/
#define IT_CMD_SEQ                              0
#define IT_ADDR_SEQ                             1
#define IT_DATA_SEQ                             2
#define IT_READ                                 3
#define IT_WRITE                                4
#define IT_RESET                                5
#define IT_ERASE                                6
#define IT_READ_STATUS                          7
#define IT_READ_STATUS_ENHANCED                 8
#define IT_READ_CACHE_RANDOM                    9
#define IT_COPYBACK_READ                        10
#define IT_COPYBACK_PROGRAM                     11
#define IT_CHANGE_READ_COLUMN                   12
#define IT_CHANGE_READ_COLUMN_ENHANCED          13
#define IT_CHANGE_READ_COLUMN_JEDEC             14
#define IT_MULTI_PLANE_READ                     15
#define IT_MULTI_PLANE_BLOCK_ERASE              16
#define IT_MULTI_PLANE_BLOCK_ERASE_ONFI_JEDEC   17
#define IT_CHANGE_WRITE_COLUMN                  18
#define IT_SMALL_DATA_MOVE                      19
#define IT_SYNCHRONOUS_RESET                    20
#define IT_VOLUME_SELECT                        21
#define IT_ODT_CONFIGURE                        22
#define IT_SET_FEATURES                         23
#define IT_GET_FEATURES                         24
#define IT_LUN_GET_FEATURES                     25
#define IT_LUN_SET_FEATURES                     26
#define IT_READ_ID                              27
#define IT_READ_PARAMETER_PAGE                  28
#define IT_ZQ_CALIBRATION_SHORT                 29
#define IT_ZQ_CALIBRATION_LONG                  30
#define IT_LUN_RESET                            31

#define LSb_INST_TYPE                           0
#define LSb_TWB_ACTIVE                          6
#define LSb_JEDEC_SUPP                          7

#define LSb_CMD                                 16

#define LSb_NBYTES                              11

#define LSb_DIRECTION                           11
#define LSb_ECC_EN                              12
#define LSb_SCRAMBLER_EN                        13
#define LSb_ERASED_PAGE_DET_EN                  14
#define LSb_SECTOR_SIZE                         16
#define LSb_SECTOR_CNT                          32
#define LSb_LAST_SECTOR_SIZE                    40
#define LSb_CORR_CAP                            56

#define LSb_F2_EN                               11
#define LSb_ADDR0                               16
#define LSb_ADDR1                               24
#define LSb_ADDR2                               32
#define LSb_ADDR3                               40
#define LSb_ADDR4                               48
#define LSb_ADDR5                               56

#define CMD_PARAM(name, value)                  (S_ULL(value) << LSb_##name)

#define DIRECTION_READ                          0   //
#define DIRECTION_WRITE                         1   //do not swap

#define DMA_SEL_SLAVE                           0   //
#define DMA_SEL_MASTER                          1   //do not swap

/**** other utilities ****/
#define DUMP_FIELD_DEC(var, fieldname) PRINT("    %16s = %10d\n", #fieldname, var.fieldname)
#define DUMP_FIELD_HEX(var, fieldname) PRINT("    %16s = 0x%08x\n", #fieldname, var.fieldname)

//#define NFC_DELAY_US(n)     delay_us(n)
#define NFC_DELAY_US(n)    {register unsigned int i; for(i=n;i!=0;i--);} 

#define P2U(v)          ((unsigned int)(unsigned long)(v))
#define U2PV(v)         ((void *)(unsigned long)(v))
#define U2PU(v)         ((unsigned int *)(unsigned long)(v))
#define U2PC(v)         ((char *)(unsigned long)(v))
#define S_ULL(v)          ((unsigned long long)(v))

#endif
