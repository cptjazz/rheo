// __expected:a2a(a => $_retval)
// In arguments get overwritten, so only a taints the return.
int a2a(int a, int b, int c, int d) {
  b = a;
  c = b + 2;
  d = a - c;

  return d;
}

// __expected:a2a2(a => $_retval, b => $_retval, c => $_retval)
int a2a2(int a, int b, int c) {
  int w = a;
  int x = b + 2;
  int y = a - c;

  return w + x + y;
}
