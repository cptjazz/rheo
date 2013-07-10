int my_global;

// __expected:some_operation(x => $_retval, y => $_retval, @my_global => @my_global)
int some_operation(int x, int y) {
  return x + y;
}

// __expected:some_other_operation(x => $_retval, y => $_retval, @my_global => @my_global)
int some_other_operation(int x, int y) {
  return x + y;
}

// __expected:some_operation2(x => $_retval, y => $_retval, @my_global => @my_global, y => y)
int some_operation2(int x, int* y) {
  return x + *y;
}

// __expected:some_other_operation2(x => $_retval, y => $_retval, @my_global => @my_global, y => y)
int some_other_operation2(int x, int* y) {
  return x + *y;
}

// __expected:with_globals(@my_global => $_retval, fu => $_retval, baz => $_retval, @my_global => @my_global, fu => @my_global, baz => @my_global)
int with_globals(int fu, int baz)
{
  int (*operation)(int x, int y);

  operation = (fu < 0) ? some_operation : some_other_operation;

  int y = operation(fu, baz);

  return y;
}

// __expected:with_globals_undef(@my_global => $_retval, fu => $_retval, baz => $_retval, @my_global => @my_global, fu => @my_global, baz => @my_global)
int with_globals_undef(int fu, int baz)
{
  int (*operation)(int x, int y);

  // Not function was bound to `operation`.
  // The compiler detects this and issues a call
  // to 'undef'. No taint-flow arises due to 
  // undefined behaviour.
  int y = operation(fu, baz);

  return y;
}

// __expected:with_globals_and_out_pointer(@my_global => $_retval, fu => $_retval, baz => $_retval, @my_global => baz, fu => baz, baz => baz, @my_global => @my_global, fu => @my_global, baz => @my_global)
int with_globals_and_out_pointer(int fu, int* baz)
{
  int (*operation2)(int x, int* y);

  operation2 = (fu < 0) ? some_operation2 : some_other_operation2;

  int y = operation2(fu, baz);

  return y;
}
