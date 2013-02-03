int main(void) {
  int a = 0;
  int b = 1;

  a = b + 5;

  while(a < 10)
    b=-a;

  return a;
}

int add(int x, int y) {
  return x + y;
}
