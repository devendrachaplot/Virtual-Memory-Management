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

struct vnode *swap_file;
page_table_entry *swapspace;
struct page_table_entry *pf_table;
uint32_t swapspace_size;
struct lock *vm_lock;
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
int last_tlb_entry;
paddr_t physical_base_addr;
int swap_insertions;

/******************************* PIYUSH AND VIPUL's MASTERPIECE ***************************************************************************/

void swapspace_init() {
	struct stat temp;
	VOP_STAT(swap_file, &temp);
	swapspace_size = temp.st_size/PAGE_SIZE;

	swapspace = (struct page_table_entry*)kmalloc(swapspace_size * sizeof(struct page_table_entry));

	struct iovec swap_iov;
	struct uio swap_uio;

    //char *zero_page=kmalloc(PAGE_SIZE);
    //bzero(zero_page,PAGE_SIZE);

	int swapspace_start=0;
	for(int i = 0; i < swapspace_size; i++)
	{
        //kprintf("printing %d\n",i);
		swapspace[i].physical_addr = (swapspace_start + (i * PAGE_SIZE));

        //int spl=splhigh();
        //unsigned int offset=i*PAGE_SIZE;
        //uio_kinit(&swap_iov, &swap_uio, (void*)zero_page, PAGE_SIZE, offset, UIO_WRITE);

        //int result=VOP_WRITE(swap_file, &swap_uio);
        //if(result) {
        //    panic("VM: SWAP out Failed");
        //}
        //splx(spl);
    }
}

void
vm_bootstrap(void)
{
	swap_insertions = 0;
	page_frame_table_bootstrap();
	char fname[] = "lhd0raw:";
	vm_lock = lock_create("vm_lock");
	int result = vfs_open(fname, 0, O_RDWR , &swap_file);
	if(result){
		panic("Virtual Memory: Swap space could not be created \n");
    }
	swapspace_init();
    last_tlb_entry=0;
}

uint32_t swapspace_insert (uint32_t vaddr, int pid) {
	swap_insertions++;
	// kprintf("Swap :\t Insert \t V%x \n",vaddr);
	int spl=splhigh();
	// find the next empty space in swap space to insert this entry
	int pos=0;
	for(;pos<swapspace_size;pos++){
		if(swapspace[pos].valid_bit==false){
			break;
		}
	}
	KASSERT(pos<swapspace_size);

	swapspace[pos].virtual_addr = vaddr;
	swapspace[pos].counter = 0;
	swapspace[pos].pid = pid;
	swapspace[pos].valid_bit=true;
	splx(spl);
	return 0;
}

void swapspace_load_as(struct addrspace *as){
    int i;
    kprintf("Swap : Segment 1  size : %d\n",as->as_npages1);
    for(i=0;i<as->as_npages1;i++){
        swapspace_insert(as->as_vbase1+i*PAGE_SIZE,as->pid);
    }
    kprintf("Swap : Segment 2  size : %d\n",as->as_npages2);
    for(i=0;i<as->as_npages2;i++){
        swapspace_insert(as->as_vbase2+i*PAGE_SIZE,as->pid);
    }
    kprintf("Swap : Stack      size : %d\n",INIT_STACKPAGES);
    for(i=0;i<INIT_STACKPAGES;i++){
        swapspace_insert(USERSTACK-(i+1)*PAGE_SIZE,as->pid);
    }
}

/*
 * remove all pages of this pid
 */
uint32_t swapspace_remove (int pid) {
	int spl=splhigh();

	int pos=0;
	for(;pos<swapspace_size;pos++){
		if(swapspace[pos].pid==pid){
			swapspace[pos].valid_bit=false;
		}
	}
	splx(spl);
	return 0;
}


struct bitmap *mem_map;

void swap_in(page_table_entry pte) {
	int spl=splhigh();
	struct iovec swap_iov;
	struct uio swap_uio;
	paddr_t paddr=pte.physical_addr;
	int offset,i;
	for(i=0;i<swapspace_size;i++){
		// if(swapspace[i].pid==pte.pid && swapspace[i].virtual_addr==pte.virtual_addr){
		if(swapspace[i].virtual_addr==pte.virtual_addr){
			offset=swapspace[i].physical_addr;
			break;
		}
	}
	// kprintf("Swap :\t In \t V%x P%x O%x\n",pte.virtual_addr,pte.physical_addr,offset);
	KASSERT(i<swapspace_size);
    // kprintf("swap_address: %x\n",paddr);
    uio_kinit(&swap_iov, &swap_uio, (void*)PADDR_TO_KVADDR(paddr & PAGE_FRAME), PAGE_SIZE, offset, UIO_READ);
	splx(spl);

	int result=VOP_READ(swap_file, &swap_uio);
	if(result) {
		panic("VM: SWAP in Failed");
	}
}

void swap_out(page_table_entry pte) {
	int spl=splhigh();

	struct iovec swap_iov;
	struct uio swap_uio;
	paddr_t paddr=pte.physical_addr;
	int offset,i;
	for(i=0;i<swapspace_size;i++){
		if(/*swapspace[i].pid==pte.pid &&*/ swapspace[i].virtual_addr==pte.virtual_addr){
			offset=swapspace[i].physical_addr;
			break;
		}
	}
	// kprintf("Swap :\t Out \t V%x P%x O%x\n",pte.virtual_addr,pte.physical_addr,offset);
	KASSERT(i<swapspace_size);
	uio_kinit(&swap_iov, &swap_uio, (void*)PADDR_TO_KVADDR(paddr & PAGE_FRAME), PAGE_SIZE, offset, UIO_WRITE);
	splx(spl);

int result=VOP_WRITE(swap_file, &swap_uio);
	if(result) {
		panic("VM: SWAP out Failed");
	}
	// tlb_invalidate(paddr);
}

int num_swap_insertions(){
	return swap_insertions;
}
