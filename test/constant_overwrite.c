/*
// _-_expected:overwrite_1(a => $_retval)
int overwrite_1(int a) {
  int x = a;
  return x;
}

// _-_expected:overwrite_2()
int overwrite_2(int a) {
  int x = a;
  a = 22;
  return a;
}

// _-_expected:overwrite_3(a => $_retval, a => out)
int overwrite_3(int a, int* out) {
  int x = a;
  *out = x;
  return a;
}
*/

// __expected:overwrite_4(a => out)
int overwrite_4(int a, int* out) {
  int x = a;
  a = 22;
  *out = x;
  return a;
}
