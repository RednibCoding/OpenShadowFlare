#include "RKC_RPG_AICONTROL.h"

#include "RKC_FILE.h"

#include <stdlib.h>
#include <string.h>

#define AID_MAX_COUNT 1000000

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
    if (c < 0 || c > AID_MAX_COUNT)
        r->ok = 0;
    return c;
}

static void ReleaseEvent(RKC_RPG_AIEVENT *event)
{
    free(event->data);
}

static void ReleaseList(RKC_RPG_AILIST *list)
{
    free(list->name);
    for (long i = 0; i < list->eventCount; i++)
        ReleaseEvent(&list->events[i]);
    free(list->events);
}

void RKC_RPG_AICONTROL_Init(RKC_RPG_AICONTROL *self)
{
    memset(self, 0, sizeof(*self));
}

void RKC_RPG_AICONTROL_Release(RKC_RPG_AICONTROL *self)
{
    for (long i = 0; i < self->listCount; i++)
        ReleaseList(&self->lists[i]);
    free(self->lists);
    memset(self, 0, sizeof(*self));
}

static int ReadEvent(Reader *r, long eventIndex, RKC_RPG_AIEVENT *out)
{
    memset(out, 0, sizeof(*out));

    long dataCount = ReadCount(r);
    if (!r->ok)
        return 0;

    RKC_RPG_AIDATA *data = dataCount > 0 ? (RKC_RPG_AIDATA *)calloc((size_t)dataCount, sizeof(RKC_RPG_AIDATA)) : NULL;
    if (dataCount > 0 && !data)
        return 0;

    for (long i = 0; i < dataCount; i++)
    {
        data[i].eventNo = eventIndex;

        long actionNo = ReadI32(r);
        if (!r->ok)
        {
            free(data);
            return 0;
        }
        data[i].actionNo = actionNo;

        const unsigned char *param = Take(r, 0x24);
        const unsigned char *cond = param ? Take(r, 0x18) : NULL;
        if (!cond)
        {
            free(data);
            return 0;
        }
        const int *p = (const int *)param;
        data[i].parameter.priority = p[0];
        data[i].parameter.durationTicks = p[1];
        data[i].parameter.weight = p[2];
        data[i].parameter.speed = p[3];
        data[i].parameter.wanderRangeA = p[4];
        data[i].parameter.wanderRangeB = p[5];
        data[i].parameter.animFlag = p[6];
        data[i].parameter.field1c = p[7];
        data[i].parameter.field20 = p[8];
        const int *c = (const int *)cond;
        data[i].condition.hpPercentCheckEnabled = c[0];
        data[i].condition.hpPercentMin = c[1];
        data[i].condition.hpPercentMax = c[2];
        data[i].condition.targetSearchEnabled = c[3];
        data[i].condition.targetMinDistance = c[4];
        data[i].condition.targetMaxDistance = c[5];
    }

    out->data = data;
    out->dataCount = dataCount;
    return 1;
}

static int ReadList(Reader *r, int version, long eventCount, RKC_RPG_AILIST *out)
{
    memset(out, 0, sizeof(*out));

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
    }

    if (version > 0)
        out->walkPointSpeed = ReadI32(r);

    RKC_RPG_AIEVENT *events = eventCount > 0 ? (RKC_RPG_AIEVENT *)calloc((size_t)eventCount, sizeof(RKC_RPG_AIEVENT)) : NULL;
    if (eventCount > 0 && !events)
    {
        free(out->name);
        return 0;
    }

    for (long e = 0; e < eventCount; e++)
    {
        if (!ReadEvent(r, e, &events[e]))
        {
            for (long j = 0; j <= e; j++)
                ReleaseEvent(&events[j]);
            free(events);
            free(out->name);
            return 0;
        }
    }

    out->events = events;
    out->eventCount = eventCount;
    return 1;
}

static int ReadFromMemory(RKC_RPG_AICONTROL *self, const unsigned char *src, unsigned long srcSize)
{
    if (srcSize < 0x10 || memcmp(src, "RKC_AIDATA v", 12) != 0)
        return 0;
    int version = (src[12] - '0') * 100 + (src[13] - '0') * 10 + (src[14] - '0');

    Reader r = {src, src + srcSize, 1};
    Take(&r, 16); /* magic */

    long listCount = ReadCount(&r);
    long eventCount = ReadCount(&r);
    if (!r.ok)
        return 0;

    RKC_RPG_AILIST *lists = listCount > 0 ? (RKC_RPG_AILIST *)calloc((size_t)listCount, sizeof(RKC_RPG_AILIST)) : NULL;
    if (listCount > 0 && !lists)
        return 0;

    for (long i = 0; i < listCount; i++)
    {
        if (!ReadList(&r, version, eventCount, &lists[i]))
        {
            for (long j = 0; j <= i; j++)
                ReleaseList(&lists[j]);
            free(lists);
            return 0;
        }
    }

    if (!r.ok)
    {
        for (long i = 0; i < listCount; i++)
            ReleaseList(&lists[i]);
        free(lists);
        return 0;
    }

    self->lists = lists;
    self->listCount = listCount;
    self->version = version;
    return 1;
}

int RKC_RPG_AICONTROL_Read(RKC_RPG_AICONTROL *self, const char *path)
{
    RKC_RPG_AICONTROL_Release(self);

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
        RKC_RPG_AICONTROL_Release(self);
    return ok;
}

const RKC_RPG_AILIST *RKC_RPG_AICONTROL_GetFromName(const RKC_RPG_AICONTROL *self, const char *name)
{
    if (!name)
        return NULL;
    for (long i = 0; i < self->listCount; i++)
    {
        if (self->lists[i].name && strcmp(self->lists[i].name, name) == 0)
            return &self->lists[i];
    }
    return NULL;
}

long RKC_RPG_AICONTROL_GetNo(const RKC_RPG_AICONTROL *self, const RKC_RPG_AILIST *list)
{
    if (!list || list < self->lists || list >= self->lists + self->listCount)
        return -1;
    return (long)(list - self->lists);
}
