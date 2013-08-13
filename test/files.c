#include <stdio.h>
#include <string.h>

// __expected:something(x => $_retval, +fopen_FILE => $_retval)
int something(int x) {
  FILE* f = fopen("secure.txt", "r");
  int result;
  fread((void*)&result, sizeof(char), 1, f);
  fclose(f);

  result += x;
  return result;
}

// __expected:read_file(filename => filename, filename => $_retval, +fopen_FILE => $_retval)
int read_file(const char* filename) {
  FILE* f = fopen(filename, "r");
  char result[10];
  fread((void*)result, sizeof(char), 10, f);
  fclose(f);

  return result[0];
}

// __expected:write_file()
int write_file(const char* filename, const char* some_text) {
  FILE* f = fopen(filename, "r");
  fwrite((void*)(some_text), sizeof(char), strlen(some_text), f);
  fclose(f);

  return 0;
}

// __expected:main()
int main() {
  int x = something(5);
  printf("Secure info: %d\n", x);
}

