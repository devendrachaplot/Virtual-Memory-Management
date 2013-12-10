/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ADDRSPACE_H_
#define _ADDRSPACE_H_

/*
 */


#include <vm.h>
#include "opt-dumbvm.h"
#define MAX_PM_SIZE 30
//#define MAX_PT_SIZE (1024*1024)
#define MAX_LEVEL1_PT 4096
#define MAX_LEVEL2_PT 256
#define OFFSET_MASK 0x00000fff
#define INDEX_MASK 0xfffff000
#define CLOCK_ANGLE 8
struct vnode;


/*
 * Address space - data structure associated with the virtual memory
 * space of a process.
 *
 * You write this.
 */
// #if OPT_DUMBVM
// struct addrspace {
//         vaddr_t as_1;
//         paddr_t as_pbase1;
//         size_t as_npages1;
//         vaddr_t as_vbase2;
//         paddr_t as_pbase2;
//         size_t as_npages2;
//         paddr_t as_stackpbase;

//         /* Put stuff here for your VM system */
// };

// #else
typedef int size_page_table;
typedef int size_pft;

struct addrspace;

typedef struct page_table_entry{
    // write this
    vaddr_t virtual_addr;
    paddr_t physical_addr;
    bool valid_bit;
    bool dirty_bit;
    bool ref_bit;
    pid_t pid;
    int counter;
    struct addrspace* p_space;
    //pid not implemented
} page_table_entry;

typedef struct page_table_level2
{
    size_page_table size;                   // number of entries
    page_table_entry table[MAX_LEVEL2_PT];      // address space

}page_table_level2;


typedef struct addrspace
{
    vaddr_t as_vbase1;
    size_t as_npages1;
    vaddr_t as_vbase2;
    size_t as_npages2;
    pid_t pid;                              // the process having this information
    size_page_table size;                   // number of entries
    page_table_level2* table[MAX_LEVEL1_PT];      // address space
    vaddr_t as_stackpbase;
    size_t as_stacknpages;
}addrspace;




// typedef struct addrspace{
// //only two segments are allowed in first cut
// 	vaddr_t as_vbase1;
//     size_t as_npages1;
//     vaddr_t as_vbase2;
//     size_t as_npages2;
//     pid_t pid;                              // the process having this information
//     size_page_table size;                   // number of entries
//     page_table_entry table[MAX_PT_SIZE];      // address space
//     vaddr_t as_stackpbase;
// }addrspace;


typedef struct page_frame_table{
	page_table_entry table[MAX_PM_SIZE];      // address space
} page_frame_table;


page_frame_table pft;
size_pft clock_hand;
size_pft rp_hand;
size_pft ep_hand;

int cnt_page_repl;  //Just a metric

// add new entry
int add_page_entry(addrspace* tbl, vaddr_t vadr, paddr_t paddr);

// change existing entry. pentr must have the intended physical_addr for the previous virtual_addr;
int change_page_entry(addrspace *tbl, page_table_entry pentr);

// remove the entry
int delete_page_entry(addrspace *tbl, vaddr_t vaddr);

// initiate the page table
int page_frame_table_bootstrap();
paddr_t get_physical_address(addrspace *as, vaddr_t vaddr, int faulttype);
paddr_t* virtual_alloc_kpages(int k);

void tlb_invalidate(vaddr_t);
paddr_t replace_page();
//initialize the page table for a process
// int page_table_bootstrap(addrspace *);


/*
 * Functions in addrspace.c:
 *
 *    as_create - create a new empty address space. You need to make
 *                sure this gets called in all the right places. You
 *                may find you want to change the argument list. May
 *                return NULL on out-of-memory error.
 *
 *    as_copy   - create a new address space that is an exact copy of
 *                an old one. Probably calls as_create to get a new
 *                empty address space and fill it in, but that's up to
 *                you.
 *
 *    as_activate - make the specified address space the one currently
 *                "seen" by the processor. Argument might be NULL,
 *                meaning "no particular address space".
 *
 *    as_destroy - dispose of an address space. You may need to change
 *                the way this works if implementing user-level threads.
 *
 *    as_define_region - set up a region of memory within the address
 *                space.
 *
 *    as_prepare_load - this is called before actually loading from an
 *                executable into the address space.
 *
 *    as_complete_load - this is called when loading from an executable
 *                is complete.
 *
 *    as_define_stack - set up the stack region in the address space.
 *                (Normally called *after* as_complete_load().) Hands
 *                back the initial stack pointer for the new process.
 */

struct addrspace *as_create(void);

int     as_copy(struct addrspace *src, struct addrspace **ret);
void    as_activate(struct addrspace *);
void    as_destroy(struct addrspace *);

int     as_define_region(struct addrspace *as,
                         vaddr_t vaddr, size_t sz,
                         int readable,
                         int writeable,
                         int executable);
int     as_prepare_load(struct addrspace *as);
int     as_complete_load(struct addrspace *as);
int     as_define_stack(struct addrspace *as, vaddr_t *initstackptr);

void swap_out(page_table_entry pte);
void swap_in(page_table_entry pte);
uint32_t get_free_offset();
uint32_t get_free_page();
paddr_t one_handed_replace();
paddr_t random_replace();
paddr_t second_chance_replace();
paddr_t lru_replace();
int num_swap_insertions();

/*
 * Functions in loadelf.c
 *    load_elf - load an ELF user program executable into the current
 *               address space. Returns the entry point (initial PC)
 *               in the space pointed to by ENTRYPOINT.
 */

int load_elf(struct vnode *v, vaddr_t *entrypoint);

// #endif
// #endif /* _ADDRSPACE_H_ */
#endif /* _ADDRSPACE_H_ */
