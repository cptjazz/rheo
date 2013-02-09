// __expected:out_pointer_if_1(a => b)
void out_pointer_if_1(int a, int* b) {
  if (a)
    *b = a;
  else
    *b = 5;

  return b;
}

// __expected:out_pointer_if_2(a => b)
void out_pointer_if_2(int a, int* b) {
  if (a)
    *b = 4;
  else
    *b = 5;
}

// __expected:out_pointer_if_3(a => b)
void out_pointer_if_3(int a, int* b) {
  if (a)
    *b = 1;
  else
    *b = 2;
}
