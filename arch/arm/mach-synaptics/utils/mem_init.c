/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright © 2013-2018 Synaptics Incorporated. All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */
#include <common.h>
#include <linux/types.h>
#include "mem_region.h"
#include "tz_nw_boot.h"
#include "mem_region_userdata.h"

#define MAX_REGION_COUNT 16
#ifdef CONFIG_BERLIN_LOWMEM_1P5G_SUPPORT
/* This address must be accessed by TZK*/
static struct mem_region *mem = (struct mem_region*)0x05000000;
#else
static struct mem_region mem[MAX_REGION_COUNT] __attribute__((aligned(4096)));
#endif
static int region_count = 0;
#include "mmgr.h"

//#define ION_DEBUG

#if defined(ION_DEBUG)
#define dbg_printf(LEVEL, FORMAT, ...)	\
do { \
	if (LEVEL <= PRN_INFO) { \
		printf(FORMAT, __VA_ARGS__); \
	} \
} while(0)
#else
#define dbg_printf(LEVEL, FORMAT, ...)	\
do { \
} while(0)
#endif

#define INFO(FORMAT, ...) dbg_printf(PRN_INFO, FORMAT, ...)
#define NOTICE(FORMAT, ...) dbg_printf(PRN_RES, FORMAT, ...)
#define ERR printf


#define CACHEABLE_MEM_POOL_SIZE (128*1024*1024)
void init_ion_cacheable_mempool(void)
{
	int i = 0;
	unsigned long mstart = 0;
	unsigned int size = 0;
	for(i = 0; i < region_count; i++) {
		if(TEE_MR_ION_FOR_NONSECURE(&mem[i]) && TEE_MR_ION_FOR_CACHEABLE(&mem[i])) {
			mstart = (unsigned long)(mem[i].base);
			size = mem[i].size;
			if(size > CACHEABLE_MEM_POOL_SIZE) {
				dbg_printf(PRN_RES, "nonsecure cacheable memory region base = 0x%08lx\n", mstart);
				init_mmgr_by_type(MEM_ION_CACHEABLE, mstart, CACHEABLE_MEM_POOL_SIZE);
				return;
			}
		}
	}
	ERR("cann't find non-secure ION cacheable memory!\n");
}
void init_ion_noncacheable_mempool(void)
{
	int i = 0;
	unsigned long mstart = 0;
	unsigned int size = 0;
	for(i = 0; i < region_count; i++) {
		if(TEE_MR_ION_FOR_NONSECURE(&mem[i]) && TEE_MR_ION_FOR_NONCACHEABLE(&mem[i])) {
			mstart = (unsigned long)(mem[i].base);
			size = mem[i].size;
			//nonsecure noncacheable memory is small
			//if(size > NONCACHEABLE_MEM_POOL_SIZE) {
				dbg_printf(PRN_RES, "nonsecure noncacheable memory region base = 0x%08lx\n", mstart);
				init_mmgr_by_type(MEM_ION_NONCACHEABLE, mstart, size);
				return;
			//}
		}
	}
	ERR("cann't find non-secure ION noncacheable memory!\n");
}

void get_mem_region_list_from_tz(void)
{
	int i;
	region_count = tz_nw_get_mem_region_list(mem, MAX_REGION_COUNT, 0, 0);
	for(i=0;i<region_count;i++)
		dbg_printf(PRN_RES, "pool[%d]: name: %s, base =0x%08x, size = 0x%08x, attr = 0x%08x, userdata0 = 0x%08x \n",
		i, mem[i].name, mem[i].base, mem[i].size, mem[i].attr, mem[i].userdata[0]);

	init_ion_cacheable_mempool();
	init_ion_noncacheable_mempool();
}

void get_mem_region_by_name(u64 *start, u64 *size, char *zone_name)
{
	u64 mem_base = 0, mem_size = 0;
	int region_count = 0;
	// For VS640, bootloader zone is from 0x04100000 to 0x04800000
	// For VS680, bootloader zone is from 0x04100000 to 0x04A00000
	// DRAM doesn't init when call this API, so hardcode to 0x04600000, TZK can access it
	struct mem_region *mem_r = (struct mem_region*)0x04600000;

	region_count = tz_nw_get_mem_region_list(mem_r, MAX_REGION_COUNT, 0, 0);
	for(int i=0; i<region_count; i++) {
		if(!memcmp(mem_r[i].name, zone_name, strlen(zone_name))) {
			mem_base = mem_r[i].base;
			mem_size = mem_r[i].size;
			break;
		}
	}
	if(mem_size == 0)
		printf("Error: get_mem_region_by_name (%s) fail \n", zone_name);

	*start = mem_base;
	*size  = mem_size;
}

void * malloc_ion_cacheable(int size)
{
	return mmgr_alloc_by_type(MEM_ION_CACHEABLE, size);
}

void free_ion_cacheable(void * m)
{
	mmgr_free_by_type(MEM_ION_CACHEABLE, m);
}

void * malloc_ion_noncacheable(int size)
{
	return mmgr_alloc_by_type(MEM_ION_NONCACHEABLE, size);
}

void free_ion_noncacheable(void * m)
{
	mmgr_free_by_type(MEM_ION_NONCACHEABLE, m);
}
