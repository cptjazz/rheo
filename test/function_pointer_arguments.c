// __expected:L(y => $_retval)
int L(int x, int y) {
  return y;
}

// Both arguments taint because of heuristic
// __expected:function_with_function_pointer_arg(a => $_retval, b => $_retval, lambda => $_retval, lambda => lambda)
int function_with_function_pointer_arg(int a, int b, int (lambda)(int, int)) {
  return lambda(a, b);
}

// Both arguments taint because of heuristic
// __expected:call_function_with_lambda(a => $_retval, b => $_retval)
int call_function_with_lambda(int a, int b) {
  return function_with_function_pointer_arg(a, b, L);
}
