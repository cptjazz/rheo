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
