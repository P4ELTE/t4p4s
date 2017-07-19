#ifndef VECTOR_H
#define VECTOR_H

typedef struct vector_s {
    int size;
    int capacity;
    int data_size;
    void (*value_init)(void *);
    void **data;
    int socketid;
} vector_t;

void vector_init(vector_t *vector, int size, int capacity, int data_size, void (*value_init)(void *), int socketid);
void vector_append(vector_t *vector, void* value);
void*  vector_get(vector_t *vector, int index);
void vector_set(vector_t *vector, int index, void* value);
void vector_double_capacity_if_full(vector_t *vector);
void vector_free(vector_t *vector);

#endif // VECTOR_H
