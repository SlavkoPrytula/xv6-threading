struct stat;
struct rtcdate;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);

// NEW
int hello_world(void);
int clone(void(*function)(void*), void*, void*);
int join(void**);


int thread_create(void (*function)(void*), void *arg);
int thread_join();

struct lock {
    volatile unsigned int locked;

    volatile unsigned int next_ticket;
    volatile int now_serving;
};

void lock_acquire(struct lock *lock);
void lock_release(struct lock *lock);

void ticket_acquire(struct lock *lock);
void ticket_release(struct lock *lock);

struct qnode {
    volatile void *next;
    volatile char locked;
    char __pad[0] __attribute__((aligned(CACHELINE)));
};

typedef struct {
    struct qnode *v  __attribute__((aligned(64)));
    int lock_idx  __attribute__((aligned(64)));
} mcslock_t;

void mcs_init(mcslock_t *l);
void mcs_lock(mcslock_t *l, volatile struct qnode *mynode);
void mcs_unlock(mcslock_t *l, volatile struct qnode *mynode);

int change_tickets(int, int);

