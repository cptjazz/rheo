
extern int my_external_func(int a, int b, int* c, float* d);

// __expected:depends_on_undef_external(x => $_retval, x => out1, y => $_retval, y => out1, a => $_retval, a => out1, b => $_retval, b => out1)
int depends_on_undef_external(int x, int y, double a, double b, int* out1, float* out2) {
  int v;
  float* w;

  v = (int) a;
  *w = b;
  // Here, Clang notices that `w` is
  // undefined, so dereferencing it has
  // strange behaviour.

  int r = my_external_func(x, y, &v, w);

  *out1 = v;
  *out2 = (float) *w;
  // Copying over from `w` here is a copy
  // from LLVM's 'undef' value and results
  // in no taint-flow to out2

  return r;
}

// __expected:depends_on_undef_external_2(x => $_retval, x => out1, x => out2, y => $_retval, y => out1, y => out2, a => $_retval, a => out1, a => out2, b => $_retval, b => out1, b => out2)
int depends_on_undef_external_2(int x, int y, double a, double b, int* out1, float* out2) {
  int v;
  float asdf;
  float* w = &asdf;
  // Now the dereferenced `w` below is no 
  // longer 'undef' which results in taint-flow
  // to out2.

  v = (int) a;
  *w = b;

  int r = my_external_func(x, y, &v, w);

  *out1 = v;
  *out2 = (float) *w;

  return r;
}

