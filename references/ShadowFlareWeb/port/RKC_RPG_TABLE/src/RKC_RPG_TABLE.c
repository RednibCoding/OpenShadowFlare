#include "RKC_RPG_TABLE.h"

#include "RKC_FILE.h"
#include "RK_FUNCTION.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_MAX_COUNT 1000000

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
    if (c < 0 || c > TABLE_MAX_COUNT)
        r->ok = 0;
    return c;
}

void RKC_RPG_TABLE_Init(RKC_RPG_TABLE *self)
{
    memset(self, 0, sizeof(*self));
}

void RKC_RPG_TABLE_Release(RKC_RPG_TABLE *self)
{
    for (long i = 0; i < self->count; i++)
        free(self->tables[i].values);
    free(self->tables);
    memset(self, 0, sizeof(*self));
}

static int ReadTable(Reader *r, RKC_RPG_TABLEDATA *out)
{
    memset(out, 0, sizeof(*out));
    out->tableNo = ReadI32(r);
    out->rowCount = ReadCount(r);
    out->colCount = ReadCount(r);
    if (!r->ok)
        return 0;

    long cellCount = out->rowCount * out->colCount;
    if (cellCount < 0 || cellCount > TABLE_MAX_COUNT)
        return 0;

    out->values = cellCount > 0 ? (long *)malloc(sizeof(long) * (size_t)cellCount) : NULL;
    if (cellCount > 0 && !out->values)
        return 0;
    for (long i = 0; i < cellCount; i++)
        out->values[i] = ReadI32(r);
    if (!r->ok)
        return 0;

    for (long i = 0; i < cellCount; i++)
    {
        long len = ReadCount(r);
        if (!r->ok)
            return 0;
        if (len > 0 && !Take(r, len))
            return 0;
    }
    return 1;
}

static int ReadFromMemory(RKC_RPG_TABLE *self, const unsigned char *payload, unsigned long payloadSize)
{
    Reader r = {payload, payload + payloadSize, 1};
    long count = ReadCount(&r);
    if (!r.ok)
        return 0;

    RKC_RPG_TABLEDATA *tables = count > 0 ? (RKC_RPG_TABLEDATA *)calloc((size_t)count, sizeof(RKC_RPG_TABLEDATA)) : NULL;
    if (count > 0 && !tables)
        return 0;

    for (long i = 0; i < count; i++)
    {
        if (!ReadTable(&r, &tables[i]))
        {
            for (long j = 0; j <= i; j++)
                free(tables[j].values);
            free(tables);
            return 0;
        }
    }

    self->tables = tables;
    self->count = count;
    return 1;
}

int RKC_RPG_TABLE_Read(RKC_RPG_TABLE *self, const char *path)
{
    RKC_RPG_TABLE_Release(self);

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

    int ok = 0;
    if ((unsigned long)size >= 20 && memcmp(buf, "TABLE DATA V000\x1A", 16) == 0)
    {
        Reader header = {buf, buf + size, 1};
        Take(&header, 16); /* magic */
        unsigned long compressed = (unsigned long)ReadI32(&header);
        if (header.ok)
        {
            if (compressed == 0)
            {
                unsigned long payloadSize = (unsigned long)ReadCount(&header);
                const unsigned char *raw = Take(&header, (long)payloadSize);
                if (header.ok && raw)
                    ok = ReadFromMemory(self, raw, payloadSize);
            }
            else
            {
                void *decoded = NULL;
                unsigned long decodedSize = 0;
                unsigned long remaining = (unsigned long)size - 20;
                if (RK_LzDecodeMemoryToMemory(buf + 20, remaining, &decoded, &decodedSize))
                {
                    ok = ReadFromMemory(self, (const unsigned char *)decoded, decodedSize);
                    free(decoded);
                }
            }
        }
    }

    free(buf);
    if (!ok)
        RKC_RPG_TABLE_Release(self);
    return ok;
}

const RKC_RPG_TABLEDATA *RKC_RPG_TABLE_GetFromTableNo(const RKC_RPG_TABLE *self, long tableNo)
{
    for (long i = 0; i < self->count; i++)
        if (self->tables[i].tableNo == tableNo)
            return &self->tables[i];
    return NULL;
}

long RKC_RPG_TABLEDATA_GetValue(const RKC_RPG_TABLEDATA *self, long row, long col)
{
    if (row < 0 || row >= self->rowCount || col < 0 || col >= self->colCount)
        return -1;
    return self->values[row * self->colCount + col];
}
