int add(int x, int y) {
  return x + y;
}

int foo(int a, int b, int* c) {
  int d = a;
  int x = a + b;

  *c = 5;
  int y = 9;

  if (y)
    a++;
  else
    *c = b;

  while(x)
    b = a + *c;

  *c = add(y, 3);

  return d - b;
}
