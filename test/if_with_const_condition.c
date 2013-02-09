// __expected:bar()
int bar(int x) {
  char false = 0;

  if (false)
    return x;
  else
    return 5;
}
