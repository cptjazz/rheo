// __expected:for_loop_1(a => c)
void for_loop_1(int a, int b, double* c) {
  int tmp_val_0 = 0;

  for (int i; i < a; i++) {
    tmp_val_0 += 3;
  }

  *c = tmp_val_0;
}

// __expected:for_loop_2(b => c)
void for_loop_2(int a, int b, double* c) {
  int tmp_val_0 = 0;

  for (int i = b; i < 100; i++) {
    tmp_val_0 += 3;
  }

  *c = tmp_val_0;
}

// __expected:for_loop_3(a => c)
void for_loop_3(int a, int b, double* c) {
  int tmp_val_0 = 0;

  for (int i = 0; i < 100; i+=a) {
    tmp_val_0 += 3;
  }

  *c = tmp_val_0;
}
