#ifndef __MEM_INIT_H__
#define __MEM_INIT_H__

void get_mem_region_list_from_tz(void);
void get_mem_region_by_name(u64 *start, u64 *size, char *zone_name);

void * malloc_ion_cacheable(int size);
void free_ion_cacheable(void * m);
void * malloc_ion_noncacheable(int size);
void free_ion_noncacheable(void * m);

#endif
