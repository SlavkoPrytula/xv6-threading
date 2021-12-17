#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"

mcslock_t mymcslock;
struct qnode myqnode;

void func_01(void *arg) {
    mcs_lock(&mymcslock, &myqnode);
    for (int i = 0; i < 100; i++) {
        printf(1, "locked 01 ");
    }
    mcs_unlock(&mymcslock, &myqnode);
    exit();
}

void func_02(void *arg) {
    mcs_lock(&mymcslock, &myqnode);
    for (int i = 0; i < 100; i++) {
        printf(1, "locked 02 ");
    }
    mcs_unlock(&mymcslock, &myqnode);
    exit();
}

int main(int argc, char *argv[]) {
    printf(1, "Starting test runs!\n\n");

    mcs_init(mymcslock);

    thread_create(func_01, "hello world 01\n");
    thread_create(func_02, "hello world 02\n");
    thread_join();
    thread_join();

    printf(1, "\n\nTest passed!\n");

    exit();
}

