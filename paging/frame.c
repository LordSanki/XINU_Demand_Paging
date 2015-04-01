/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#define DBG(...)
fr_map_t frm_tab[NFRAMES + 4];
#define FREE_HEAD 1024
#define FREE_TAIL 1025
#define FIFO_HEAD 1026
#define FIFO_TAIL 1027
#define Q_EMPTY(HEAD) (frm_tab[frm_tab[(HEAD)].q.next].q.key == 1)
#define Q_INVALID(X) ( ((X) < 0) && ((X) >= NFRAMES) )

void qpush(int tail, int n);
void qrem(int n);
int qpop(int head);
extern int page_replace_policy;
static int free_frm_num = NFRAMES;
unsigned int findLRU();
void updateLRU();
/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
  int i;
  
  fr_map_t *head = &frm_tab[FREE_HEAD];
  fr_map_t *tail = &frm_tab[FREE_TAIL];

  fr_map_t *fifo_head = &frm_tab[FIFO_HEAD];
  fr_map_t *fifo_tail = &frm_tab[FIFO_TAIL];

  tail->q.key = 1;
  head->q.key = 1;
  head->q.next = FREE_TAIL;
  tail->q.prev = FREE_HEAD;

  fifo_tail->q.key = 1;
  fifo_head->q.key = 1;
  fifo_head->q.next = FIFO_TAIL;
  fifo_tail->q.prev = FIFO_HEAD;

  for(i=0; i<NFRAMES; i++){
    bzero(&(frm_tab[i]), sizeof(fr_map_t));
    qpush(FREE_TAIL, i);
  }
  if(Q_EMPTY(FREE_HEAD))
    kprintf("PANIC_PANIX\n");
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  STATWORD ps;
  disable(ps);
  if( Q_EMPTY(FREE_HEAD) ){
    // free q is empty we need to replace a frame
    if(page_replace_policy == FIFO){
      do{
        if(Q_EMPTY(FIFO_HEAD)){
          kprintf("Out of memory\n");
          return SYSERR;
        }
        *avail = qpop(FIFO_HEAD);
      }while(frm_tab[*avail].fr_status != FRM_MAPPED);
      free_frm(*avail);
    }
    else{
      *avail = findLRU();
      qrem(*avail);
      kprintf("OUT of MEM PANIC!!\n");
    }
  }
  else{
    *avail = qpop(FREE_HEAD);
    if(Q_INVALID(*avail)){
      kprintf("Unable to get a free frame\n");
      restore (ps);
      return SYSERR;
    }
  }
  frm_tab[(*avail)].fr_status = FRM_MAPPED;
  frm_tab[(*avail)].fr_pid = currpid;
  frm_tab[(*avail)].fr_vpno = 0;
  frm_tab[(*avail)].fr_loadtime = 0;
  frm_tab[(*avail)].fr_refcnt = 0;
  qpush(FIFO_TAIL, (*avail));
  restore(ps);
  DBG(" Frames %d/%d\n",--free_frm_num, NFRAMES);
  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
  STATWORD ps;
  DBG("Freeing %d frame ",i);
  disable(ps);
  if(frm_tab[i].fr_status == FRM_UNMAPPED){
    restore(ps);
    return OK;
  }
  else if(frm_tab[i].fr_status == FRM_MAPPED){
    DBG("FRM_MAPPED\n");
    int vadd = VPN2VAD(frm_tab[i].fr_vpno);
    virt_addr_t *pv = (virt_addr_t*)&(vadd);
    pd_t *pd = (pd_t*)(proctab[frm_tab[i].fr_pid].pdbr);

    pt_t *pt = (pt_t*)VPN2VAD(pd[pv->pd_offset].pd_base);
    int bsid, bspage;
    if(OK != bsm_lookup(frm_tab[i].fr_pid, VPN2VAD(frm_tab[i].fr_vpno), &bsid, &bspage))
    {
      kprintf("Unable to bsm find mapping for frame %d\n", i);
      restore (ps);
      return SYSERR;
    }
    pt[pv->pt_offset].pt_pres = 0;
    if(pt[pv->pt_offset].pt_dirty){
      write_bs((char*)FRAME_ADDR(i), bsid, bspage);
      pt[pv->pt_offset].pt_dirty = 0;
    }
    if(--(frm_tab[FRAME_ID(pt)].fr_refcnt) == 0){
      free_frm(FRAME_ID(pt));
    }
  }
  else if(frm_tab[i].fr_status == FRM_MAPPED_PT){
    int vadd = VPN2VAD(frm_tab[i].fr_vpno);
    virt_addr_t *pv = (virt_addr_t*)&(vadd);
    pd_t *pd = (pd_t*)(proctab[frm_tab[i].fr_pid].pdbr);
    DBG("FRM_MAPPED_PT Proc %d vpno %x\n", frm_tab[i].fr_pid,frm_tab[i].fr_vpno );
    pd[pv->pd_offset].pd_pres = 0;
  }
  else if(frm_tab[i].fr_status == FRM_MAPPED_PD){
  }
  frm_tab[i].fr_status = FRM_UNMAPPED;
  qrem(i);
  qpush(FREE_TAIL, i);
  write_cr3(read_cr3());
  restore(ps);
  DBG("Frame %d freed\n",i);
  DBG(" Frames %d/%d\n",++free_frm_num, NFRAMES);
  return OK;
}

int qpop(int head)
{
  if(Q_EMPTY(head)) return -1;
  
  int n = frm_tab[head].q.next;
  frm_tab[head].q.next = frm_tab[n].q.next;
  frm_tab[frm_tab[head].q.next].q.prev = FREE_HEAD;
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

void updateLRU()
{
  static int timeCount = 0;
  pd_t *pd;
  pt_t *pt;
  int vadd;
  virt_addr_t *pv;
  timeCount++;
  fr_map_t *head = &frm_tab[FIFO_HEAD];
  fr_map_t *tail = &frm_tab[FIFO_TAIL];
  fr_map_t *next = &frm_tab[head->q.next];
  while(next != tail){
    if(next->fr_status == FRM_MAPPED){
      vadd = VPN2VAD(next->fr_vpno);
      pv = (virt_addr_t*)vadd;
      pd = (pd_t*)proctab[next->fr_pid].pdbr;
      pt = VPN2VAD(pd[pv->pd_offset].pd_base);
      if(pt[pv->pt_offset].pt_acc == 1){
        next->fr_loadtime = timeCount;
        pt[pv->pt_offset].pt_acc = 0;
      }
    }
    next = &frm_tab[next->q.next];
  }
}

unsigned int findLRU()
{
  fr_map_t *id;
  fr_map_t *head = &frm_tab[FIFO_HEAD];
  fr_map_t *tail = &frm_tab[FIFO_TAIL];
  fr_map_t *next = &frm_tab[head->q.next];
  if(next == tail) return 1023;
  id = next;
  while(next != tail){
    if(next->fr_status == FRM_MAPPED){
      if(id->fr_loadtime > next->fr_loadtime)
        id = next;
    }
  }
  head = &frm_tab[0];
  return ((unsigned int)(id-head))/sizeof(fr_map_t);
}

int write_back_frames(int pid, int bsid)
{
  int i;
  for(i=0; i<NFRAMES; i++)
  {
    if((frm_tab[i].fr_status == FRM_MAPPED)
      &&(frm_tab[i].fr_pid == pid)
      &&(frm_tab[i].fr_bsid == bsid))
    {
      free_frm(i);      
    }
  }
  return OK;
}

