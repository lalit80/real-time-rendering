#include <stdio.h>
#include <stdlib.h>

#define SUCCESS     1
#define TRUE        1
#define FALSE       0

typedef int                 GLint;
typedef int                 status_t;
typedef struct vector_int   vector_int;

struct vector_int
{
    GLint* p_array;
    int size;
};

// declarations
vector_int* allocate_vector_int(void);
status_t push_back_vector_int(vector_int* p_vec_int, int new_data);
void show_vector_int(vector_int* p_vec_int);
status_t release_vector_int(vector_int* p_vec_int);


int main(void)
{
    vector_int* vec = allocate_vector_int();
    int choice;
    int num;

    while (TRUE) {
        printf("Enter more (1-y/0-n): ");
        scanf("%d", &choice);
        if (choice == 0) break;
        printf("Enter integer: ");
        scanf("%d", &num);
        push_back_vector_int(vec, num);
    }

    show_vector_int(vec);
    release_vector_int(vec);
    return 0;
}

// definitions
vector_int* allocate_vector_int(void)
{
    vector_int* p = (vector_int*)malloc(sizeof(vector_int));
    if (NULL == p) {
        fprintf(stderr, "malloc(): failed\n");
        exit(1);
    }
    p->p_array = NULL;
    p->size = 0;

    return p;
}

status_t push_back_vector_int(vector_int* p_vec_int, int new_data)
{
    p_vec_int->p_array = realloc(p_vec_int->p_array, (p_vec_int->size + 1) * sizeof(GLint));
    if (NULL == p_vec_int->p_array) {
        fprintf(stderr, "malloc(): failed\n");
        exit(1);
    }

    p_vec_int->size += 1;
    p_vec_int->p_array[p_vec_int->size - 1] = new_data;
    return (SUCCESS);
}

void show_vector_int(vector_int* p_vec_int)
{
    for(int i = 0; i < p_vec_int->size; ++i) {
        printf("p_vec_int->p_array[%d]:%d\n", i, p_vec_int->p_array[i]);
    }
}

status_t release_vector_int(vector_int* p_vec_int)
{
    free(p_vec_int->p_array);
    free(p_vec_int);
    return (SUCCESS);
}
