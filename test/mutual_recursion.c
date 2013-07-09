// From: http://en.wikipedia.org/wiki/Mutual_recursion
//

#include <stdlib.h>

#define true 1
#define false 0

char isEven(int number);
char isOdd(int number);

char isEven_2(int number);
char isOdd_2(int number);

int mutual_first(int a, int b, int* c);
int mutual_second(int a, int* b, int c);

// __define:abs(0 => $_retval)

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
    return isOdd_2(abs(number) - 1);

  return 0;
}

// __expected:isOdd_2(number => $_retval)
char isOdd_2(int number) {
  if (number)
    return isEven_2(abs(number) - 1);

  return 0;
}

// __expected:mutual_first(a => $_retval, b => $_retval, b => c, a => c, c => c)
int mutual_first(int a, int b, int* c) {

  if (a)
    return mutual_second(a % 5, c, b);
  else
    return 0;

}

// __expected:mutual_second(a => $_retval, c => $_retval, a => b, c => b, b => b)
int mutual_second(int a, int* b, int c) {

  if (c) {
    int x = mutual_first(c, a, b);
    *b = a + x;
    return x;
  }
  else
    return 0;

}
