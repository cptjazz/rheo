// __expected:const_expr(x => out)
int const_expr(int x, int* out) {
  if (x)
    *out = 9;

  int y = 1234;
  return y;
}


// __expected:const_expr2(x => $_retval)
int const_expr2(int x) {
  int a = 1;
  int b = -1;

  if (a + b)
    return x;
  
  return 0;
}


// __expected:const_arith()
int const_arith(int x) {
  if (5 - 5)
    return x;

  return 0;
}

// __expected:const_arith2(x => $_retval)
int const_arith2(int x) {
  if (1 * 7 + 2 / 3 + 8)
    return x;

  return 5;
}

// __expected:const_arith3()
int const_arith3(int x) {
  if (5 - 5 * 6 + 100 / 4)
    return x;

  return 5;
}
