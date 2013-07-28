#include <stdio.h>

int something(int x) {
  FILE* f = fopen("secure.txt", "r");
  int result;
  fread((void*)&result, sizeof(char), 1, f);
  fclose(f);

  result += x;
  return result;
}

int main() {
  int x = something(5);
  printf("Secure info: %d\n", x);
}
