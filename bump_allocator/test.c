#include<stdio.h>
#include "allocator.h"

int alloc_init(size_t);
int alloc_deinit();
void* alloc_malloc(size_t);
int alloc_free(void *);

int test_1(){
    int res = alloc_init(4096);
    if (res != 0){
        printf("1. Allocation valid does not work properly");
        return 1;
    }
    res = alloc_deinit();
    if (res != 0){
        printf("1. De-llocation valid does not work properly");
        return 1;
    }
    printf("Test 1 - Success\n");
    return 0;
}

int test_2(){
    int res = alloc_deinit();
    if (res != 1){
        printf("2. De-llocation invalid unallocated is not working");
        return 1;
    }
    printf("Test 2 - Success\n");
    return 0;
}

int test_3(){
    int res = alloc_init(4096);
    int* memory = (int *)alloc_malloc(sizeof(int));
    if (memory == 0){
        printf("3. Unable to allocate valid memory");
        return 1;
    }
    *memory = 5;
    if (*memory != (int)5){
        printf("3. Incorrect value in memory");
        return 1;
    }
    alloc_deinit();
    printf("Test 3 - Success\n");
    return 0;
}

int test_4(){
    alloc_init(4096);
    int* val1 = (int *)alloc_malloc(sizeof(int));
    int* val2 = (int *)alloc_malloc(sizeof(int));
    int* val3 = (int *)alloc_malloc(sizeof(int));

    if (val1 + 1 != val2 || val2 + 1 != val3) {
        printf("4. Invalid layout");
        return 1;
    }

    *val1 = 1;
    *val2 = 2;
    *val3 = 3;
    
    if (*val1 != 1 || *val2 != 2 || *val3 != 3){
        printf("4. Invalid values in the address");
        return 1;
    }

    alloc_deinit();
    printf("Test 4 - Success\n");
    return 0;
}

int test_5(){
    int* val1 = (int *)alloc_malloc(sizeof(int));
    if (val1 != 0){
        printf("5. Cannot allocate before initializing");
        return 1;
    }
    printf("Test 5 - Success\n");
    return 0;
}

int test_6(){
    alloc_init(1);
    int* val1 = (int *)alloc_malloc(sizeof(int));

    if (val1 != 0) {
        printf("6. Should not initialize over dimension of buffer");
        return 1;
    }

    alloc_deinit();
    printf("Test 6 - Success\n");
    return 0;
}

int test_7(){
    alloc_init(sizeof(int));
    int* val1 = (int *)alloc_malloc(sizeof(int));

    if (val1 == 0) {
        printf("7. Should initialize correctly");
        return 1;
    }
    char* val2 = (char *)alloc_malloc(sizeof(char));

    if (val2 != 0) {
        printf("7. Should not initialize over dimension of buffer");
        return 1;
    }

    alloc_deinit();
    printf("Test 7 - Success\n");
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
    test_7();
    fflush(stdout);
}