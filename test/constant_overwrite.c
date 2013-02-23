
// __expected:overwrite_1(a => $_retval)
int overwrite_1(int a) {
  int x = a;
  return x;
}

// __expected:overwrite_2()
int overwrite_2(int a) {
  int x = a;
  a = 22;
  return a;
}

// __expected:overwrite_3(a => $_retval, a => out)
int overwrite_3(int a, int* out) {
  int x = a;
  *out = x;
  return a;
}

// __expected:overwrite_4(a => out)
int overwrite_4(int a, int* out) {
  int x = a;
  a = 22;
  *out = x;
  return a;
}

// __expected:overwrite_5(a => $_retval)
int overwrite_5(int a) {
  int x;

  if (5)
    x = a;
  else
    x = 3;

  return x;
}

// __expected:overwrite_6()
int overwrite_6(int a) {
  int x;

  if (5)
    x = a;
  else
    x = 3;

  x = 4;
  return x;
}

// __expected:overwrite_7(a => $_retval)
int overwrite_7(int a) {
  int x;

  if (5)
    x = 3;
  else
    x = a;

  return x;
}

// __expected:overwrite_8()
int overwrite_8(int a) {
  int x;

  if (5)
    x = 3;
  else
    x = a;

  x = 4;
  return x;
}
