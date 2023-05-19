/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#include "com_type.h"
#include "tee_ca_vpp.h"
#include "tee_client_api.h"
#include "vpp_cmd.h"
#include "vbuf.h"
#include "vdec_com.h"

#include "mmgr.h"
#include "global.h"

static bool initialized = false;
const TEEC_UUID TAFastLogo_UUID = TA_FASTLOGO_UUID;
TEEC_Context context;
TAVPPContext TAVPPInstance[MAX_TAVPP_INSTANCE_NUM];

int VppRegisterShm(unsigned int *PhyAddr, unsigned int Size)
{
#if !defined(CONFIG_FASTLOGO)
    TEEC_SharedMemory TzShm;

    memset(&TzShm, 0, sizeof(TzShm));
    TzShm.phyAddr = PhyAddr;
    TzShm.size = Size;
    TzShm.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

    TEEC_RegisterSharedMemory(&context, &TzShm);
#endif
    return TEEC_SUCCESS;
}


int VppUnregisterShm(unsigned int *PhyAddr, unsigned int Size)
{
#if !defined(CONFIG_FASTLOGO)
    TEEC_SharedMemory TzShm;

    memset(&TzShm, 0, sizeof(TzShm));
    TzShm.phyAddr = PhyAddr;
    TzShm.size = Size;
    TzShm.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

    return TEEC_ReleaseSharedMemory(&TzShm);
#else
    return TEEC_SUCCESS;
#endif
}

int VppGetInstanceID(void)
{
    int InstanceID = TAVPP_API_INSTANCE;
    return InstanceID;
}


void VppCloseSession(void)
{
    int index;
    for (index = 0; index < MAX_TAVPP_INSTANCE_NUM; index++) {
        if (TAVPPInstance[index].initialized) {
            TEEC_CloseSession(&(TAVPPInstance[index].session));
            memset(&TAVPPInstance[index], 0, sizeof(TAVPPInstance[index]));
            TAVPPInstance[index].initialized = false;
        }
    }
}

int VppInitialize(void)
{
    int index;
    TEEC_Result result = TEEC_SUCCESS;

    if (initialized)
        return TEEC_SUCCESS;

    initialized = true;
    for (index = 0; index < MAX_TAVPP_INSTANCE_NUM; index++)
    {
        result = TEEC_OpenSession(&(TAVPPInstance[index].session), &TAFastLogo_UUID,
                TEEC_LOGIN_USER, NULL, NULL, NULL);
        if (result != TEEC_SUCCESS) {
            printf("OpenSession error, ret=0x%08x\n", result);
            return -1;
        }
        TAVPPInstance[index].initialized = true;
    }
    return (result == TEEC_SUCCESS) ? 0 : -1;
}

TEEC_Result InvokeCommandHelper(int session_index,TEEC_Session *pSession,VPP_CMD_ID commandID,
                                                        TEEC_Operation *pOperation,UINT32 *returnOrigin)
{
#ifdef TZ_3_0
    int cmdID=commandID;
    int instID=session_index;
    commandID = CREATE_CMD_ID(cmdID,instID);
#endif
    return TEEC_InvokeCommand(pSession,commandID,pOperation);
}

//Please keep this enabled, Otherwise TEEC_SharedMemory is not passed on to TA
#define VPP_ENABLE_TZMEM_DYNAMIC
int VppPassShm(unsigned int *VirtAddr, VPP_SHM_ID AddrId, unsigned int Size)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;
#ifdef VPP_ENABLE_TZMEM_DYNAMIC
    TEEC_SharedMemory *pTzShmMem;
#else
    TEEC_SharedMemory TzShm;
#endif

    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);
#ifdef VPP_ENABLE_TZMEM_DYNAMIC
    pTzShmMem = GaloisMalloc(sizeof(TEEC_SharedMemory));
    memset(pTzShmMem, 0, sizeof(TEEC_SharedMemory));
    pTzShmMem->buffer = pTzShmMem->phyAddr = VirtAddr;
    pTzShmMem->allocated = true;
    pTzShmMem->flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
    pTzShmMem->size = Size;
    TEEC_RegisterSharedMemory(&context, pTzShmMem);
#else
    memset(&TzShm, 0, sizeof(TEEC_SharedMemory));
    TzShm.buffer = VirtAddr;
    TzShm.size = Size;
    TzShm.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

    TEEC_RegisterSharedMemory(&context, &TzShm);
#endif

    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_MEMREF_WHOLE,
            TEEC_VALUE_OUTPUT,
            TEEC_NONE);

    operation.params[0].value.a = AddrId;
#ifdef VPP_ENABLE_TZMEM_DYNAMIC
    operation.params[1].memref.parent = pTzShmMem;
    operation.params[1].memref.size = pTzShmMem->size;
#else
    operation.params[1].memref.parent = &TzShm;
    operation.params[1].memref.size = TzShm.size;
#endif
    operation.params[1].memref.offset = 0;

    /* clear result */
    operation.params[2].value.a = 0xdeadbeef;

    operation.started = 1;
    #ifdef CONFIG_VPP_FASTLOGO_SHOW_VBUF_INFO
    printf("%s:%d: VPP_PASSSHM : buffer :%x, size : %x\n", __func__, __LINE__, VirtAddr, Size);
    #endif
    result = InvokeCommandHelper(index,
            pSession,
            VPP_PASSSHM,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

#ifdef VPP_ENABLE_TZMEM_DYNAMIC
    free(pTzShmMem);
#endif

    return operation.params[2].value.a;
}

int VppPassVbufInfo(unsigned int *Vbuf, unsigned int VbufSize,
                         unsigned int *Clut, unsigned int ClutSize,
                         int PlaneID, int ClutValid, VPP_SHM_ID ShmID)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;
#ifdef VPP_ENABLE_TZMEM_DYNAMIC
    TEEC_SharedMemory *pTzShmMem;
#else
    TEEC_SharedMemory TzShmVbuf, TzShmClut;
#endif


    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);
    /* register Vbuf info*/
#ifdef VPP_ENABLE_TZMEM_DYNAMIC
    pTzShmMem = GaloisMalloc(sizeof(TEEC_SharedMemory));
    memset(pTzShmMem, 0, sizeof(TEEC_SharedMemory));
    pTzShmMem->buffer = pTzShmMem->phyAddr = Vbuf;
    pTzShmMem->allocated = true;
    pTzShmMem->flags = TEEC_MEM_INPUT;
    pTzShmMem->size = VbufSize;
    TEEC_RegisterSharedMemory(&context, pTzShmMem);
#else
    memset(&TzShmVbuf, 0, sizeof(TEEC_SharedMemory));
    TzShmVbuf.buffer = Vbuf;
    TzShmVbuf.size = VbufSize;
    TzShmVbuf.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
#endif

    operation.paramTypes = TEEC_PARAM_TYPES(
        TEEC_VALUE_INPUT,
        TEEC_MEMREF_WHOLE,
        TEEC_VALUE_INPUT,
        TEEC_VALUE_INOUT);

    operation.params[0].value.a = ShmID;
#ifdef VPP_ENABLE_TZMEM_DYNAMIC
    operation.params[1].memref.parent = pTzShmMem;
    operation.params[1].memref.size = pTzShmMem->size;
#else
    operation.params[1].memref.parent = &TzShmVbuf;
    operation.params[1].memref.size = TzShmVbuf.size;
#endif
    operation.params[1].memref.offset = 0;
    operation.params[3].value.a = PlaneID;

    /* clear result */
    operation.params[3].value.b = 0xdeadbeef;

    operation.started = 1;
    #ifdef CONFIG_VPP_FASTLOGO_SHOW_VBUF_INFO
    printf("%s:%d: VPP_PASSSHM : buffer :%x, size : %x\n", __func__, __LINE__, Vbuf, VbufSize);
    #endif
    result = InvokeCommandHelper(index,
            pSession,
            VPP_PASSSHM,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

#ifdef VPP_ENABLE_TZMEM_DYNAMIC
    free(pTzShmMem);
#endif
    return operation.params[3].value.b;
}

int VppPassVbufInfoPhy(unsigned int *Vbuf, unsigned int VbufSize,
                         unsigned int *Clut, unsigned int ClutSize,
                         int PlaneID, int ClutValid, VPP_SHM_ID ShmID)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;

    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);

    /* register Clut info*/
    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_VALUE_INPUT,
            TEEC_VALUE_INPUT,
            TEEC_VALUE_INOUT);

    operation.params[0].value.a = ShmID;
    operation.params[0].value.b = (unsigned int)(intptr_t)Vbuf;
    operation.params[3].value.a = PlaneID;

    /* clear result */
    operation.params[3].value.b = 0xdeadbeef;

    operation.started = 1;
    #ifdef CONFIG_VPP_FASTLOGO_SHOW_VBUF_INFO
    printf("%s:%d: VPP_PASSPHY : buffer :%x, size : %x\n", __func__, __LINE__, Vbuf, VbufSize);
    #endif
    result = InvokeCommandHelper(index,
            pSession,
            VPP_PASSPHY,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[3].value.b;
}

#ifdef TZ_3_0
UINT32 vpp_addr=0;
UINT32 ta_heapHandle=0;
#endif

int VppInit(VPP_INIT_PARM *vpp_init_parm)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;
#ifdef TZ_3_0
    int k;
    TEEC_Operation operation2;
#endif
    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);
#ifdef TZ_3_0
        operation.paramTypes = TEEC_PARAM_TYPES(
                TEEC_VALUE_INOUT,
                TEEC_VALUE_OUTPUT,
                TEEC_VALUE_INPUT,
                TEEC_NONE);
#else
    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_VALUE_OUTPUT,
            TEEC_NONE,
            TEEC_NONE);
#endif
    operation.params[0].value.a = vpp_init_parm->iHDMIEnable;
    operation.params[0].value.b = vpp_init_parm->iVdacEnable;

#if defined (TZ_3_0)&&defined(VPP_ENABLE_INTERNAL_MEM_MGR)
    operation.params[2].value.a = vpp_init_parm->uiShmPA;
    operation.params[2].value.b = vpp_init_parm->uiShmSize;
#endif
    /* clear result */
    operation.params[1].value.a = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_INIT,
            &operation,
            NULL);

    VPP_TEEC_LOGIFERROR(result);

#ifdef TZ_3_0
    vpp_init_parm->g_vpp=operation.params[1].value.b;
    vpp_addr = vpp_init_parm->g_vpp;
#if defined(VPP_ENABLE_INTERNAL_MEM_MGR)
    vpp_init_parm->gMemhandle=operation.params[0].value.b;
    ta_heapHandle = vpp_init_parm->gMemhandle;
#endif
    operation2.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_VALUE_OUTPUT,
            TEEC_NONE,
            TEEC_NONE);

    operation2.params[1].value.a = 0xdeadbeef;
    operation2.params[0].value.a = vpp_addr;
#if defined(VPP_ENABLE_INTERNAL_MEM_MGR)
    operation2.params[0].value.b= ta_heapHandle;
#endif
    operation2.started = 1;

    for(k=TAVPP_API_INSTANCE;k<MAX_TAVPP_INSTANCE_NUM;k++)
    {
        pSession = &(TAVPPInstance[k].session);
        InvokeCommandHelper(k,
            pSession,
            VPP_INITIATE_VPPS,
            &operation2,
            NULL);
    }
#endif

    return operation.params[1].value.a;

}

int VppCreate(void)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;

    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);
    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_OUTPUT,
            TEEC_NONE,
            TEEC_NONE,
            TEEC_NONE);

    /* clear result */
    operation.params[0].value.a = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_CREATE,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[0].value.a;
}


int VppReset(void)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;

    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);
    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_OUTPUT,
            TEEC_NONE,
            TEEC_NONE,
            TEEC_NONE);

    /* clear result */
    operation.params[0].value.a = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_RESET,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[0].value.a;
}


int VppConfig(void)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;

    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);
    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_OUTPUT,
            TEEC_NONE,
            TEEC_NONE,
            TEEC_NONE);

    /* clear result */
    operation.params[0].value.a = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_CONFIG,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[0].value.a;
}

int VppIsrHandler(unsigned int MsgId, unsigned int IntSts)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;
    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);

    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_VALUE_OUTPUT,
            TEEC_NONE,
            TEEC_NONE);

    operation.params[0].value.a = MsgId;
    operation.params[0].value.b = IntSts;

    /* clear result */
    operation.params[1].value.a = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_HANDLEINT,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[1].value.a;
}


int VppSetOutRes(int CpcbId, int ResId, int BitDepth)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;

    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);

    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_VALUE_INPUT,
            TEEC_VALUE_OUTPUT,
            TEEC_NONE);

    operation.params[0].value.a = CpcbId;
    operation.params[0].value.b = ResId;
    operation.params[1].value.a = BitDepth;

    /* clear result */
    operation.params[2].value.a = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_SETRES,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[2].value.a;
}

int VppOpenDispWin(int PlaneId, int WinX, int WinY, int WinW, int WinH, int BgClr, int Alpha, int GlobalAlpha)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;

    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);

    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_VALUE_INPUT,
            TEEC_VALUE_INPUT,
            TEEC_VALUE_INOUT);

    operation.params[0].value.a = 0;
    VPP_ATTR_PARAM_SET_GLOBALPHA(operation.params[0].value.a, GlobalAlpha);
    VPP_ATTR_PARAM_SET_PLANEID(operation.params[0].value.a, PlaneId);
    operation.params[0].value.b = WinX;
    operation.params[1].value.a = WinY;
    operation.params[1].value.b = WinW;
    operation.params[2].value.a = WinH;
    operation.params[2].value.b = BgClr;
    operation.params[3].value.a = Alpha;

    /* clear result */
    operation.params[3].value.b = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_OPENDISPWIN,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[3].value.b;
}

int VppRecycleFrame(int PlaneId)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;

    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);
    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_VALUE_OUTPUT,
            TEEC_NONE,
            TEEC_NONE);

    operation.params[0].value.a = PlaneId;


    /* clear result */
    operation.params[1].value.a = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_RECYCLEFRAME,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[1].value.a;
}

int VppSetDispMode(int PlaneId, int Mode)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;

    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);
    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_VALUE_OUTPUT,
            TEEC_NONE,
            TEEC_NONE);

    operation.params[0].value.a = PlaneId;
    operation.params[0].value.b = Mode;

    /* clear result */
    operation.params[1].value.a = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_SETDISPMODE,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[1].value.a;
}

int VppHdmiSetVidFmt(int ColorFmt, int BitDepth, int PixelRep)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;
    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);

    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_VALUE_INPUT,
            TEEC_VALUE_OUTPUT,
            TEEC_NONE);

    operation.params[0].value.a = ColorFmt;
    operation.params[0].value.b = BitDepth;
    operation.params[1].value.a = PixelRep;

    /* clear result */
    operation.params[2].value.a = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_HDMISETVIDFMT,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[2].value.a;
}

int VppSetHdmiTxControl(int Enable)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;
    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);

    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_VALUE_OUTPUT,
            TEEC_NONE,
            TEEC_NONE);

    operation.params[0].value.a = Enable;

    /* clear result */
    operation.params[1].value.a = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_HDMISETTXCTRL,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[1].value.a;
}

int VppSetRefWin(int PlaneId, int WinX, int WinY, int WinW, int WinH)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;

    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);
    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_VALUE_INPUT,
            TEEC_VALUE_INPUT,
            TEEC_VALUE_OUTPUT);

    operation.params[0].value.a = PlaneId;
    operation.params[0].value.b = WinX;
    operation.params[1].value.a = WinY;
    operation.params[1].value.b = WinW;
    operation.params[2].value.a = WinH;

    /* clear result */
    operation.params[3].value.a = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_SETREFWIN,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[3].value.a;
}

int VppChangeDispWin(int PlaneId, int WinX, int WinY, int WinW, int WinH, int BgClr, int Alpha, int GlobalAlpha)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;

    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);

    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_VALUE_INPUT,
            TEEC_VALUE_INPUT,
            TEEC_VALUE_INOUT);

    operation.params[0].value.a = 0;
    VPP_ATTR_PARAM_SET_GLOBALPHA(operation.params[0].value.a, GlobalAlpha);
    VPP_ATTR_PARAM_SET_PLANEID(operation.params[0].value.a, PlaneId);
    operation.params[0].value.b = WinX;
    operation.params[1].value.a = WinY;
    operation.params[1].value.b = WinW;
    operation.params[2].value.a = WinH;
    operation.params[2].value.b = BgClr;
    operation.params[3].value.a = Alpha;

    /* clear result */
    operation.params[3].value.b = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_CHANGEDISPWIN,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[3].value.b;
}

int VppSetPlaneMute(int planeID, int mute)
{
    TEEC_Result result;
    TEEC_Operation operation;
    int index;
    TEEC_Session *pSession;
    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);

    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_VALUE_OUTPUT,
            TEEC_NONE,
            TEEC_NONE);

    operation.params[0].value.a = planeID;
    operation.params[0].value.b = mute;

    /* clear result */
    operation.params[1].value.a = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_SETPLANEMUTE,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[1].value.a;
}

int Vppstop(void)
{
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;

    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);
    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_OUTPUT,
            TEEC_NONE,
            TEEC_NONE,
            TEEC_NONE);

    /* clear result */
    operation.params[0].value.a = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_STOP,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[0].value.a;
}

int VppDestroy(void) {
    int index;
    TEEC_Session *pSession;
    TEEC_Result result;
    TEEC_Operation operation;

    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);
    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_OUTPUT,
            TEEC_NONE,
            TEEC_NONE,
            TEEC_NONE);

    /* clear result */
    operation.params[0].value.a = 0xdeadbeef;

    operation.started = 1;
    result = InvokeCommandHelper(index,
            pSession,
            VPP_DESTROY,
            &operation,
            NULL);
    VPP_TEEC_LOGIFERROR(result);

    return operation.params[0].value.a;
}

int VppPassVbufInfoPar(unsigned int *Vbuf, unsigned int VbufSize,
                         unsigned int *Clut, unsigned int ClutSize,
                         int PlaneID, int ClutValid, VPP_SHM_ID ShmID)
{
    int index, i, flag;
    TEEC_Session *pSession;
    TEEC_Operation operation;
    VBUF_INFO *pVBufInfo = (VBUF_INFO*) Vbuf;
    TEEC_Result result;

    index = VppGetInstanceID();
    pSession = &(TAVPPInstance[index].session);

    operation.paramTypes = TEEC_PARAM_TYPES(
        TEEC_VALUE_INOUT,
        TEEC_VALUE_INOUT,
        TEEC_VALUE_INOUT,
        TEEC_VALUE_INOUT);
    for(i=0; i < 5; i++) {
        flag = 0;
        switch(i) {
            case 0:
                operation.params[1].value.a = (unsigned int)((intptr_t)pVBufInfo->m_pbuf_start);
                operation.params[1].value.b = pVBufInfo->m_bytes_per_pixel;
                operation.params[2].value.a = pVBufInfo->m_bits_per_pixel;
                operation.params[2].value.b = pVBufInfo->m_srcfmt;
                operation.params[3].value.a = pVBufInfo->m_order;
                flag = 1;
            break;
            case 1:
                operation.params[1].value.a = pVBufInfo->m_content_width;
                operation.params[1].value.b = pVBufInfo->m_content_height;
                operation.params[2].value.a = pVBufInfo->m_buf_stride;
                operation.params[2].value.b = pVBufInfo->m_buf_pbuf_start_UV;
                operation.params[3].value.a = pVBufInfo->m_buf_size;
                flag = 1;
            break;
            case 2:
                operation.params[1].value.a = pVBufInfo->m_allocate_type;
                operation.params[1].value.b = pVBufInfo->m_buf_type;
                operation.params[2].value.a = pVBufInfo->m_buf_use_state;
                operation.params[2].value.b = pVBufInfo->m_flags;
                operation.params[3].value.a = pVBufInfo->m_is_frame_seq;
                flag = 1;
            break;
            case 3:
                operation.params[1].value.a = pVBufInfo->m_frame_rate_num;
                operation.params[1].value.b = pVBufInfo->m_frame_rate_den;
                operation.params[2].value.a = pVBufInfo->m_active_width;
                operation.params[2].value.b = pVBufInfo->m_active_height;
                operation.params[3].value.a = pVBufInfo->m_active_left;
                flag = 1;
            break;
            case 4:
                operation.params[1].value.a = pVBufInfo->m_active_top;
                operation.params[1].value.b = pVBufInfo->m_content_offset;
                operation.params[2].value.a = pVBufInfo->m_is_progressive_pic;
                operation.params[2].value.b = pVBufInfo->m_hDesc;
                flag = 1;
            break;
        }
        if(flag) {
            operation.params[0].value.a = (ShmID | (PlaneID<<8));
            operation.params[0].value.b = i;
            /* clear result */
            operation.params[3].value.b = 0xdeadbeef;

            operation.started = 1;
            #ifdef CONFIG_VPP_FASTLOGO_SHOW_VBUF_INFO
            printf("%s:%d: VPP_PASSPAR : buffer :%x, size : %x\n", __func__, __LINE__, Vbuf, VbufSize);
            #endif
            result = InvokeCommandHelper(index,
                    pSession,
                    VPP_PASSPAR,
                    &operation,
                    NULL);

            //if(operation.params[3].value.b == 0xdeadbeef) { }
        }
    }

    return result;
}
