struct vec3;

struct vec3 {
  int x;
  int y;
  int z;
};

typedef struct vec3 vec3_t;


vec3_t non_pointer_foo(int a, int b) {
  vec3_t coords;

  coords.x = a;
  coords.y = b;
  coords.z = 9;
  
  return coords;
}

void pointer_foo(vec3_t* coords, int a, int b) {

  coords->x = a;
  coords->y = b;
  coords->z = 9;
}
