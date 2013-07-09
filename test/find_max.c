// __expected:findMaxElem(list => $_retval, size => $_retval, list => list)
int findMaxElem(int* list, int size) {
  int max = list[0];

  for (unsigned int i = 1; i < size; i++) {
    if (list[i] > max)
      max = list[i];
  }

  return max;
}
