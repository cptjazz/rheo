// __expected:callee_two_taints(c => $_retval, d => $_retval)
int callee_two_taints(int c, int d) {
  return c + d;
}

// __expected:callee_no_taint()
int callee_no_taint(int c, int d) {
  int y = 55;
  return y;
}

// __expected:callee_one_taint(c => $_retval)
int callee_one_taint(int c, int d) {
  int y = c + 1;
  return y;
}

// __expected:caller1(a => $_retval, b => $_retval)
int caller1(int a, int b) {
  return callee_two_taints(a, b);
}

// __expected:caller2(b => $_retval)
int caller2(int a, int b) {
  int x = callee_two_taints(a, 5);

  return b;
}

// __expected:caller3()
int caller3(int a, int b) {
  int x = callee_two_taints(1, 5);

  return x;
}

// __expected:caller4(a => $_retval)
int caller4(int a, int b) {
  int x = callee_two_taints(a, 5);

  return x;
}

// __expected:caller5(a => $_retval)
int caller5(int a, int b) {
  return callee_one_taint(a, b);
}

// __expected:caller6(a => $_retval)
int caller6(int a, int b) {
  int x = callee_one_taint(a, 5);

  return b;
}

// __expected:caller7()
int caller7(int a, int b) {
  int x = callee_one_taint(5, b);

  return b;
}

// __expected:caller8()
int caller8(int a, int b) {
  int x = callee_one_taint(1, 5);

  return x;
}

// __expected:caller9(a => $_retval)
int caller9(int a, int b) {
  int x = callee_one_taint(a, 5);

  return x;
}

// __expected:caller10()
int caller10(int a, int b) {
  int x = callee_two_taints(a, b);
  x = 2;
  return x;
}

// __expected:caller11()
int caller11(int a, int b) {
  return callee_no_taint(a, b);
}

// __expected:caller12()
int caller12(int a, int b) {
  int x = callee_no_taint(a, 5);

  return b;
}

// __expected:caller13()
int caller13(int a, int b) {
  int x = callee_no_taint(5, b);

  return b;
}

// __expected:caller14()
int caller14(int a, int b) {
  int x = callee_no_taint(1, 5);

  return x;
}

// __expected:caller15()
int caller15(int a, int b) {
  int x = callee_no_taint(a, 5);

  return x;
}
