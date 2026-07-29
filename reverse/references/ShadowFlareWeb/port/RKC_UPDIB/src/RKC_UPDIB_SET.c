#include "RKC_UPDIB_SET.h"

#include <stdlib.h>
#include <string.h>

void RKC_UPDIB_SET_Init(RKC_UPDIB_SET *self)
{
    memset(self, 0, sizeof(*self));
}

void RKC_UPDIB_SET_Release(RKC_UPDIB_SET *self)
{
    if (self->slots)
    {
        for (long i = 0; i < self->slotCount; i++)
            RKC_UPDIB_Release(&self->slots[i]);
        free(self->slots);
    }
    memset(self, 0, sizeof(*self));
}

int RKC_UPDIB_SET_Create(RKC_UPDIB_SET *self, long slotCount)
{
    RKC_UPDIB_SET_Release(self);

    if (slotCount <= 0)
        return 0;

    RKC_UPDIB *slots = calloc((size_t)slotCount, sizeof(RKC_UPDIB));
    if (!slots)
        return 0;
    for (long i = 0; i < slotCount; i++)
        RKC_UPDIB_Init(&slots[i]);

    self->slots = slots;
    self->slotCount = slotCount;
    return 1;
}

int RKC_UPDIB_SET_ReadSlot(RKC_UPDIB_SET *self, long index, const char *path)
{
    if (index < 0 || index >= self->slotCount)
        return 0;
    return RKC_UPDIB_Read(&self->slots[index], path);
}

long RKC_UPDIB_SET_GetSlotCount(const RKC_UPDIB_SET *self)
{
    return self->slotCount;
}

RKC_UPDIB *RKC_UPDIB_SET_GetSlot(RKC_UPDIB_SET *self, long index)
{
    if (index < 0 || index >= self->slotCount)
        return NULL;
    return &self->slots[index];
}

RKC_DIB *RKC_UPDIB_SET_GetPatternIcon(RKC_UPDIB_SET *self, long slotIndex, long localPatternIndex)
{
    RKC_UPDIB *slot = RKC_UPDIB_SET_GetSlot(self, slotIndex);
    if (!slot)
        return NULL;
    return RKC_UPDIB_GetPatternIcon(slot, localPatternIndex);
}

void RKC_UPDIB_SET_GetPatternOffset(RKC_UPDIB_SET *self, long slotIndex, long localPatternIndex, long *outOffsetX,
                                     long *outOffsetY)
{
    RKC_UPDIB *slot = RKC_UPDIB_SET_GetSlot(self, slotIndex);
    if (!slot)
    {
        *outOffsetX = 0;
        *outOffsetY = 0;
        return;
    }
    RKC_UPDIB_GetPatternOffset(slot, localPatternIndex, outOffsetX, outOffsetY);
}
