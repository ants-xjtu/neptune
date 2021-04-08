#ifndef NEPTUNE_TIANGOU_H
#define NEPTUNE_TIANGOU_H

#include <stddef.h>

typedef struct
{
    void *(*malloc)(size_t);
    void *(*realloc)(void *, size_t);
    void *(*calloc)(size_t, size_t);
    void (*free)(void *);
} Interface;

#endif