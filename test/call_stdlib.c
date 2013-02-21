#include <math.h>

// __expected:call_fabs(val => $_retval)
double call_fabs(int val) {
  double x = fabs(val);
  // __define:fabs(0 => $_retval)
  return x;
}

// __expected:triangle(a => result, b => result)
void triangle(double a, double b, double* result) {
  double c = hypot(a, b);
  // __define:hypot(0 => $_retval, 1 => $_retval)
  *result = c;
}
