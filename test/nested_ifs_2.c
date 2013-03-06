// __expected:nested_if_2(b => $_retval, f => $_retval)
int nested_if_2(int a, int b, int c, int d, int f) {
  int e;

  if (d)
    e = 0;
  else
    e = 1;

  a = 0;
  if (a) {
xxx:
    if (b) {
      int x = 7;
      if (f)
        goto yyy;

      return x;
    } else {
      int y = 8;
      return y;
    }
  } else {
    int z = 9;
yyy:
    if (f)
      goto xxx;
    return z;
  }
}
