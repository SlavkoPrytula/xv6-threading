#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"

struct lock mylock;

void func_01(void *arg) {
    lock_acquire(&mylock);
    for (int i = 0; i < 100; i++) {
        printf(1, "locked 01 ");
    }
    lock_release(&mylock);
    exit();
}

void func_02(void *arg) {
    lock_acquire(&mylock);
    for (int i = 0; i < 100; i++) {
        printf(1, "locked 02 ");
    }
    lock_release(&mylock);
    exit();
}

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

