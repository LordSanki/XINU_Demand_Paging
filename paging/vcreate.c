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

  //ERROR_CHECK( get_bsm(&bsid) );
  if( OK != get_bsm(&bsid)){
    DBG("Unable to get bsm for %d killing it\n", pid);
    kill(pid);
    restore(ps);
    return SYSERR;
  }

  
  //ERROR_CHECK( bsm_map(pid, 4096, bsid, hsize));
  if( OK != bsm_map(pid, 4096, bsid, hsize)){
    DBG("Unable to get bsm for %d killing it\n", pid);
    kill(pid);
    restore(ps);
    return SYSERR;
  }

  pptr->vmemlist.mnext = (struct mblock*) (4096*NBPG);
  pptr->vhpno = 4096;
  pptr->vhpnpages = 2;
  pptr->store = bsid;
  first_block = (struct mblock*)BSID2PA(bsid);
  first_block->mnext = 0;
  first_block->mlen = hsize*NBPG;
  DBG("Creating Proc %s with pid %d and heap %d bytes at %x\n",proctab[pid].pname, pid, first_block->mlen, (unsigned int)first_block);
  restore(ps);
  return pid;
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

