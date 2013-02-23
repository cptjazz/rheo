// __expected:switch1(a => $_retval)
int switch1(int a, int b) {
  int y = 5;
  int x;

  switch (y) {
    case 1:
      x = a;
      break;
    case 2:
      x = a + 5;
      break;
    default:
      x = 0;
      break;
  }

  return x;
}

// __expected:tainted_condition(a => $_retval)
int tainted_condition(int a) {
  switch (a) {
    case 1:
      return 1;
      break;

    case 2:
      return 2;
      break;
  }

  return 0;
}
