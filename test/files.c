#include <stdio.h>
#include <string.h>

// __expected:something(x => $_retval, +fopen_FILE@files.c:6 => $_retval, +fopen_FILE@files.c:6 => +fopen_FILE@files.c:6)
int something(int x) {
  FILE* f = fopen("/dev/random", "r");
  int result;
  fread((void*)&result, sizeof(char), 1, f);
  fclose(f);

  result += x;
  return result;
}

// __expected:read_file(filename => filename, filename => $_retval, +fopen_FILE@files.c:17 => $_retval, +fopen_FILE@files.c:17 => +fopen_FILE@files.c:17, filename => +fopen_FILE@files.c:17)
int read_file(const char* filename) {
  FILE* f = fopen(filename, "r");
  char result[10];
  fread((void*)result, sizeof(char), 10, f);
  fclose(f);

  return result[0];
}

// __expected:write_file(+fopen_FILE@files.c:27 => +fopen_FILE@files.c:27, +fopen_FILE@files.c:27 => some_text, filename => +fopen_FILE@files.c:27, filename => filename, filename => some_text, some_text => +fopen_FILE@files.c:27, some_text => some_text)
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

  int res = read_file("/dev/random");
  printf("readfile result: %d\n", res);
  char buf[1];
  buf[0] = (char)res;

  return write_file("/dev/null", buf);
}

