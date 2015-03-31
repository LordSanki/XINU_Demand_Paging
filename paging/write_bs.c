#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <mark.h>
#include <bufpool.h>
#include <paging.h>

SYSCALL write_bs(char *src, bsd_t bs_id, int page) {

  /* write one page of data from src
     to the backing store bs_id, page
     page.
  */

  void * phy_addr = BACKING_STORE_BASE + bs_id*BACKING_STORE_UNIT_SIZE + page*NBPG;
  if( INVALID_BSID(bs_id) )
    return SYSERR;

  bcopy((void*)src, phy_addr, NBPG);
  return OK;
}

