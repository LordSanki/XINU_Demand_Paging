#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>


#define PROC1_VADDR     0x40000000
#define PROC1_VPNO      0x40000
#define PROC2_VADDR     0x80000000
#define PROC2_VPNO      0x80000
#define TEST1_BS        1
#define ASSERT(X) {if((X)==0)kprintf("Assertion failed %s\n",#X);}
void proc1_test1(char *msg ) {
  char *addr;
  int i;

  get_bs(TEST1_BS, 100);

  if (xmmap(PROC1_VPNO, TEST1_BS, 100) == SYSERR) {
    kprintf("xmmap call failed\n");
    sleep(3);
    return;
  }

  addr = (char*) PROC1_VADDR;
  for (i = 0; i < 26; i++) {
	  register char a;
    *(addr + i * NBPG) = a = 'A' + i;
    kprintf("%s Writing 0x%x: %c a=%c\n", msg, (addr + i * NBPG), *(addr + i * NBPG),a);
  }

  sleep(1);

  for (i = 25; i >=0; i--) {
    kprintf("%s 0x%08x: %c\n",msg, addr + i * NBPG, *(addr + i * NBPG));
  }

  xmunmap(PROC1_VPNO);
  sleep(10);
  release_bs(TEST1_BS);
  return;
}

void proc2_test1(char *msg, int a, int b, char *string) {
  char *addr;
  int i;

  get_bs(TEST1_BS, 100);

  if (xmmap(PROC2_VPNO, TEST1_BS, 100) == SYSERR) {
    kprintf("xmmap call failed\n");
    sleep(3);
    return;
  }

  addr = (char*) PROC2_VADDR;
  for (i = 0; i < 26; i++) {
    kprintf("%s 0x%08x: %c\n",msg, addr + i * NBPG, *(addr + i * NBPG));
  }

  xmunmap(PROC2_VPNO);
  release_bs(TEST1_BS);
  kprintf("Multi arg check: %d %d %s \n",a,b,string);
  return;
}

void proc1_test2(char *msg, int lck) {
  int *x;

  kprintf("ready to allocate heap space\n");
  x = vgetmem(1024);
  kprintf("heap allocated at %x\n", x);
  *x = 100;
  *(x + 1) = 200;

  kprintf("heap variable: %d %d\n", *x, *(x + 1));
  vfreemem(x, 1024);
}

void proc1_test3(char *msg, int lck) {

  char *addr;
  int i;

  addr = (char*) 0x0;  // The last address = 0x3fffff

  for (i = 0; i < 1024; i++) {
	  kprintf("Trying to write to %x\n", (addr + i * NBPG) );
    *(addr + i * NBPG) = 'B';
  }

  for (i = 0; i < 1024; i++) {
    kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
  }

  return;
}

void memTest(char *msg, int a, int b, char *string){
	int i,sum,exp;
	int *x,*y;

	kprintf("Let's check one allocation\n");
	x = vgetmem(1);
	if(x != SYSERR) {
		kprintf("heap allocated at %x =  %d -before\n", x, *x);
		*x = 100;
		kprintf("heap allocated at %x =  %d -after\n", x, *x);
		vfreemem(x,1);
	}


	kprintf("\nLet's check one page allocation.\n");
	x = vgetmem(NBPG);
	if(x != SYSERR) {
		kprintf("heap allocated at %x\n", x);
		for(i=0,sum=0,exp=0; i<1024; i++) { x[i] = i; sum+=x[i]; exp+=i; }
		if(sum != exp) { kprintf("Sum mismatch. Test FAIL\n"); }
		else kprintf("Initial sum = %d, exp = %d. Test PASS\n",sum,exp);
		vfreemem(x,NBPG);
	}

	kprintf("\nLet's check 2 page allocation with non-multiple size.\n");
	x = vgetmem(1000*4);
	y = vgetmem(1000*4);
	if(x != SYSERR && y!=SYSERR) {
		kprintf("heap allocated at %x and %x\n", x, y);
		for(i=0,sum=0,exp=0; i<1000; i++) { x[i] = i; y[i]=i*2; sum+=x[i]+y[i]; exp+=i*3; }
		if(sum != exp) { kprintf("Sum mismatch. Test FAIL\n"); }
		else kprintf("Initial sum = %d, exp = %d. Test PASS\n",sum,exp);
	}
	if(x != SYSERR) {  vfreemem(x,1000*4); }
	if(y != SYSERR) {  vfreemem(y,1000*4); }

	kprintf("\nLet's check full allocation.\n");
	x = vgetmem(2*NBPG);
	if(x != SYSERR) {
		kprintf("heap allocated at %x\n", x);
		for(i=0,sum=0,exp=0; i<(2*NBPG)/4; i++) { x[i] = i; sum+=x[i]; exp+=i; }
		if(sum != exp) { kprintf("Sum mismatch. Test FAIL\n"); }
		else kprintf("Initial sum = %d, exp = %d. Test PASS\n",sum,exp);
		if(vfreemem(x,8193) == SYSERR){
			vfreemem(x,2*NBPG);
			kprintf("Doing illegal free check: Test PASS\n");
		} else {
			kprintf("Too many data elements freed: Test FAIL\n");
		}
	}

	kprintf("\nLet's ask for extra space. Check for SYSERR\n");
	x = vgetmem(8193);
	if(x != SYSERR) {
		kprintf("ERROR: Too many data elements mapped\n");
		kprintf("heap allocated at %x\n", x);
		vfreemem(x,8193);
	} else {
		kprintf("Warning of Insufficient space should have printed. Test PASS\n");
	}
	kprintf("\nMulti arg check: %d %d %s \n",a,b,string);
}

void pageLoader(char *msg, u_long vpage){
	int i,sum,j,inc;
	int *x;
	get_bs(TEST1_BS, 100);
	if (xmmap(vpage, TEST1_BS, 100) == SYSERR) {
		kprintf("xmmap call failed\n");
		sleep(3);
		return;
	}
	for(i=0,sum=1,j=0,inc=-1;i<100;i++) {
		if(i%4==0) inc = -inc;
		j+=inc;
		x = (int *) ((vpage<<12) + (4+j)*NBPG);
		*x=i;
		sum += *x;
		kprintf("%s: Writing to @[0x%08x] (j=%d) (inc=%d) = %d\n",msg,x,j,inc,*x);
		sleep(1);
	}
	kprintf("%s: Final sum = %d\n",msg,sum);
	xmunmap(vpage);
	release_bs(TEST1_BS);
}

void policyTest(){
	int pid1,pid2;
	pid1 = create(pageLoader, 2000, 20, "Pol1", 2, "Pol1",8192);
	if(pid1>0) { kprintf("Created process %s=%d\n","Pol1",pid1); }
	pid2 = create(pageLoader, 2000, 20, "Pol2", 2, "Pol2",0x80000);
	if(pid2>0) { kprintf("Created process %s=%d\n","Pol2",pid2); }
	resume(pid1);
	resume(pid2);
}

void simpleRecursion(int i) {
  int j;
  for(j=0; j<i*10000; j++);
//	if(i==0) return 1;
//	else return i * simpleRecursion(i-1);
  kprintf("done %d\n",currpid);
}

void privateHeapLoadTest(){
	int i,pid;
	for(i=1; i<40;i++) {
		while(SYSERR == (pid = vcreate(simpleRecursion, 2000, 2, 20, "simpleRec", 1,i)) ) {
			kprintf("Failed to create process at i=%d. Waiting for 10 more sleep cycles..\n",i);
			sleep(10);
		}
		resume(pid);
	}
	kprintf("heap load test PASS.\n");
	return;
}

#define PEAK_VPNO      0xFFF80

void peakLoadTest(int pIdx){
	char *x,*y;
	int i,mypno,m;
	x = vgetmem(128*NBPG);
	mypno = PEAK_VPNO - pIdx;
	get_bs(15,128);
	xmmap(mypno,15,128);
	for(i=0; i<128; i++) {
		x[i] = (char) (pIdx + i);
		x[i+NBPG-1] = (char) (pIdx*2 +i);
		if(i%15 == pIdx && i!=127) {
			*(char *)( (mypno + i)*NBPG ) = (char) (pIdx+3);
			*(char *)( (mypno + i)*NBPG + NBPG -1) = (char) (pIdx+4);
		}
	}
	xmunmap(mypno);

	kprintf("Writing part of test done Now will remap to new address and test. pIdx=%d currpid=%d\n",pIdx,currpid);
	sleep(20);
	mypno--;
	xmmap(mypno,15,128);
	y= (char *)((mypno)*NBPG );
	for(i=0; i<128; i++){
		if(x[i] != (char) (pIdx + i) ) kprintf("pIdx=%d Test Fail. x[%d] did not match\n",pIdx, i);
		if(x[i+NBPG-1] != (char) (pIdx*2 + i) ) kprintf("pIdx=%d Test Fail. x[%d] did not match\n",pIdx, i+NBPG-1);
		if(i!=127) {
			m=i%15;
			if( y[i*NBPG] != (char) (m+3) ) kprintf("pIdx=%d Test Fail. y[%d] at i=%d did not match.\n", pIdx, i*NBPG, i);
			if( y[i*NBPG + NBPG -1 ] != (char) (m+4) ) kprintf("pIdx=%d Test Fail. y[%d] at i=%d did not match\n", pIdx, i*NBPG + NBPG -1, i);
		}
	}
	i = NBPG*128 -1;
	if( y[i] != 'L' ) kprintf("pIdx=%d mypno=%x Test Fail. y[0x%08x]=%d (@ 0x%08x )is not L (%d) \n", pIdx,mypno,i,y[i],&y[i],'L');
	kprintf("peakLoadTest pIdx=%d finished. If no errors have been printed yet, it is a pass.\n",pIdx);
	vfreemem(x,128*NBPG);
	xmunmap(mypno);
	sleep(10);
	release_bs(15);
}
void peakLoadTestLaunch(){
	int i, pid[15];

	get_bs(15,128);
	xmmap(0xFFF80,15,128);
//	*(char *)( 0xFFF80*NBPG) = 'P';
	*(char *)( (0xFFF80+127)*NBPG + NBPG -1) = 'L';
	for(i=0;i<15;i++) {
		pid[i] = vcreate(peakLoadTest, 2000, 128, 10, "peakLoadTest", 1,i);
		if(pid[i] == SYSERR) {
			kprintf("Unable to launch virtual mem process. Test FAIL\n");
		}
		resume(pid[i]);
	}
	xmunmap(0xFFF80);
	sleep(10);
	release_bs(15);
}

void badAccessTest(){
	int *x;
	x= vgetmem(sizeof(int));
	*x=123;
	kprintf("Value @ 0x%08x = %d\n",x,*x);
	x= (int *) 0x789ABCD;
	*x = 432;
	vfreemem(x,sizeof(int));
}


int main() {

	int pid1;
	int pid2;

	srpolicy(FIFO);
	kprintf("Current policy : %d\n",grpolicy());

#if 0
	kprintf("\n1: shared memory\n");
	pid1 = create(proc1_test1, 2000, 20, "proc1_test1", 1, "P1");
	resume(pid1);
	sleep(10);
  kprintf("Main Waking\n");
	pid2 = create(proc2_test1, 2000, 20, "proc2_test1", 4, "P2",63,56,"special");
	resume(pid2);
	sleep(10);
#endif

#if 0
	kprintf("\n2: vgetmem/vfreemem\n");
	pid1 = vcreate(memTest, 2000, 2, 20, "memTest", 4,"MT",78,92,"specialMem");
	kprintf("pid %d has private heap\n", pid1);
	ASSERT(pid1 != SYSERR);
	resume(pid1);
	sleep(3);
#endif

#if 1
  sleep(10);
	kprintf("\n3: Heap and Stack Load Test: Many processes with virtual mem, created and destroyed in a loop.\n");
	pid1 = create(privateHeapLoadTest, 2000, 20, "privateHeapLoadTest", 0,NULL);
	if(pid1 == SYSERR) { kprintf("Cannot create process. Test launch FAIL.\n"); }
	resume(pid1);
	sleep(3);
#endif

#if 0
	kprintf("\n4: Illegal Address Access Test. These 2 processes should get killed.\n");
	pid1 = vcreate(badAccessTest, 2000, 50, 20, "badAccessTest", 0,NULL);
	pid2 = vcreate(badAccessTest, 2000, 50, 20, "badAccessTest", 0,NULL);
	if(pid1 == SYSERR) { kprintf("Cannot create process1. Test launch FAIL.\n"); }
	if(pid2 == SYSERR) { kprintf("Cannot create process2. Test launch FAIL.\n"); }
	resume(pid1);
	sleep(3);
	resume(pid2);
	sleep(3);
	kprintf("\n6: Test Done. Both the processes should have been killed now.\n");
	sleep(60);
#endif

#if 0
	kprintf("\n5: Peak Load Test: All backing stores fully mapped with first and last byte of every page written and tested. BS-15 shared amongst all process.\n");
	pid1 = create(peakLoadTestLaunch, 2000, 20, "peakLoadTestLaunch", 0,NULL);
	if(pid1 == SYSERR) { kprintf("Cannot create process. Test launch FAIL.\n"); }
	resume(pid1);
	sleep(60);
#endif
  return 0;
}
