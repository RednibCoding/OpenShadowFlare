#include "RKC_RPGSCRN_OBJECT.h"

#include "RKC_FILE.h"

#include <stdlib.h>
#include <string.h>

void RKC_RPGSCRN_OBJECTBLOCK_Init(RKC_RPGSCRN_OBJECTBLOCK *self)
{
    memset(self, 0, sizeof(*self));
}

void RKC_RPGSCRN_OBJECTBLOCK_Release(RKC_RPGSCRN_OBJECTBLOCK *self)
{
    free(self->objects);
    memset(self, 0, sizeof(*self));
}

static void ReadRecord(const unsigned char **p, int version, long resourceBase, RKC_RPGSCRN_OBJECT_Entry *out)
{
    const unsigned char *r = *p;
    out->posX = (long)*(const int *)(r + 0x00);
    out->posY = (long)*(const int *)(r + 0x04);
    out->slotIndex = (long)*(const short *)(r + 0x08);
    if (out->slotIndex >= 0)
        out->slotIndex += resourceBase;
    out->localPattern = (long)*(const short *)(r + 0x0A);
    out->field3 = (long)*(const short *)(r + 0x0C);
    out->field4 = (long)*(const short *)(r + 0x0E);
    out->field5 = (long)*(const short *)(r + 0x10);
    out->field6 = (long)*(const short *)(r + 0x12);

    const unsigned char *tail = r + 0x14;
    if (version > 0)
    {
        out->field7 = (long)*(const short *)(r + 0x14);
        out->field8 = (long)*(const short *)(r + 0x16);
        out->field9 = (long)*(const short *)(r + 0x18);
        tail = r + 0x1A;
    }
    else
    {
        out->field7 = out->field8 = out->field9 = 0;
    }

    out->rectL = (long)*(const int *)(tail + 0x00);
    out->rectT = (long)*(const int *)(tail + 0x04);
    out->rectR = (long)*(const int *)(tail + 0x08);
    out->rectB = (long)*(const int *)(tail + 0x0C);

    *p += (version > 0) ? 42 : 36;
}

static int ReadFromMemory(RKC_RPGSCRN_OBJECTBLOCK *self, const unsigned char *src, unsigned long srcSize, long resourceBase)
{
    if (srcSize < 0x14 || memcmp(src, "RPGSCRN_OBJ", 11) != 0)
        return 0;

    char verStr[4] = {(char)src[12], (char)src[13], (char)src[14], 0};
    int version = atoi(verStr);
    long count = (long)*(const unsigned int *)(src + 0x10);
    if (count < 0)
        return 0;

    unsigned long recSize = (version > 0) ? 42 : 36;
    const unsigned char *p = src + 0x14;
    const unsigned char *end = src + srcSize;
    if ((unsigned long)(end - p) < (unsigned long)count * recSize)
        return 0;

    RKC_RPGSCRN_OBJECT_Entry *objects = count > 0 ? calloc((size_t)count, sizeof(RKC_RPGSCRN_OBJECT_Entry)) : NULL;
    if (count > 0 && !objects)
        return 0;

    for (long i = 0; i < count; i++)
        ReadRecord(&p, version, resourceBase, &objects[i]);

    self->objects = objects;
    self->count = count;
    return 1;
}

int RKC_RPGSCRN_OBJECTBLOCK_Read(RKC_RPGSCRN_OBJECTBLOCK *self, const char *path, long resourceBase)
{
    RKC_RPGSCRN_OBJECTBLOCK_Release(self);

    RKC_FILE file;
    RKC_FILE_Init(&file);
    if (!RKC_FILE_Create(&file, path, 0))
        return 0;

    long size = RKC_FILE_GetSize(&file);
    unsigned char *buf = size > 0 ? malloc((size_t)size) : NULL;
    if (!buf || !RKC_FILE_Read(&file, buf, size))
    {
        RKC_FILE_Close(&file);
        free(buf);
        return 0;
    }
    RKC_FILE_Close(&file);

    int ok = ReadFromMemory(self, buf, (unsigned long)size, resourceBase);
    free(buf);
    if (!ok)
        RKC_RPGSCRN_OBJECTBLOCK_Release(self);
    return ok;
}

long RKC_RPGSCRN_OBJECTBLOCK_GetCount(const RKC_RPGSCRN_OBJECTBLOCK *self)
{
    return self->count;
}

const RKC_RPGSCRN_OBJECT_Entry *RKC_RPGSCRN_OBJECTBLOCK_Get(const RKC_RPGSCRN_OBJECTBLOCK *self, long index)
{
    if (!self->objects || index < 0 || index >= self->count)
        return NULL;
    return &self->objects[index];
}

RKC_DIB *RKC_RPGSCRN_OBJECTBLOCK_GetIcon(const RKC_RPGSCRN_OBJECTBLOCK *self, RKC_UPDIB_SET *set, long index)
{
    const RKC_RPGSCRN_OBJECT_Entry *entry = RKC_RPGSCRN_OBJECTBLOCK_Get(self, index);
    if (!entry || entry->slotIndex < 0)
        return NULL;
    return RKC_UPDIB_SET_GetPatternIcon(set, entry->slotIndex, entry->localPattern);
}

void RKC_RPGSCRN_OBJECTBLOCK_GetOffset(const RKC_RPGSCRN_OBJECTBLOCK *self, RKC_UPDIB_SET *set, long index,
                                        long *outOffsetX, long *outOffsetY)
{
    const RKC_RPGSCRN_OBJECT_Entry *entry = RKC_RPGSCRN_OBJECTBLOCK_Get(self, index);
    if (!entry || entry->slotIndex < 0)
    {
        *outOffsetX = 0;
        *outOffsetY = 0;
        return;
    }
    RKC_UPDIB_SET_GetPatternOffset(set, entry->slotIndex, entry->localPattern, outOffsetX, outOffsetY);
}


static const RKC_RPGSCRN_OBJECT_Entry *GetValidatedShadowEntry(const RKC_RPGSCRN_OBJECTBLOCK *self, RKC_UPDIB_SET *set,
                                                                long index)
{
    const RKC_RPGSCRN_OBJECT_Entry *entry = RKC_RPGSCRN_OBJECTBLOCK_Get(self, index);
    if (!entry || entry->slotIndex < 0 || !(entry->field5 & RKC_RPGSCRN_OBJECT_STATUS_SHADOW_BIT))
        return NULL;
    RKC_UPDIB *shadowSlot = RKC_UPDIB_SET_GetSlot(set, entry->slotIndex + 1);
    if (!shadowSlot || !shadowSlot->isShadowType)
        return NULL;
    return entry;
}

RKC_DIB *RKC_RPGSCRN_OBJECTBLOCK_GetShadowIcon(const RKC_RPGSCRN_OBJECTBLOCK *self, RKC_UPDIB_SET *set, long index)
{
    const RKC_RPGSCRN_OBJECT_Entry *entry = GetValidatedShadowEntry(self, set, index);
    if (!entry)
        return NULL;
    return RKC_UPDIB_SET_GetPatternIcon(set, entry->slotIndex + 1, entry->localPattern);
}

void RKC_RPGSCRN_OBJECTBLOCK_GetShadowOffset(const RKC_RPGSCRN_OBJECTBLOCK *self, RKC_UPDIB_SET *set, long index,
                                              long *outOffsetX, long *outOffsetY)
{
    const RKC_RPGSCRN_OBJECT_Entry *entry = GetValidatedShadowEntry(self, set, index);
    if (!entry)
    {
        *outOffsetX = 0;
        *outOffsetY = 0;
        return;
    }
    RKC_UPDIB_SET_GetPatternOffset(set, entry->slotIndex + 1, entry->localPattern, outOffsetX, outOffsetY);
}
