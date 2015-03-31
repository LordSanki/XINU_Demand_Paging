/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;
  int i;
	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}

  // releasing all frames held
  for(i=0; i<NFRAMES; i++){
    if(frm_tab[i].fr_pid == pid){
      free_frm(i);
    }
  }

  // remove BS mappings
  for(i=0; i<NBS; i++){
    if(pptr->bs_map[i].bs_status == BSM_MAPPED_SH){
      if( --bsm_tab[i].bs_ref == 0){
        free_bsm(i);
      }
    }
    if(pptr->bs_map[i].bs_status == BSM_MAPPED_PR){
        free_bsm(i);
    }
  }
  
  // removing PD and PT
  delete_pd((pd_t*)pptr->pdbr);

	restore(ps);
	return(OK);
}
