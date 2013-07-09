int my_global;

// __expected:with_globals(my_global => $_retval, fu => $_retval, baz => $_retval)
int with_globals(int fu, int baz)
{
  int (*operation)(int x, int y);

  // Heuristic should find (fu, baz, my_global) => $_retval
  int y = operation(fu, baz);

  return y;
}

// __expected:with_globals_and_out_pointer(my_global => $_retval, fu => $_retval, baz => $_retval, my_global => baz, fu => baz, baz => baz)
int with_globals_and_out_pointer(int fu, int* baz)
{
  int (*operation2)(int x, int* y);

  // Heuristic should find (fu, baz, my_global) => $_retval and (fu, my_global) => baz
  int y = operation2(fu, baz);

  return y;
}
