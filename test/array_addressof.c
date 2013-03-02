#include <stdlib.h>

// __expected:get_index()
int get_index(int i, int j) {
  return 20;
}

// __expected:create_neighbours(l => neighbours, values => neighbours)
void create_neighbours(short* values, short*** neighbours, int l) {
  for (int i = 0; i < l * l; i++) {
    neighbours[i] = (short**) malloc(sizeof(short*) * 4);

    for (int j = 0; j < 4; j++)
      neighbours[i][j] = &values[get_index(i, j)];
  }
}

// __expected:store_addressof(source => target)
void store_addressof(int* source, int** target) {
  target[0] = &source[1];
}

// __expected:store_addressof_double_indexing(source => target)
void store_addressof_double_indexing(int* source, int*** target) {
  target[0][1] = &source[1];
}
