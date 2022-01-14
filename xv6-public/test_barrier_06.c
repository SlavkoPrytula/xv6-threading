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
    sleep(10);
    printf(1, "function 01 entered \n");
    barrier_lock();
    printf(1, "function 01 exited \n");

    exit();
}

void func_02(void *arg) {
    sleep(200);
    printf(1, "function 02 entered \n");
    barrier_lock();
    printf(1, "function 02 exited \n");

    exit();
}

void func_03(void *arg) {
    sleep(600);
    printf(1, "function 03 entered \n");
    barrier_lock();
    printf(1, "function 03 exited \n");

    exit();
}


int main(int argc, char *argv[]) {
    printf(1, "Starting test runs!\n\n");

    barrier_init(3);

    thread_create(func_01, "");
    thread_create(func_02, "");
    thread_create(func_03, "");
    thread_join();
    thread_join();
    thread_join();

    printf(1, "\n\nTest passed!\n");

    exit();
}

