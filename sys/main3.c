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

void fifo_test()
{
  // 7 frames allocated before this
  int i,j,k;
  int vpno = 8192;
  int frames = 7;
  frames++;
  for(i=0; i<8; i++){
    char *c = (char *)((vpno+(i*128))*NBPG);
    get_bs(i,128);
    xmmap(VAD2VPN(c), i, 128);
    // 1 frame for PT every 8 cycles
    for(j=0; j<128; j++){
      frames++;
      // 128 frames for data
      c[j*NBPG] = 'A'+i;
    }

    if(i == 7){
      for(j=8; j<16; j++){
        char * b = (char*)((1024 +j)*NBPG);
        if(b[0] != 'H'){
          kprintf("Test Failed expected H got %c at frame %d\n", b[0], j);
          break;
        }
      }
      if(j == 16)
        kprintf("Test Passed \n");
    }
  }
  sleep(2);
  for(i=0; i<8; i++){
    xmunmap( vpno+ (i*128) );
    release_bs(i);
  }
}

void lru_test()
{
  // 7 frames allocated before this
  int i,j,k;
  int vpno = 8192;
  int frames = 7;
  frames++;
  for(i=0; i<8; i++){
    char *c = (char *)((vpno+(i*128))*NBPG);
    get_bs(i,128);
    xmmap(VAD2VPN(c), i, 128);
    // 1 frame for PT every 8 cycles
    for(j=0; j<128; j++){
      frames++;
      // 128 frames for data
      if(frames == 1000)
      {
        *((char*)(vpno*NBPG)) = 'R';
      }
      if(frames >1024)
      {
        c[j*NBPG] = 'B';
      }
      else{
        c[j*NBPG] = 'A';
      }
    }

    if(i == 7){
      int res = 1;
      for(j=9; j<17; j++){
        char * b = (char*)((1024 +j)*NBPG);
        if(b[0] != 'B'){
          kprintf("Test Failed expected B got %c at frame %d\n", b[0], j);
          res = 0;
          break;
        }
      }
      {
        char * b = (char*)((1024 +8)*NBPG);
        if(b[0] != 'R'){
          kprintf("Test Failed expected R got %c at frame %d\n", b[0], 8);
          res = 0;
        }
      }
      {
        char * b = (char*)((1024 +17)*NBPG);
        if(b[0] != 'A'){
          kprintf("Test Failed expected A got %c at frame %d\n", b[0], 17);
          res = 0;
        }
      }

      if(res == 1)
        kprintf("Test Passed \n");
    }
  }
  sleep(10);
  for(i=0; i<8; i++){
    xmunmap( vpno+ (i*128) );
    release_bs(i);
  }
}
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
//#define kprintf(...)

void memTest(char *msg, int a, int b, char *string){
	int i,sum,exp;
	int *x,*y;
#if 1
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

#endif
	//kprintf("\nMulti arg check: %d %d %s \n",a,b,string);
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
	get_bs(15,128);
	mypno = PEAK_VPNO - pIdx;
	//kprintf("get_bs DOne pIdx=%d currpid=%d\n",pIdx,currpid);
	x = vgetmem(128*NBPG);
	//kprintf("vgetmem DOne pIdx=%d currpid=%d x=%x-%x\n",pIdx,currpid, x, x+128*NBPG);
	xmmap(mypno,15,128);
	//kprintf("xmmap DOne pIdx=%d currpid=%d\n",pIdx,currpid);
#if 1
	for(i=0; i<128; i++) {
		x[i] = (char) (pIdx + i);
    //print_PA(x);
		x[i+NBPG-1] = (char) (pIdx*2 +i);
#if 0
    i++;
    x[i] = (char) (pIdx + i);
		x[i+NBPG-1] = (char) (pIdx*2 +i);
    i++;
    x[i] = (char) (pIdx + i);
		x[i+NBPG-1] = (char) (pIdx*2 +i);
    i++;
    x[i] = (char) (pIdx + i);
		x[i+NBPG-1] = (char) (pIdx*2 +i);

    i++;
    x[i] = (char) (pIdx + i);
		x[i+NBPG-1] = (char) (pIdx*2 +i);
    //print_PA(x+i);
#endif

    //sleep(20);
    //kprintf(" writing to %x %x\n", (unsigned int)&x[i], (unsigned int)&x[i+NBPG-1]);
		if(i%15 == pIdx && i!=127) {
			*(char *)( (mypno + i)*NBPG ) = (char) (pIdx+3);
			*(char *)( (mypno + i)*NBPG + NBPG -1) = (char) (pIdx+4);
		}
	}
#endif
	kprintf("Writing DOne pIdx=%d currpid=%d\n",pIdx,currpid);
	xmunmap(mypno);
	//kprintf("Unmap DOne pIdx=%d currpid=%d\n",pIdx,currpid);
	sleep(20);
	//kprintf("Wake DOne pIdx=%d currpid=%d\n",pIdx,currpid);
	mypno--;
	xmmap(mypno,15,128);
#if 1

	xmunmap(mypno);
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
#endif
	xmunmap(mypno);
	vfreemem(x,128*NBPG);
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
    //sleep(1);
	}
	xmunmap(0xFFF80);
	sleep(100);
  kprintf("Wake Launch\n");
	release_bs(15);
  kprintf("Exit Launch\n");
}

void badAccessTest(){
	int *x;
	x= vgetmem(sizeof(int));
	*x=123;
	kprintf("Value @ 0x%08x = %d\n",x,*x);
	vfreemem(x,sizeof(int));
	x= vgetmem(sizeof(int));
	x += 10*NBPG;
	*x = 432;
  kprintf("THIS should NOT print. Process Should have been killed\n");
	vfreemem(x,sizeof(int));
}


int main() {

	int pid1;
	int pid2;

	srpolicy(FIFO);

#if 0
	kprintf("\n1: shared memory\n");
	pid1 = create(proc1_test1, 2000, 20, "proc1_test1", 1, "P1");
	resume(pid1);
	sleep(10);
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

#if 0
  sleep(10);
	kprintf("\n3: Heap and Stack Load Test: Many processes with virtual mem, created and destroyed in a loop.\n");
	pid1 = create(privateHeapLoadTest, 2000, 20, "privateHeapLoadTest", 0,NULL);
	if(pid1 == SYSERR) { kprintf("Cannot create process. Test launch FAIL.\n"); }
	resume(pid1);
	sleep(3);
#endif

#if 0
	kprintf("\n4:FIFO test.\n");
	srpolicy(FIFO);
	pid1 = create(fifo_test, 2000, 20, "fifo", 0, NULL);
  resume(pid1);
  sleep(20);
#endif

#if 1
	kprintf("\n4:LRU test.\n");
	srpolicy(LRU);
	kprintf("Current policy : %d\n",grpolicy());
	pid1 = create(lru_test, 2000, 20, "lru", 0, NULL);
  resume(pid1);
  sleep(10);
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
	sleep(6000);
#endif
  kprintf("=====================Main End============\n");
  return 0;
}
