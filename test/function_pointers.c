#include <stdio.h>

int (*operation)(int x, int y);

// __expected:op1(x => $_retval, y => $_retval, @operation => @operation)
int op1(int x, int y)
{
  return x + y;
}

// __expected:op2(@operation => @operation)
int op2(int x, int y)
{
  return 33;
}


// __expected:variable_operation(foo => $_retval, bar => $_retval)
int variable_operation(int foo, int bar, int fu, int baz)
{
  operation = op1;
  // operation is known to be op1() at compile time,
  // so the call is replaced by a non-function-pointer call.
  int x = operation(foo, bar);

  operation = op2;
  // operation is known to be op2 at compile time,
  // so the call is replaced by a non-function-pointer call.
  int y = operation(fu, baz);
  // No information flow since op2 has constant result.

  return x + y;
}

// __expected:op3(x => $_retval, @operation => @operation)
int op3(int x, int y)
{
  return x;
}

// __expected:op4(y => $_retval, @operation => @operation)
int op4(int x, int y)
{
  return y;
}

// __expected:variable_operation_2(fswitch => $_retval, fu => $_retval, baz => $_retval, @operation => $_retval, fu => @operation, baz => @operation, @operation => @operation)
int variable_operation_2(int fswitch, int fu, int baz)
{
  int (*operation2)(int x, int y);

  if (fswitch) {
    operation2 = op3;
  } else {
    operation2 = op4;
  }

  // Since the function pointer `@operation` is global, this
  // global also taints our call (due to heuristics)
  int y = operation2(fu, baz);

  return y;
}
