// __expected:switch_goto(a => $_retval, a => c)
int switch_goto(int a, int* b, int* c) {
  int y = 5;
  int x;

  switch (a) {
    case 1:
      goto xxx;
yyy:
      *c = y;
      break;
    case 2:
xxx:
      goto yyy;
      // this assignment will never be executed
      *b = 2;
      break;
    default:
      x = 1 + y;
      break;
  }

  return x;
}

// __expected:switch_goto_2(a => $_retval, a => b)
int switch_goto_2(int a, int* b, int* c) {
  int y = 5;
  int x;

  switch (a) {
    case 1:
      goto xxx;
      // this assignment will never be executed
      *c = y;
      break;
    case 2:
      x = y + 5;
      break;
    //default:
      //x = 0;
      //break;
  }

  return x;
xxx:
  *b = 99;
  return 0;
}
