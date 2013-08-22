// __expected:foo()
int foo(int x, int y) {
  int a;
  double b;
  double* c = &b;
  double d;

  *c = x;
  b = x + y;

  d = 33;

  return d;
}
