#include <stdarg.h>
#include <stdio.h>
 
// __expected:manipulate_pointers(count => ..., ... => ...)
void manipulate_pointers(int count, ...)
{
  va_list ap;
  int j;

  va_start(ap, count); //Requires the last fixed parameter (to get the address)

  for(j=0; j<count; j++) {
    int* ptr = va_arg(ap, int*); 
    *ptr = j + 5;
  }

  va_end(ap);
}


// __expected:call_manipulate(count => $_retval, a => $_retval, b => $_retval, c => $_retval)
int call_manipulate(int count, int a, int b, int c) {

  manipulate_pointers(count, &a, &b, &c);
  //printf("a: %d, b: %d, c: %d\n", a, b, c);

  return a + b + c;
}
