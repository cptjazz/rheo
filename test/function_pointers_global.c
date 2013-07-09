#include <stdio.h>

int (*operation)(int x, int y);

// __expected:op1(x => $_retval, operation => operation)
int op1(int x, int y)
{
  return x;
}

// __expected:op2(y => $_retval, operation => operation)
int op2(int x, int y)
{
  return y;
}


// __expected:set_to_op2()
void set_to_op2()
{
  operation = op2;
}

// __expected:variable_operation_global(fu => $_retval, baz => $_retval)
int variable_operation_global(int fu, int baz)
{
  operation = op1;

  // After this line, operation is op2
  set_to_op2();

  int y = operation(fu, baz);
  return y;
}

// __expected:variable_operation_global_2(fu => $_retval, baz => $_retval)
int variable_operation_global_2(int fu, int baz)
{
  operation = op1;

  // After this line, operation is op2
  // set_to_op2();
  // even without calling this, we should find the function pointer
  // alias generated in the functino. #conservatism

  int y = operation(fu, baz);
  return y;
}
