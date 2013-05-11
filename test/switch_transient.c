// __expected:switch_transient(a => $_retval, a => c)
int switch_transient(int a, int b, int* c) {
  int y = 5;
  int x;

  switch (a) {
    case 1:
      goto xxx;
xxx:
      *c = y;
      break;
    case 2:
      x = y + 5;
      break;
    default:
      x = 0;
      break;
  }

  return x;
}
