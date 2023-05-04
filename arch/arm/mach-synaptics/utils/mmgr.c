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
/* a simple function to implement free/malloc
 * 1. don't support mutilple thread now
 * 2. maximum 200 memory nodes
 */
#ifdef __linux
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INFO printf
#define NOTICE printf
#define ERR printf

typedef enum {
	MEM_COMMON = 0,
	MEM_ION_CACHEABLE,
	MEM_ION_NONCACHEABLE,
	MEM_TYPE_MAX,
}MEM_TYPE;

#else
#define INFO printf
#define NOTICE printf
#define ERR printf

#include <linux/string.h>
#include "mmgr.h"
#endif

//#define DEBUG 1

// reserver 8KB to store Memmgr
#define RESERVE_MEM_SIZE (8 * 1024)
#define MAXNUM_POOL 200

#define PAGE	(4096)
#define PAGE_MASK   (PAGE - 1)

//size under 64bits system is 32 bytes
typedef struct Node{
	struct Node * prev;
	struct Node * next;
	unsigned long base;
	unsigned int size;
	unsigned int isused;
}Mnode;

enum {
	NODE_FREE = 0,
	NODE_USED = 1,
};

typedef struct{
	unsigned long membase;
	unsigned int totalsize;
	unsigned int num;
	Mnode * p_free;
	Mnode * p_alloc;
	Mnode pool[MAXNUM_POOL];
}Memmgr;

static Memmgr * g_mmgr[MEM_TYPE_MAX];

#if DEBUG
static int nnum = 0;
unsigned long dump_nodes(Mnode * list)
{
	Mnode * p = list;
	unsigned long size = 0;
	while(p) {
		size += p->size;
		nnum++;
		NOTICE("base = %lx\n", p->base);
		NOTICE("size = %x\n", p->size);
		NOTICE("------------------------\n");
		p = p->next;
	}
	return size;
}

void dump_mgr()
{
	Memmgr * mmgr = NULL;
	int i = 0;
	unsigned long size = 0;

	for(i = 0; i < MEM_TYPE_MAX; i++) {
		size = 0;
		nnum = 0;
		mmgr = g_mmgr[i];
		if(mmgr == NULL) {
			NOTICE("@@@@@@@@@@ type = %d, not initialized\n", i);
			return;
		}
		NOTICE("######################## type = %d, %p\n", i, mmgr);
		NOTICE("membase = %lx\n", mmgr->membase);
		NOTICE("totalsize = %x\n", mmgr->totalsize);
		NOTICE("node num = %d\n", mmgr->num);
		NOTICE("allocated list\n");
		size += dump_nodes(mmgr->p_alloc);
		NOTICE("\nfree list\n");
		size += dump_nodes(mmgr->p_free);
		NOTICE("size = %lx, node = %d\n", size, nnum);
		if(size != mmgr->totalsize)
			ERR("size ERRRRRRRRRRRRRRRRRRRR\n");
		if(nnum != mmgr->num)
			ERR("num ERRRRRRRRRRRRRRRRRRRR\n");
		NOTICE("########################\n");
	}
}
#endif

static Mnode * find_valid_node_from_pool(Memmgr * mmgr, Mnode *pool)
{
	int i = 0;
	for(i = 0; i < MAXNUM_POOL; i++)
		if(NODE_FREE == pool[i].isused) {
			pool[i].isused = NODE_USED;
			mmgr->num++;
			INFO("find a valid node %d\n", mmgr->num);
			return &pool[i];
		}

	return NULL;
}

static void return_node_2_pool(Memmgr * mmgr, Mnode *node)
{
	memset(node, 0, sizeof(Mnode));
	node->isused = NODE_FREE;
	mmgr->num--;
}

void init_mmgr_by_type(MEM_TYPE type, unsigned long mstart, unsigned int size)
{
	Memmgr * mmgr = g_mmgr[type];
	if(mmgr != NULL) {
		ERR("mmgr has been initialized\n");
		return;
	}

	//clear the reserved memory
	memset((void *)mstart, 0, RESERVE_MEM_SIZE);
	mmgr = (Memmgr *)mstart;
	mmgr->membase = mstart + RESERVE_MEM_SIZE;
	mmgr->totalsize = size - RESERVE_MEM_SIZE;

	//init the first point
	mmgr->num = 0;
	mmgr->p_free = find_valid_node_from_pool(mmgr, mmgr->pool);
	mmgr->p_free->base = mmgr->membase;
	mmgr->p_free->size = mmgr->totalsize;
	mmgr->p_free->prev = NULL;
	mmgr->p_free->next = NULL;

	//no allocate node
	mmgr->p_alloc = NULL;
	g_mmgr[type] = mmgr;
}

void init_mmgr(unsigned long mstart, unsigned int size)
{
	init_mmgr_by_type(MEM_COMMON, mstart, size);
}


void * mmgr_alloc_by_type(MEM_TYPE type, int size)
{
	Memmgr * mmgr = g_mmgr[type];
	Mnode *p = NULL;
	unsigned int alloc_size = 0x0;

	if(mmgr == NULL) {
		ERR("mmgr has not been initialized\n");
		return NULL;
	}

	p = mmgr->p_free;

	if(size <= 0) {
		ERR("size of allocated memory is not correct!\n");
		return NULL;
	}

	/*
	*  the size should be 4KB aligned if allocated from ION
	*  for bootloader heap, the size should be 64B aligned
	*/
	if(MEM_ION_CACHEABLE == type || MEM_ION_NONCACHEABLE == type)
		alloc_size = (size & PAGE_MASK) ? ((size & (~PAGE_MASK)) + PAGE) : size;
	else
		alloc_size = (size & 0x3f) ? ((size & (~0x3f)) + 64) : size;

	if(mmgr->num > MAXNUM_POOL) {
		ERR("exceed the max num of memory node!\n");
		return NULL;
	}

	while(p) {
		if(p->size >= alloc_size) {
			//find the free node

			//handle the new allocted node
			Mnode * new = find_valid_node_from_pool(mmgr, mmgr->pool);
			new->base = p->base;
			new->size = alloc_size;
			//put to the head of allocated list
			new->next = mmgr->p_alloc;
			mmgr->p_alloc = new;
			if(new->next)
				new->next->prev = new;

			//handle the old free node
			if(p->size == alloc_size) {
				//delete the current node
				if((NULL == p->prev) && (NULL == p->next)) { //only one free node
					mmgr->p_free = NULL; // no free memory
				} else {
					if(p == mmgr->p_free) { //head
						mmgr->p_free = p->next;
						mmgr->p_free->prev = NULL;
					}
					if(p->prev)
						p->prev->next = p->next;
					if(p->next)
						p->next->prev = p->prev;
				}
				return_node_2_pool(mmgr, p);
			} else {
				//keep the node and decrease the size
				p->base += alloc_size;
				p->size -= alloc_size;
			}

			return (void *)(mmgr->p_alloc->base);
		}
		p = p->next;
	};

	if(p == NULL) { // no free memory
		ERR("can't alloc %dB memory!\n", size);
		return NULL;
	}

	return NULL;
}

void * mmgr_alloc(int size)
{
	return mmgr_alloc_by_type(MEM_COMMON, size);
}

// only support in common memory
void * mmgr_calloc(int n, int size)
{
	void * ptr = mmgr_alloc(n * size);
	memset(ptr, 0, (n*size));
	return ptr;
}

static Mnode * check_prevnode_and_merge(Memmgr * mmgr, Mnode *p)
{
	Mnode *prev = NULL;
	if(p->prev) {
		if((p->prev->base + p->prev->size) == p->base){
			p->prev->size += p->size;
			p->prev->next = p->next;
			if(p->next)
				p->next->prev = p->prev;
			prev = p->prev;
			return_node_2_pool(mmgr, p);
			return prev;
		}
	}
	return p;
}

static void insert_node_2_free(Memmgr * mmgr, Mnode * p)
{
	Mnode * pf = NULL;

	//if free list is empty, insert directly
	if(mmgr->p_free == NULL) {
		mmgr->p_free = p;
		p->prev = NULL;
		p->next = NULL;
		return;
	}

	pf = mmgr->p_free;
	while(pf) {
		if(p->base > pf->base) {
			if(pf->next)
				pf = pf->next;
			else
				break; // tail
		} else
			break;
	};

	//insert first
	if(p->base > pf->base) {//tail
		//append to the last
		pf->next = p;
		p->prev = pf;
		p->next = NULL;
	} else {
		//insert before the node found
		if(NULL == pf->prev) {//head
			mmgr->p_free = p;
		}
		p->prev = pf->prev;
		if(pf->prev)
			pf->prev->next = p;
		p->next = pf;
		pf->prev = p;
	}

	//check and merge
	pf = check_prevnode_and_merge(mmgr, p);
	check_prevnode_and_merge(mmgr, pf->next);
}

void mmgr_free_by_type(MEM_TYPE type, void * m)
{
	Memmgr * mmgr = g_mmgr[type];
	Mnode *p = NULL;

	if(mmgr == NULL) {
		ERR("mmgr has not been initialized\n");
		return;
	}

	p = mmgr->p_alloc;

	//remove the node from allocated list
	{
		while(p) {
			if(p->base == (unsigned long)m)
				break;
			p = p->next;
		};

		if(NULL == p) {
			ERR("can't find the allocated memory\n");
			return;
		}

		if((NULL == p->prev) && (NULL == p->next)) {// only one allocated node
			mmgr->p_alloc = NULL; // all free memory
		} else {
			if(p == mmgr->p_alloc) { // head
				mmgr->p_alloc = p->next;
				mmgr->p_alloc->prev = NULL;
			}
			if(p->prev)
				p->prev->next = p->next;
			if(p->next)
				p->next->prev = p->prev;
		}
	}

	//insert/merge the node to free list
	insert_node_2_free(mmgr, p);
}

void mmgr_free(void * m)
{
	mmgr_free_by_type(MEM_COMMON, m);
}

#if DEBUG
#ifdef __linux
void * m[200];
int count = 0;

#define RANDOM(x) (random()%x)

int free_or_malloc()
{
	return RANDOM(2);
}

int malloc_size()
{
	return RANDOM(102400);
}


void free_one()
{
	Memmgr * mmgr = g_mmgr[MEM_COMMON];
	int i = 0;
	if((count <= RANDOM(201)) && (mmgr->p_free))
		return;
	NOTICE("\n~~~~~~~~begin free\n");
	for(;;) {
		i = RANDOM(200);
		if(m[i]) {
			mmgr_free(m[i]);
			m[i] = NULL;
			break;
		}
	}
	dump_mgr();
	NOTICE("free end\n\n");
}

void malloc_one()
{
	int i = 0;
	int size = malloc_size();
	NOTICE("\n~~~~~~~~begin malloc %x\n", size);
	for(;;) {
		i = RANDOM(200);
		if(m[i] == NULL) {
			size = (size & 0x3) ? ((size & (~0x3)) + 4) : size;
			m[i] = mmgr_alloc(size);
			if(m[i] == NULL) {
				NOTICE("can't malloc %x\n", size);
				return;
			}
			count++;
			break;
		}
	}
	dump_mgr();
	NOTICE("malloc end\n\n");
}

int main()
{
	void * mm = NULL;
	int i = 0;
	unsigned int j = 0;

	NOTICE("test start %ld\n", RANDOM(2));
	mm = malloc(1024*1024);
	NOTICE("%p\n", mm);
	init_mmgr((unsigned long)mm, 1024*1024);
	dump_mgr();
	for(i = 0; i < 100; i++)
		m[i] = NULL;

	while(1) {
		sleep(1);
		if(free_or_malloc()) {
			free_one();
		} else {
			malloc_one();
		}
	}

	return 0;
}
#endif
#endif
