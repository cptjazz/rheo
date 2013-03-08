// From: http://en.wikipedia.org/wiki/Mutual_recursion
//

#include <stdlib.h>

#define true 1
#define false 0

char isEven(int number);
char isOdd(int number);

// __expected:isEven(number => $_retval)
char isEven(int number) {
  if (number == 0)
    return true;
  else
    return isOdd(abs(number) - 1);
}

// __expected:isOdd(number => $_retval)
char isOdd(int number) {
  if (number == 0)
    return false;
  else
    return isEven(abs(number) - 1);
}
