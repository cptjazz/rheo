#include <stdio.h>

int a;

int foo(int a, int b) {
  extern int a;
  a = b;
  return a;
}

int bar(int c, int d) {
  int x = foo(c, d);
  printf("%d\n", a);
  return x;
}

int main() {
  bar(1,2);
}
