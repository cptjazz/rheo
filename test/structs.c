struct point {
  int x;
  int y;
};

typedef struct point point_t;
typedef unsigned int size_t;
#define NULL 0

// __expected:point_compare(biggest => $_retval, current => $_retval, biggest => biggest, current => current)
point_t* point_compare(point_t* biggest, point_t* current) {
  if (current->x > biggest->x && current->y > biggest->y)
    return current;
  else
    return biggest;
}

// __expected:biggest_point(size => $_retval, points => $_retval, points => points, size => points)
point_t* biggest_point(size_t size, point_t* points)
{
  int i;
  point_t* biggest = NULL;

  for (i = 0; i < size; i++) {
    biggest = point_compare(biggest, points + i);
  }

  return biggest;
}
