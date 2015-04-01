/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  /* sanity check ! */
  STATWORD ps;
  if ( (virtpage < 4096) || INVALID_BSID(source) ||(npages < 1) || ( npages > 128)){
    kprintf("xmmap call error: parameter error! \n");
    return SYSERR;
  }
  if(bsm_tab[source].bs_status == BSM_MAPPED_PR){
    kprintf("Error: Trying to map private heap\n");
    return SYSERR;
  }
  disable(ps);
  ERROR_CHECK2( bsm_map(currpid, virtpage, source, npages), ps );
  
  restore(ps);
  return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage )
{
  STATWORD ps;
  int bsid, page;
  /* sanity check ! */
  if ( (virtpage < 4096) ){ 
    kprintf("xmummap call error: virtpage (%d) invalid! \n", virtpage);
    return SYSERR;
  }
  if(OK == bsm_lookup(currpid, virtpage, &bsid, &page)){
    write_back_frames(currpid, bsid);
  }
  ERROR_CHECK2( bsm_unmap(currpid, virtpage), ps);
  return OK;
}

