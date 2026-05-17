#include<stdio.h>
#include "allocator.h"

int test_1(){
    Arena arena = {0};
    int res = alloc_init(&arena, 4096);
    if (res != 0){
        printf("1. Allocation valid does not work properly\n");
        return 1;
    }
    res = alloc_deinit(&arena);
    if (res != 0){
        printf("1. De-llocation valid does not work properly\n");
        return 1;
    }
    printf("Test 1 - Success\n");
    return 0;
}

int test_2(){
    Arena arena = {0};
    int res = alloc_deinit(&arena);
    if (res != 1){
        printf("2. De-llocation invalid unallocated is not working\n");
        return 1;
    }
    printf("Test 2 - Success\n");
    return 0;
}

int test_3(){
    Arena arena = {0};
    if (alloc_init(&arena, 4096) != 0){
        printf("3. Init failed\n");
        return 1;
    }
    int* memory = (int *)alloc_malloc(&arena, sizeof(int));
    if (memory == 0){
        printf("3. Unable to allocate valid memory\n");
        return 1;
    }
    *memory = 5;
    if (*memory != (int)5){
        printf("3. Incorrect value in memory\n");
        return 1;
    }
    alloc_deinit(&arena);
    printf("Test 3 - Success\n");
    return 0;
}

int test_4(){
    Arena arena = {0};
    if (alloc_init(&arena, 4096) != 0){
        printf("4. Init failed\n");
        return 1;
    }
    int* val1 = (int *)alloc_malloc(&arena, sizeof(int));
    int* val2 = (int *)alloc_malloc(&arena, sizeof(int));
    int* val3 = (int *)alloc_malloc(&arena, sizeof(int));
    int diff = ALIGN / sizeof(int);

    if (val1 + diff != val2 || val2 + diff != val3) {
        printf("4. Invalid layout\n");
        return 1;
    }

    *val1 = 1;
    *val2 = 2;
    *val3 = 3;

    if (*val1 != 1 || *val2 != 2 || *val3 != 3){
        printf("4. Invalid values in the address\n");
        return 1;
    }

    alloc_deinit(&arena);
    printf("Test 4 - Success\n");
    return 0;
}

int test_5(){
    Arena arena = {0};
    if (alloc_init(&arena, 1) != 0){
        printf("6. Init failed\n");
        return 1;
    }
    int* val1 = (int *)alloc_malloc(&arena, sizeof(int));

    if (val1 != 0) {
        printf("6. Should not initialize over dimension of buffer");
        return 1;
    }

    alloc_deinit(&arena);
    printf("Test 5 - Success\n");
    return 0;
}

int test_6(){
    Arena arena = {0};
    if (alloc_init(&arena, ALIGN) != 0){
        printf("6. Init failed\n");
        return 1;
    }
    char* val1 = (char *)alloc_malloc(&arena, sizeof(char));

    if (val1 == 0) {
        printf("6. Should initialize correctly");
        return 1;
    }
    char* val2 = (char *)alloc_malloc(&arena, sizeof(char));

    if (val2 != 0) {
        printf("6. Should not initialize over dimension of buffer\n");
        return 1;
    }
    alloc_reset(&arena);
    int* val3 = (int*)alloc_malloc(&arena, sizeof(int));
    if (val3 == 0){
        printf("6. After restart should have available space\n");
        return 1;
    }

    alloc_deinit(&arena);
    printf("Test 6 - Success\n");
    return 0;
}

int main(){
    test_1();
    fflush(stdout);
    test_2();
    fflush(stdout);
    test_3();
    fflush(stdout);
    test_4();
    fflush(stdout);
    test_5();
    fflush(stdout);
    test_6();
    fflush(stdout);
}