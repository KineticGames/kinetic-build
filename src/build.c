// std
#include <stdio.h>

char *get_name() { return "build"; }

void execute(int argc, char *argv[]) {
  printf("%s\n", argv[0]);
  printf("%s\n", argv[1]);
}
