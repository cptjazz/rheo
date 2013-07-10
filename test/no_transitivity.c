int b;
int c;
int d;

// __expected:no_transitivity_1(a => @b, @d => @c, @d => @d)
int no_transitivity_1(int a) {
  c = d;
  b = c;
  // Overwrite
  b = a;

  return 0;
}

// __expected:no_transitivity_2(@c => @d, @b => @c, a => @b)
int no_transitivity_2(int a) {
  d = c;
  c = b;
  b = a;

  return 0;
}

// __expected:no_transitivity_3(@b => @d, @b => @c, a => @b)
int no_transitivity_3(int a) {
  c = b;
  d = c;
  b = a;

  return 0;
}

// __expected:no_transitivity_4(a => @d, a => @b)
int no_transitivity_4(int a) {
  d = a;
  a = 5;
  c = a;
  b = d;

  return 0;
}

