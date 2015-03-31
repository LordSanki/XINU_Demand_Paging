/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */

SYSCALL pfint()
{
  STATWORD ps;
  int bsid, bspage;
  unsigned long vadd;
  virt_addr_t *pvadd;
  unsigned int frmid;
  struct pentry * pptr;
  pt_t *pt;
  pd_t *pd;
  DBG("ISR\n");
  // Disable interrupts
  disable(ps);

  pptr = &(proctab[currpid]);

  vadd = read_cr2();
  pvadd = (virt_addr_t*)&vadd;
  
  // Update the LRU counters of frames
  //updateLRU();

  ERROR_CHECK3( bsm_lookup(currpid, vadd, &bsid, &bspage), ps, kill(currpid));

  pd = (pd_t*)pptr->pdbr;
  // If the Page Table does not exist create it.
  if(pd[ pvadd->pd_offset ].pd_pres != 1){
    // Create a new page table and fill in its properties in
    // the corresponding page directory entry
    ERROR_CHECK3( create_pt(&pt), ps, kill(currpid) );

    pd[ pvadd->pd_offset ].pd_pres  = 1;
    pd[ pvadd->pd_offset ].pd_write = 1;
    pd[ pvadd->pd_offset ].pd_base  = VAD2VPN(pt);
  }

  // Check to see if the page from the backing store is already in 
  // physical memory (a frame). If not we will have to allocate a 
  // new frame and bring the data in from disk (backing store).
  ERROR_CHECK3( get_frm(&frmid), ps, kill(currpid) );

  frm_tab[frmid].fr_vpno = VAD2VPN(vadd);
  
    // Copy the page from the backing store into the frame
  ERROR_CHECK3( read_bs((void *)FRAME_ADDR(frmid), bsid, bspage), ps, kill(currpid) );

  // Update the page table
  pt[pvadd->pt_offset].pt_pres  = 1;
  pt[pvadd->pt_offset].pt_write = 1;
  pt[pvadd->pt_offset].pt_base  = VAD2VPN(FRAME_ADDR(frmid));

  // reset TLB
  write_cr3(read_cr3());

  restore(ps);
  return OK;
}


