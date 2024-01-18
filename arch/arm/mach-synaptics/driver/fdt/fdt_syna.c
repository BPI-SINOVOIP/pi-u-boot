/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 * Author: Terry Zhou <Terry.Zhou@synaptics.com>
 *
 */
#include <common.h>
#include <command.h>
#include <env.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <asm/global_data.h>
#include <linux/libfdt.h>

#include "mem_region.h"
#include "mem_region_userdata.h"
#include "mem_init.h"

#define DTB_SPACE (0x20000 - 0x100)
#define BERLIN_HEAP_COMPATIBLE_NAME "syna,berlin-heaps"
#define POOL_NUM "pool-num"
#define ATTR_NUM_PER_POOL "attributes-num-per-pool"
#define POOL_ATTRIBUTES "pool-attributes"

struct reserve_entry {
	unsigned int reserved_mem_start;
	unsigned int reserved_mem_size;
};

static void setup_cma_param(char *bootargs)
{
	char tmp_buf[128];

	/* Notice: strict check for the cma_pool_addr should be non-zero */
	if(get_ion_cma_pool_size() && get_ion_cma_pool_addr()) {
		memset(tmp_buf, 0x0, sizeof(tmp_buf));
		snprintf(tmp_buf, (sizeof(tmp_buf) - 1), "cma=%d@%d", get_ion_cma_pool_size(), get_ion_cma_pool_addr());
		strcat(bootargs, tmp_buf);
	}
}

int setup_reserved_memory(void *fdt, void * mem, unsigned int reserved_num)
{
	struct fdt_header * header = (struct fdt_header *)fdt;
	unsigned int offset = fdt32_to_cpu(header->off_mem_rsvmap);
	struct reserve_entry * usr_reserve = (struct reserve_entry *)mem;
	int n;

	struct fdt_reserve_entry * mem_reserve = (struct fdt_reserve_entry *)(((unsigned char *)fdt) + offset);

	for (n = 0; ; n++) {
		mem_reserve += n;

		if(n < reserved_num) {
			if(NULL == usr_reserve)
				return -1;

			usr_reserve += n;
			mem_reserve->address = cpu_to_fdt64(usr_reserve->reserved_mem_start);
			mem_reserve->size = cpu_to_fdt64(usr_reserve->reserved_mem_size);
		}
		else {
			if(!fdt64_to_cpu(mem_reserve->size))
				break;
			mem_reserve->address = cpu_to_fdt64(0x0);
			mem_reserve->size = cpu_to_fdt64(0x0);
		}
	}

	return 0;
}

int setup_ion_memory(void *fdt, struct mem_region *ion_mem, int pool_num)
{
	unsigned int mem_region_property[4 * MAX_REGION_COUNT];
	int offset;
	int i, ret, len, memcount=0;
	char reg_names[MAX_REGION_COUNT * 16];

	if (pool_num <= 0 || ion_mem == NULL)
		return -1;

	/* let's give it all the room it could need */
	ret = fdt_open_into(fdt, fdt, DTB_SPACE);
	if (ret < 0)
		return ret;

	offset = fdt_node_offset_by_compatible(fdt, -1, BERLIN_HEAP_COMPATIBLE_NAME);
	if (offset == -FDT_ERR_NOTFOUND) {
		printf("can't find ion node in dtb\n");
		return offset;
	}

	fdt_get_property(fdt, offset, POOL_NUM, &len);
	mem_region_property[0] = cpu_to_fdt32(pool_num);
	fdt_setprop(fdt, offset, POOL_NUM, mem_region_property, 4);

	for (memcount=0, i = 0; i < pool_num; i++) {
		mem_region_property[memcount++] = cpu_to_fdt32(0x0);
		mem_region_property[memcount++] = cpu_to_fdt32(ion_mem[i].base);
		mem_region_property[memcount++] = cpu_to_fdt32(0x0);
		mem_region_property[memcount++] = cpu_to_fdt32(ion_mem[i].size);
	}

	fdt_setprop(fdt, offset, "reg", mem_region_property, 4 * memcount);

	for (len=0,i = 0; i < pool_num; i++) {
		strcpy(reg_names + len, ion_mem[i].name);
		len += strlen(ion_mem[i].name) + 1;
	}
	ret = fdt_setprop(fdt, offset, "reg-names", reg_names, len);

	mem_region_property[0] = cpu_to_fdt32(2);
	fdt_setprop(fdt, offset, ATTR_NUM_PER_POOL, mem_region_property, 4);

	for (memcount=0, i = 0; i < pool_num; i++) {
		mem_region_property[memcount++] = cpu_to_fdt32(TEE_MR_ION_ALG(ion_mem + i));
		mem_region_property[memcount++] = cpu_to_fdt32(TEE_MR_ION_ATTRIB(ion_mem + i));
	}
	fdt_setprop(fdt, offset, POOL_ATTRIBUTES, mem_region_property, 4 * memcount);

	return fdt_pack(fdt);
}

int setup_system_memory(void *fdt, struct mem_region *system_mem, int pool_num)
{
	unsigned int mem_region_property[8];
	unsigned int value[8];
	int memcount = 0;
	int offset = 0;
	int len = 0, i = 0;
	int ret;

	// linux only support two system region maximumly
	if((pool_num > 2) || (pool_num <=0))
		return -1;

	/* let's give it all the room it could need */
	ret = fdt_open_into(fdt, fdt, DTB_SPACE);
	if (ret < 0)
		return ret;

	offset = fdt_path_offset(fdt, "/memory");
	fdt_get_property(fdt, offset, "reg", &len);

	for(i = 0; i < pool_num; i++) {
		mem_region_property[memcount++] = cpu_to_fdt32(0x0);
		mem_region_property[memcount++] = cpu_to_fdt32(system_mem[i].base);

		mem_region_property[memcount++] = cpu_to_fdt32(0x0);
		mem_region_property[memcount++] = cpu_to_fdt32(system_mem[i].size);
	}

	fdt_setprop(fdt, offset, "reg", mem_region_property, 4*memcount);

	return fdt_pack(fdt);
}

int setup_tz_mem(void *fdt)
{
	struct mem_region * mem = NULL;
	struct mem_region ion_mem[MAX_REGION_COUNT];
	struct mem_region system_mem[2];
	struct mem_region bl_mem[1];
	struct reserve_entry reserved_mem[MAX_REGION_COUNT];

	int i, region_count = 0, ion_num = 0, system_num = 0, bl_num = 0, reserved_num = 0;
	u32 sys_start = 0, sys_end = 0;

	mem = get_mem_region_list_from_tz();
	if (!mem)
		return -1;

	region_count = get_mem_region_count();

	for (i = 0; i < region_count; i++) {
		if (TEE_MR_USER_IS_SYSTEM(mem+i)) {
			memcpy(system_mem + system_num++, mem + i, sizeof(struct mem_region));
			if((sys_start == 0) && (sys_end == 0)) {
				//only care the first system memory region
				sys_start = mem[i].base;
				sys_end = sys_start + mem[i].size;
			}
		}

		if (TEE_MR_USER_IS_BL(mem+i)) {
			memcpy(bl_mem, mem + i, sizeof(struct mem_region));
			bl_num = 1;
		}

		if (TEE_MR_USER_IS_ION(mem+i))
			memcpy(ion_mem + ion_num++, mem + i, sizeof(struct mem_region));
	}

	//set the region of memreserve, it's the overlap between system and ion
	memset(reserved_mem, 0x0, sizeof(reserved_mem));
	for(i = 0; i < region_count; i++) {
		if(TEE_MR_ION_IS_CMA(mem+i) && TEE_MR_USER_IS_ION(mem+i))
			continue;

		/* Continue below reserve memory check for Non-CMA ION */
		/* Notice:
		 *     1. Only the first system is the case we handle.
		 *     2. ION regions should not have overlay with each other.
		 */
		if(TEE_MR_USER_IS_ION(mem+i)) {
			//ion is overlap with system, then set the reserved memory
			if((mem[i].base >= sys_start) && (mem[i].base < sys_end)) {
				printf("memory layout find overlap, 0x%x, 0x%x, (0x%x:0x%x)\n", mem[i].base, mem[i].size, sys_start, sys_end);
				if(mem[i].size <= (sys_end - mem[i].base)) {
					reserved_mem[reserved_num].reserved_mem_start = mem[i].base;
					reserved_mem[reserved_num].reserved_mem_size = mem[i].size;
					reserved_num++;
				} else {
					//fdt_set_reserved_mem(fdt, mem[i].base, (sys_end - mem[i].base));
					printf("memory layout configuration error!!!\n");
				}
			}
		}
	}
	if(reserved_num) {
		setup_reserved_memory(fdt, (void *)&reserved_mem, reserved_num);
		for(i = 0; i < reserved_num; i++)
			printf("reserved memory %d: Addr 0x%x, Size 0x%x\n", i, reserved_mem[i].reserved_mem_start,
					reserved_mem[i].reserved_mem_size);
	}

	if(bl_num) {
		if (system_num < 2)
			memcpy(system_mem + system_num++, bl_mem, sizeof(struct mem_region));
		else
			printf("NO system slot for Bootloader\n");
	}

	if(system_num)
		setup_system_memory(fdt, system_mem, system_num);

	if (ion_num) {
		setup_ion_memory(fdt, ion_mem, ion_num);
	}

	return 0;
}

#ifdef CONFIG_SYNA_OEMBOOT_AB
#define ROOTFS_CONFIG_LINE "root=/dev/mmcblk0p"
#define ROOTFS_A "rootfs_a"
extern int f_mmc_get_part_index(char *part_name);

static void setup_rootfs(char *bootargs)
{
	int index = -1;
	char index_str[8];

	/*upate rootfs partition*/
	strcat (bootargs, ROOTFS_CONFIG_LINE);
	index = f_mmc_get_part_index(ROOTFS_A);

	if (index >= 0) {
		sprintf(index_str, "%d", index);
		strcat(bootargs, index_str);
		strcat(bootargs, " ro");
	}
}
#endif

int setup_bootargs(void *fdt)
{
	int len = 0;
	char *newbootargs = NULL;

	char *bootargs = env_get("bootargs");
	if (!bootargs) {
		printf("Don't see bootargs ENV, skip updating\n");
		return 0;
	}

	len = strlen(bootargs);
	newbootargs = malloc(len + 256);
	if (!newbootargs) {
		printf("Error: malloc in setup_bootargs failed!\n");
		return -ENOMEM;
	}

	strcpy(newbootargs, bootargs);
	strcat(newbootargs, " ");
#ifdef CONFIG_SYNA_OEMBOOT_AB
	setup_rootfs(newbootargs);
	strcat(newbootargs, " ");
#endif
	setup_cma_param(newbootargs);
	env_set("bootargs", newbootargs);

	return 0;
}

int fdt_update(struct fdt_header *fdt)
{
	setup_tz_mem(fdt);
	setup_bootargs(fdt);

	return 0;
}

