// __expected:const_expr()
int const_expr(int x) {
  char false = 0;

  if (false)
    return x;
  else
    return 5;
}

/*
// __expected:const_expr2()
int const_expr2(int x) {
  int a = 1;
  int b = -1;

  if (a + b)
    return x;
  else
    return 5;
}
*/
/*
// __expected:const_arith()
int const_arith(int x) {
  if (5 - 5)
    return x;
  else
    return 5;
}

// __expected:const_arith2(x => $_retval)
int const_arith2(int x) {
  if (1 * 7 + 2 / 3 + 8)
    return x;
  else
    return 5;
}
*/