#include <stdio.h>

// __expected:const_expr()
int const_expr(int x) {
  char false = 0;
  char true = 1;

  if (false) {
    if (true) { //should start new block
      int y = 5;
      goto smthng;
      int z = 3 * y;
    } else {
      int u = 1000;
      int v = 200 * u;
    }
    return x;
  } else {
    return 5;
  }

smthng:
  return 99;
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
