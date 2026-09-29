//
// Build:
//   gcc -O0 -o bench_O0 bench.c
//   gcc -O2 -o bench_O2 bench.c
//   gcc -O3 -march=native -o bench_O3 bench.c
//
// Run each and compare.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N 4096
#define REPS 5

// ---------- Timing helper ----------
static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// ---------- Variant 1: flat array, row-major access (cache-friendly) ----------
double sum_flat_rowmajor(const double* mat, int n) {
    double total = 0;
    for (int i = 0; i < n; i++){
        for (int j = 0; j < n; j+=2){
            total = total + (mat[i * n + j] + mat[i * n + j + 1]);
        }
    }
    return total;
}

// ---------- Allocators ----------

// One big block, indexed as flat[i*n+j]
double* alloc_flat(int n) {
    double* m = malloc((size_t)n * n * sizeof(double));
    if (!m) { perror("malloc"); exit(1); }
    for (int i = 0; i < n * n; i++) m[i] = (double)(rand() % 100);
    return m;
}

typedef double (*flat_fn)(const double*, int);
typedef double (*ptr_fn)(double**, int);

double bench_flat(const char* name, flat_fn fn, const double* mat, int n) {
    volatile double warm = fn(mat, n);
    (void)warm;

    double best = 1e9;
    double result = 0;
    for (int r = 0; r < REPS; r++) {
        double t0 = now_seconds();
        result = fn(mat, n);
        double t1 = now_seconds();
        double dt = t1 - t0;
        if (dt < best) best = dt;
    }
    printf("%-30s best=%.4fs  sum=%.0f\n", name, best, result);
    return best;
}

double bench_ptr(const char* name, ptr_fn fn, double** mat, int n) {
    volatile double warm = fn(mat, n);
    (void)warm;

    double best = 1e9;
    double result = 0;
    for (int r = 0; r < REPS; r++) {
        double t0 = now_seconds();
        result = fn(mat, n);
        double t1 = now_seconds();
        double dt = t1 - t0;
        if (dt < best) best = dt;
    }
    printf("%-30s best=%.4fs  sum=%.0f\n", name, best, result);
    return best;
}

// ---------- Main ----------
int main(void) {
    srand(42); // fixed seed for reproducibility

    printf("Matrix: %d x %d doubles = %.0f MB\n",
           N, N, (double)N * N * sizeof(double) / (1024.0 * 1024.0));
    printf("Reps per benchmark: %d (reporting best time)\n\n", REPS);

    // Allocate all three layouts
    printf("Allocating flat...\n");
    double* flat = alloc_flat(N);

    printf("\n--- Benchmarks ---\n");
    double t_row   = bench_flat("flat, row-major",       sum_flat_rowmajor,    flat, N);

    return 0;
}