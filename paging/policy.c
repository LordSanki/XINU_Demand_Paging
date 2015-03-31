/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

extern pt_t* global_page_tables[4];
extern int page_replace_policy;
int delete_pt(pt_t *pt);
/*-------------------------------------------------------------------------
 * srpolicy - set page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL srpolicy(int policy)
{
  /* sanity check ! */
  if(policy != LRU && policy != FIFO)
    return SYSERR;
//  kprintf("To be implemented!\n");
  page_replace_policy = policy;
  return OK;
}

/*-------------------------------------------------------------------------
 * grpolicy - get page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL grpolicy()
{
  return page_replace_policy;
}

int create_pd(pd_t **pd)
{
  create_pd_pid(pd, currpid);
}

int create_pd_pid(pd_t **pd, int pid)
{
  int frame_id;
  int i;

  ERROR_CHECK( get_frm(&frame_id) );
  
  frm_tab[frame_id].fr_status = FRM_MAPPED_PD;

  (*pd) = (pd_t*)FRAME_ADDR(frame_id);

  frm_tab[frame_id].fr_pid = pid;
  
  // cleaning the page directory
  bzero((void*)(*pd), NBPG);
  // setting first four page table entries
  for(i=0; i<4; i++){
    (*pd)[i].pd_pres = 1;
    (*pd)[i].pd_write = 1;
    (*pd)[i].pd_base = VAD2VPN(global_page_tables[i]);
  }
  return OK;
}

int delete_pd(pd_t *pd)
{
  int i;
  for(i=0; i<NPTE; i++){
    if(pd[i].pd_pres){
      ERROR_CHECK( delete_pt( (pt_t*)(VPN2VAD(pd[i].pd_base)) ) );
    }
  }
  ERROR_CHECK( free_frm(FRAME_ID(pd)) );
  return OK;
}

int create_pt(pt_t **pt)
{
  int frame_id;
  // getting a free frame id
  ERROR_CHECK( get_frm(&frame_id) );

  frm_tab[frame_id].fr_status = FRM_MAPPED_PT;

  // getting address of frame
  (*pt) = (pt_t*)FRAME_ADDR(frame_id);

  // clearing the page
  bzero((void*)(*pt), NBPG);

  return OK;
}

int delete_pt(pt_t *pt)
{
  ERROR_CHECK( free_frm(FRAME_ID(pt)) );
  return OK;
}


