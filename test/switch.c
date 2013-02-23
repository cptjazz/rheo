// __expected:switch1(a => $_retval)
int switch1(int a, int b) {
  int y = 5;
  int x;

  switch (y) {
    case 1:
      x = a;
    case 2:
      x = a + 5;
    default:
      x = 0;
  }

  return x;
}
