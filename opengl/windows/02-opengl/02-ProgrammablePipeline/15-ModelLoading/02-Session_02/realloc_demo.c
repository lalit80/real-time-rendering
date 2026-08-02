#include <stdio.h>
#include <stdlib.h>

#define SUCCESS     1
#define TRUE        1
#define FALSE       0

void test(void);

int main(void)
{
    test();

    return 0;
}

void test(void)
{
    int *p = NULL;
    int size = 5;

    p = malloc(size * sizeof(int));
    if (!p) {
        puts("malloc error");
        exit(1);
    }

    for (int i = 0; i < size; ++i) p[i] = ((i+1) * 10);
    for (int i = 0; i < size; ++i) printf("p[%d]:%d\n", i, p[i]);

    int new_size = 12;
    int new_size_bytes = new_size * sizeof(int);
    int* p_resized = NULL;
    
    p_resized = realloc(p, new_size_bytes);
    if (!p_resized) {
        puts("malloc error");
        exit(1);
    }
    p = NULL;

    for (int i = 0; i < size; ++i) printf("p_resized[%d]:%d\n", i, p_resized[i]);
    
    for(int i = 5; i < new_size; ++i) p_resized[i] = ((i+1) * 10);

    for (int i = 5; i < new_size; ++i) printf("p_resized[%d]:%d\n", i, p_resized[i]);

    free(p_resized);
    p_resized = NULL;

}
