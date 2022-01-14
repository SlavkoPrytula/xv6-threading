#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

//#include "stat.h"
//#include "user.h"
//#include <stdio.h>

#define DEFAULT_TICKETS 1;

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct {
    struct spinlock lock;
    struct mutex_struct mutex[MAX_MUTEXES];
} mtable;

static struct proc *initproc;

int nextpid = 1;
int nextmid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;

  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S
  p->tickets = DEFAULT_TICKETS;

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;
  np->tickets = DEFAULT_TICKETS;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

// ---------------------------------------------------------------------
int get_tickets(void) {
    struct proc *p;
    int sum = 0;

    // Searching for RUNNABLE processes to get their tickets
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state == RUNNABLE) {
            sum += p->tickets;
        }
    }

    return sum;
}

int change_tickets(int pid, int tickets) {
    struct proc *p;
    cprintf("process: %d, set tickets to: %d\n", pid, tickets);

    acquire(&ptable.lock);
    // Looking for process with specified pid id to change its ticket count
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if (p->pid == pid) {
            p->tickets = tickets;
            break;
        }
    }
    release(&ptable.lock);

    return pid;
}


int random(int max) {
    // LFSR alogrithm found here: http://goo.gl/At4AIC
    if (max <= 0) {
        return 1;
    }

    static int z1 = 12345; // 12345 for rest of zx
    static int z2 = 12345; // 12345 for rest of zx
    static int z3 = 12345; // 12345 for rest of zx
    static int z4 = 12345; // 12345 for rest of zx

    int b;
    b = (((z1 << 6) ^ z1) >> 13);
    z1 = (((z1 & 4294967294) << 18) ^ b);
    b = (((z2 << 2) ^ z2) >> 27);
    z2 = (((z2 & 4294967288) << 2) ^ b);
    b = (((z3 << 13) ^ z3) >> 21);
    z3 = (((z3 & 4294967280) << 7) ^ b);
    b = (((z4 << 3) ^ z4) >> 12);
    z4 = (((z4 & 4294967168) << 13) ^ b);

    // if we have an argument, then we can use it
    int num = ((z1 ^ z2 ^ z3 ^ z4)) % max;

    if (num < 0) {
        num = num * -1;
    }

    return num;
}


void scheduler(void) {
    struct proc *p;
    struct cpu *c = mycpu();
    c->proc = 0;

    for(;;) {
        // Enable interrupts on this processor.
        sti();

        // Loop over process table looking for process to run.
        acquire(&ptable.lock);
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if(p->state != RUNNABLE)
              continue;

            int tickets_sum = get_tickets();
            int choice = -1;

          	if (tickets_sum > 0) {
                choice = random(tickets_sum);
//                cprintf("-> Random returned: %d\n", choice);
            }

            if (choice >= p->tickets) {
                continue;
            }

            c->proc = p;
            switchuvm(p);
            p->state = RUNNING;

            swtch(&(c->scheduler), p->context);
            switchkvm();

            c->proc = 0;
        }

        release(&ptable.lock);
    }
}

// ---------------------------------------------------------------------

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
//void
//scheduler(void)
//{
//  struct proc *p;
//  struct cpu *c = mycpu();
//  c->proc = 0;
//
//  for(;;){
//    // Enable interrupts on this processor.
//    sti();
//
//    // Loop over process table looking for process to run.
//    acquire(&ptable.lock);
//    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
//      if(p->state != RUNNABLE)
//        continue;
//
//      // Switch to chosen process.  It is the process's job
//      // to release ptable.lock and thechprn reacquire it
//      // before jumping back to us.
//      c->proc = p;
//      switchuvm(p);
//      p->state = RUNNING;
//
//      swtch(&(c->scheduler), p->context);
//      switchkvm();
//
//      // Process is done running for now.
//      // It should have changed its p->state before coming back.
//      c->proc = 0;
//    }
//    release(&ptable.lock);
//
//  }
//}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}


// NEW
int hello_world(void) {
  cprintf("hello world\n");
  return 1;
}


// Clone (aka modified fork)
int clone(void(*function)(void*), void *arg, void *stack) {
    struct proc *new_process;
    struct proc *curr_process = myproc();

    // make sure that the stack is aligned and not outside the set boundaries
    if ((uint)stack + PGSIZE > curr_process->sz || (uint)stack % PGSIZE) {
        exit();
    }

    // Allocate process
    if ((new_process = allocproc()) == 0) {
        return -1;
    }
    // Get the pid of the new process to return if success
    int pid = new_process->pid;

    /** share pgdir of parent instead of copying and set stack to new stack **/

    // new kernel thread shares the calling process's address space
    new_process->pgdir = curr_process->pgdir;
    new_process->sz = curr_process->sz;
    new_process->parent = curr_process;
    *new_process->tf = *curr_process->tf;

    // Clear %eax so that clone returns 0 in the child thread
    new_process->tf->eax = 0;

    // set up new stack, build from top down
    new_process->ustack = stack; // set top (memory bottom) of thread's user stack
    // stack index for building stack. The "bottom" is actually the top of the memory space, hence we subtract
    uint stack_bottom = (uint)stack + PGSIZE;
    *((uint*)(stack_bottom - (1 * sizeof(uint)))) = (uint)arg;
    *((uint*)(stack_bottom - (2 * sizeof(uint)))) = 0xffffffff; // setup the fake return address

    new_process->tf->esp = stack_bottom - (2 * sizeof(uint));
    new_process->tf->eip = (uint)function;

    for(uint i = 0; i < NOFILE; i++) {
        if(curr_process->ofile[i]) {
            new_process->ofile[i] = filedup(curr_process->ofile[i]);
        }
    }
    new_process->cwd = idup(curr_process->cwd);
    safestrcpy(new_process->name, curr_process->name, sizeof(curr_process->name));

    acquire(&ptable.lock);
    new_process->tickets = new_process->parent->tickets;
    new_process->state = RUNNABLE;
    release(&ptable.lock);

    // increment thread count by one
    new_process->refs = curr_process->refs;
    *new_process->refs += 1;

    // Save the name for child
    safestrcpy(new_process->name, curr_process->name, sizeof(curr_process->name));

    return pid;
}


// Join
int join(void **stack) {
    // This method mostly copy from wait().
    // Indeed, this method only wait for children who share the same address space
    struct proc *process;
    int pid;
    struct proc *curr_process = myproc();

    acquire(&ptable.lock);
    for(;;){
        // Scanning through the page table to look out for exited child processes
        for(process = ptable.proc; process < &ptable.proc[NPROC]; process++){
            // Get the pid of the maybe running process to return if success
            pid = process->pid;

            // found a different process, but not a thread
            // check if share the same address space
            if(process->parent != curr_process || process->pgdir != curr_process->pgdir) {
                continue;
            }

            if(process->state == ZOMBIE) {
                // set stack to thread's user stack
                // the location of child's user stack is copied into argument *stack
                // which will be freed later
                *stack = process->ustack;

                // Found one.
                kfree(process->kstack);
                process->kstack = 0;
                process->pid = 0;
                process->parent = 0;
                process->name[0] = 0;
                process->killed = 0;
                process->state = UNUSED;

                // decrement thread count
                *process->refs -= 1;

                release(&ptable.lock);
                return pid;
            }
        }
        sleep(curr_process, &ptable.lock);  //DOC: wait-sleep
    }
}


// Mutex
int mutex_init() {
    // Initialize mutexes to ready state (unlocked)
    // Returns id of the new mutex
    struct mutex_struct *m;
    acquire(&mtable.lock);

    for (m = mtable.mutex; m->state != LOCKED && m < &mtable.mutex[MAX_MUTEXES]; m++);
    if (m == &mtable.mutex[MAX_MUTEXES]) {
        release(&mtable.lock);
        return -1;
    }
    m->id = nextmid++;
    m->state = UNLOCKED;

    release(&mtable.lock);
    return m->id;
}

int mutex_lock(int mutex_id) {
    // Locks the mutex by its unique id
    struct mutex_struct *m;
    acquire(&mtable.lock);

    // Iterate to find the specific id
    for (m = mtable.mutex; m->id != mutex_id && m < &mtable.mutex[MAX_MUTEXES]; m++);
    if (m == &mtable.mutex[MAX_MUTEXES]) {
        release(&mtable.lock);
        return -1;
    }

    // If already locked by another process create a wait loop
    // The mutex will be acquired when another thread finishes using it
    while (m->state == LOCKED) {
        sleep(m, &mtable.lock);
    }

    m->state = LOCKED;  // Set the state to locked

    release(&mtable.lock);
    return 0;
}


int mutex_unlock(int mutex_id) {
    // Unlocks the mutes by unique id
    struct mutex_struct *m;
    acquire(&mtable.lock);

    // Iterate to find the specific id
    for (m = mtable.mutex; m->id != mutex_id && m < &mtable.mutex[MAX_MUTEXES]; m++);
    if (m == &mtable.mutex[MAX_MUTEXES] || m->state != LOCKED) {
        release(&mtable.lock);
        return -1;
    }
    m->state = UNLOCKED;    // Set the state to unlocked
    wakeup1(m);

    release(&mtable.lock);
    return 0;
}


// Barrier
int count = 0;
int proc_count;
struct spinlock lock;

int barrier_init(int n) {
    // Initialize the barrier with specific target of threads to be blocked
    count = 0;
    initlock(&lock, "barrier_lock");
    proc_count = n;

    return 0;
}

int barrier_lock(void) {
    // The first n-1 calls will be blocked by barrier
    // The nth call will unlock all the waiting threads
    acquire(&lock);

    count += 1;
    while(count < proc_count) {
        // Waiting loop
        sleep(&proc_count, &lock);
    }
    // nth call reached this part - unlock the barrier
    wakeup(&proc_count);
    release(&lock);

    return 0;
}