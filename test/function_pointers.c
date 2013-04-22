#include <stdio.h>

int (*operation)(int x, int y);

// __expected:op1(x => $_retval, y => $_retval)
int op1(int x, int y)
{
  return x + y;
}

// __expected:op2()
int op2(int x, int y)
{
  return 33;
}


// __expected:variable_operation(foo => $_retval, bar => $_retval)
int variable_operation(int foo, int bar, int fu, int baz)
{
  operation = op1;
  int x = operation(foo, bar);

  operation = op2;
  int y = operation(fu, baz);

  return x + y;
}
