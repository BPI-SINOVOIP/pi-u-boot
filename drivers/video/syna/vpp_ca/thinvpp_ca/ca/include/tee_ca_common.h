/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#ifndef _TEE_COMMON_CA_H_
#define _TEE_COMMON_CA_H_

#include "tee_client_type.h"
#include "tee_client_api.h"
#include "OSAL_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define TAVPP_PASS_SHMSIZE (1024*4)

#define VPP_PASS_SHM_LOCK(hLock)      do {                        \
        HRESULT ret;                                    \
        ret = MV_OSAL_Mutex_Lock(hLock);   \
        ASSERT(ret == SUCCESS);                         \
        (void)ret; \
    } while (0)

#define VPP_PASS_SHM_UNLOCK(hLock)    do {                      \
        HRESULT ret;                                    \
        ret = MV_OSAL_Mutex_Unlock(hLock); \
        ASSERT(ret == SUCCESS);                         \
        (void)ret; \
    } while (0)

#define VPP_TEEC_LOGIFERROR(result) \
        do { \
            if (result != TEEC_SUCCESS) \
                printf("TEEC Error ret=0x%08x\n", result); \
        } while (0)

typedef struct TAVPPPassShm_T{
    AMP_SHM_HANDLE              shm_handle;     /* shm handle */
    unsigned int                *phy_addr;      /* shm physical addr */
    unsigned int                *vir_addr;      /* shm virtul addr */
    MV_OSAL_HANDLE_MUTEX_t      lock_handle;    /* Lock for shm */
} TAVPPPassShm;

typedef struct TAVPPContext_T {
    bool                        initialized;
    TAVPPPassShm                shmContext;
    TEEC_Session                session;
} TAVPPContext;

enum{
    TAVPP_API_INSTANCE = 0,
    MAX_TAVPP_INSTANCE_NUM,
};

//TEEC_Session session;

#ifdef __cplusplus
}
#endif

#endif /* _TEE_VPP_CA_H_ */
