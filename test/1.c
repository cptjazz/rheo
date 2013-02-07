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

  do
    b = a + *c;
  while(x);

l1:
    b = a + *c;
l2:
    b++;

  if (x) {
      if (!b) {
        goto l3;
      }
     goto l1;
  } else {
     goto l2;
  }

l3:

  *c = add(y, 3);

  return d - b;
}
