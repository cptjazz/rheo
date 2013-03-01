double PI = 3.1415;

// __expected:nasty_function(a => PI, b => PI)
void nasty_function(int a, int b) {
  if (a)
    PI = b;
  else
    PI = a << 2;
}
