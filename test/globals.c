double PI = 3.1415;
const int E = 2;

// __expected:nasty_function(a => PI, b => PI)
void nasty_function(int a, int b) {
  if (a)
    PI = b;
  else
    PI = a << 2;
}

// __expected:friendly_function(PI => $_retval)
int friendly_function() {
  return 2 * PI;
}

// __expected:block_tainted_by_global(a => $_retval, PI => $_retval)
int block_tainted_by_global(int a) {
  if (PI > 4)
    return a;
  else
    return 0;
}

// __expected:const_globals_do_not_taint(a => $_retval)
int const_globals_do_not_taint(int a) {
  return a * E;
}

// __expected:const_globals_do_not_taint_with_pointers(a => b)
void const_globals_do_not_taint_with_pointers(int a, int* b) {
  *b = a * E;
}

// __expected:const_globals_do_not_taint_block(a => $_retval)
int const_globals_do_not_taint_block(int a) {
  int sum = 0;

  for (int i = 0; i < E; i++)
    sum += a;
 
  return sum;
}

// __expected:const_globals_do_not_taint_block_with_pointers(a => b)
void const_globals_do_not_taint_block_with_pointers(int a, int* b) {
  *b = 0;

  for (int i = 0; i < E; i++)
    *b += a;
}
