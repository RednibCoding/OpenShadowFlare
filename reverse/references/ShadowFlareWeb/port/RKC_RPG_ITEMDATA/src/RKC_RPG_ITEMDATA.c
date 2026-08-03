#include "RKC_RPG_ITEMDATA.h"

#include "RKC_FILE.h"
#include "RK_FUNCTION.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ITEMDATA_MAX_COUNT 1000000

static const unsigned char SBOX[256] = {
    0xbe, 0x66, 0xb3, 0x2f, 0x01, 0x6e, 0x6d, 0xc8, 0x1f, 0x98, 0xa5, 0x46, 0x76, 0x5c, 0x3d, 0x0e,
    0xaa, 0x5e, 0x9d, 0xff, 0xea, 0xa0, 0x0d, 0x4b, 0x75, 0xf6, 0x61, 0x85, 0x5d, 0xbb, 0xdc, 0xfb,
    0x8b, 0xc3, 0x4f, 0x45, 0x04, 0x90, 0x81, 0x1e, 0x6b, 0xc9, 0xd3, 0x73, 0xc6, 0xe7, 0x24, 0xba,
    0x32, 0xf3, 0xc0, 0xec, 0x57, 0xcc, 0xc4, 0xb6, 0xc1, 0xae, 0xaf, 0x88, 0xf2, 0x84, 0xce, 0x4a,
    0xfc, 0x3c, 0x9f, 0x1a, 0x56, 0xc5, 0xe2, 0xf5, 0x47, 0xd9, 0xd7, 0x8c, 0xcd, 0x97, 0xf0, 0x7b,
    0x31, 0x06, 0xe5, 0x14, 0xe6, 0xda, 0x48, 0x26, 0xac, 0x87, 0x9a, 0xd8, 0xa6, 0xeb, 0x92, 0xcf,
    0x0f, 0x94, 0x41, 0xb4, 0x74, 0x2a, 0xd1, 0x70, 0x1c, 0xd4, 0xb0, 0xc2, 0x09, 0x08, 0x16, 0x9b,
    0xfd, 0x77, 0x1d, 0x21, 0x9e, 0x36, 0x35, 0x53, 0x3e, 0xd0, 0xd5, 0x62, 0x58, 0x5f, 0x63, 0x7c,
    0xb5, 0x8d, 0x2b, 0xd2, 0x89, 0xb7, 0x99, 0xa1, 0x30, 0x65, 0x54, 0x40, 0x96, 0x71, 0xfe, 0xbf,
    0xf4, 0xa9, 0x5b, 0xf7, 0x22, 0x60, 0x5a, 0x6f, 0xfa, 0x1b, 0x79, 0xe9, 0x17, 0xb1, 0x00, 0x9c,
    0x7e, 0x52, 0x29, 0x12, 0x2c, 0x78, 0x05, 0x91, 0x55, 0xe3, 0xa2, 0xb9, 0xf8, 0x50, 0x95, 0x13,
    0x80, 0x7f, 0x11, 0x27, 0xcb, 0x37, 0x4e, 0x51, 0x15, 0xef, 0xa7, 0x72, 0x4d, 0x83, 0x49, 0xa4,
    0x69, 0xde, 0x20, 0xa3, 0x67, 0xdf, 0x10, 0x42, 0x39, 0x6c, 0x2d, 0xc7, 0x23, 0xe4, 0xdd, 0xed,
    0xd6, 0xf9, 0x59, 0xb2, 0xad, 0x6a, 0x7d, 0xbc, 0xee, 0xe0, 0x3a, 0x3f, 0xca, 0x4c, 0x25, 0x68,
    0x93, 0x18, 0x33, 0x28, 0x0b, 0x07, 0x03, 0x82, 0x02, 0x43, 0x8a, 0x86, 0xdb, 0x38, 0x34, 0x19,
    0x64, 0x2e, 0x7a, 0xab, 0xf1, 0xe8, 0x44, 0x0c, 0xb8, 0x8f, 0xa8, 0x0a, 0x8e, 0xbd, 0xe1, 0x3b,
};

static const long TAIL_SIZE[5] = {804, 764, 672, 140, 100};

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
    if (c < 0 || c > ITEMDATA_MAX_COUNT)
        r->ok = 0;
    return c;
}

static char *ReadEncryptedString(Reader *r)
{
    long len = ReadCount(r);
    if (!r->ok || len == 0)
        return NULL;
    const unsigned char *raw = Take(r, len);
    if (!raw)
        return NULL;
    char *out = (char *)malloc((size_t)len + 1);
    if (!out)
    {
        r->ok = 0;
        return NULL;
    }
    for (long i = 0; i < len; i++)
        out[i] = (char)~raw[i];
    out[len] = '\0';
    return out;
}

void RKC_RPG_ITEMDATA_Init(RKC_RPG_ITEMDATA *self)
{
    memset(self, 0, sizeof(*self));
}

static void ReleaseKind(RKC_RPG_ITEMDATA_Kind *kind)
{
    for (long i = 0; i < kind->count; i++)
    {
        free(kind->records[i].name);
        free(kind->records[i].description);
        free(kind->records[i].tail);
    }
    free(kind->records);
}

void RKC_RPG_ITEMDATA_Release(RKC_RPG_ITEMDATA *self)
{
    for (int k = 0; k < 5; k++)
        ReleaseKind(&self->kinds[k]);
    memset(self, 0, sizeof(*self));
}

static int ReadRecord(Reader *r, long tailSize, RKC_RPG_ITEMDATA_Record *out)
{
    memset(out, 0, sizeof(*out));
    out->name = ReadEncryptedString(r);
    out->description = ReadEncryptedString(r);
    if (!r->ok)
        return 0;
    const unsigned char *tail = Take(r, tailSize);
    if (!tail)
        return 0;
    out->tail = (unsigned char *)malloc((size_t)tailSize);
    if (!out->tail)
    {
        r->ok = 0;
        return 0;
    }
    memcpy(out->tail, tail, (size_t)tailSize);
    out->tailSize = tailSize;
    out->templateId = *(const int *)(tail + 4);
    return 1;
}

static int ReadKind(Reader *r, long tailSize, RKC_RPG_ITEMDATA_Kind *out)
{
    long count = ReadCount(r);
    if (!r->ok)
        return 0;

    RKC_RPG_ITEMDATA_Record *records =
        count > 0 ? (RKC_RPG_ITEMDATA_Record *)calloc((size_t)count, sizeof(RKC_RPG_ITEMDATA_Record)) : NULL;
    if (count > 0 && !records)
        return 0;

    for (long i = 0; i < count; i++)
    {
        if (!ReadRecord(r, tailSize, &records[i]))
        {
            for (long j = 0; j <= i; j++)
            {
                free(records[j].name);
                free(records[j].description);
                free(records[j].tail);
            }
            free(records);
            return 0;
        }
    }

    out->records = records;
    out->count = count;
    return 1;
}

static int ReadFromMemory(RKC_RPG_ITEMDATA *self, const unsigned char *src, unsigned long srcSize)
{
    if (srcSize < 0x18 || memcmp(src, "SFItemDataV0000\x1A", 16) != 0)
        return 0;

    Reader header = {src, src + srcSize, 1};
    Take(&header, 16); /* magic */
    long checksum = ReadI32(&header);
    unsigned long compressed = (unsigned long)ReadI32(&header);
    if (!header.ok)
        return 0;

    unsigned char *payload = NULL;
    unsigned long payloadSize = 0;
    unsigned char *toFree = NULL;

    if (compressed == 0)
    {
        payloadSize = (unsigned long)ReadCount(&header);
        const unsigned char *raw = Take(&header, (long)payloadSize);
        if (!header.ok || !raw)
            return 0;
        payload = (unsigned char *)raw;
    }
    else
    {
        const unsigned char *subHeader = Take(&header, 8); /* "RCLIB-L\x1A" */
        long decompressedSize = ReadI32(&header);
        long compressedSize = ReadI32(&header);
        const unsigned char *compressedPayload = Take(&header, compressedSize);
        if (!header.ok || !subHeader || !compressedPayload)
            return 0;
        (void)decompressedSize;

        unsigned long lzInputSize = 16 + (unsigned long)compressedSize;
        unsigned char *lzInput = (unsigned char *)malloc(lzInputSize);
        if (!lzInput)
            return 0;
        memcpy(lzInput, subHeader, 8);
        memcpy(lzInput + 8, &decompressedSize, 4);
        memcpy(lzInput + 12, &compressedSize, 4);
        for (long i = 0; i < compressedSize; i++)
            lzInput[16 + i] = SBOX[compressedPayload[i]];

        void *decoded = NULL;
        unsigned long decodedSize = 0;
        int ok = RK_LzDecodeMemoryToMemory(lzInput, lzInputSize, &decoded, &decodedSize);
        free(lzInput);
        if (!ok)
            return 0;
        payload = (unsigned char *)decoded;
        payloadSize = decodedSize;
        toFree = payload;
    }

    long sum = 0;
    for (unsigned long i = 0; i < payloadSize; i++)
        sum += (signed char)payload[i];
    if (sum != checksum)
    {
        free(toFree);
        return 0;
    }

    Reader r = {payload, payload + payloadSize, 1};
    RKC_RPG_ITEMDATA parsed;
    RKC_RPG_ITEMDATA_Init(&parsed);
    int ok = 1;
    for (int k = 0; k < 5 && ok; k++)
        ok = ReadKind(&r, TAIL_SIZE[k], &parsed.kinds[k]);

    if (ok && r.ok && r.p == r.end)
    {
        *self = parsed;
    }
    else
    {
        RKC_RPG_ITEMDATA_Release(&parsed);
        ok = 0;
    }

    free(toFree);
    return ok;
}

int RKC_RPG_ITEMDATA_Read(RKC_RPG_ITEMDATA *self, const char *path)
{
    RKC_RPG_ITEMDATA_Release(self);

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
    return ok;
}

const RKC_RPG_ITEMDATA_Record *RKC_RPG_ITEMDATA_GetFromTemplateId(const RKC_RPG_ITEMDATA *self, int kind,
                                                                   long templateId)
{
    if (kind < 0 || kind >= 5)
        return NULL;
    const RKC_RPG_ITEMDATA_Kind *k = &self->kinds[kind];
    for (long i = 0; i < k->count; i++)
        if (k->records[i].templateId == templateId)
            return &k->records[i];
    return NULL;
}

void RKC_RPG_ITEMDATA_GetGridSize(const RKC_RPG_ITEMDATA_Record *record, long *outWidth, long *outHeight)
{
    if (!record || record->tailSize < 0x24)
    {
        *outWidth = 1;
        *outHeight = 1;
        return;
    }
    *outWidth = *(const long *)(record->tail + 0x1C);
    *outHeight = *(const long *)(record->tail + 0x20);
}

long RKC_RPG_ITEMDATA_GetAffixTier(const RKC_RPG_ITEMDATA_Record *record)
{
    if (!record || record->tailSize < 0x0C)
        return 0;
    return *(const long *)(record->tail + 0x08);
}

int RKC_RPG_ITEMDATA_GetGroundSprite(const RKC_RPG_ITEMDATA_Record *record, long *outFolder, long *outFrame)
{
    if (!record || record->tailSize < 0x38)
        return 0;
    *outFolder = *(const long *)(record->tail + 0x30);
    *outFrame = *(const long *)(record->tail + 0x34);
    return 1;
}

int RKC_RPG_ITEMDATA_GetBagIcon(const RKC_RPG_ITEMDATA_Record *record, long *outSheet, long *outPattern)
{
    if (!record || record->tailSize < 0x30)
        return 0;
    *outSheet = *(const long *)(record->tail + 0x28);
    *outPattern = *(const long *)(record->tail + 0x2C);
    return 1;
}

static int RollTableOffsets(long tailSize, long *rt1, long *rt2)
{
    if (tailSize == 804)
    {
        *rt1 = 0xF0;
        *rt2 = 0x2C4;
        return 1;
    }
    if (tailSize == 764)
    {
        *rt1 = 0xC8;
        *rt2 = 0x29C;
        return 1;
    }
    if (tailSize == 672)
    {
        *rt1 = 0x6C;
        *rt2 = 0x240;
        return 1;
    }
    return 0;
}

int RKC_RPG_ITEMDATA_GetResistancesFromTail(const void *tail, long tailSize, long out[8])
{
    for (int i = 0; i < 8; i++)
        out[i] = 0;
    if (!tail)
        return 0;
    long off;
    if (tailSize == 804)
        off = 0xD0; /* kind0 weapon */
    else if (tailSize == 764)
        off = 0xA8; /* kind1 armor */
    else
        return 0; /* kind2/3/4: no resistance block */
    const unsigned char *t = (const unsigned char *)tail;
    for (int i = 0; i < 8; i++)
        out[i] = *(const long *)(t + off + (long)i * 4);
    return 1;
}

long RKC_RPG_ITEMDATA_GetBonusFromTail(const void *tail, long tailSize, int index)
{
    long rt1, rt2;
    if (!tail || index < 0 || index >= 39 || !RollTableOffsets(tailSize, &rt1, &rt2))
        return 0;
    const unsigned char *t = (const unsigned char *)tail;
    return *(const long *)(t + rt1 + (long)index * 12); /* entry.value */
}

int RKC_RPG_ITEMDATA_GetTooltipStatsFromTail(const void *tail, long tailSize, RKC_RPG_ITEMDATA_TooltipStats *out)
{
    if (!out)
        return 0;
    RKC_RPG_ITEMDATA_TooltipStats z = {0};
    *out = z;
    if (!tail || tailSize < 0x98)
        return 0;
    const unsigned char *t = (const unsigned char *)tail;
    out->weight = *(const long *)(t + 0x24);
    out->durabilityMax = *(const long *)(t + 0x64);
    out->attack = *(const long *)(t + 0x68);
    out->hitRate = *(const long *)(t + 0x6C);
    out->defense = *(const long *)(t + 0x70);
    out->evasionRate = *(const long *)(t + 0x74);
    out->magicalAttack = *(const long *)(t + 0x78);
    out->magicalHitRate = *(const long *)(t + 0x7C);
    out->magicalDefense = *(const long *)(t + 0x80);
    out->magicalEvasionRate = *(const long *)(t + 0x84);
    out->speedOfAttack = *(const long *)(t + 0x88);
    out->requiredLevel = *(const long *)(t + 0x94); 
    return 1;
}

int RKC_RPG_ITEMDATA_GetTooltipStats(const RKC_RPG_ITEMDATA_Record *record, RKC_RPG_ITEMDATA_TooltipStats *out)
{
    if (!record)
    {
        if (out)
        {
            RKC_RPG_ITEMDATA_TooltipStats z = {0};
            *out = z;
        }
        return 0;
    }
    return RKC_RPG_ITEMDATA_GetTooltipStatsFromTail(record->tail, record->tailSize, out);
}

typedef char RKC_RPG_ITEMDATA_Kind0Tail_SizeCheck[(sizeof(RKC_RPG_ITEMDATA_Kind0Tail) == 804) ? 1 : -1];

const RKC_RPG_ITEMDATA_Kind0Tail *RKC_RPG_ITEMDATA_GetKind0Tail(const RKC_RPG_ITEMDATA_Record *record)
{
    if (!record || record->tailSize != 804)
        return NULL;
    return (const RKC_RPG_ITEMDATA_Kind0Tail *)record->tail;
}

typedef char RKC_RPG_ITEMDATA_Kind1Tail_SizeCheck[(sizeof(RKC_RPG_ITEMDATA_Kind1Tail) == 764) ? 1 : -1];
typedef char RKC_RPG_ITEMDATA_Kind2Tail_SizeCheck[(sizeof(RKC_RPG_ITEMDATA_Kind2Tail) == 672) ? 1 : -1];

const RKC_RPG_ITEMDATA_Kind1Tail *RKC_RPG_ITEMDATA_GetKind1Tail(const RKC_RPG_ITEMDATA_Record *record)
{
    if (!record || record->tailSize != 764)
        return NULL;
    return (const RKC_RPG_ITEMDATA_Kind1Tail *)record->tail;
}

const RKC_RPG_ITEMDATA_Kind2Tail *RKC_RPG_ITEMDATA_GetKind2Tail(const RKC_RPG_ITEMDATA_Record *record)
{
    if (!record || record->tailSize != 672)
        return NULL;
    return (const RKC_RPG_ITEMDATA_Kind2Tail *)record->tail;
}

static void RollEntry(RKC_RPG_ITEMDATA_RollEntry *e)
{
    if (rand() % 100 < e->chancePercent)
    {
        if (e->max != e->value)
            e->value = rand() % (e->max - e->value + 1) + e->value;
    }
    else
    {
        e->value = 0;
    }
}

void RKC_RPG_ITEMDATA_RollKind0Instance(const RKC_RPG_ITEMDATA_Kind0Tail *tmpl, RKC_RPG_ITEMDATA_Kind0Instance *out)
{
    out->tail = *tmpl;
    for (int i = 0; i < 39; i++)
        RollEntry(&out->tail.rollTable1[i]);
    for (int i = 0; i < 8; i++)
        RollEntry(&out->tail.rollTable2[i]);
    out->durability = tmpl->durabilityMax;
    out->identified = 0;
}

void RKC_RPG_ITEMDATA_RollKind1Instance(const RKC_RPG_ITEMDATA_Kind1Tail *tmpl, RKC_RPG_ITEMDATA_Kind1Instance *out)
{
    out->tail = *tmpl;
    for (int i = 0; i < 39; i++)
        RollEntry(&out->tail.rollTable1[i]);
    for (int i = 0; i < 8; i++)
        RollEntry(&out->tail.rollTable2[i]);
    out->durability = tmpl->durabilityMax;
}

void RKC_RPG_ITEMDATA_RollKind2Instance(const RKC_RPG_ITEMDATA_Kind2Tail *tmpl, RKC_RPG_ITEMDATA_Kind2Instance *out)
{
    out->tail = *tmpl;
    for (int i = 0; i < 39; i++)
        RollEntry(&out->tail.rollTable1[i]);
    for (int i = 0; i < 8; i++)
        RollEntry(&out->tail.rollTable2[i]);
}
