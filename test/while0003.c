//============================================================================
//
// ---=== while0003.c ===---
// Tests information flow in while loop. Specifically constructed to prevent
// compiler from figuring out what the loop does. Consequently, even "works"
// with compiler optimization (-O2) turned on.
//
// ---------------------------------------------------------------------------
// Flags
// OL = 2
//============================================================================

#include <stdio.h>
#include <limits.h>

int cpy = 23;
int global_base_ = 0;

// __expected:copy(base => global_base_, a => global_base_, base => $_retval, a => $_retval, global_base_ => global_base_)
int copy(int base, int a) {
  //int i = INT_MIN;
  while (base < a) {
    base -= 1;
    global_base_++;
  }
  return base;
}

// __expected:main(cpy => global_base_, global_base_ => global_base_)
int main() {
  int base = 0;
  // __define:scanf()
  scanf("Enter base: %d\n", &base);
  global_base_ = base;
  int cpyd = copy(base, cpy);
  // __define:printf(1 => 0, 2 => 0)
  printf("copy(%d)=%d\n", cpy, cpyd);
  return 0;
}

