#include "co.h"
#include <stdio.h>

int count = 1; // 协程之间共享



void entry(void *arg) {
  for (int i = 0; i < 1; i++) {
    printf("%s[%d]\n", (const char *)arg, count++);
    co_yield();
  }
}

int main() {
  Co* co1 = co_start("co1", entry, "a");
  Co* co2 = co_start("co2", entry, "b");
  co_wait(co1);
  co_wait(co2);
  printf("Done\n");
  return 0;
}