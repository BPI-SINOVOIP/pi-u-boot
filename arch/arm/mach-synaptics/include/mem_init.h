#ifndef __MEM_INIT_H__
#define __MEM_INIT_H__

#define MAX_REGION_COUNT 16

struct mem_region* get_mem_region_list_from_tz(void);
int get_mem_region_count(void);
void get_mem_region_by_name(u64 *start, u64 *size, char *zone_name);

unsigned int get_ion_cma_pool_addr(void);
unsigned int get_ion_cma_pool_size(void);

void *malloc_ion_cacheable(int size);
void free_ion_cacheable(void *m);

void * malloc_ion_noncacheable(int size);
void free_ion_noncacheable(void * m);

#endif
