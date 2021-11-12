#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"

void func_01(void *arg) {
    for (int i = 0; i < 100; i++) {
        printf(1, arg);
    }
    exit();
}

void func_02(void *arg) {
    for (int i = 0; i < 100; i++) {
        printf(1, arg);
    }
    exit();
}

int main(int argc, char *argv[]) {
    printf(1, "Starting test runs!\n\n");

    thread_create(func_01, "hello world 01\n");
    thread_create(func_02, "hello world 02\n");
    thread_join();
    thread_join();

    printf(1, "\n\nTest passed!\n");
    exit();
}
