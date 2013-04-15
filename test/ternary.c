#include <stdlib.h>

// __expected:ternary_1(a => $_retval)
int ternary_1(int a) {
  int result = a > 5 ? 1 : 0; 
  return result;
}

// __expected:ternary_2(a => $_retval)
int ternary_2(int a) {
  // __define:rand()
  return rand() > 0.5 ? a : 0; 
}
