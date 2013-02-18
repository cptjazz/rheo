// __expected:f1(a => $_retval)
int f1(int a, int b) {
  return a;
}

// __expected:f2(c => $_retval)
int f2(int c, int d) {
  return f1(c, d);
}
