#include <stdarg.h>

double call_avg(int a, int b, int c);
double average(int count, ...);
void manipulate_pointers(int count, ...);
int call_manipulate(int count, int a, int b, int c);

// __expected:call_avg(a => $_retval, b => $_retval, c => $_retval)
double call_avg(int a, int b, int c) {
  return average(5, a, 4, b, 8, c);
}

// __expected:average(count => $_retval, ... => $_retval, count => ..., ... => ...)
double average(int count, ...)
{
  va_list ap;
  int j;
  double tot = 0;

  va_start(ap, count); //Requires the last fixed parameter (to get the address)

  for(j=0; j<count; j++)
    tot += va_arg(ap, double); //Requires the type to cast to. Increments ap to the next argument.

  va_end(ap);

  return tot / call_avg(j, count, 2);
}


// __expected:manipulate_pointers(count => ..., ... => ...)
void manipulate_pointers(int count, ...)
{
  va_list ap;
  int j, g = 1, h = 2;

  va_start(ap, count); //Requires the last fixed parameter (to get the address)

  for(j=0; j<count; j++) {
    int* ptr = va_arg(ap, int*); 
    *ptr = j + call_manipulate(count, j, g, h);
  }

  va_end(ap);
}


// __expected:call_manipulate(count => $_retval, a => $_retval, b => $_retval, c => $_retval)
int call_manipulate(int count, int a, int b, int c) {

  manipulate_pointers(count, &a, &b, &c);
  //printf("a: %d, b: %d, c: %d\n", a, b, c);

  return a + b + c;
}
