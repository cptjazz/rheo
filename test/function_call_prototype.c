int callee_two_taints(int, int);

// __expected:caller1(a => $_retval, b => $_retval)
int caller1(int a, int b) {
  return callee_two_taints(a, b);
}

// __expected:callee_two_taints(c => $_retval, d => $_retval)
int callee_two_taints(int c, int d) {
  return c + d;
}


