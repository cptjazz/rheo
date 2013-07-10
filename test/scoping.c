#include <stdio.h>

int a;

// __expected:foo(b => $_retval, b => @a)
int foo(int a, int b) {
  extern int a;
  a = b;
  return a;
}

// __expected:bar(d => @a, d => $_retval)
int bar(int c, int d) {
  int x = foo(c, d);
  // __define:printf(1 => 0)
  printf("%d\n", a);
  return x;
}
