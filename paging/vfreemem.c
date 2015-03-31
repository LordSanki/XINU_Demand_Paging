/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
  struct	mblock	*block;
  unsigned size;
{
  STATWORD ps;
  struct mblock * q;
  struct mblock * p;
  struct pentry * pptr;
  unsigned top;

  if ( (size == 0) || ((unsigned int) block < (unsigned int)(NBPG*4096))){
    kprintf("ERROR: (vfreemem) invalid address or range\n");
    return SYSERR;
  }

  pptr = &proctab[currpid];
  size = (unsigned)roundmb(size);
  disable(ps);
  
  q = &(pptr->vmemlist);
  p = q->mnext;
  while((p != (struct mblock *) NULL) && (p < block)) {
    q=p;
    p=p->mnext;
  }

  top = q->mlen + (unsigned)q;
  if (((q != &(pptr->vmemlist)) && (top > (unsigned)block))
      || ((p != NULL) && ((size + (unsigned)block) > (unsigned)p))) {
    restore(ps);
    return SYSERR;
  }

  if ((q != &(pptr->vmemlist)) && (top == (unsigned)block)) {
    q->mlen += size;
  } else {
    block->mlen = size;
    block->mnext = p;
    q->mnext = block;
    q = block;
  }
  if ((unsigned)(q->mlen + (unsigned)q) == (unsigned)p) {
    q->mlen += p->mlen;
    q->mnext = p->mnext;
  }
  restore(ps);
  return(OK);
}
