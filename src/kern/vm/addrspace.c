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

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <bitmap.h>
#include <mainbus.h>
#include <vfs.h>
#include <vnode.h>
#include <current.h>
#include <synch.h>
#include <kern/fcntl.h>
#include <stat.h>
#include <uio.h>
#include <spl.h>
#include <generic/random.h>
#include <clock.h>
#include <mips/trapframe.h>


#include <spinlock.h>
#include <thread.h>
#include <mips/tlb.h>


/* under dumbvm, always have 48k of user stack */
#define INIT_STACKPAGES    12
#define PHYSICAL_STACKPAGES 1
#define PHYSICAL_SEG1PAGES 1
#define PHYSICAL_SEG2PAGES 1
#define SWAP_TEST true

#define LEVEL1INDEX(x) ((x) & 0xfff00000)>>20
#define LEVEL2INDEX(x) ((x) & 0x000ff000)>>12

//#define TLB_ENTRIES_INVALIDATE


struct vnode *swap_file;
page_table_entry *swapspace;
struct page_table_entry *pf_table;
struct lock *vm_lock;
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
int last_tlb_entry;
paddr_t physical_base_addr;
int num_vm_faults;

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

//dirty bit not implemented for the first implementation

int add_page_entry(addrspace * tbl, vaddr_t vadr, paddr_t paddr){
	//assuming the page table entry is well made ie valid bit is correctly set
	vaddr_t vaddr1=LEVEL1INDEX(vadr);
	if(tbl->table[vaddr1]== NULL){
		tbl->table[vaddr1] = kmalloc(sizeof(struct page_table_level2));
		int i;
		for( i=0;i<MAX_LEVEL2_PT;i++)
		{
			tbl->table[vaddr1]->table[i].valid_bit = false;
		}
	}

	vaddr_t vaddr2=LEVEL2INDEX(vadr);

	//if(tbl->size==MAX_PT_SIZE) return 1;   // not found
	tbl->table[vaddr1]->table[vaddr2].physical_addr = paddr;
	tbl->table[vaddr1]->table[vaddr2].virtual_addr = vadr;
    #ifdef TLB_ENTRIES_INVALIDATE
	tbl->table[vaddr1]->table[vaddr2].valid_bit = false;
    #else
    tbl->table[vaddr1]->table[vaddr2].valid_bit = true;
    #endif
	tbl->table[vaddr1]->table[vaddr2].dirty_bit = false;
	tbl->table[vaddr1]->table[vaddr2].ref_bit = false;
	tbl->table[vaddr1]->size++;
	paddr_t p12 = (paddr-physical_base_addr)>>12;
	pft.table[p12].physical_addr = paddr;
	pft.table[p12].virtual_addr = vadr;
    #ifdef TLB_ENTRIES_INVALIDATE
	pft.table[p12].valid_bit = false;
    #else
    pft.table[p12].valid_bit = true;
    #endif
	pft.table[p12].dirty_bit = false;
	pft.table[p12].ref_bit = false;
    pft.table[p12].p_space = tbl;
	return 0;
}
/*
   int change_page_entry(page_table tbl&, page_table_entry pentr){
   int i=0;
   for(; i<tbl.size; i++){
   if(pentr.virtual_addr==table[i].virtual_addr){
   table[i].physical_addr=pentr.physical_addr;
   return 0;
   }
   }
// not found
return 1;
}
 */

static
	paddr_t
getppages(unsigned long npages)
{
	paddr_t addr;
	spinlock_acquire(&stealmem_lock);
	addr = ram_stealmem(npages);
	//change this to implement non contiguous
	spinlock_release(&stealmem_lock);
	return addr;
}

paddr_t get_physical_address(addrspace *as, vaddr_t vaddr, int faulttype)
{
	int offset=vaddr & OFFSET_MASK;

    bool page_entry_exists=true;

	vaddr_t vaddr1=LEVEL1INDEX(vaddr);
	vaddr_t vaddr2=LEVEL2INDEX(vaddr);

	if(as->table[LEVEL1INDEX(vaddr)] == NULL){
        page_entry_exists=false;
        // TODO make new level if not already

		//panic("Virtual page not allocated. Second level page table not found\n");
	} else if(as->table[vaddr1]->table[vaddr2].valid_bit==false){
        page_entry_exists=false;
    }


    if(!page_entry_exists){
        // kprintf("page entry not exists, finding\n");
        // get some physical pages using virtual_alloc_kpages
        paddr_t* target_page=virtual_alloc_kpages(1);
        // add_page_entry
        add_page_entry(as,vaddr,*target_page);
        // add tlb entry
        insert_tlb_entry(vaddr,*target_page);
        // call swap in
        page_table_entry tmp_pte;
        tmp_pte.virtual_addr=vaddr;
        tmp_pte.physical_addr=*target_page;
        swap_in(tmp_pte);
    }

    paddr_t pad = as->table[vaddr1]->table[vaddr2].physical_addr;

	switch (faulttype) {
		case VM_FAULT_READONLY:
			/* We always create pages read-write, so we can't get this */
			panic("dumbvm: got VM_FAULT_READONLY\n");
		case VM_FAULT_READ:
			as->table[vaddr1]->table[vaddr2].ref_bit=true;


            pad -= physical_base_addr;
            pad /= 4096;
            pft.table[pad].ref_bit = true;

			break;
		case VM_FAULT_WRITE:
			as->table[vaddr1]->table[vaddr2].dirty_bit=true;
            as->table[vaddr1]->table[vaddr2].ref_bit=true;

            pad -= physical_base_addr;
            pad /= 4096;
            pft.table[pad].ref_bit = true;

			break;
		default:
			return EINVAL;
	}
	return (as->table[vaddr1]->table[vaddr2].physical_addr)+offset;
}
/*
int delete_page_entry(page_table tbl&, vaddr_t vaddr){
// check if dirty, and perform a swap out if it is
int i=0;
for(;i<size;i++){
if(pentr.virtual_addr==table[i].virtual_addr){
table[i].physical_addr=pentr.physical_addr
present_bit[i]=false;
return 0;
}
}
// not found
return 1;
}*/
int page_frame_table_bootstrap(){
	kprintf("pft initialized\n");
    clock_hand = 0;
    ep_hand = 0;
    rp_hand = CLOCK_ANGLE;
    cnt_page_repl = 0;
	int total_pages=mainbus_ramsize()/PAGE_SIZE - 10;
	paddr_t pa;
	pa = getppages(MAX_PM_SIZE);
	paddr_t physical_base_addr = pa;
//	alloc_kpages(MAX_PM_SIZE);
	int i=0;
	//initialize all the elemnts
	for(;i<MAX_PM_SIZE;i++)
	{
		pft.table[i].valid_bit=false;
		pft.table[i].physical_addr=pa;
		pa+=PAGE_SIZE;
		//change a few elements
	}

	return 0;
}
/* Directly implemented in as_create
   int page_table_bootstrap(addrspace& pt){
   pt.size=0;
   pt.as_stackpbase = 0;
   int i;
   for( i=0;i<MAX_PT_SIZE;i++)
   {
   pt.able[i].valid_bit=false;
   }
   return 0;
   }
 */


/* Allocate/free some kernel-space virtual pages */
paddr_t virtual_alloc_page()
{
	paddr_t addr;
	spinlock_acquire(&stealmem_lock);
	int i;
	for(i=0;i<MAX_PM_SIZE;i++)
	{
		if(pft.table[i].valid_bit==false)
			break;

	}
    #ifdef TLB_ENTRIES_INVALIDATE
	pft.table[i].valid_bit=false;
    #else
    pft.table[i].valid_bit=true;
    #endif

	pft.table[i].dirty_bit=false;
	pft.table[i].ref_bit=false;
	addr=pft.table[i].physical_addr;

	//change a few elements
	//change this to implement non contiguous
	spinlock_release(&stealmem_lock);
	return addr;
}

//This function will use kmalloc which does not have a counterpart to free it.
paddr_t* virtual_alloc_kpages(int k)
{
    //kprintf("%d\n",k);
	paddr_t* p_list=kmalloc(sizeof(paddr_t)*k);
	paddr_t addr;
	spinlock_acquire(&stealmem_lock);
	int i,j=0;
	for(i=0;i<MAX_PM_SIZE && j<k;i++)
	{
		if(pft.table[i].valid_bit==false)
		{
			#ifdef TLB_ENTRIES_INVALIDATE
            pft.table[i].valid_bit=false;
            #else
            pft.table[i].valid_bit=true;
            #endif
			pft.table[i].dirty_bit=false;
			pft.table[i].ref_bit=false;
			addr=pft.table[i].physical_addr;
			p_list[j] = addr;
			j++;
		}
	}
	if(i==MAX_PM_SIZE)
	{
        // kprintf("Replacing pages\n");
        for(int i=j; i<k; i++){
            p_list[j] = replace_page();
            int new_frame_index = (p_list[j]-physical_base_addr)/4096;
            addrspace* p_space_old = pft.table[new_frame_index].p_space;
            vaddr_t old_page = pft.table[new_frame_index].virtual_addr;
            vaddr_t old_page_lvl_1 = LEVEL1INDEX(old_page);
            vaddr_t old_page_lvl_2 = LEVEL2INDEX(old_page);
            p_space_old->table[old_page_lvl_1]->table[old_page_lvl_2].valid_bit = false;
            page_table_entry tmp_pte;
            tmp_pte.virtual_addr=old_page;
            tmp_pte.physical_addr=p_list[j];
            swap_out(tmp_pte);
	    tlb_invalidate(old_page);
        }

        // TODO do page replacement
		//return ENOMEM;
	}
	//change a few elements
	//change this to implement non contiguous
	spinlock_release(&stealmem_lock);
	return p_list;
}


	vaddr_t
alloc_kpages(int npages)
{
	paddr_t pa;
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

	void
free_kpages(vaddr_t addr)
{
	/* nothing - leak the memory. */

	(void)addr;
}

	struct addrspace *
as_create(void)
{
	num_vm_faults = 0;
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}
	as->as_vbase1 = 0;
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	as->as_npages2 = 0;
	as->size=0;
	as->as_stackpbase = 0;
	int i;
	for( i=0;i<MAX_LEVEL1_PT;i++)
	{
		as->table[i] = NULL;
	}
	return as;
}

	int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}
	newas->as_vbase1 = old->as_vbase1;
	newas->as_npages1 = old->as_npages1;
	newas->as_vbase2 = old->as_vbase2;
	newas->as_npages2 = old->as_npages2;
	newas->pid = old->pid + 1;
	newas->size = old->size;
	newas->as_stackpbase = old->as_stackpbase;

	paddr_t temp_paddr;
	int i;
	for( i=0;i<MAX_LEVEL1_PT;i++)
	{
		if(old->table[i] != NULL)

		{
			int j=0;
			for(;j<MAX_LEVEL2_PT;j++)
			{
				if(old->table[i]->table[j].valid_bit==true)
				{
					newas->table[i]->table[j].valid_bit = old->table[i]->table[j].valid_bit;
					temp_paddr = virtual_alloc_page();
					newas->table[i]->table[j].physical_addr = temp_paddr;
					memmove((void *)PADDR_TO_KVADDR(temp_paddr),
							(const void *)PADDR_TO_KVADDR(old->table[i]->table[j].physical_addr),
							PAGE_SIZE);

					newas->table[i]->table[j].dirty_bit = old->table[i]->table[j].dirty_bit;
					newas->table[i]->table[j].ref_bit = old->table[i]->table[j].ref_bit;
					newas->table[i]->table[j].pid = old->table[i]->table[j].pid + 1;
				}
			}
			*ret = newas;
			return 0;
		}
	}
}

void
as_destroy(struct addrspace *as)
{
	/*
	 * Clean up as needed.
	 */
	//can be optimised by putting pointers to the allocated pages.
	int i;
	for( i=0;i<MAX_LEVEL1_PT;i++)
	{
		if(as->table[i]!= NULL)

		{
			int j=0;
			for(;j<MAX_LEVEL2_PT;j++)
			{
				if(as->table[i]->table[j].valid_bit==true)
				{

					paddr_t p = as->table[i]->table[j].physical_addr;
					pft.table[(p-physical_base_addr)/4096].valid_bit=false;

					// int k=0;
					// for(;k<MAX_PM_SIZE;k++){
					// 	if(pft.table[i][physical_addr] == p){
					// 		pft.table[i].valid_bit=false;
					// 	}
					// }
				}
			}
			kfree(as->table[i]);
		}
	}
	kfree(as);

    kprintf("Number of pages replaced = %d\n", count_page_replacement_calls());
    kprintf("Number of faults  = %d\n", num_vm_faults);
    kprintf("Number of swap insertions  = %d\n", num_swap_insertions());
}

	void
as_activate(struct addrspace *as)
{
	/*
	 * Write this.
	 */
	int i, spl;

	(void)as;

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
	int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		int readable, int writeable, int executable)
{
	size_t npages;

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;   //vaddr & ~PAGE_FRAME gives the offset
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;


	(void)readable;
	(void)writeable;
	(void)executable;
	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;
		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
		return 0;
	}
	return EUNIMP;
}

/*static
  void
  as_zero_region(paddr_t paddr, unsigned npages)
  {
  bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);
  }*/


	void
as_zero_page(paddr_t paddr)
{
	void* x = (void*)PADDR_TO_KVADDR(paddr);
	int y = PAGE_SIZE;
	bzero(x, y);
}


int
as_prepare_load(struct addrspace *as)
{

	int seg1pages, seg2pages,stackpages;
	if(SWAP_TEST)
	{
		seg1pages=as->as_npages1;
		seg2pages=PHYSICAL_SEG2PAGES;
		stackpages=PHYSICAL_STACKPAGES;
	}
	else
	{
		seg1pages=as->as_npages1;
		seg2pages=as->as_npages2;
		stackpages=INIT_STACKPAGES;
	}

	paddr_t* p_list = virtual_alloc_kpages(seg1pages);
	paddr_t* p_list2 = virtual_alloc_kpages(seg2pages);
	paddr_t* p_list3 = virtual_alloc_kpages(stackpages);

	int i;

	kprintf("\nVirtual \t Physical\n");
	kprintf("------------------------\n");

	for(i = 0 ; i< /*1*/ seg1pages ; i++)
	{
		kprintf("%x \t %x \n",as->as_vbase1+i*PAGE_SIZE, p_list[i]);
		add_page_entry(as,as->as_vbase1+i*PAGE_SIZE,p_list[i]);
		as_zero_page(p_list[i]);
	}


	for(i = 0 ; i< /*1*/ seg2pages ; i++)
	{
		kprintf("%x \t %x \n",as->as_vbase2+i*PAGE_SIZE, p_list2[i]);
		add_page_entry(as,as->as_vbase2+i*PAGE_SIZE,p_list2[i]);
		as_zero_page(p_list2[i]);
	}


	//as->as_stackpbase = p_list[0];
	as->as_stacknpages=stackpages;
	for(i = 0 ; i< stackpages ; i++)
	{
		kprintf("%x \t %x \n",USERSTACK-(i+1)*PAGE_SIZE, p_list3[i]);
		add_page_entry(as,USERSTACK-(i+1)*PAGE_SIZE,p_list3[i]);
		as_zero_page(p_list3[i]);
	}
	kfree(p_list);
	kfree(p_list2);
	kfree(p_list3);

	//as->as_stackpbase = getppages(DUMBVM_STACKPAGES);


	return 0;

}

	int
as_complete_load(struct addrspace *as)
{
	//swapspace_load_as(as);
	(void)as;
	return 0;
}

	int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	//KASSERT(as->as_stackpbase != 0);

	*stackptr = USERSTACK;
	return 0;
}

	void
vm_tlbshootdown_all(void)
{
	panic("dumbvm tried to do tlb shootdown?!\n");
}

	void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

int vm_fault(int faulttype, vaddr_t faultaddress)
{
	num_vm_faults++;
    // kprintf("INSIDE VM FALUT :eee Fault type: %d\n", faulttype);
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	struct addrspace *as;

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
		case VM_FAULT_READONLY:
			/* We always create pages read-write, so we can't get this */
			panic("dumbvm: got VM_FAULT_READONLY\n");
		case VM_FAULT_READ:
		case VM_FAULT_WRITE:
			break;
		default:
			return EINVAL;
	}

	as = curthread->t_addrspace;
	if (as == NULL) {
		/*
		 * No address space set up. This is probably a kernel
		 * fault early in boot. Return EFAULT so as to panic
		 * instead of getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	KASSERT(as->as_vbase1 != 0);
	// KASSERT(as->as_pbase1 != 0);
	KASSERT(as->as_npages1 != 0);
	KASSERT(as->as_vbase2 != 0);
	// KASSERT(as->as_pbase2 != 0);
	KASSERT(as->as_npages2 != 0);
	// KASSERT(as->as_stackpbase != 0);
	KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	// KASSERT((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	// KASSERT((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
	// KASSERT((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - INIT_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

	// TODO use translations    and remove redundant cases
	if (faultaddress >= vbase1 && faultaddress < vtop1) {
        //kprintf("boo1\n");
		paddr = get_physical_address(as, faultaddress, faulttype);
		// paddr = (faultaddress - vbase1) + as->as_pbase1;   /** translation of addresses !!! ***/
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
        //kprintf("boo2\n");
		paddr = get_physical_address(as, faultaddress, faulttype);
		// paddr = (faultaddress - vbase2) + as->as_pbase2;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
        //kprintf("boo3\n");
		paddr = get_physical_address(as, faultaddress, faulttype);
		// paddr = (faultaddress - stackbase) + as->as_stackpbase;
	}
	else {
        kprintf("bad mem reference\n");
		return EFAULT;
	}
	// kprintf("boo %d %x %x \n",faulttype, faultaddress, paddr);


	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);
    return insert_tlb_entry(faultaddress,paddr);
}


/****************Page Replacement policies**************************/


paddr_t replace_page(){
    cnt_page_repl++;
	return second_chance_replace();
}

int count_page_replacement_calls(){
    return cnt_page_repl;
}

paddr_t random_replace(){
    size_pft num_page_frames = MAX_PM_SIZE;
    int n = random()%MAX_PM_SIZE;
    paddr_t new_page_frame = pft.table[n].physical_addr;
    return new_page_frame;
}

paddr_t one_handed_replace(){
    size_pft num_page_frames = MAX_PM_SIZE;
    for(int i=0; i<=num_page_frames+1; i++){
        if(pft.table[clock_hand].ref_bit == false){
	        clock_hand++;
                break;
        }
        else {
            pft.table[clock_hand].ref_bit = false;
            clock_hand++;
            clock_hand %= num_page_frames;
        }
    }

    paddr_t new_page_frame = pft.table[clock_hand%num_page_frames].physical_addr;
    clock_hand++;
    clock_hand %= num_page_frames;
    return new_page_frame;
}

paddr_t second_chance_replace(){
    size_pft num_page_frames = MAX_PM_SIZE;
    size_pft i = 0;
    for(i; i<=CLOCK_ANGLE+1; i++){
        if(pft.table[ep_hand].ref_bit == false){
                break;
        }
        else{
            pft.table[rp_hand].ref_bit = false;
            ep_hand++;
            rp_hand++;
            ep_hand %= num_page_frames;
            rp_hand %= num_page_frames;
        }
    }


    paddr_t new_page_frame = pft.table[ep_hand%num_page_frames].physical_addr;

    ep_hand++;
    rp_hand++;
    ep_hand %= num_page_frames;
    rp_hand %= num_page_frames;

    return new_page_frame;
}

paddr_t lru_replace(){

    bool found = false;

    size_pft num_page_frames = MAX_PM_SIZE;
    size_pft i = 0;
    size_pft selected_page_index = 0;
    for(i=0; i<num_page_frames; i++){
        if(pft.table[i].ref_bit==false){
            selected_page_index = i;
            found = true;
           break;
        }
    }

    if(found==false) selected_page_index = random_replace();

    return  pft.table[selected_page_index].physical_addr;
}

/*************************TLB MANIPULATION FUNCTIONS********************/
int insert_tlb_entry(vaddr_t faultaddress, paddr_t paddr)
{
/* Disable interrupts on this CPU while frobbing the TLB. */
    int i;
    int ret=0;
    uint32_t ehi, elo;
    int spl;
    ehi = faultaddress;
    int prev=tlb_probe(ehi,0);
    if(prev>=0)
    {
        return 0;
    }
    spl = splhigh();
    // kprintf("vm_fault %x\n", faultaddress);
    for (i=0; i<NUM_TLB; i++) {
        tlb_read(&ehi, &elo, i);
        if (elo & TLBLO_VALID) {
            continue;
        }
    }
    if(i==NUM_TLB)
    {
        i=last_tlb_entry;
        last_tlb_entry++;
        last_tlb_entry%=NUM_TLB;
    }
    elo=0;
    ehi=0;
    ehi = faultaddress;
    elo = paddr | TLBLO_DIRTY | TLBLO_VALID;   /*** BINGO ***/

    DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
    //kprintf("insert_tlb_entry: 0x%x -> 0x%x\n", faultaddress, paddr);
    tlb_write(ehi, elo, i);

    splx(spl);
    return ret;
}

void tlb_invalidate(vaddr_t vaddr)
{
	uint32_t ehi, elo;
	ehi = vaddr;
	int index=tlb_probe(ehi,0);
	if(index>0)
		tlb_write(TLBHI_INVALID(index), TLBLO_INVALID(), index);
}
/*
bool tlb_is_referenced(vaddr_t vaddr)
{
	uint32_t ehi, elo;
	ehi = vaddr;
	int index=tlb_probe(ehi,0);
	if(index<0)
	{
		return false;
	}
        tlb_read(&ehi, &elo, i);
        if (elo & TLBLO_VALID) {
            continue;
        }
}
*/



