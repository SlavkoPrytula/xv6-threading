#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"

int global;

void func_01(void *arg) {
  for (int i = 0; i < 50; i++) {
	printf(1, "executing function 01\n");
  }
  exit();
}
void func_02(void *arg) {
  for (int i = 0; i < 50; i++) {
	printf(1, "executing function 02\n");
  }
  exit();
}

int main(int argc, char *argv[]) {
  thread_create(func_01, 0);
  thread_create(func_02, 0);
  thread_join();
  thread_join();

  //printf(1, "XV6_TEST_OUTPUT : global = %d i = %d\n", global, i);

  exit();
}
