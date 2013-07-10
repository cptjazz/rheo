// __expected:copy_1(len => dest, src => dest, src => src, dest => dest)
void copy_1(int len, int* src, int* dest) {
  for (int i = 0; i < len; i++)
    dest[i] = src[i];
}

// __expected:copy_2(len => dest, src => dest, src => src, dest => dest)
void copy_2(int len, int* src, int* dest) {
  for (int i = 0; i < len; i++) {
    *dest = *src;
    dest++;
    src++;
  }
}

// __expected:call_array_manipulator_1(len => b, a => b, a => a, b => b)
void call_array_manipulator_1(int len, int* a, int* b) {
  copy_1(len, a, b);
}

// __expected:call_array_manipulator_2(len => b, a => b, a => a, b => b)
void call_array_manipulator_2(int len, int* a, int* b) {
  copy_1(len, a, b + 50);
}

// __expected:call_array_manipulator_3(len => b, a => b, a => a, b => b)
void call_array_manipulator_3(int len, int* a, int* b) {
  copy_2(len, a, b);
}

// __expected:call_array_manipulator_4(len => b, a => b, a => a, b => b)
void call_array_manipulator_4(int len, int* a, int* b) {
  copy_2(len, a, b + 50);
}

// __expected:call_array_manipulator_5(len => b, a => b, a => a, b => b, index => b)
void call_array_manipulator_5(int len, int* a, int* b, int index) {
  copy_2(len, a, b + (++index));
}

// __expected:call_array_manipulator_6(len => b, a => b, a => a, b => b, index => b)
void call_array_manipulator_6(int len, int* a, long* b, int index) {
  copy_2(len, a, ((int*)b) + (++index));
}
