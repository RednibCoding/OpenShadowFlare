#ifndef SFDE_RKC_MEMORY_H
#define SFDE_RKC_MEMORY_H

typedef struct RKC_MEMORY
{
    char *data;
    long size;
} RKC_MEMORY;

void RKC_MEMORY_Init(RKC_MEMORY *self);
char *RKC_MEMORY_Allocation(RKC_MEMORY *self, long size, int flag);
int RKC_MEMORY_Clear(RKC_MEMORY *self, char value, long offset, long size);
int RKC_MEMORY_Copy(RKC_MEMORY *self, const char *src, long offset, long size);
char *RKC_MEMORY_Get(RKC_MEMORY *self);
long RKC_MEMORY_GetSize(RKC_MEMORY *self);
void RKC_MEMORY_Release(RKC_MEMORY *self);

#endif
