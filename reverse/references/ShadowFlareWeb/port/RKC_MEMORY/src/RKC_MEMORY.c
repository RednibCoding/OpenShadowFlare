#include "RKC_MEMORY.h"

#include <stdlib.h>
#include <string.h>

void RKC_MEMORY_Init(RKC_MEMORY *self)
{
    self->data = NULL;
    self->size = 0;
}

char *RKC_MEMORY_Allocation(RKC_MEMORY *self, long size, int flag)
{
    RKC_MEMORY_Release(self);

    if (size <= 0)
        return NULL;

    self->data = flag ? calloc((size_t)size, 1) : malloc((size_t)size);
    if (!self->data)
        return NULL;

    self->size = size;
    return self->data;
}

int RKC_MEMORY_Clear(RKC_MEMORY *self, char value, long offset, long size)
{
    if (!self->data || offset < 0 || size < 0 || offset + size > self->size)
        return 0;

    memset(self->data + offset, value, (size_t)size);
    return 1;
}

int RKC_MEMORY_Copy(RKC_MEMORY *self, const char *src, long offset, long size)
{
    if (!self->data || !src || offset < 0 || size < 0 || offset + size > self->size)
        return 0;

    memcpy(self->data + offset, src, (size_t)size);
    return 1;
}

char *RKC_MEMORY_Get(RKC_MEMORY *self)
{
    return self->data;
}

long RKC_MEMORY_GetSize(RKC_MEMORY *self)
{
    return self->size;
}

void RKC_MEMORY_Release(RKC_MEMORY *self)
{
    free(self->data);
    self->data = NULL;
    self->size = 0;
}
