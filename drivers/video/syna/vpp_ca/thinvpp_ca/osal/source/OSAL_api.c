/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#include "OSAL_api.h"

extern void * malloc_ion_noncacheable(int size);
extern void * malloc_ion_cacheable(int size);

//malloc- allocate memory from NonSecure,Cache
void GaloisInit(void)
{
    // do nothing
}

void * GaloisMalloc(unsigned int size)
{
    if (size > 0)
    {
        void *ptr = NULL;
        ptr = (void *)((((unsigned long)malloc_ion_noncacheable(size + 32) + 31) >> 5) << 5);
        return(ptr);
    }
    return(0);
}

void * GMalloc(unsigned int size)
{
    if (size > 0)
    {
        void *ptr = NULL;
        ptr = (void *)((((unsigned long)malloc_ion_noncacheable(size) + 32) >> 5) << 5);
        return(ptr);
    }
    return(0);
}

UINT  VPP_ALLOC(unsigned int  eMemType, unsigned int  uiSize, unsigned int  uiAlign, VPP_MEM_HANDLE  *phm)
{
#ifdef VPP_ENABLE_SECURE_MEM_ALLOCATION
    *phm = (uintptr_t) get_secure_membuff();
#else
    *phm =(uintptr_t) GMalloc(uiSize);
#endif
    return (0);
}

//#define VPP_ENABLE_USE_NonCache_For_Cached_Memory
void * VPP_TZ_ALLOC(unsigned int  uiSize)
{
    void *pPtr = NULL;

#ifdef VPP_ENABLE_TZ_MALLOC_MEM_ALLOCATION
    pPtr = malloc_ion_cacheable(uiSize);
#else
    pPtr = GMalloc(uiSize);
#endif
    return pPtr;
}

