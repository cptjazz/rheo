// __expected:switch1()
int switch1(int a, int b) {
  int y = 5;
  int x;

  switch (y) {
    // Optimized away to always use the `default' target
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

// __expected:switch2(a => $_retval)
int switch2(int a, int b) {
  int y = 2;
  int x;

  switch (y) {
    // Optimized away to always use the `2' target
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

// __expected:default_block(a => $_retval)
int default_block(int a, int* b) {
  int x;

  switch (a) {
    case 1:
      x = 11;
      break;

    case 2:
      x = 22;
      break;
  }

  // In the '0' block of switch -- but should not be tainted
  *b = 5;

  return x;
}

// __expected:default_block_2(a => $_retval, a => b, b => b)
int default_block_2(int a, int* b) {
  int x;

  switch (a) {
    case 1:
      x = 11;
      break;

    case 2:
      x = 22;
      break;

    default:
      *b = 5;
      break;
  }

  return x;
}
