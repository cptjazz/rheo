#include <stdio.h>

// __expected:fibonacci(n => $_retval)
int fibonacci(int n)
{
   if (n == 0)
      return 0;
   else if (n == 1)
      return 1;
   else
      return fibonacci(n - 1) + fibonacci(n - 2);
} 

// __expected:call_fib()
void call_fib() {
  int f = fibonacci(10);
  printf("fib of 10 is: %d", f);
}

// __expected:multiple_iterations(a => $_retval, b => $_retval, n => $_retval)
int multiple_iterations(double a, double b, int n) {
  int x;

  if (n == 0)
    return 0;

  x += multiple_iterations(b, n, n - 1);

  if (x)
    return 2;

  if (a)
    return 1;

  return 3;
}

// __expected:nasty_recursion(b => $_retval);
int nasty_recursion(int a, int b, int c) {
  if (b < 10)
    return 0;

  return nasty_recursion(c, b, a);
}

// __expected:nasty_recursion_2(a => $_retval, b => $_retval, c => $_retval);
int nasty_recursion_2(int a, int b, int c) {
  if (b < 10)
    return 0;

  return nasty_recursion_2(c, a, b);
}

// __expected:nasty_recursion_3(a => $_retval, c => $_retval);
int nasty_recursion_3(int a, int b, int c) {
  if (c < 10)
    return 0;

  return nasty_recursion_3(0, 0, a);
}
