#ifndef PARROT_BOT_VECTOR_H
#define PARROT_BOT_VECTOR_H

#define VECTOR_DEFAULT_CAPACITY 8

#include <stdlib.h>

struct vector {
    char** data;
    size_t size, capacity;
};

struct vector* vector_create();
void vector_push(struct vector* vector, char* value);
void vector_destroy(struct vector* vector);

#endif  // PARROT_BOT_VECTOR_H
