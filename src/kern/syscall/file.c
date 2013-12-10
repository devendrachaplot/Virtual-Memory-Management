#include <types.h>
#include <lib.h>
#include <synch.h>
#include <array.h>
#include <vfs.h>
#include <vnode.h>
#include <fs.h>
#include <uio.h>
#include <device.h>
#include <kern/limits.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <thread.h>
#include <current.h>
#include <file.h>
#include <kern/fcntl.h>
#include <copyinout.h>
#include <kern/seek.h>
#include <file.h>
#include <limits.h>
#include <kern/stat.h>
#include <spl.h>
#include <current.h>


int _write(int filehandle,void *buf, size_t size,int* retval){
	
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
}
