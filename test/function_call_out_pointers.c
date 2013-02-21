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

// __expected:caller1(a => out, b => out)
void caller1(int a, int b, int* out) {
  *out = callee_two_taints(a, b);
}

// __expected:caller2(b => out)
void caller2(int a, int b, int* out) {
  int x = callee_two_taints(a, 5);

  *out = b;
}

// __expected:caller3()
void caller3(int a, int b, int* out) {
  int x = callee_two_taints(1, 5);

  *out = x;
}

// __expected:caller4(a => out)
void caller4(int a, int b, int* out) {
  int x = callee_two_taints(a, 5);

  *out = x;
}

// __expected:caller5(a => out)
void caller5(int a, int b, int* out) {
  *out = callee_one_taint(a, b);
}

// __expected:caller6(a => out)
void caller6(int a, int b, int* out) {
  int x = callee_one_taint(a, 5);

  *out = x;
}

// __expected:caller7()
void caller7(int a, int b, int* out) {
  int x = callee_one_taint(5, b);

  *out = x;
}

// __expected:caller8()
void caller8(int a, int b, int* out) {
  int x = callee_one_taint(1, 5);

  *out = x;
}

// __expected:caller9(a => out)
void caller9(int a, int b, int* out) {
  int x = callee_one_taint(a, 5);

  *out = x;
}

// __expected:caller10()
void caller10(int a, int b, int* out) {
  int x = callee_two_taints(a, b);
  x = 2;
  *out = x;
}

// __expected:caller11()
void caller11(int a, int b, int* out) {
  *out = callee_no_taint(a, b);
}

// __expected:caller12(b => out)
void caller12(int a, int b, int* out) {
  int x = callee_no_taint(a, 5);

  *out = b;
}

// __expected:caller13()
void caller13(int a, int b, int* out) {
  int x = callee_no_taint(5, b);

  *out = x;
}

// __expected:caller14()
void caller14(int a, int b, int* out) {
  int x = callee_no_taint(1, 5);

  *out = x;
}

// __expected:caller15()
void caller15(int a, int b, int* out) {
  int x = callee_no_taint(a, 5);

  *out = x;
}
