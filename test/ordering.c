int a(int x);
int b(int x);
int c(int x);
int d(int x);
int e(int x);

// __expected:e()
int e(int x) {
  return 0;
}

// __expected:a()
int a(int x) {
  return e(x);
}

// __expected:b()
int b(int x) {
  return d(x);
}

// __expected:d()
int d(int x) {
  return c(x);
}

// __expected:c()
int c(int x) {
  return a(x);
}

