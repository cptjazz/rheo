// __expected:simple_if_1(a => $_retval)
int simple_if_1(int a) {
  int b;

  if (a)
    b = a;
  else
    b = 5;

  return b;
}

// __expected:simple_if_2(a => $_retval)
int simple_if_2(int a) {
  int b;

  if (a)
    b = 4;
  else
    b = 5;

  return b;
}

// __expected:simple_if_3(a => $_retval)
int simple_if_3(int a) {
  if (a)
    return 1;
  else
    return 2;
}
