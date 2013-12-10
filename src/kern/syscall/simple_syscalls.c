/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <thread.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */

int
_helloworld()
{
	char *buf=kmalloc(30*sizeof(char));
	strcpy(buf,"Hello World\n");
	int err=kprintf("%s",buf);
	kfree(buf);
	return err;
}

int
_printint(int n)
{
	int tmp=n;
	int err=kprintf("%d",tmp);
	return err;
}

int
_printstring(char* c,int numchars)
{
    numchars;
	int err=kprintf("%s\n",c);
	return err;
}


int cnt=0;
void *thread_handle(void* ptr, unsigned int nargs){
    int local=++cnt,i;


    int j;
    for(j=0;j<10;j++){
        if(j%local==0 && j%1000==0){
            kprintf("%d",local);
        }
    }

    return NULL;
}

int
_threaddemo()
{
    cnt=0;
    int j;
    kprintf("Demonstrating thread working\n");
    int i=0;
    for(;i<10;i++){
        char name[4];
        name[0]=i+'0';
        name[1]='\0';
        thread_fork(name,thread_handle,NULL,0,NULL);
    }
    return 0;
}



int
_sleepdemo()
{
	kprintf("step1\n");
	clocksleep(1);
	kprintf("step2\n");
}

/*
int 
_write(int filehandle,void *buf, size_t size,int* retval)
{
	if(filehandle < 0 || filehandle > MAX_FILE_FILETAB){
		*retval = -1;
		return EBADF;
	}	
	if(curthread->fdesc[filehandle] == NULL)
	{
		*retval = -1;
		return EBADF;
	}
	if(! (curthread->fdesc[filehandle]->flags != O_WRONLY || curthread->fdesc[filehandle]->flags!=O_RDWR) )
	{
		*retval = -1;
		return EINVAL;
	}
	lock_acquire(curthread->fdesc[filehandle]->f_lock);
       
	struct uio writeuio;
	struct iovec iov;
	size_t size1;
	char *buffer = (char*)kmalloc(size);
	copyinstr((userptr_t)buf,buffer,strlen(buffer),&size1);
	
	uio_kinit(&iov, &writeuio, (void*) buffer, size, curthread->fdesc[filehandle]->offset, UIO_WRITE);
	int result=VOP_WRITE(curthread->fdesc[filehandle]->vnode, &writeuio);
        if (result) {
		kfree(buffer);
		lock_release(curthread->fdesc[filehandle]->f_lock);
		*retval = -1; 
                return result;
        }
	curthread->fdesc[filehandle]->offset = writeuio.uio_offset;
	*retval = size - writeuio.uio_resid;
	kfree(buffer);
	lock_release(curthread->fdesc[filehandle]->f_lock);
        return 0;
}*/
