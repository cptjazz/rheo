// __expected:foo()
int foo(int x, int y) {
  int a;
  int b;
  double* c;
  double d;

  *c = x;
  b = x + y;

  d = 33;

  return d;
}
