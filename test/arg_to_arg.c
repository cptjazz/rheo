// __expected:a2a(a => $_retval, b => $_retval, c => $_retval, d => $_retval)
int a2a(int a, int b, int c, int d) {
  b = a;
  c = b + 2;
  d = a - c;

  return d;
}
