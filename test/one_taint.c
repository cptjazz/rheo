#include <stdio.h>

typedef char bool;

// __expected:one_taint(x => $_retval)
char one_taint(char x, bool isDebug) {
  if (isDebug)
    printf("fancy debug output...");

  return x * x;
}
