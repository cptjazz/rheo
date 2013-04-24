#include <stdarg.h>
 
// __expected:average(count => $_retval, ... => $_retval)
double average(int count, ...)
{
  va_list ap;
  int j;
  double tot = 0;

  va_start(ap, count); //Requires the last fixed parameter (to get the address)

  for(j=0; j<count; j++)
    tot += va_arg(ap, double); //Requires the type to cast to. Increments ap to the next argument.

  va_end(ap);

  return tot / count;
}

// __expected:call_avg(a => $_retval, b => $_retval, c => $_retval)
double call_avg(int a, int b, int c) {
  return average(5, a, 4, b, 8, c);
}
