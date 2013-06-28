// __expected:add(x => $_retval, y => $_retval)
int add(int x, int y) {
  return x + y;
}

// __expected:indirect(a => $_retval, b => $_retval)
int indirect(int a, int b) {
  int c = a;
  int d = b;

  int e = c;
  int f = d;

  int g = e;
  int h = f;

  int i = g;
  int j = h;

  return i + j;
}

// __expected:foo(a => $_retval, b => $_retval, b => c, a => c, c => $_retval) 
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
