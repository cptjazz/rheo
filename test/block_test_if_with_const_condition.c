#include <stdio.h>

// __expected:const_expr(x => $_retval)
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
