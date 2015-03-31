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

  disable(ps);
  if( bsm_tab[source].bs_status == BS_UNMAPPED ){
    bsm_tab[source].bs_status = BS_MAPPED_SH;
    bsm_tab[source].bs_ref = 0;
    if(OK !=  bsm_map(currpid, source, virtpage, npages) ){
      bsm_tab[source].bs_status = BS_UNMAPPED;
      restore(ps);
      return SYSERR;
    }
  }
  else{
    ERROR_CHECK2( bsm_map(currpid, source, virtpage, npages), ps );
  }

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
  /* sanity check ! */
  if ( (virtpage < 4096) ){ 
    kprintf("xmummap call error: virtpage (%d) invalid! \n", virtpage);
    return SYSERR;
  }
  
  ERROR_CHECK2( bsm_unmap(currpid, virtpage), ps);

  //kprintf("To be implemented!");
  return OK;
}

