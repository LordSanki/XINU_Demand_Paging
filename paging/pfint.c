/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */

void self_test(int vadd, int pid);
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
  // Disable interrupts
  disable(ps);

 // char *addr = (char*)0x00800000;
 // *addr = 'A';

  pptr = &(proctab[currpid]);

  vadd = read_cr2();
  pvadd = (virt_addr_t*)&vadd;
  
  // Update the LRU counters of frames
  //updateLRU();

  ERROR_CHECK3( bsm_lookup(currpid, vadd, &bsid, &bspage), ps, kill(currpid));

  pd = (pd_t*)pptr->pdbr;
  DBG("PD at %x\n",(unsigned int)pd);
  // If the Page Table does not exist create it.
  if(pd[ pvadd->pd_offset ].pd_pres != 1){
    // Create a new page table and fill in its properties in
    // the corresponding page directory entry
    ERROR_CHECK3( create_pt(&pt), ps, kill(currpid) );
    DBG("Creating PT %d at %x\n",pvadd->pd_offset, (unsigned int)pt);

    pd[ pvadd->pd_offset ].pd_pres  = 1;
    pd[ pvadd->pd_offset ].pd_write = 1;
    pd[ pvadd->pd_offset ].pd_base  = VAD2VPN(pt);
  }

  DBG("PT at %x\n",(unsigned int)pt);
  if(pt[pvadd->pt_offset].pt_pres != 1) {
    // Check to see if the page from the backing store is already in 
    // physical memory (a frame). If not we will have to allocate a 
    // new frame and bring the data in from disk (backing store).
    ERROR_CHECK3( get_frm(&frmid), ps, kill(currpid) );
    DBG("Mapping FRM %d(%x) to VPN %x\n", frmid, FRAME_ADDR(frmid), VAD2VPN(vadd));
    frm_tab[frmid].fr_vpno = VAD2VPN(vadd);
    frm_tab[FRAME_ID(pt)].fr_refcnt++;

    // Copy the page from the backing store into the frame
    ERROR_CHECK3( read_bs((void *)FRAME_ADDR(frmid), bsid, bspage), ps, kill(currpid) );

    // Update the page table
    pt[pvadd->pt_offset].pt_pres  = 1;
    pt[pvadd->pt_offset].pt_write = 1;
    pt[pvadd->pt_offset].pt_base  = VAD2VPN(FRAME_ADDR(frmid));
  }
  // reset TLB
  write_cr3(pptr->pdbr);
  //self_test(vadd, currpid);
  //while(1);
  restore(ps);
  return OK;
}

void self_test(int vadd, int pid)
{
  struct pentry *pptr = &proctab[pid];
  virt_addr_t *pv = (virt_addr_t*)&vadd;
  pt_t *pt;
  pd_t *pd;
  char  *page;
  pd = (pd_t*)pptr->pdbr;
  if(pd == NULL) {DBG("pd");goto error;}

  if(pd[pv->pd_offset].pd_pres == 0) {DBG("pd_pres");goto error;}

  pt = (pt_t*)VPN2VAD(pd[pv->pd_offset].pd_base);
  if(pt == NULL) {DBG("pt");goto error;}
  if(pt[pv->pt_offset].pt_pres == 0) {DBG("pt_pres");goto error;}
  kprintf( "Page at %x\n", VPN2VAD(pt[pv->pt_offset].pt_base) );
  page = (char*)VPN2VAD(pt[pv->pt_offset].pt_base);
  page = &page[pv->pg_offset];
  kprintf(" Value is %x->%c \n", (unsigned int)page, page[0]);
  return;
  
error:
    kprintf("TEst failed\n");
}



