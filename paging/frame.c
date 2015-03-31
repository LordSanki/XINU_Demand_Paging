/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

fr_map_t frm_tab[NFRAMES + 4];
#define FREE_FRM_HEAD 1024
#define FREE_FRM_TAIL 1025
#define MAP_HEAD 1026
#define MAP_TAIL 1027
#define Q_EMPTY(HEAD) (frm_tab[frm_tab[(HEAD)].q.next].q.key != 1)
#define Q_INVALID(X) ( ((X) < 0) && ((X) >= NFRAMES) )

pt_t* find_page_entry(int pid, int vpno);
pt_t* find_page_table(int pid, int vpno);
void qpush(int tail, int n);
void qrem(int n);
int qpop(int head);
/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
  int i;
  
  fr_map_t *head = &frm_tab[FREE_FRM_HEAD];
  fr_map_t *tail = &frm_tab[FREE_FRM_TAIL];

  tail->q.key = 1;
  head->q.key = 1;
  head->q.next = FREE_FRM_TAIL;
  tail->q.prev = FREE_FRM_HEAD;
#if 0
  bzero(&(frm_tab[0]), sizeof(fr_map_t));
  bzero(&(frm_tab[NFRAMES-1]), sizeof(fr_map_t));
  head->q.next = 0;
  frm_tab[0].q.prev = FREE_FRM_HEAD;
  frm_tab[0].q.next = 1;
  tail->q.prev = NFRAMES-1;
  frm_tab[NFRAMES-1].q.prev = NFRAMES-2;
  frm_tab[NFRAMES-1].q.next = FREE_FRM_TAIL;

  for(i=1; i<NFRAMES-1; i++){
    bzero(&(frm_tab[i]), sizeof(fr_map_t));
    frm_tab[i].q.prev = i-1;
    frm_tab[i].q.next = i+1;
  }
#endif
  for(i=0; i<NFRAMES; i++){
    bzero(&(frm_tab[i]), sizeof(fr_map_t));
    qpush(FREE_FRM_TAIL, i);
//    frm_tab[i].q.prev = i-1;
//    frm_tab[i].q.next = i+1;
  }
  if(Q_EMPTY(FREE_FRM_HEAD))
    kprintf("PANIC_PANIX\n");
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  if( Q_EMPTY(FREE_FRM_HEAD) ){
    // free q is empty we need to replace a frame
    kprintf("OUT of MEM PANIC!!\n");
  }
  else{
    *avail = qpop(FREE_FRM_HEAD);
    if(Q_INVALID(*avail)){
      kprintf("Unable to get a free frame\n");
      return SYSERR;
    }
  }
  frm_tab[(*avail)].fr_status = FRM_MAPPED;
  frm_tab[(*avail)].fr_pid = currpid;
  frm_tab[(*avail)].fr_vpno = 0;
  frm_tab[(*avail)].fr_loadtime = 0;

  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
  pt_t * pt = find_page_entry(frm_tab[i].fr_pid, frm_tab[i].fr_vpno);
  int bsid, bspage;
  ERROR_CHECK( bsm_lookup(frm_tab[i].fr_pid, VPN2VAD(frm_tab[i].fr_vpno), &bsid, &bspage) );
  pt->pt_pres = 0;
  if(pt->pt_dirty){
    write_bs((char*)FRAME_ADDR(i), bsid, bspage);
    pt->pt_dirty = 0;
  }
  qrem(i);
  qpush(FREE_FRM_TAIL, i);
  return OK;
}

int qpop(int head)
{
  if(Q_EMPTY(head)) return -1;
  
  int n = frm_tab[head].q.next;
  frm_tab[head].q.next = frm_tab[n].q.next;
  frm_tab[frm_tab[head].q.next].q.prev = FREE_FRM_HEAD;
  frm_tab[n].q.prev = frm_tab[n].q.next = n;
  return n;
}

void qpush(int tail, int n)
{
  int prev = frm_tab[ tail ].q.prev;
  frm_tab[ n ].q.prev = prev;
  frm_tab[ prev ].q.next = n;
  frm_tab[ tail ].q.prev = n;
  frm_tab[ n ].q.next = tail;
}

void qrem(int n)
{
  int prev = frm_tab[n].q.prev;
  int next = frm_tab[n].q.next;
  if(prev == next)
    return;
  frm_tab[prev].q.next = next;
  frm_tab[next].q.prev = prev;
  frm_tab[n].q.prev = frm_tab[n].q.next = n;
}

pt_t* find_page_entry(int pid, int vpno){
  int vadd = VPN2VAD(vpno);
  virt_addr_t *pvadd = (virt_addr_t*)&vadd;
  pd_t *pd = (pd_t*)proctab[pid].pdbr;
  pt_t *pt = (pt_t*)VPN2VAD(pd[pvadd->pd_offset].pd_base);
  return &pt[pvadd->pt_offset];
}

pt_t* find_page_table(int pid, int vpno){
  int vadd = VPN2VAD(vpno);
  virt_addr_t *pvadd = (virt_addr_t*)&vadd;
  pd_t *pd = (pd_t*)proctab[pid].pdbr;
  pt_t *pt = (pt_t*)VPN2VAD(pd[pvadd->pd_offset].pd_base);
  return pt;
}

