double PI = 3.1415;
const int E = 2;
int some_global;

// __expected:nasty_function(a => PI, b => PI, some_global => some_global)
void nasty_function(int a, int b) {
  if (a)
    PI = b;
  else
    PI = a << 2;
}

// __expected:friendly_function(PI => $_retval, some_global => some_global, PI => PI)
int friendly_function() {
  return 2 * PI;
}

// __expected:block_tainted_by_global(a => $_retval, PI => $_retval, PI => PI, some_global => some_global)
int block_tainted_by_global(int a) {
  if (PI > 4)
    return a;
  else
    return 0;
}

// __expected:const_globals_do_not_taint(a => $_retval, some_global => some_global, PI => PI)
int const_globals_do_not_taint(int a) {
  return a * E;
}

// __expected:const_globals_do_not_taint_with_pointers(a => b, PI => PI, some_global => some_global)
void const_globals_do_not_taint_with_pointers(int a, int* b) {
  *b = a * E;
}

// __expected:const_globals_do_not_taint_block(a => $_retval, some_global => some_global, PI => PI)
int const_globals_do_not_taint_block(int a) {
  int sum = 0;

  for (int i = 0; i < E; i++)
    sum += a;
 
  return sum;
}

// __expected:const_globals_do_not_taint_block_with_pointers(a => b, PI => PI, some_global => some_global)
void const_globals_do_not_taint_block_with_pointers(int a, int* b) {
  *b = 0;

  for (int i = 0; i < E; i++)
    *b += a;
}

// __expected:global_flow_over_functions_1(some_global => $_retval, PI => PI, some_global => some_global)
int global_flow_over_functions_1() {
  return some_global ? 22 : 11;  
}

// __expected:global_flow_over_functions_2(some_global => $_retval, PI => PI, some_global => some_global)
int global_flow_over_functions_2(int a) {
  // The global is used in the callee, hence it 
  // taints our return value.
  int ret = global_flow_over_functions_1();
  return ret;
}

// __expected:global_flow_over_functions_3(a => some_global, a => $_retval, PI => PI)
int global_flow_over_functions_3(int a) {
  // Global is overwritten thus it does not 
  // affect the call below anymore.
  // But `a` now affects the return value because
  // the callee uses the global which now effectively is `a`
  some_global = a;  

  int ret = global_flow_over_functions_1();

  return ret;
}

// __expected:recursive_global(x => $_retval, some_global => $_retval, PI => PI, some_global => some_global)
int recursive_global(int x) {

  if (some_global && x == 0)
    return 0;

  return recursive_global(x - 1);
}
