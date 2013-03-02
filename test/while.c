// __expected:loop_test_1(a => $_retval, b => $_retval)
int loop_test_1(int a, int b) {
  int c;
  char d;
  long f;

  d = 9;
  c = 0;
  f = 100;

  // a tains retval
  while (a) {
    return 6;
  }

  while (c < 20) {
    // b taints f and f taints retval
    f += b;    
  }

  return f;
}

// __expected:loop_test_2(a => $_retval, b => $_retval)
int loop_test_2(int a, int b) {
  int c;
  char d;
  long f;

  d = 9;
  c = 0;
  f = 100;

  int r = a;
  // a tains retval
  do {
    if (d > c)
      return 6;
  } while (r);

  do {
    // b taints f and f taints retval
    f += b;    
  } while (c < 20);

  return f;
}
