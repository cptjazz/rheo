int glob;
extern int eee(int a, int b);

// __expected:test_extern(c => $_retval, d => $_retval, @glob => $_retval, @glob => @glob, c => @glob, d => @glob)
int test_extern(int c, int d) {
  // Should use heuristic
  return eee(c, d);
}
