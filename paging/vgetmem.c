/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>
//#define DBG kprintf
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
    return (WORD*)SYSERR;

  //disable(ps);
  pptr = &proctab[currpid];
  if (pptr->vmemlist.mnext == NULL) {
    //restore(ps);
    return (WORD*)SYSERR;
  }
  nbytes = (unsigned int) roundmb(nbytes);
  //DBG("Trying to allocate %d bytes\n",nbytes);
  q = &(pptr->vmemlist);
  p = q->mnext;
  rem = (struct mblock*)0x880000;
  DBG("FIrst block is at %x of size %d | %d\n",(unsigned int)p, p->mlen, rem->mlen);
  //print_PA((unsigned int)p);
  while(p != 0)
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
      q=p; 
      p=p->mnext;
  }

  //restore(ps);
  kprintf("ERROR: (vgetmem) out of memory\n");
  return (WORD*)SYSERR;
}


