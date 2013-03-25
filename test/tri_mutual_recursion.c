int some_a(int x);
int some_b(int x);
int some_c(int x);

// __expected:some_a(x => $_retval)
int some_a(int x) {
  if (x == 0)
    return 0;
  else
    return some_b(x - 1);
}

// __expected:some_b(x => $_retval)
int some_b(int x) {
  if (x == 0)
    return 0;
  else
    return some_c(x - 1);
}

// __expected:some_c(x => $_retval)
int some_c(int x) {
  if (x == 0)
    return 0;
  else
    return some_a(x - 1);
}
