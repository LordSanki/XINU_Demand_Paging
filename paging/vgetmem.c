/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{
  STATWORD ps;
  struct mblock * q;
  struct mblock * p;
  struct mblock * rem;
  struct pentry * pptr;

  if (nbytes == 0)
    return SYSERR;

  disable(ps);
  pptr = &proctab[qpid];
  if (pptr->vmemlist.mnext == NULL) {
    restore(ps);
    return SYSERR;
  }
  nbytes = (unsigned int) roundmb(nbytes);

  for( q = &(pptr->vmemlist), p = q->mnext;
        p != NULL;
        q=p,p=p->mnext)
  {
      if (p->mlen == nbytes) {
        q->mnext = p->mnext;
        restore(ps);
        return (WORD *)p;
      } else if ( p->mlen > nbytes ) {
        rem = (struct mblock *)( (unsigned)p + nbytes );
        q->mnext = rem;
        rem->mnext = p->mnext;
        rem->mlen = p->mlen - nbytes;
        restore(ps);
        return (WORD *)p;
      }
  }

  restore(ps);
  kprintf("ERROR: (vgetmem) out of memory\n");
  return SYSERR;
}


