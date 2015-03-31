/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */

SYSCALL pfint()
{
  int bsid, bs_page;
  unsigned long vadd;
  virt_addr_t *pvadd;
  unsigned int frmid;
  frame_t * ptframe;
  unsigned long cr2;
  struct pentry * pptr;
  pt_t *pt;

  // Disable interrupts
  disable(ps);

  pptr = &(proctab[currpid]);

  vadd = read_cr2();
  pvadd = (virt_addr_t*)&vadd;
  
  // Update the LRU counters of frames
  //updateLRU();

  ERROR_CHECK3( bsm_lookup(currpid, vadd, &bsid, &bs_page), ps, kill(currpid));

  // If the Page Table does not exist create it.
  if(pptr->pdbr[ pvadd->pd_offset ].pd_pres != 1){
    // Create a new page table and fill in its properties in
    // the corresponding page directory entry
    ERROR_CHECK3( create_pt(&pt), ps, kill(currpid) );

    pptr->pdbr[ pvadd->pd_offset ].pt_pres  = 1;
    pptr->pdbr[ pvadd->pd_offset ].pt_write = 1;
    pptr->pdbr[ pvadd->pd_offset ].pt_base  = VAD2VPN(pt);
  }

  // Check to see if the page from the backing store is already in 
  // physical memory (a frame). If not we will have to allocate a 
  // new frame and bring the data in from disk (backing store).
  ERROR_CHECK3( get_frame(&frmid), ps, kill(currpid) );

//  frm_tab[frmid].fr_status = FR_MAPPED;
//  frm_tab[frmid].fr_pid = currpid;
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


