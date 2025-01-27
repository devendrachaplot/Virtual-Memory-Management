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

/*
 * huge.c
 *
 * 	Tests the VM system by accessing a large array (sparse) that
 *	cannot fit into memory.
 *
 * When the VM system assignment is done, your system should be able
 * to run this successfully.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PageSize	4096
#define NumPages	310

int sparse[NumPages][PageSize];	/* use only the first element in the row */

int
main()
{
	int i,j;

	PrintString("Entering the huge program - I will stress test your VM\n",0);

	/* move number in so that sparse[i][0]=i */
	for (i=0; i<NumPages; i++) {
		sparse[i][0]=i;
	}

	PrintString("stage [1] done\n",0);

	/* increment each location 5 times */
	for (j=0; j<5; j++) {
		for (i=0; i<NumPages; i++) {
			sparse[i][0]++;
		};
	}

	PrintString("stage [2] done\n",0);

	/* check if the numbers are sane */
	for (i=NumPages-1; i>=0; i--) {
		if (sparse[i][0]!=i+5) {
			PrintString("BAD NEWS!!! - your VM mechanism has a bug!\n",0);
			exit(1);
		}
	}

	PrintString("You passed!\n",0);

	return 0;
}

