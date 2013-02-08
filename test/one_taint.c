#include <stdio.h>

typedef char bool;

char one_taint(char x, bool isDebug) {
  if (isDebug)
    printf("fancy debug output...");

  return x * x;
}
