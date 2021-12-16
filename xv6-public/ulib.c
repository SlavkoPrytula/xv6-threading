#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"


#define PGSIZE 4096


char* strcpy(char *s, const char *t) {
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int strcmp(const char *p, const char *q) {
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint strlen(const char *s) {
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void* memset(void *dst, int c, uint n) {
  stosb(dst, c, n);
  return dst;
}

char* strchr(const char *s, char c) {
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char* gets(char *buf, int max) {
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int stat(const char *n, struct stat *st) {
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int atoi(const char *s) {
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void* memmove(void *vdst, const void *vsrc, int n) {
  char *dst;
  const char *src;

  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}


// Threads | Lock & Join
int thread_create(void (*function)(void *), void *arg) {
    // alloc two pages of memory for stack and use
    void *stack = malloc(PGSIZE * 2); // stack per process + guard page
    if (stack <= 0) {
        exit();
    }

    // make sure stack is page aligned
    if ((uint)stack % PGSIZE) {
        stack = stack + (PGSIZE - (uint) stack % PGSIZE);
    }
    return clone(function, arg, stack);
}

int thread_join() {
    // get pid of thread
    int pid;
    void *stack;

    // if not parent process, free the thread's stack
    if ((pid = join(&stack)) != -1) {
        free(stack);
    }

    return pid;
}

void lock_acquire(struct lock *lock) {
    // acquire lock
    while(test_and_set(&lock->locked) == 1)
        ; // wait
}

void lock_release(struct lock *lock) {
    // release lock
    lock->locked = 0;
}

