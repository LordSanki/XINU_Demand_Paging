/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#define DBG(...)
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
  unsigned int top;
  top = (unsigned int) block;
  pptr = &proctab[currpid];
  if ( (size == 0) || (size > pptr->vhpnpages*NBPG) 
      || ( top < pptr->vhpno*NBPG)
      || ( top > (pptr->vhpno*NBPG + pptr->vhpnpages*NBPG))
      || ( (top + size) > (pptr->vhpno*NBPG + pptr->vhpnpages*NBPG))
     ){
    kprintf("ERROR: (vfreemem) invalid address or range\n");
    return SYSERR;
  }

  size = (unsigned)roundmb(size);
  disable(ps);
  
  q = &(pptr->vmemlist);
  p = q->mnext;
  DBG("First block at %x\n",(unsigned int)p);
  DBG("Freeing mem at %x of size %d\n", (unsigned int)block, size);

  if(p == NULL) // entire heap was taken
  {
    q->mnext = block;
    p = q->mnext;
    p->mlen = size;
    restore (ps);
    return OK;
  }
  while((p != (struct mblock *) NULL) && (p < block)) {
    q=p;
    p=p->mnext;
  }

  top = q->mlen + (unsigned)q;
  if( (q != &(pptr->vmemlist)) && (top > (unsigned)block) )
  {
    restore(ps);
    return SYSERR;
  }
  if( (p != NULL) && ((size + (unsigned)block) > (unsigned)p) )
  {
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
