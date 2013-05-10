#include <stdio.h>

// __expected:no_realisation(foo => $_retval, bar => $_retval, fu => $_retval, baz => $_retval, operation => $_retval)
int no_realisation(int foo, int bar, int fu, int baz, int (operation)(int, int))
{
  int x = operation(foo, bar);
  int y = operation(fu, baz);

  return x + y;
}
