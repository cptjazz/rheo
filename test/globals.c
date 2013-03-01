double PI = 3.1415;

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
