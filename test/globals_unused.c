int c = 0;

// __expected:not_using_global(a => $_retval, b => $_retval, @c => @c)
int not_using_global(int a, int b) {
  return a + b;
}

// __expected:reading_global(a => $_retval, b => $_retval, @c => $_retval, @c => @c)
int reading_global(int a, int b) {
  return a + b + c;
}

// __expected:writing_global(a => @c, b => @c)
void writing_global(int a, int b) {
  c = a + b;
}
