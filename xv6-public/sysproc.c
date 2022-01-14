#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


// NEW
int sys_hello_world(void) {
	return hello_world();
}

// Clone
int sys_clone(void) {
    void *function;
    void *arg;
    void *stack;

    // Passing arguments from user-level functions to kernel-level functions
    // argint uses the user-space esp register to locate the n’th argument: esp points at the
    // return address for the system call stub. The arguments are right above it, at esp+4.
    // Then the nth argument is at esp+4+4*n

    // argptr is similar in purpose to argint: it interprets the nth system call argument.
    // argptr calls argint to fetch the argument as an integer and then checks if the
    // integer as a user pointer is indeed in the user part of the address space
    if(argint(1, (void *)&arg) < 0 || argint(2, (void *)&stack) < 0 || argptr(0, (void *)&function, sizeof(void *)) < 0 || (uint)stack % PGSIZE != 0 || (uint)myproc()->sz - (uint)stack == PGSIZE / 2) {
        return -1;
    }

    return clone(function, arg, stack);
}


int sys_join(void) {
  void **stack;

  if(argptr(0, (void *)&stack, sizeof(void *)) < 0)
    return -1;

  return join(stack);
}

extern int change_tickets(int, int);

int sys_change_tickets(void) {
    int pid, tickets;
    if (argint(0, &pid) < 0 || argint(1, &tickets) < 0) {
        return -1;
    }

    return change_tickets(pid, tickets);
}

extern int mutex_init(void);
extern int mutex_lock(int);
extern int mutex_unlock(int);

int sys_mutex_init(void) {
    return mutex_init();
}

//int
//sys_kthread_mutex_dealloc(void)
//{
//    int mutex_id;
//    if(argint(0, &mutex_id) < 0)
//        return -1;
//
//    return kthread_mutex_dealloc(mutex_id);
//}

int sys_mutex_lock(void) {
    int mutex_id;
    if(argint(0, &mutex_id) < 0)
        return -1;

    return mutex_lock(mutex_id);
}

int sys_mutex_unlock(void) {
    int mutex_id;
    if(argint(0, &mutex_id) < 0)
        return -1;

    return mutex_unlock(mutex_id);
}
