#ifndef SFDE_RKC_UPDIB_SET_H
#define SFDE_RKC_UPDIB_SET_H

#include "RKC_UPDIB.h"


typedef struct RKC_UPDIB_SET
{
    RKC_UPDIB *slots; /* slotCount entries, each independently RKC_UPDIB_Init'd/loadable */
    long slotCount;
} RKC_UPDIB_SET;

void RKC_UPDIB_SET_Init(RKC_UPDIB_SET *self);
void RKC_UPDIB_SET_Release(RKC_UPDIB_SET *self);

int RKC_UPDIB_SET_Create(RKC_UPDIB_SET *self, long slotCount);
int RKC_UPDIB_SET_ReadSlot(RKC_UPDIB_SET *self, long index, const char *path);

long RKC_UPDIB_SET_GetSlotCount(const RKC_UPDIB_SET *self);

RKC_UPDIB *RKC_UPDIB_SET_GetSlot(RKC_UPDIB_SET *self, long index);

RKC_DIB *RKC_UPDIB_SET_GetPatternIcon(RKC_UPDIB_SET *self, long slotIndex, long localPatternIndex);

void RKC_UPDIB_SET_GetPatternOffset(RKC_UPDIB_SET *self, long slotIndex, long localPatternIndex, long *outOffsetX,
                                     long *outOffsetY);

#endif
