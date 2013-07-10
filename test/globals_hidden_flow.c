int global_option, g2;

// __expected:m1(@global_option => @g2, @global_option => @global_option, @g2 => @g2)
void m1() {
  if (global_option) {
    g2 = 5;
  }
}


// __expected:hidden_flow(local_option => $_retval, @g2 => $_retval)
int hidden_flow(int local_option, int b) {
  // local_option => global_option
  global_option = local_option;

  // Taint is lost on 'local_option' and 'b'
  local_option = 5;
  b = local_option;

  // local_option => global_option => g2
  m1();

  // local_option =/> global_option
  global_option = 99;

  // local_option => g2 => b
  if (g2)
    b = 1;

  // local_option =/> g2
  g2 = 88;

  // b => ret
  return b;
}
