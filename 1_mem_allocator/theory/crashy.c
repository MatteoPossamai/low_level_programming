#include <stdio.h>

int dereference(int *p, int offset) {
    int value = p[offset];     // crashes here when p is NULL
    return value;
}

int compute(int seed) {
    int multiplier = seed + 1;
    int *ptr = NULL;           // bug: never initialized
    int v = dereference(ptr, seed);
    return v * multiplier;
}

int main(void) {
    int result = compute(42);
    printf("result = %d\n", result);
    return 0;
}