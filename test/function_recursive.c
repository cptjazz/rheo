#include <stdio.h>

// __expected.fibonacci(n => $_retval)
int fibonacci(int n)
{
   if (n == 0)
      return 0;
   else if (n == 1)
      return 1;
   else
      return fibonacci(n - 1) + fibonacci(n - 2);
} 

void call_fib() {
  int f = fibonacci(10);
  printf("fib of 10 is: %d", f);
}
