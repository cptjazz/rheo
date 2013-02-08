// __expected:out_params(x => c, y => c)
void out_params(int x, int y, int* c) {
  *c = x;
  *c *= y;
}
