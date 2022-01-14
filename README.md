  # xv6-threading

#### **Kernel level support for threads along with threading library as wrapper for creating adn running multiple processes**

---

This project is about understating how threads on xv6 OS work. Another task which underlies behind the idea of this repository is the implementation of own thread making wrapper functions and corresponding system calls.

---
# ➡️ Getting Started

```bash
git clone https://github.com/SlavkoPrytula/xv6-threading
git cd xv6-threading
bash start_up.sh 
```
<br/>
<br/>
<br/>

---
# 🧠 Basics
- xv6 uses 32-bit virtual addresses, resulting in a virtual address space of 4GB. xv6 uses paging to manage its memory allocations. However, xv6 does not do demand paging, so there is no concept of virtual memory
- xv6 uses a page size of 4KB, and a two level page table structure
  1. xv6 uses a page size of 4KB, and a two level page table structure. The CPU register CR3 contains a pointer to the page table of the current running process. The translation from virtual to physical addresses is performed by the MMU as follows
  2. The first 10 bits of a 32-bit virtual address are used to index into a page table directory, which points to a page of the inner page table. The next 10 bits index into the inner page table to locate the page table entry (PTE). The PTE contains a 20-bit physical frame number and flags. Every page table in xv6 has mappings
for user pages as well as kernel pages
  3. The part of the page table dealing with kernel pages is the same across all processes


---
# Overview
--- 
## ❓ The first process
![image](https://user-images.githubusercontent.com/25413268/141524043-09c28ecd-cfc1-486b-bad0-f8b01b20e951.png)


A process is an abstraction that provides the illusion to a program that it has its own abstract machine. A process provides a program with what appears to be a private memory system, or address space, which other processes cannot read or write.

- Xv6 uses page tables (which are implemented by hardware) to give each process its own address space. The x86 page table translates  a virtual address to a physical address. Xv6 maintains a separate page table for each process that defines that process’s address space

Each process’s address space maps the kernel’s instructions and data as well as the user program’s memory. When a process invokes a system call, the system call executes in the kernel mappings of the process’s address space.

- In order to leave room for user memory to grow, xv6’s address spaces map the kernel at high addresses, starting at 0x80100000

```diff
- A process’s most important pieces of kernel state are its page table, its kernel stack, and its run state. 
```
```diff 
@@ Each process has two stacks: a user stack and a kernel stack (p->kstack) @@
@@ p->state indicates whether the process is allocated, ready to run, running, waiting for I/O, or exiting @@
@@ Each process has two stacks: a user stack and a kernel stack (p->kstack) @@
```

---
## ❓ Running the first process

Scheduler looks for a process with p->state set to **RUNNABLE**
- It sets the per-cpu variable proc to the process it found and calls switchuvm to tell the hardware to start using the target process’s page table
- Changing page tables while executing in the kernel works



---

# ✔️ Implementaing Threads
---

## Memory 

- xv6 allocates pages to the following part of the process in with below order
- Note xv6 uses stack per process which is 1 PAGESIZE and guard page which is also 1 PAGESIZE

![image](https://user-images.githubusercontent.com/25413268/141498627-08bed35d-d536-46cd-86ec-2a9eacb5a33f.png)

---

## Implementation of kernel threads

 - The concept of threads arises by the fact of *shared virtual address space*. Indeed threads execute concurently and share text-code, globals and heap spaces of virtual address space
 - However each individual thread has its seperate stack and registers when being executed
 
 ![image](https://user-images.githubusercontent.com/25413268/141496562-4e4d4535-08f1-4884-80df-c17268a87dc9.png)
 ![image](https://user-images.githubusercontent.com/25413268/141496041-62ba4a36-a1be-4783-8edf-c2765d7b2849.png)

 The main concept of running a new thread becomes the process of allocating the required memory with the new process and sharing the calling process's address space. 
 
---

# ✋ Locks
 
 - Xv6 runs on multiprocessors, computers with multiple CPUs executing code independently. These multiple CPUs operate on a single physical address space and share
data structures; xv6 must introduce a coordination mechanism to keep them from interfering with each other
 - A lock provides mutual exclusion, ensuring that
only one CPU at a time can hold the lock


## Race Condition

- The very first proble that we are trying to solve is regarting race conditions

![image](https://user-images.githubusercontent.com/25413268/146637443-8a5f4145-c61a-458d-af6c-8d0fc4544200.png)

- Unfortunately, the usual implementation does not guarantee mutual exclusion on a modern multiprocessor system. After all, it is possible that two or more processes simultaneously access the lock structure to determine whether they will work or wait
- The solution is to introduce **atomicity**. To execute these two lines atomically, xv6 relies on the xchg statement. In one atomic operation, xchg changes the word in memory to the contents of the register. In this way we can ensure the correct execution of successive programs

- test-and-set:
  - changes the contents of a memory location and returns it to its old value as a single atomic operation
- compare-and-swap:
  - atomically compares the contents of the memory area with the specified value and, only if they are the same, changes the contents of this memory area to the specified new value

## Usage

```cpp
void lock_acquire(struct lock *lock);
void lock_release(struct lock *lock);
```

```cpp
struct lock mylock;

void func_02(void *arg) {
    lock_acquire(&mylock);
    for (int i = 0; i < 100; i++) {
        printf(1, "locked 02 ");
    }
    lock_release(&mylock);
    exit();
}
```


---

# 🕖 Scheduling

 - Any operating system is likely to run more processes than the processors on your computer, so you need a plan to distribute the processors between processes. The ideal plan is transparent to user processes
 - The general approach is to create for each process the illusion that it has its own virtual processor and multiplex operating system, multiple virtual processors on one physical processor


## Multiplexing

 - Xv6 uses this multiplexing approach. When a process waits for a disk prompt, xv6 puts it to sleep and plans to start another process
 - In addition, xv6 uses timer interrupts to force the process to stop running on the processor after a fixed period of time ~ 100ms ~ so that it can schedule another process on the processor

```diff
- This multiplexing creates the illusion that each process has its own CPU, just as xv6 used a memory allocator and hardware page tables to create the illusion that each process has its own memory
```

## Context switching

![image](https://user-images.githubusercontent.com/25413268/146637673-e3518d35-e696-4122-866c-1d2eef616387.png)

- xv6 performs two types of low-level context switching:
  - from the process core flow to the current CPU scheduler flow
  - from the scheduler flow to the process kernel flow

- Xv6 itself never switches directly from one process in user space to another


## Scheduling

The plan for this is as follows:
- A process that wants to give CPU resources to another process must be intact, so locks are used
- Then release any other locks it holds. And update your own state (proc -> state), and then call the scheduler

- **scheduler** goes through the table of processes in search of a process that can be executed, which has p-> state == **RUNNABLE**. Once it finds a process, it sets the current process variable for each processor, switches to the process page table, marks the process as RUNNING, and then calls swtch to start it


![image](https://user-images.githubusercontent.com/25413268/146637742-36df254c-ad24-4a49-ae17-2e90e548d02b.png)

## Algorithms

### Round Robin

Xv6 itself uses the Round Robing algorithm as a scheduler. It is the simplest algorithm of advanced planning

It runs the program for until:
 - it can no longer work, either
 - it works too long (exceeds the "quantum of time")

![image](https://user-images.githubusercontent.com/25413268/146637791-783a4df9-4fca-48a1-bdbd-df359982317f.png)


## Lottery Ticket

In our example we implement a weel known Lottery Ticket scheduling algorithm for process execution mangement

 - The lottery is a probabilistic scheduling algorithm where at each process are each assigned some number of lottery tickets and the scheduler draws a random ticket to select the next process to run
 - The distribution of tickets need not be uniform
  - granting a process more tickets provides it a relative higher chance of selection

- This requires a random number generation and O(n) operations to traverse a client list of length n, accumulating a running ticket sum until it reaches the winning value

![image](https://user-images.githubusercontent.com/25413268/146637892-d53c7513-063f-4120-80c8-091c222dfe98.png)

### Usage 

```cpp
int main(int argc, char *argv[]) {
    printf(1, "Starting test runs!\n\n");

    int process_01 = thread_create(func_01, "hello world 01\n");
    int process_02 = thread_create(func_02, "hello world 02\n");

    // Changing second process' ticket count for it to have a
    // higher probability of being executed first
    change_tickets(process_01, 1);
    change_tickets(process_02, 200);

    thread_join();
    thread_join();

    printf(1, "\n\nTest passed!\n");

    exit();
}
```

---

 
## Mainly Used SysCalls

```cpp
int clone(void(*function)(void*), void *arg, void *stack)   // allocates memory and creates a new process
```

```cpp
int join(void **stack)  // waits for the processes to reach ZOMBIE state - aka. to finish their work
```

```cpp
void sleep(void*, struct spinlock*)   // waits
```

```cpp
int getpid(void)    //  return current process’s id
```

---

## Mainly used Functions

```cpp
void acquire(struct spinlock*);    // lock - used to set state for the process without race condition
void release(struct spinlock*)    // unlock
```

```cpp
void* malloc(uint)    // allocates memory
```

```cpp  
void free(void*)    // free memory
```

```cpp
int change_tickets(int pid, int tickets)    // change tickets for some process
```

```cpp
int get_tickets(void)   // get total accumulation of tickets in the system
```

```cpp
int random(int max)   // generates random number
```

```cpp
void lock_acquire(struct lock *lock)    // acquire lock
```

```cpp
void lock_release(struct lock *lock)    // release lock
```

--- 
# 💻 Work in Progress...

![Alt Text](https://media.giphy.com/media/7frSUXgbGqQPKNnJRS/giphy.gif)

---

## References & Literature
 - [1] xv6 a simple, Unix-like teaching operating system, Russ Cox, Frans Kaashoek, Robert Morris [https://www.cs.columbia.edu/~junfeng/11sp-w4118/lectures/mem.pdf]
 - [2] Processes [https://pdos.csail.mit.edu/6.828/2012/xv6/book-rev7.pdf]
 - [3] Processes in xv6 [https://www.youtube.com/watch?v=oQjolyqfUa8]
 - [4] Locks in xv6 [https://www.youtube.com/watch?v=pLgd-2gZikw&t]
 - [5] Memory Management in xv6 [https://www.cse.iitb.ac.in/~mythili/teaching/cs347_autumn2016/notes/08-xv6-memory.pdf]
 - [6] xv6 Scheduling [https://www.cs.virginia.edu/~cr4bd/4414/F2018/slides/20180911--slides-1up.pdf]
 - [7] Lottery Ticket Scheduler [https://www.usenix.net/legacy/publications/library/proceedings/osdi/full_papers/waldspurger.pdf]
