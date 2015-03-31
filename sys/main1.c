/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>
//#include <bs.h>
//#include <frame.h>
#include <sem.h>

//////////////////////////////////////////////////////////////////////////
//  basic_test ( given code from initial main.c )
//////////////////////////////////////////////////////////////////////////
void basic_test() {

    char *addr = (char*) 0x40000000; //1G
    bsd_t bs = 0;
    int i;
    int vpno = VAD2VPN(addr);	// the ith page

    kprintf("\n\nBasic Xinu Test\n\n");

    get_bs(bs, 50);

    if (xmmap(vpno, bs, 50) == SYSERR) {
    	kprintf("xmmap call failed\n");
    	return;
    }
    for (i = 'A'; i <= 'Z'; i++) 
    {
    	*addr = i;
    	addr += NBPG;	//increment by one page each time
    }

    addr = (char*) 0x40000000; //1G
    for (i = 0; i < 16; i++) 
    {
    	kprintf("0x%08x: %c\n", addr, *addr);
    	addr += NBPG;       //increment by one page each time
    }
    xmunmap(vpno);
}
//////////////////////////////////////////////////////////////////////////
//  random_access (tests replacement policies)
//////////////////////////////////////////////////////////////////////////
void random_access_test() {
    int i, x, rc;
    char *addr = (char*) 0x50000000;
    char *w;
    bsd_t bsid = 7;
    int npages = 40;
    pt_t * pt;
    virt_addr_t * vaddr;
    pd_t *pd;

    kprintf("\nRandom access test\n");
    srand(25); // some random seed


    rc = get_bs(bsid, npages);
    if (rc == SYSERR) {
    	kprintf("get_bs call failed\n");
    	return 0;
    }

    rc = xmmap(VAD2VPN(addr), bsid, npages);
    if (rc == SYSERR) {
    	kprintf("xmmap call failed\n");
    	return 0;
    }


    for (i=0; i<50; i++) {

        // Get a number between 0 and npages
        // This number will be the page offset from the 
        // beginning of the backing store that we will access
        x = ((int) rand()) % npages; 

        // Get the virtual address
        w = addr + x*NBPG;


        *w = 'F';

        vaddr = &w;
        pd = (pd_t*)proctab[currpid].pdbr;
        pt = (pt_t*)VPN2VAD(pd[vaddr->pd_offset].pd_base);
        kprintf("Just accessed vaddr:0x%08x frame:%d bspage:%d\n",
              w,
              FRAME_ID(VPN2VAD(pt[vaddr->pt_offset].pt_base)),
              x);
    }

    xmunmap(VAD2VPN(addr));

    release_bs(bsid);
}

//////////////////////////////////////////////////////////////////////////
//  kill_test
//////////////////////////////////////////////////////////////////////////
void kill_task1() {
    int i,j;
    int rc;
    char * x;
    char *addr = (char*) 0x40000000; //1G
    bsd_t bsid = 5;
    int npages = 5;

    kprintf("kill_task1... start producing.\n");

    get_bs(bsid, npages);

    rc = xmmap(VAD2VPN(addr), bsid, npages);
    if (rc == SYSERR) {
    	kprintf("xmmap call failed\n");
    	return 0;
    }

    kprintf("kill_task1.. sleeping 1\n");
    sleep(1);


    // Produce
    x = addr;
    for (i = 0; i < npages; i++) {
        for (j = 0; j < 5; j++) {
            
            kprintf("write: 0x%08x = %c\n", (x + j), 'A' + j);
            *(x + j) = 'A' + j;

        }
        x += NBPG;	//go to the next page
    }

    kprintf("kill_task1.. sleeping 5\n");
    sleep(5);

    kprintf("kill_task1.. KILLING\n");
    kill(currpid);
    kprintf("THIS SHOULD NOT RUN\n");
}

void kill_task2(int sem) {
    int i,j;
    int rc;
    char * x;
    char *addr = (char*) 0x50000000;
    bsd_t bsid = 5;
    int npages = 5;

    kprintf("kill_task2... start consuming.\n");

    get_bs(bsid, npages);

    rc = xmmap(VAD2VPN(addr), bsid, npages);
    if (rc == SYSERR) {
    	kprintf("xmmap call failed\n");
    	return 0;
    }

    kprintf("kill_task2.. sleeping 5\n");
    sleep(5);

    // Read back all of the values from memory and print them out
    x = addr;
    kprintf("kill_task2 end: ");
    for (i = 0; i < npages; i++) {
        for (j = 0; j < 5; j++) {
            kprintf("%c", *(x + j));
        }
        kprintf(" ");
        x += NBPG;	//go to the next page
    }
    kprintf("\n");

    kprintf("kill_task2.. sleeping 5\n");
    sleep(5);

    // NOTE: This is after other task has been killed

    // Read back all of the values from memory and print them out
    x = addr;
    kprintf("kill_task2 end: ");
    for (i = 0; i < npages; i++) {
        for (j = 0; j < 5; j++) {
            kprintf("%c", *(x + j));
        }
        kprintf(" ");
        x += NBPG;	//go to the next page
    }
    kprintf("\n");

    sleep(1);

    xmunmap(VAD2VPN(addr));

    release_bs(bsid);
}

void kill_test() {
    int p1, p2;

    kprintf("\nKill proc test\n");

    // Create a process that is always ready to run at priority 15
    p1 = create(kill_task1, 2000, 20, "kill_task1", 0, NULL); 
    p2 = create(kill_task2, 2000, 20, "kill_task2", 0, NULL); 

    // Start the task
    kprintf("start producer (pprio 20)\n");
    resume(p1);

    // Start the task
    kprintf("start consumer (pprio 20)\n");
    resume(p2);
    
}

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */
int main() {

//  basic_test();
//  random_access_test();
  kill_test();

  return 0;
}
