// __expected:nest1(a => $_retval, b => $_retval)
int nest1(int a, int b) {
  if (a) {
    if (b) {
      int x = 7;
      return x;
    } else {
      int y = 8;
      return y;
    }
  } else {
    int z = 9;
    return z;
  }
}
