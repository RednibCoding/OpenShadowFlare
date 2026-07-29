#include "RKC_RPGSCRN_MCT.h"

#include "RKC_FILE.h"

#include <stdlib.h>
#include <string.h>

#define MCT_MAX_COUNT 1000000

typedef struct
{
    const unsigned char *p, *end;
    int ok;
} Reader;

static const unsigned char *Take(Reader *r, long n)
{
    if (!r->ok || n < 0 || (unsigned long)n > (unsigned long)(r->end - r->p))
    {
        r->ok = 0;
        return NULL;
    }
    const unsigned char *cur = r->p;
    r->p += n;
    return cur;
}

static long ReadI32(Reader *r)
{
    const unsigned char *d = Take(r, 4);
    return d ? *(const int *)d : 0;
}

static long ReadCount(Reader *r)
{
    long c = ReadI32(r);
    if (c < 0 || c > MCT_MAX_COUNT)
        r->ok = 0;
    return c;
}

static void *TakeCopy(Reader *r, long n)
{
    if (n == 0)
        return NULL;
    const unsigned char *d = Take(r, n);
    if (!d)
        return NULL;
    void *copy = malloc((size_t)n);
    if (!copy)
    {
        r->ok = 0;
        return NULL;
    }
    memcpy(copy, d, (size_t)n);
    return copy;
}

static void ReleaseRecord(RKC_RPGSCRN_MCT_Record *rec)
{
    free(rec->name);
    free(rec->waypoints);
    free(rec->sub17);
    free(rec->sub18);
    free(rec->sub19);
    free(rec->sub20);
    free(rec->tail);
}

static void ReleaseRecordArray(RKC_RPGSCRN_MCT_Record *records, long count)
{
    for (long i = 0; i < count; i++)
        ReleaseRecord(&records[i]);
    free(records);
}

void RKC_RPGSCRN_MCT_Init(RKC_RPGSCRN_MCT *self)
{
    memset(self, 0, sizeof(*self));
}

void RKC_RPGSCRN_MCT_Release(RKC_RPGSCRN_MCT *self)
{
    free(self->taggedIds);
    ReleaseRecordArray(self->block1, self->block1Count);
    ReleaseRecordArray(self->block2, self->block2Count);
    ReleaseRecordArray(self->block3, self->block3Count);
    ReleaseRecordArray(self->block4, self->block4Count);
    free(self->block5);
    memset(self, 0, sizeof(*self));
}

static int ReadRecord(Reader *r, long tailSize, RKC_RPGSCRN_MCT_Record *out)
{
    memset(out, 0, sizeof(*out));

    out->sortKey = ReadI32(r);
    out->field8 = ReadI32(r);

    long nameLen = ReadCount(r);
    if (!r->ok)
        return 0;
    if (nameLen != 0)
    {
        const unsigned char *nameBytes = Take(r, nameLen);
        if (!nameBytes)
            return 0;
        out->name = (char *)malloc((size_t)nameLen + 1);
        if (!out->name)
        {
            r->ok = 0;
            return 0;
        }
        memcpy(out->name, nameBytes, (size_t)nameLen);
        out->name[nameLen] = '\0';
        ReadI32(r);
    }

    out->field158 = ReadI32(r);
    out->x = ReadI32(r);
    out->y = ReadI32(r);
    out->rectL = ReadI32(r);
    out->rectT = ReadI32(r);
    out->rectR = ReadI32(r);
    out->rectB = ReadI32(r);
    out->field15 = ReadI32(r);

    out->waypointCount = ReadCount(r);
    if (!r->ok)
        return 0;
    out->waypoints = (long *)TakeCopy(r, out->waypointCount * 4);
    if (!r->ok)
        return 0;

    long flag16 = ReadI32(r);
    if (flag16 == 1)
    {
        out->hasSubArray = 1;
        out->subCount = ReadCount(r);
        if (!r->ok)
            return 0;
        out->sub17 = (unsigned int *)TakeCopy(r, out->subCount * 4);
        out->sub18 = (unsigned short *)TakeCopy(r, out->subCount * 2);
        out->sub19 = (unsigned short *)TakeCopy(r, out->subCount * 2);
        out->sub20 = (unsigned short *)TakeCopy(r, out->subCount * 2);
        if (!r->ok)
            return 0;
    }

    out->field160 = ReadI32(r);

    out->tailSize = tailSize;
    out->tail = (unsigned char *)TakeCopy(r, tailSize);
    if (!r->ok)
        return 0;

    return 1;
}

static int ReadRecordBlock(Reader *r, long tailSize, RKC_RPGSCRN_MCT_Record **outRecords, long *outCount)
{
    long count = ReadCount(r);
    if (!r->ok)
        return 0;

    RKC_RPGSCRN_MCT_Record *records = count > 0 ? (RKC_RPGSCRN_MCT_Record *)calloc((size_t)count, sizeof(RKC_RPGSCRN_MCT_Record)) : NULL;
    if (count > 0 && !records)
        return 0;

    for (long i = 0; i < count; i++)
    {
        if (!ReadRecord(r, tailSize, &records[i]))
        {
            ReleaseRecordArray(records, i + 1);
            return 0;
        }
    }

    *outRecords = records;
    *outCount = count;
    return 1;
}

static int ReadFromMemory(RKC_RPGSCRN_MCT *self, const unsigned char *src, unsigned long srcSize)
{
    if (srcSize < 0x10 || memcmp(src, "MCED DATA v0000", 15) != 0)
        return 0;

    Reader r = {src, src + srcSize, 1};
    Take(&r, 16); /* magic */

    const unsigned char *aidPath = Take(&r, 0x104);
    const unsigned char *mapPath = Take(&r, 0x104);
    if (!aidPath || !mapPath)
        return 0;
    memcpy(self->aidPath, aidPath, 0x104);
    memcpy(self->mapPath, mapPath, 0x104);

    self->fieldC = ReadI32(&r);
    self->fieldD = ReadI32(&r);
    self->bgmIndex = ReadI32(&r);

    const unsigned char *displayName = Take(&r, 0x100);
    if (!displayName)
        return 0;
    memcpy(self->displayName, displayName, 0x100);

    RKC_RPGSCRN_MCT_TaggedId *taggedIds = NULL;
    long taggedIdCount = 0;
    for (int g = 0; g < 3; g++)
    {
        long count = ReadCount(&r);
        if (!r.ok)
        {
            free(taggedIds);
            return 0;
        }
        if (count > 0)
        {
            RKC_RPGSCRN_MCT_TaggedId *grown =
                (RKC_RPGSCRN_MCT_TaggedId *)realloc(taggedIds, (size_t)(taggedIdCount + count) * sizeof(RKC_RPGSCRN_MCT_TaggedId));
            if (!grown)
            {
                free(taggedIds);
                return 0;
            }
            taggedIds = grown;
            for (long i = 0; i < count; i++)
            {
                taggedIds[taggedIdCount + i].id = ReadI32(&r);
                taggedIds[taggedIdCount + i].groupType = g;
            }
            taggedIdCount += count;
        }
    }
    if (!r.ok)
    {
        free(taggedIds);
        return 0;
    }
    self->taggedIds = taggedIds;
    self->taggedIdCount = taggedIdCount;
    if (!r.ok)
        return 0;

    if (!ReadRecordBlock(&r, 0x34, &self->block1, &self->block1Count))
        return 0;
    if (!ReadRecordBlock(&r, 0x2c, &self->block2, &self->block2Count))
        return 0;
    if (!ReadRecordBlock(&r, 0x13c, &self->block3, &self->block3Count))
        return 0;
    if (!ReadRecordBlock(&r, 0x10, &self->block4, &self->block4Count))
        return 0;

    long block5Count = ReadCount(&r);
    if (!r.ok)
        return 0;
    self->block5 = block5Count > 0 ? (RKC_RPGSCRN_MCT_Block5Entry *)malloc((size_t)block5Count * sizeof(RKC_RPGSCRN_MCT_Block5Entry)) : NULL;
    if (block5Count > 0 && !self->block5)
        return 0;
    for (long i = 0; i < block5Count; i++)
    {
        self->block5[i].a = ReadI32(&r);
        self->block5[i].b = ReadI32(&r);
        self->block5[i].c = ReadI32(&r);
        self->block5[i].d = ReadI32(&r);
    }
    self->block5Count = block5Count;
    if (!r.ok)
        return 0;

    self->fieldG = ReadI32(&r);
    self->fieldH = ReadI32(&r);
    self->darknessIntensity = ReadI32(&r);

    return r.ok;
}

int RKC_RPGSCRN_MCT_Read(RKC_RPGSCRN_MCT *self, const char *path)
{
    RKC_RPGSCRN_MCT_Release(self);

    RKC_FILE file;
    RKC_FILE_Init(&file);
    if (!RKC_FILE_Create(&file, path, 0))
        return 0;

    long size = RKC_FILE_GetSize(&file);
    unsigned char *buf = size > 0 ? (unsigned char *)malloc((size_t)size) : NULL;
    if (!buf || !RKC_FILE_Read(&file, buf, size))
    {
        RKC_FILE_Close(&file);
        free(buf);
        return 0;
    }
    RKC_FILE_Close(&file);

    int ok = ReadFromMemory(self, buf, (unsigned long)size);
    free(buf);
    if (!ok)
        RKC_RPGSCRN_MCT_Release(self);
    return ok;
}

int RKC_RPGSCRN_MCT_GetBlock4Tail(const RKC_RPGSCRN_MCT_Record *record, RKC_RPGSCRN_MCT_Block4Tail *out)
{
    if (!record || record->tailSize != 0x10 || !record->tail)
        return 0;

    const int *fields = (const int *)record->tail;
    out->kind = fields[0];
    out->templateId = fields[1];
    out->randMin = fields[2];
    out->randMax = fields[3];
    return 1;
}

int RKC_RPGSCRN_MCT_GetBlock3AiName(const RKC_RPGSCRN_MCT_Record *record, char *out)
{
    if (!record || record->tailSize != 0x13c || !record->tail)
        return 0;

    memcpy(out, record->tail + 0x3c, 0x100);
    out[0x100] = '\0';
    return 1;
}

int RKC_RPGSCRN_MCT_GetBlock3BaseDamage(const RKC_RPGSCRN_MCT_Record *record, int actionIndex, long *out)
{
    if (!record || record->tailSize != 0x13c || !record->tail || actionIndex < 0 || actionIndex > 2)
        return 0;

    const int *fields = (const int *)(record->tail + 0x78);
    *out = fields[actionIndex];
    return 1;
}

int RKC_RPGSCRN_MCT_GetBlock3MaxHP(const RKC_RPGSCRN_MCT_Record *record, long *out)
{
    if (!record || record->tailSize != 0x13c || !record->tail)
        return 0;

    *out = *(const int *)(record->tail + 0x20);
    return 1;
}

int RKC_RPGSCRN_MCT_GetBlock3Level(const RKC_RPGSCRN_MCT_Record *record, long *out)
{
    if (!record || record->tailSize != 0x13c || !record->tail)
        return 0;

    *out = *(const int *)(record->tail + 0x34);
    return 1;
}

int RKC_RPGSCRN_MCT_GetBlock3LootTableRow(const RKC_RPGSCRN_MCT_Record *record, long *out)
{
    if (!record || record->tailSize != 0x13c || !record->tail)
        return 0;

    *out = *(const int *)(record->tail + 0x38);
    return 1;
}

int RKC_RPGSCRN_MCT_GetBlock3SpeedPercent(const RKC_RPGSCRN_MCT_Record *record, long *out)
{
    if (!record || record->tailSize != 0x13c || !record->tail)
        return 0;

    *out = *(const int *)(record->tail + 0x5c);
    return 1;
}

int RKC_RPGSCRN_MCT_GetBlock3MoveSpeedX1000(const RKC_RPGSCRN_MCT_Record *record, long *out)
{
    if (!record || record->tailSize != 0x13c || !record->tail)
        return 0;

    *out = *(const int *)(record->tail + 0x134);
    return 1;
}

int RKC_RPGSCRN_MCT_GetBlock3AttackChart(const RKC_RPGSCRN_MCT_Record *record, int actionIndex, long *out)
{
    if (!record || record->tailSize != 0x13c || !record->tail || actionIndex < 0 || actionIndex > 2)
        return 0;

    const int *fields = (const int *)(record->tail + 0x11c);
    *out = fields[actionIndex];
    return 1;
}

int RKC_RPGSCRN_MCT_GetBlock3SpeedIndex(const RKC_RPGSCRN_MCT_Record *record, int actionIndex, long *out)
{
    if (!record || record->tailSize != 0x13c || !record->tail || actionIndex < 0 || actionIndex > 2)
        return 0;

    const int *fields = (const int *)(record->tail + 0x128);
    *out = fields[actionIndex];
    return 1;
}

int RKC_RPGSCRN_MCT_GetBlock1PatternIndex(const RKC_RPGSCRN_MCT_Record *record, long *out)
{
    if (!record || record->tailSize != 0x34 || !record->tail)
        return 0;

    const int *fields = (const int *)record->tail;
    *out = fields[1];
    return 1;
}

int RKC_RPGSCRN_MCT_GetBlock1DrawFlag(const RKC_RPGSCRN_MCT_Record *record, long *out)
{
    if (!record || record->tailSize != 0x34 || !record->tail)
        return 0;

    const int *fields = (const int *)record->tail;
    *out = fields[0];
    return 1;
}

int RKC_RPGSCRN_MCT_GetBlock1Height(const RKC_RPGSCRN_MCT_Record *record, long *out)
{
    if (!record || record->tailSize != 0x34 || !record->tail)
        return 0;

    const int *fields = (const int *)record->tail;
    *out = fields[4];
    return 1;
}

int RKC_RPGSCRN_MCT_GetBlock1WalkableFlag(const RKC_RPGSCRN_MCT_Record *record, long *out)
{
    if (!record || record->tailSize != 0x34 || !record->tail)
        return 0;

    const int *fields = (const int *)record->tail;
    *out = fields[3];
    return 1;
}

int RKC_RPGSCRN_MCT_GetBlock2WanderRect(const RKC_RPGSCRN_MCT_Record *record, long *outL, long *outT, long *outR,
                                        long *outB)
{
    if (!record || record->tailSize != 0x2c || !record->tail)
        return 0;

    const int *fields = (const int *)record->tail;
    long flag = fields[3];
    long l = fields[4], t = fields[5], r = fields[6], b = fields[7]; /* tail+0x10..0x1c */
    if (flag == 0)
    {
        l += record->x;
        t += record->y;
        r += record->x;
        b += record->y;
    }
    *outL = l;
    *outT = t;
    *outR = r;
    *outB = b;
    return 1;
}

int RKC_RPGSCRN_MCT_GetBlock2FacesTalker(const RKC_RPGSCRN_MCT_Record *record)
{
    if (!record || record->tailSize != 0x2c || !record->tail)
        return 1;

    const int *fields = (const int *)record->tail;
    return fields[8] != 0;
}

int RKC_RPGSCRN_MCT_IsBlock2Stationary(const RKC_RPGSCRN_MCT_Record *record)
{
    if (!record || record->tailSize != 0x2c || !record->tail)
        return 1;

    const int *fields = (const int *)record->tail;
    return fields[9] != 0;
}

int RKC_RPGSCRN_MCT_FindEntryPoint(const RKC_RPGSCRN_MCT *self, long playerCharacterNo, long entryPoint,
                                    RKC_RPGSCRN_MCT_Block5Entry *out)
{
    long key = playerCharacterNo + entryPoint * 4;
    for (long i = 0; i < self->block5Count; i++)
    {
        if (self->block5[i].a == key)
        {
            *out = self->block5[i];
            return 1;
        }
    }
    return 0;
}
