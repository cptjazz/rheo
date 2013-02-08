// __expected:simple_if_1(a => $_retval)
int simple_if_1(int a) {
  int b;

  if (a)
    b = a;
  else
    b = 5;

  return b;
}
