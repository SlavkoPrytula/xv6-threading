#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"


struct lock mylock;
int mutex;

void func_01(void *arg) {
    if (mutex_lock(mutex) < 0) {
        printf(1, "\n----Error while locking a mutex!----\n");
    }

    for (int i = 0; i < 100; i++) {
        printf(1, "locked 01 ");
    }

    if (mutex_unlock(mutex) < 0) {
        printf(1, "\n----Error while unlocking a mutex!----\n");
    }

    exit();
}

void func_02(void *arg) {
    if (mutex_lock(mutex) < 0) {
        printf(1, "\n----Error while locking a mutex!----\n");
    }

    for (int i = 0; i < 100; i++) {
        printf(1, "locked 02 ");
    }

    if (mutex_unlock(mutex) < 0) {
        printf(1, "\n----Error while unlocking a mutex!----\n");
    }
    exit();
}


int main(int argc, char *argv[]) {
    printf(1, "Starting test runs!\n\n");

    mutex = mutex_init(); // returns mutex id
    if (mutex < 0) {
        printf(1, "\n----Error while creating a mutex!----\n");
    }


    thread_create(func_01, "hello world 01\n");
    thread_create(func_02, "hello world 02\n");
    thread_join();
    thread_join();

    printf(1, "\n\nTest passed!\n");

    exit();
}

