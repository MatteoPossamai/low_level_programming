#include <stdio.h>

int CONST = 1;

int factorial(int n) {
    int result = 1;
    for (int i = 1; i <= n; i++) result *= i;
    return result;
}

int main() {
    int x = 6;
    int f = factorial(x);
    printf("%d! = %d\n", x, f);
    return 0;
}