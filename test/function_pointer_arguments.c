// __expected:L(y => $_retval)
int L(int x, int y) {
  return y;
}

// __expected:function_with_function_pointer_arg(b => $_retval)
int function_with_function_pointer_arg(int a, int b, int (lambda)(int, int)) {
  return lambda(a, b);
}

// __expected:call_function_with_lambda(b => $_retval)
int call_function_with_lambda(int a, int b) {
  return function_with_function_pointer_arg(a, b, L);
}
