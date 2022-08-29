#include "vector.h"

struct vector* vector_create() {
    struct vector* vector = calloc(1, sizeof(*vector));

    vector->data = calloc(VECTOR_DEFAULT_CAPACITY, sizeof(*vector->data));
    vector->capacity = VECTOR_DEFAULT_CAPACITY;

    return vector;
}

void vector_push(struct vector* vector, void* value) {
    if (vector->size >= vector->capacity) {
        vector->capacity *= 2;
        vector->data = realloc(vector->data, vector->capacity);
    }

    vector->data[vector->size++] = value;
}

void vector_destroy(struct vector* vector) {
    for (int i = 0; i < vector->size; ++i) {
        if (vector->data[i]) {
            free(vector->data[i]);
        }
    }

    free(vector->data);
    free(vector);
}