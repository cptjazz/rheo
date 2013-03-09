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
