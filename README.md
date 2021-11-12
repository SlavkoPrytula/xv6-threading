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

--- 
# 💻 Work in Progress...

![Alt Text](https://media.giphy.com/media/7frSUXgbGqQPKNnJRS/giphy.gif)

---

## References & Literature
 - [1] xv6 a simple, Unix-like teaching operating system, Russ Cox, Frans Kaashoek, Robert Morris [https://www.cs.columbia.edu/~junfeng/11sp-w4118/lectures/mem.pdf]
 - [2] Processes [https://pdos.csail.mit.edu/6.828/2012/xv6/book-rev7.pdf]
 - [3] Processes in xv6 [https://www.youtube.com/watch?v=oQjolyqfUa8]
 - [4] Locks in xv6 [https://www.youtube.com/watch?v=pLgd-2gZikw&t]
