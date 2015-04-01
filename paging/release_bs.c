#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {

  /* release the backing store with ID bs_id */
  if( INVALID_BSID(bs_id) )
    return SYSERR;

  free_bsm(bs_id);
  proctab[currpid].bs_map[bs_id].bs_status = BSM_UNMAPPED;
  return OK;
}

