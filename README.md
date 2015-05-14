pa2
===

1 Introduction

Demand paging is a method of mapping a large address space into a relatively small amount of physical memory. This is accomplished by using a "backing store" (usually disk) to hold pages of memory not currently needed. Paging is what allows us to use more address space than that is physically available, and to run programs in non-contiguous sections of memory.

From this point on only the details of this project are discussed. It is assumed that you have read the Intel documents and are comfortable with paging concepts and the Intel specific details.
2 Goals

The ultimate goal of this project is to implement the system calls listed below and the underlying facilities to support them.
3 System Calls to Implement
SYSCALL xmmap (int virtpage, bsd_t source, int npages)

Much like its Unix counterpart (see man mmap) it will map a source "file" ("backing store" here) of size npages pages to the virtual page virtpage. A process may call this multiple times to map data structures, code, etc.
SYSCALL xmunmap (int virtpage)

This call, like munmap, should remove a virtual memory mapping. See man munmap for the details of the Unix call.
SYSCALL vcreate (int *procaddr, int ssize, int hsize, int priority, char *name, int nargs, long args)

This call will create a new Xinu process. The difference from create() is that the process's heap will be private and exist in its virtual memory. The size of the heap (in number of pages) is specified by the user.

create() should be left (mostly) unmodified. Processes created with create should not have a private heap but should still be able to use xmmap().
WORD\* vgetmem (int nbytes)

Much like getmem(), vgetmem() will allocate the desired amount of memory if possible. The difference is that vgetmem() will get the memory from a process' private heap located in virtual memory. A call to getmem() will continue to allocate memory from the regular Xinu kernel heap.
SYSCALL srpolicy (int policy)

This function will be used to set the page replacement policy to FIFO or Least Recently Used (LRU). You can declare constant FIFO as 3 and LRU as 4 for this purpose.
SYSCALL vfreemem (block_ptr, int size_in_bytes)

You will implement a corresponding vfreemem() call for the vgetmem(bytes) call. It will take two parameters and will return OK or SYSERR. The two parameters are similar to those of the freemem() call in the original Xinu. The type of the first argument block_ptr will depend on your own implementation.
4 Overall Organization

The following sections discuss at a high level the organization of the system, the various pieces we need to implement demand paging in Xinu and how they relate to each other. This handout describes ideas for implementation in Xinu. You are welcome to use a different implementation strategy if you think it is easier or better as long as it has the same functionality and challenges.
4.1 Memory and Backing Store
4.1.1 Backing Stores

Virtual memory commonly uses disk space to extend the memory of the machine. However, the version of Xinu we use has no file system support. Also, we cannot use the network to communicate with a page server which will manage the backing stores. Instead, we will emulate the backing stores (how they are emulated will be detailed in 4.1.3). To access the backing store you are given the following (These calls are in the directory paging):

    bsd_t The backing store descriptor type is used to refer to a backing store. The type declaration for it is in paging.h. This type is merely unsigned int.

    There are 16 backing stores. You will use the IDs zero through fifteen to identify them.
    int get_bs (bsd_t store, unsigned int npages) This call requests a new backing store with id store of size npages (in pages, not bytes). If the new backing store cannot be created, or a backing store with this ID already exists, the size of the new or existing backing store is returned. This size is in pages. If a size of 0 is requested, or an error is encountered, SYSERR is returned.

    Also for practical reasons, the npages will be no more than 128.
    int release_bs (bsd_t store) This call releases the backing store with ID store.

    SYSCALL read_bs (char *dst, bsd_t store, int page) This copies pageth page from the backing store referenced by store to dst. It returns OK on success, SYSERR otherwise. The first page of a backing store is page zero.

    SYSCALL write_bs (char *src, bsd_t store, int page) This copies a page pointed to by src to the pageth page of the backing store referenced by store. It returns OK on success, SYSERR otherwise.

4.1.2 Memory Layout
The basic Xinu memory layout will be as follows:

Virtual memory

(pages 4096 and beyond)

3072 frames

(pages 1024 - 4095)
 

Kernel Memory (pages 406 - 1023)

Kernel Memory, HOLE (pages 160 - 405)

Kernel Memory (pages 25 - 159)
 
Xinu text, data, bss (pages 0 - 24)

Our Xinu will compile to about 100k, or 25 pages. The PC has an area of memory from page 160 through the end of page 405 that cannot be used (this is referred to as the "HOLE" in initialize.c). We will place the free frames into pages 1024 through 4095, giving 3072 frames.

The frames will be used to store resident pages, page directories, and page tables. The remaining free memory below page 4096 is used for Xinu's kernel heap (organized as a freelist). getmem() and getstk() will obtain memory from this area (from the bottom and top respectively).

All memory below page 4096 will be "global." That is, it is usable and visible by all processes and accessible by simply using its actual physical addresses. As a result, the first four inner page tables for every process will be the same, and thus should be shared.

Memory at page 4096 and above constitute a process' virtual memory. This address space is private and visible only to the process which owns it. Note that the process' private heap and (optionally) stack are located somewhere in this area.
4.1.3 Backing Store Emulation
Since our version of Xinu does not have file system and network support, we need to emulate the backing store with physical memory. In particular, consider the following Xinu memory layout:

Virtual Memory

(pages 4096 and beyond)

16 Backing stores

(pages 2048 - 4095)

1024 frames

(pages 1024 - 2047)


Kernel Memory (pages 406 - 1023)

Kernel Memory, HOLE (pages 160 - 405)

Kernel Memory (pages 25 - 159)
 
Xinu text, data, bss (pages 0 - 24)

Each backend machine has 16 MB (4096 pages) of real memory. We reserve the top 8MB real memory as backing stores. We have 16 backing stores and each backing store will only map maximum 128 pages (each page is 4K size). As a result, we have the following map between the backing store and the corresponding physical memory range:

backing store 0: 0x00800000 - 0x0087ffff

backing store 1: 0x00880000 - 0x008fffff

backing store 2: 0x00900000 - 0x0097ffff

backing store 3: 0x00980000 - 0x009fffff

backing store 4: 0x00a00000 - 0x00a7ffff

backing store 5: 0x00a80000 - 0x00afffff

backing store 6: 0x00b00000 - 0x00b7ffff

backing store 7: 0x00b80000 - 0x00bfffff

backing store 8: 0x00c00000 - 0x00c7ffff

backing store 9: 0x00c80000 - 0x00cfffff

backing store 10: 0x00d00000 - 0x00d7ffff

backing store 11: 0x00d80000 - 0x00dfffff

backing store 12: 0x00e00000 - 0x00e7ffff

backing store 13: 0x00e80000 - 0x00efffff

backing store 14: 0x00f00000 - 0x00f7ffff

backing store 15: 0x00f80000 - 0x00ffffff
In the implementation, you need to "steal" physical memory frames 2048 - 4095 (take a close look at sys/i386.c, and pay attention to the variables npages and maxaddr). As a result, this portion of memory will not be used for other purposes. You can assume that our grading program will not modify this part of memory.
4.1.4 Page Tables and Page Directories
Page tables and page directories (i.e. outer page tables) can be placed in any free frame. For this project you will not be paging either the page tables or page directories. Because page tables are always resident it is not practical to allocate all potential page tables for a process when it is created (you will, however, allocate a page directory). To map all 4 GB of memory would require 4 MB of page tables! To conserve memory, page tables must be created on-demand. That is, the first time a page is legally touched (i.e. it has been mapped by the process) for which no page table is present a page table should be allocated. Conversely, when a page table is no longer needed it should be removed to conserve space.
4.2 Supportting Data Structures
4.2.1 Finding the backing store for a Virtual Address
You may realize that there is a problem: if a process can map multiple address ranges to different backing stores, how does one figure out which backing store a page needs to be read from (or written to) when it is being brought into (removed from) a frame?

To accomplish this you need to keep track of which backing store the process was allocated when it was created using vcreate. Finding the offset (i.e the particular page within the store to write/read from) can be calculated using the virtual page number. You may need to declare a new kernel data structure which maps virtual address spaces to backing store descriptors. We will call this the backing store map. It should be a tuple like:

{ pid, vpage, npages, store }

You should write a function that performs the lookup:

f (pid , vaddr)= > {store, pageoffset within store}

The function xmmap() will add a mapping to this table. xmunmap() will remove a mapping from this table.
4.2.2 Inverted Page Table
When writing out a dirty page you may have noticed the only way to figure out which virtual page and process (and thus which backing store) a dirty frame belongs to would be to traverse the page tables of every process looking for a frame location that corresponds to the frame we wish to write out. This is highly inefficient. To prevent this, we can use another kernel data structure, an inverted page table. The inverted page table contains tuples like:

{ framenumber, pid, virtual pagenumber }

Of course, if we use an array of size NFRAMES the frame number is implicit and just the index into the array. With this structure we can easily find the pid and virtual page number of the page held within any frame i. From that we can easily find the backing store (using the backing store map) and compute which page within the backing store corresponds with the page in frame i.

You may also want to use this table to hold other information for page replacement (i.e., any data needed to estimate page replacement policy).
4.3 Process Considerations
With each process having its own page directory page tables there are some new considerations in dealing with processes.
4.3.1 Process Creation
When a process is created we must now also create page directory and record the address. Also remember that the first 16 megabytes of each process will be mapped to the 16 megabytes of physical memory, so we must initialize the process' page directory accordingly. This is important as our backing stores also depend on this correct mapping.

A mapping must be created for the new process' private heap and stack, if created with vcreate(). Because you are limited to 16 backing stores you may want to use just one mapping for both the heap and the stack (as with the kernel heap), vgetmem() taking from one end and the stack growing from the other. (Keeping a private stack and paging it is optional, but a private heap must be maintained).
4.3.2 Process Destruction
When a process dies the following should happen.

    All frames which currently hold any of its pages should be written to the backing store and be freed.
    All of it's mappings should be removed from the backing store map.
    The backing stores for its heap (and stack if private stack is implemented) should be released (remember backing stores allocated by a process should persist unless the process explicitly releases them).
    The frame used for the process' page directory should be released.

4.3.3 Context Switch
It should also be clear that now as we switch between processes we must also switch between memory spaces. This is accomplished by adjusting the PDBR register with every context switch. We must be careful, however, as this register must always point to a valid page directory which is in RAM at a page boundary.

Think carefully about where you place this switch if you put it in resched(): before or after the actual context switch.
4.3.4 System Initialization
The NULL process is somewhat of a special case, as it builds itself in the function sysinit(). The NULL process should not have a private heap (like any process created with create()).

The following should occur at system initialization:

    Set the DS and SS segments' limits to their highest values. This will allow processes to use memory up to the 4 GB limit without generating general protection faults. Make sure the initial stack pointer (initsp) is set to a real physical address (the highest physical address) as it is in normal Xinu. See i386.c. Don't forget to "steal" physical memory frames 2048 - 4096 for backing store purposes.
    Initialize all necessary data structures.
    Create the page tables which will map pages 0 through 4095 to the physical 16 MB. These will be called the global page tables.
    Allocate and initialize a page directory for the NULL process.
    Set the PDBR register to the page directory for the NULL process.
    Install the page fault interrupt service routine.
    Enable paging.

4.4 The Interrupt Service Routine (ISR)
A page fault triggers an interrupt 14. When an interrupt occurs the machine pushes CS:IP and then an error code

error code
IP
CS

It then jumps to a predetermined point, the ISR. To specify the ISR we use the routine:

set_evec(int interrupt, (void (\*isr)(void))) (see evec.c)

4.5 Faults and Replacement Policies
4.5.1 Page Faults
A page fault indicates one of two things: the virtual page on which the faulted address exists is not present or the page table which contains the entry for the page on which the faulted address exists is not present. To deal with a page fault you must do the following:

    Get the faulted address a.
    Let vp be the virtual page number of the page containing of the faulted address.
    Let pd point to the current page directory.
    Check that a is a legal address (i.e., it has been mapped). If it is not, print an error message and kill the process.
    Let p be the upper ten bits of a. [What does p represent?]
    Let q be the bits [21:12] of a. [What does q represent?]
    Let pt point to the pth page table. If the pth page table does not exist obtain a frame for it and initialize it.
    To page in the faulted page do the following:
        Using the backing store map, find the store s and page offset o which correspond to vp.
        In the inverted page table increment the reference count of the frame which holds pt. This indicates that one more of pt's entries is marked "present."
        Obtain a free frame, f.
        Copy the page o of store s to f.
        Update pt to mark the appropriate entry as present and set any other fields. Also set the address portion within the entry to point to frame f.

4.5.2 Obtaining a Free Frame
When a free frame is needed it may be necessary to remove a resident page from frame. How you pick the page to remove depends on your page replacement policy,

Your function to find a free page should do the following:

    Search inverted page table for an empty frame. If one exists stop.
    Else, pick a page to replace.
    Using the inverted page table, get vp, the virtual page number of the page to be replaced.
    Let a be vp*4096 (the first virtual address on page vp).
    Let p be the high 10 bits of a. Let q be bits [21:12] of a.
    Let pid be the pid of the process owning vp.
    Let pd point to the page directory of process pid.
    Let pt point to the pid's pth page table.
    Mark the appropriate entry of pt as not present.
    If the page being removed belongs to the current process, invalidate the TLB entry for the page vp using the invlpg instruction (see Intel Volume III/II).
    In the inverted page table decrement the reference count of the frame occupied by pt. If the reference count has reached zero, you should mark the appropriate entry in pd as being not present. This conserves frames by keeping only page tables which are necessary.
    If the dirty bit for page vp was set in its page table you must do the following:
        Using the backing store map to find the store and page offset within store given pid and a. If the lookup fails, something is wrong. Print an error message and kill the process pid.
        Write the page back to the backing store.

4.5.3 Page Replacement Policies

You must implement two page replacement algorithms: FIFO and Least Recently Used. For this you will want to make use of page's accessed bits. There are also three bits in the page table entries which you may use for any purpose. You are free to add whatever structures you'd like in your inverted page table, or extra processes if needed.

FIFO: Basically for FIFO you can maintain a FIFO Queue. When a new page comes, put it at the tail of the queue, and while swapping a page out, choose the page at the head of the queue to replace.

LRU: Because there is no way of knowing when a page was accessed, we cannot keep a counter access time to track which page was least recently used. An estimate of LRU policy can be implemented in the following way: When a frame is allocated for a page, the OS increases a timeCount by 1. It then goes through the pt_acc bit of all pages. If the bit of a page is 1, it records timeCount for that page as its estimated last access time, and then reset that bit. The timeCount of the pages can then reflect roughly how recent the page has been accessed. If there are multiple pages having the same timeCount value that is the least among all pages, the page having the largest Virtual Page Number among them is selected as the victim.
5 Required API Calls
You must implement the system calls listed in the beginning of this handout exactly as specified. Be sure to check the parameters for validity. For example, no process should be allowed to remap the lowest 16 MB of the system (global memory).

None of Xinu's other system calls interfaces should be modified.
6 Details on the Intel Architecture and Xinu

The following might be useful for you to know:

    We are using the Intel Pentium chip, not the Pentium Pro or Pentium II. Some details of those chips do not apply.
    Xinu uses the flat memory model, i.e. physical address = linear addresses.
    The segments are set in i386.c in the function setsegs().
    Pages are 4k (4096 bytes) in size. Do not use 2M or 4M page size
    The backend machines have 16 MB (4096 pages) of real memory.
    Some example code is given in the project directory for getting and setting the control registers. A useful function, dump32(unsigned long), for dumping a binary number with labeled bits is also given.

