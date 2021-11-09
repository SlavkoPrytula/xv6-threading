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
    // CHECK HERE FOR ARGUMENTS !!!
//    if (!arg || !function) {
//        exit();
//    }

    printf(1, "Create thread\n");
    // alloc two pages of memory for stack and use
    void *stack = malloc(PGSIZE * 2 + 4);
    if (stack <= 0) {
        printf(1, "Failed to malloc memory for stack in thread_create.\n");
        exit();
    }

    printf(1, "thread_create stack: %d\n", stack);

    // make sure stack is page aligned
    if ((uint)stack % PGSIZE) {
        printf(1, "malloced stack is not aligned in thread_create. Shift to next page.\n");
        stack = stack + (PGSIZE - (uint)stack % PGSIZE);
        printf(1, "new stack: %d\n", stack);
    }
    printf(1, "thread_create: clone() on stack %d.\n", stack);

    return clone(function, arg, stack);
}

int thread_join() {
    void *stack;
    printf(1, "thread_join\n");

    // get pid of thread
    int pid;

    // if not parent process, free the thread's stack
    if ((pid = join(&stack)) != -1) {
        printf(1, "joining thread %d\n", pid);
        free(stack);
    }
    printf(1, "stack freed\n");

  return pid;
}
