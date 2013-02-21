// __expected:callee_two_taints(c => out, d => out)
void callee_two_taints(int c, int d, int* out) {
  *out = c + d;
}

// __expected:callee_no_taint()
void callee_no_taint(int c, int d, int* out) {
  int y = 55;
  *out = y;
}

// __expected:callee_one_taint(c => out)
void callee_one_taint(int c, int d, int* out) {
  int y = c + 1;
  *out = y;
}

// __expected:caller1(a => $_retval, b => $_retval)
int caller1(int a, int b) {
  int* result;
  callee_two_taints(a, b, result);
  return *result;
}

// __expected:caller2(b => $_retval)
int caller2(int a, int b) {
  int* x;
  callee_two_taints(a, 5, x);

  return b;
}

// __expected:caller3()
int caller3(int a, int b) {
  int* x;
  callee_two_taints(1, 5, x);
  return *x;
}

// __expected:caller4(a => $_retval)
int caller4(int a, int b) {
  int* x;
  callee_two_taints(a, 5, x);
  return *x;
}

// __expected:caller5(a => $_retval)
int caller5(int a, int b) {
  int* x;
  callee_one_taint(a, b, x);
  return *x;
}

// __expected:caller6(a => $_retval)
int caller6(int a, int b) {
  int* x;
  callee_one_taint(a, 5, x);
  return *x;
}

// __expected:caller7()
int caller7(int a, int b) {
  int* x;
  callee_one_taint(5, b, x);
  return *x;
}

// __expected:caller8()
int caller8(int a, int b) {
  int* x;
  callee_one_taint(1, 5, x);
  return *x;
}

// __expected:caller9(a => $_retval)
int caller9(int a, int b) {
  int* x;
  callee_one_taint(a, 5, x);
  return *x;
}

// __expected:caller10()
int caller10(int a, int b) {
  int* x;
  callee_two_taints(a, b, x);
  *x = 2;
  return *x;
}

// __expected:caller11()
int caller11(int a, int b) {
  int* x;
  callee_no_taint(a, b, x);
  return *x;
}

// __expected:caller12(b => $_retval)
int caller12(int a, int b) {
  int* x;
  callee_no_taint(a, 5, x);
  return b;
}

// __expected:caller13()
int caller13(int a, int b) {
  int* x;
  callee_no_taint(5, b, x);
  return *x;
}

// __expected:caller14()
int caller14(int a, int b) {
  int* x;
  callee_no_taint(1, 5, x);
  return *x;
}

// __expected:caller15()
int caller15(int a, int b) {
  int* x;
  callee_no_taint(a, 5, x);
  return *x;
}
