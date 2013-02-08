// __expected:o2o(a => b)
void o2o(int* a, double* b) {
  *b = (int)*a;
}


// __expected:o2o2(d => a, d => $_retval)
char o2o2(int* a, double* b, char c, float* d) {
  *a = (int)*d;
  c = (*b > 9);
  return (char)d;
}
