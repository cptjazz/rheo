// From: http://en.wikipedia.org/wiki/Mutual_recursion
//

#include <stdlib.h>

#define true 1
#define false 0

char isEven(int number);
char isOdd(int number);

char isEven_2(int number);
char isOdd_2(int number);

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

// __expected:isEven_2(number => $_retval)
char isEven_2(int number) {
  if (number)
    isOdd_2(abs(number) - 1);

  return 0;
}

// __expected:isOdd_2(number => $_retval)
char isOdd_2(int number) {
  if (number)
    isEven_2(abs(number) - 1);

  return 0;
}
