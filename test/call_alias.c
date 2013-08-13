#include <stdio.h>

// __expected:manipulate(b => a)
void manipulate(int* a, int b) {
  *a = b;
}

// __expected:test(d => $_retval)
int test(int c, int d) {
  c = 0;

  printf("c before: %d\n", c);
  manipulate(&c, d);
  printf("c after:  %d\n", c);

  return c;
}
