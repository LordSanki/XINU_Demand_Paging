/* paging.h */
#ifndef __PAGING_H__
#define __PAGING_H__

typedef unsigned int	 bsd_t;

/* Structure for a page directory entry */

typedef struct {

  unsigned int pd_pres	: 1;		/* page table present?		*/
  unsigned int pd_write : 1;		/* page is writable?		*/
  unsigned int pd_user	: 1;		/* is use level protection?	*/
  unsigned int pd_pwt	: 1;		/* write through cachine for pt?*/
  unsigned int pd_pcd	: 1;		/* cache disable for this pt?	*/
  unsigned int pd_acc	: 1;		/* page table was accessed?	*/
  unsigned int pd_mbz	: 1;		/* must be zero			*/
  unsigned int pd_fmb	: 1;		/* four MB pages?		*/
  unsigned int pd_global: 1;		/* global (ignored)		*/
  unsigned int pd_avail : 3;		/* for programmer's use		*/
  unsigned int pd_base	: 20;		/* location of page table?	*/
} pd_t;

/* Structure for a page table entry */

typedef struct {

  unsigned int pt_pres	: 1;		/* page is present?		*/
  unsigned int pt_write : 1;		/* page is writable?		*/
  unsigned int pt_user	: 1;		/* is use level protection?	*/
  unsigned int pt_pwt	: 1;		/* write through for this page? */
  unsigned int pt_pcd	: 1;		/* cache disable for this page? */
  unsigned int pt_acc	: 1;		/* page was accessed?		*/
  unsigned int pt_dirty : 1;		/* page was written?		*/
  unsigned int pt_mbz	: 1;		/* must be zero			*/
  unsigned int pt_global: 1;		/* should be zero in 586	*/
  unsigned int pt_avail : 3;		/* for programmer's use		*/
  unsigned int pt_base	: 20;		/* location of page?		*/
} pt_t;

typedef struct{
  unsigned int pg_offset : 12;		/* page offset			*/
  unsigned int pt_offset : 10;		/* page table offset		*/
  unsigned int pd_offset : 10;		/* page directory offset	*/
} virt_addr_t;


typedef struct{
  unsigned int bs_status : 2;			/* MAPPED_SH or UNMAPPED	or MAPPED_PR	*/
  unsigned int bs_npages : 15;			/* number of pages in the store */
  unsigned int bs_ref : 15;				/* process id using this slot   */
  unsigned int bs_vpno : 32;				/* starting virtual page number */
  //int bs_sem;				/* semaphore mechanism ?	*/
} bs_map_t;

typedef struct{
  unsigned int prev : 15;
  unsigned int next : 15;
  unsigned int key  : 2;
}frm_queue_t;

typedef struct{
  int fr_status;			/* MAPPED or UNMAPPED	or PT	*/
  int fr_pid;				/* process id using this frame  */
  int fr_vpno;				/* corresponding virtual page no*/
  int fr_refcnt;			/* reference count		*/
  int fr_bsid;
  //int fr_type;				/* FR_DIR, FR_TBL, FR_PAGE	*/
  //int fr_dirty;
  //void *cookie;				/* private data structure	*/
  unsigned long int fr_loadtime;	/* when the page is loaded 	*/
  frm_queue_t q; /* structure to support queues*/
}fr_map_t;

extern bs_map_t bsm_tab[];
extern fr_map_t frm_tab[];
/* Prototypes for required API calls */
SYSCALL xmmap(int, bsd_t, int);
SYSCALL xunmap(int);

/* given calls for dealing with backing store */

int get_bs(bsd_t, unsigned int);
SYSCALL release_bs(bsd_t);
SYSCALL read_bs(char *, bsd_t, int);
SYSCALL write_bs(char *, bsd_t, int);
void clear_bs_map(bs_map_t *map);

SYSCALL get_bsm(int* avail);
SYSCALL free_bsm(int i);
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth);
SYSCALL bsm_map(int pid, int vpno, int source, int npages);
SYSCALL bsm_unmap(int pid, int vpno);
SYSCALL free_frm(int i);
SYSCALL get_frm(int* avail);
SYSCALL init_frm();
int create_pd(pd_t **pd);
int delete_pd(pd_t *pd);
int create_pt(pt_t **pt);
#define NBPG		4096	/* number of bytes per page	*/
#define FRAME0		1024	/* zero-th frame */
#define NPTE 1024 /* Num PT Entries = 2^pt_offset */

#define FRAME_BASE (FRAME0*NBPG)

#define FRAME_ADDR(ID) ( FRAME_BASE + (((unsigned int)ID)*NBPG) )
#define FRAME_ID(ADDR) ( (((unsigned int)ADDR)-FRAME_BASE)/NBPG )

//default 3072 frames --> 1024+3072=4096=16M
//#define NFRAMES 	3072	/* number of frames		*/
#define NFRAMES 	1024	/* number of frames		*/

#define BSM_UNMAPPED	0
#define BSM_MAPPED_PR	1
#define BSM_MAPPED_SH	2

#define FRM_UNMAPPED	0
#define FRM_MAPPED	1
#define FRM_MAPPED_PT	2
#define FRM_MAPPED_PD	3

#define FIFO		3
#define LRU		4

#define MAX_ID          15              /* You get 10 mappings, 0 - 9 */

#define BACKING_STORE_BASE	0x00800000
#define BACKING_STORE_UNIT_SIZE 0x00080000

#define NBS 16

#define BS_SIZE 128 // in pages

#define INVALID_BSID(ID) ( ((ID) < 0) || ((ID) > MAX_ID) )

#define ERROR_CHECK(X) if(OK != X) { kprintf("Error Calling %s \n",#X ); return SYSERR;}
#define ERROR_CHECK2(X,P) { if(OK != X) { kprintf("Error Calling %s \n",#X ); restore(P); return SYSERR;} }
#define ERROR_CHECK3(X,P,F){ if(OK != X) { kprintf("Error Calling %s \n",#X ); restore(P); F; return SYSERR;} }
// 32 - pd_base = 12 hence we shift pd base by 12 bits and write to cr3
#define SET_PDBR(X) (write_cr3((unsigned long)X))

#define VAD2VPN(X) (((unsigned long)X)>>12)
#define VPN2VAD(X) (((unsigned long)X)<<12)

#define BSID2PA(ID) (BACKING_STORE_BASE + ((unsigned int)(ID))*BACKING_STORE_UNIT_SIZE)

#define DBG(...)
//#define DBG kprintf

#endif // __PAGING_H__


