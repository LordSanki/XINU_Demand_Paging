#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

  // STATWORD ps;

  // input sanity check
  if( (npages < 1) || (npages > BS_SIZE) || INVALID_BSID(bs_id) )
    return SYSERR;

  /* requests a new mapping of npages with ID map_id */
  if(bsm_tab[bs_id].bs_status == BSM_MAPPED)
    return bsm[bs_id].bs_npages;

  // disable(ps);
  bsm_tab[bs_id].bs_status = BSM_MAPPED;
  bsm_tab[bs_id].bs_npages = npages;
  // restore(ps);
    
  /*If the new backing store cannot be created,
   *  or a backing store with this ID already exists,
   *   the size of the new or existing backing store is returned */
  
  return npages;
}


