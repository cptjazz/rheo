#include <stdlib.h>

// __expected:get_some_index()
int get_some_index(int i, int j) {
  return 20;
}

// __expected:create_2d_array(l => neighbours, values => neighbours, values => values, neighbours => neighbours)
void create_2d_array(short* values, short*** neighbours, int l) {
  for (int i = 0; i < l * l; i++) {
    // __define:malloc()
    neighbours[i] = (short**) malloc(sizeof(short*) * 4);

    for (int j = 0; j < 4; j++)
      neighbours[i][j] = &values[get_some_index(i, j)];
  }
}

// __expected:store_addressof(source => target, source => source, target => target)
void store_addressof(int* source, int** target) {
  target[0] = &source[1];
}

// __expected:store_addressof_double_indexing(source => target, source => source, target => target)
void store_addressof_double_indexing(int* source, int*** target) {
  target[0][1] = &source[1];
}
