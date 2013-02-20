#include <math.h>

// __expected:call_fabs(val => $_retval)
double call_fabs(int val) {
  double x = fabs(val);
  return x;
}

// _-_expected:triangle(a => result, b => result)
/*void triangle(double a, double b, double* result) {
  double c = hypot(a, b);
  *result = c;
}*/
