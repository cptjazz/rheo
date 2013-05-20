// __expected:switch_default_only()
int switch_default_only(int a) {
  switch (a) {
    default:
      return 2;
      break;
  }

  return 0;
}

// __expected:switch_default_only_2(b => $_retval)
int switch_default_only_2(int a, int b) {
  switch (a) {
    default:
      return b;
      break;
  }

  return 0;
}

// __expected:switch_default_only_3(a => $_retval)
int switch_default_only_3(int a) {
  switch (a) {
    case 1:
    case 2:
      return 2;
      break;
  }

  return 0;
}

// __expected:switch_default_only_4()
int switch_default_only_4(int a) {
  switch (a) {
    // All cases DIRECTLY fall through default!
    case 1:
    case 2:
    default:
      return 2;
      break;
  }

  return 0;
}

// __expected:switch_default_only_5(a => $_retval)
int switch_default_only_5(int a) {
  int x = 0;
  int y = 0;

  switch (a) {
    // Fall-through, but not all blocks are without
    // instructions!
    case 1:
      x = 5 << 3;
    case 2:
    default:
      y = 33;
      break;
  }

  return x;
}
