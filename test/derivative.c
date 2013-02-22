#include <stdio.h>
#include <math.h>

const int N = 100;
double H = 0.001;
double PI = 3.14159265;


// __expected:derive(input => output, elements => output, elements => input)
void derive(double* input, long elements, double* output) {
  for (int i = 1; i < elements - 1; i++) {
    output[i - 1] = (input[i + 1] - input[i - 1])/ (2 * H);
  }
}

// __expected:main()
int main() {
  double f[N];
  double f_prime[N - 2];

  for (int i = 0; i < N; i++) {
    f[i] = sin(i * 2 * PI / (double)N);
    // __define:sin(0 => $_retval)
  }

  derive(f, N, f_prime);
  
  for (int i = 0; i < N - 2; i++) {
    printf("%f  %f\n", i * 2 * PI / (double)N, f_prime[i]);
    // __define:printf()
  }

  return 0;
}
