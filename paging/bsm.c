/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

bs_map_t bsm_tab[NBS];

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
  int i;
  for(i=0; i<NBS; i++){
    free_bsm(i);
  }
  return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
  int i;
  for(i=0; i<NBS; i++){
    if(bsm_tab[i].bs_status == BSM_UNMAPPED){
      break;
    }
  }
  if(INVALID_BSID(i)){
    kprintf("All bs are already mapped\n");
    return SYSERR;
  }
  *avail = i;
  return OK;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
  clear_bs_map(&(bsm_tab[i]));
  return OK;
}

void clear_bs_map(bs_map_t *map)
{
    map->bs_status = BSM_UNMAPPED;
    map->bs_npages = 0;
    map->bs_ref = 0;
    map->bs_vpno = 0;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
  int bsid;
  int vpno = VAD2VPN(vaddr);
  int start, stop;
  DBG("Looking up vpno %x\t",vpno);
  bs_map_t *bs_map = &(proctab[pid].bs_map[0]);
  for(bsid=0; bsid<NBS; bsid++){
    if(bs_map[bsid].bs_status != BSM_UNMAPPED){
      start = bs_map[bsid].bs_vpno;
      stop = bs_map[bsid].bs_vpno + bs_map[bsid].bs_npages;
      //DBG("BSID %d mapped %x-%x\n",bsid,start,stop);
      if (vpno >= start && vpno < stop)
        break;
    }
    else{
      //DBG("BSID %d unmapped\n",bsid);
    }
  }
  if(INVALID_BSID(bsid)){
    kprintf("vpno %d of proc %s is not mapped to any bs\n"
        ,vpno, proctab[pid].pname);
    return SYSERR;
  }
  *store = bsid;
  *pageth = vpno - bs_map[bsid].bs_vpno;
  DBG("Mapping found BSID %d page %x\n",*store, *pageth);
  return OK;
}

/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
  if( proctab[pid].bs_map[source].bs_status != BSM_UNMAPPED ){
    kprintf("ERROR: (bsm_map) bsid %d already mapped to vpno %d of proc %s \n"
        , source, proctab[pid].bs_map[source].bs_vpno, proctab[pid].pname);
    return SYSERR;
  }
  if(bsm_tab[source].bs_status == BSM_MAPPED_PR){
    kprintf("ERROR: (bsm_map) Trying to map private mem of proc %s\n"
        ,proctab[bsm_tab[source].bs_ref].pname);
    return SYSERR;
  }
  else {
    if(bsm_tab[source].bs_status == BSM_UNMAPPED){
      bsm_tab[source].bs_status = BSM_MAPPED_PR;
      bsm_tab[source].bs_npages = npages;
      bsm_tab[source].bs_ref = pid;
      bsm_tab[source].bs_vpno = vpno;
    }
    else {
      bsm_tab[source].bs_npages = npages;
      bsm_tab[source].bs_ref++;
    }
  }
  DBG("Mapping VPN %x-%x to BS %d\n", vpno, vpno+npages, source);
  proctab[pid].bs_map[source].bs_status = BSM_MAPPED_PR;
  proctab[pid].bs_map[source].bs_npages = npages;
  proctab[pid].bs_map[source].bs_vpno = vpno;
  return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno)
{
  int bsid, page;
  struct pentry *pptr = &(proctab[pid]);
  ERROR_CHECK( bsm_lookup(pid, VPN2VAD(vpno), &bsid, &page) );

  pptr->bs_map[bsid].bs_npages = vpno - pptr->bs_map[bsid].bs_vpno;;
  if(pptr->bs_map[bsid].bs_npages < 1){
    pptr->bs_map[bsid].bs_status = BSM_UNMAPPED;
    if(bsm_tab[bsid].bs_status == BSM_MAPPED_PR){
      free_bsm(bsid);
    }
    else{
      bsm_tab[bsid].bs_ref--;
      if(bsm_tab[bsid].bs_ref == 0)
        free_bsm(bsid);
    }
  }
  return OK;
}



