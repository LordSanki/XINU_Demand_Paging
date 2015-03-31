/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

//LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
  STATWORD ps;
  int pid;
  int bsid;
  struct pentry * pptr;
  struct mblock *first_block;
  disable(ps);
  pid = create(procaddr, ssize, priority, name, nargs, args);
  
  if(isbadpid(pid)){
    kprintf("Unable to create proc\n");
    restore (ps);
    return SYSERR;
  }
  pptr = &proctab[pid];

  ERROR_CHECK2( get_bsm(&bsid), ps );
  
  ERROR_CHECK2( bsm_map(pid, 4096, bsid, hsize), ps);

  pptr->vmemlist.mnext = (struct mblock*) (4096*NBPG);
  first_block = (struct mblock*)BSID2PA(bsid);
  first_block->mnext = 0;
  first_block->mlen = hsize*NBPG;

  restore(ps);
  return OK;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
#if 0
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
#endif

