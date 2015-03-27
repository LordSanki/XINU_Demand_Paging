#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

  if(npages < 1 || npages > 128) return SYSERR;
  /* requests a new mapping of npages with ID map_id */

  /*If the new backing store cannot be created,
   *  or a backing store with this ID already exists,
   *   the size of the new or existing backing store is returned */

    return npages;
}


