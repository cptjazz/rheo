// __expected:gcd(a => $_retval, b => $_retval)
int gcd(int a, int b) {
  int result;

  if (b == 0) {
    result = a;
  } else {
    result = gcd(b, a % b);
  }

  return result;
}
