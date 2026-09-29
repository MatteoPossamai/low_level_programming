#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef double data_t;
#define OP    +
#define IDENT 0.0

#define N    4000      // ~32 KB of doubles -> fits in L1 cache
#define REPS 50000     // repeat each function so total time is large vs. clock noise

static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// sum_1: length hoisted, but accumulates THROUGH memory (*dest) every iteration.
void sum_1(data_t *v, int len, data_t *dest) {
    *dest = IDENT;
    for (int i = 0; i < len; i++)
        *dest = *dest OP v[i];
}

// sum_2: accumulate in a local -> breaks the store/load chain through *dest.
void sum_2(data_t *v, int len, data_t *dest) {
    data_t acc = IDENT;
    for (int i = 0; i < len; i++)
        acc = acc OP v[i];
    *dest = acc;
}

// sum_3: 2x1 loop unrolling
void sum_3(data_t *v, int len, data_t *dest) {
    data_t acc = IDENT;
    for (int i = 0; i < len; i+=2)
        acc = (acc OP v[i]) OP v[i+1];
    *dest = acc;
}

// sum_4: 2x2 loop unrolling
void sum_4(data_t *v, int len, data_t *dest) {
    data_t acc1 = IDENT, acc2 = IDENT;
    int i;
    for (i = 0; i + 1 < len; i += 2) {
        acc1 = acc1 OP v[i];
        acc2 = acc2 OP v[i + 1];
    }
    *dest = acc1 OP acc2;
}

// sum_5: 4x4 unrolling
void sum_5(data_t *v, int len, data_t *dest) {
    data_t acc1 = IDENT, acc2 = IDENT, acc3 = IDENT, acc4 = IDENT;
    int i;
    for (i = 0; i + 3 < len; i += 4) {
        acc1 = acc1 OP v[i];
        acc2 = acc2 OP v[i + 1];
        acc3 = acc3 OP v[i + 2];
        acc4 = acc4 OP v[i + 3];
    }
    *dest = (acc1 OP acc2) OP (acc3 OP acc4);
}
 
// sum_6: 2x1a loop unrolling
void sum_6(data_t *v, int len, data_t *dest) {
    data_t acc = IDENT;
    for (int i = 0; i < len; i+=2)
        acc = acc OP (v[i] OP v[i+1]);
    *dest = acc;
}
 
// sum_7: 16x4a unrolling
void sum_7(data_t *v, int len, data_t *dest) {
    data_t acc1 = IDENT, acc2 = IDENT, acc3 = IDENT, acc4 = IDENT;
    int i;
    for (i = 0; i < len; i += 16) {
        acc1 = acc1 OP (v[i] OP v[i+1] OP v[i+2] OP v[i+3]);
        acc2 = acc2 OP (v[i+4] OP v[i+5] OP v[i+6] OP v[i+7]);
        acc3 = acc3 OP (v[i+8] OP v[i+9] OP v[i+10] OP v[i+11]);
        acc4 = acc4 OP (v[i+12] OP v[i+13] OP v[i+14] OP v[i+15]);
    }
    *dest = (acc1 OP acc2) OP (acc3 OP acc4);
}

int main(void) {
    static data_t v[N];
    // Values close to 1 so repeated multiplication doesn't over/underflow.
    for (int i = 0; i < N; i++) v[i] = (i % 2) ? 1.001 : 0.999;

    data_t dest;
    volatile data_t sink;   // prevents the compiler from eliding the timed work
    double t1, t2;

    t1 = now_seconds();
    for (int k = 0; k < REPS; k++) { sum_1(v, N, &dest); sink = dest; }
    t2 = now_seconds();
    printf("Sum 1 (mem acc):  %f s\n", t2 - t1);

    t1 = now_seconds();
    for (int k = 0; k < REPS; k++) { sum_2(v, N, &dest); sink = dest; }
    t2 = now_seconds();
    printf("Sum 2 (local):    %f s\n", t2 - t1);

    t1 = now_seconds();
    for (int k = 0; k < REPS; k++) { sum_3(v, N, &dest); sink = dest; }
    t2 = now_seconds();
    printf("Sum 3 (2x1):      %f s\n", t2 - t1);

    t1 = now_seconds();
    for (int k = 0; k < REPS; k++) { sum_4(v, N, &dest); sink = dest; }
    t2 = now_seconds();
    printf("Sum 4 (2x2):      %f s\n", t2 - t1);

    t1 = now_seconds();
    for (int k = 0; k < REPS; k++) { sum_5(v, N, &dest); sink = dest; }
    t2 = now_seconds();
    printf("Sum 5 (4x2):      %f s\n", t2 - t1);

    t1 = now_seconds();
    for (int k = 0; k < REPS; k++) { sum_6(v, N, &dest); sink = dest; }
    t2 = now_seconds();
    printf("Sum 6 (2x1a):     %f s\n", t2 - t1);

    t1 = now_seconds();
    for (int k = 0; k < REPS; k++) { sum_7(v, N, &dest); sink = dest; }
    t2 = now_seconds();
    printf("Sum 7 (4x4):      %f s\n", t2 - t1);

    (void)sink;
    return 0;
}
