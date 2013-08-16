// __expected:f_1(a => a, b => b)
void f_1(int* a, int* b) {
}

// __expected:g_1(a => $_retval)
int g_1(int a, int b) {
  f_1(&a, &b);
  return a;
} 

// __expected:f_2()
void f_2(int* a, int* b) {
  *a = 0;
  *b = 0;
}


// __expected:g_2(a => $_retval)
int g_2(int a, int b) {
  f_2(&a, &b);
  return a;
}

// __expected:g(a => $_retval, b => $_retval, b => b)
int g(int a, int* b) {
  return a + *b;
}

// __expected:h(a => $_retval) 
int h(int a) {
  return a;
}

// __expected:f(a => $_retval) 
int f(int a, int b) {
  int x = g(a, &b);

  // A must not be lost, even if g() does not
  // define a => a. value types get copied over!
  return h(a);
}
