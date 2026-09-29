#include<stdio.h>
#include <stdint.h>

int main(){
    int32_t x, y;
    x = INT32_MAX;
    y = INT32_MAX;
    int32_t s = x + y;

    printf("%d + %d = %d",x, y, s);
}