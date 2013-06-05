
extern int my_external_func(int a, int b, int* c, float* d);

// __expected:depends_on_undef_external(x => $_retval, x => out1, x => out2, y => $_retval, y => out1, y => out2, a => $_retval, a => out1, a => out2, b => $_retval, b => out1, b => out2)
int depends_on_undef_external(int x, int y, double a, double b, int* out1, float* out2) {
  int v;
  float* w;

  v = (int) a;
  *w = b;

  int r = my_external_func(x, y, &v, w);

  *out1 = v;
  *out2 = (float) *w;

  return r;
}

