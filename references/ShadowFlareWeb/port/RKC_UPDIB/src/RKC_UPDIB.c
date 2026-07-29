#include "RKC_UPDIB.h"

#include "RKC_FILE.h"
#include "RK_FUNCTION.h"

#include <stdlib.h>
#include <string.h>

void RKC_UPDIB_Init(RKC_UPDIB *self)
{
    memset(self, 0, sizeof(*self));
}

void RKC_UPDIB_Release(RKC_UPDIB *self)
{
    if (self->frames)
    {
        for (long i = 0; i < self->frameCount; i++)
            RKC_DIB_Release(&self->frames[i]);
        free(self->frames);
    }
    free(self->patternFrame);
    free(self->patternOffsetX);
    free(self->patternOffsetY);
    memset(self, 0, sizeof(*self));
}

long RKC_UPDIB_GetFrameCount(const RKC_UPDIB *self)
{
    return self->frameCount;
}

RKC_DIB *RKC_UPDIB_GetFrame(RKC_UPDIB *self, long index)
{
    if (index < 0 || index >= self->frameCount)
        return NULL;
    return &self->frames[index];
}

long RKC_UPDIB_GetPatternCount(const RKC_UPDIB *self)
{
    return self->patternCount;
}

RKC_DIB *RKC_UPDIB_GetPatternIcon(RKC_UPDIB *self, long patternIndex)
{
    if (patternIndex < 0 || patternIndex >= self->patternCount)
        return NULL;
    long frame = self->patternFrame[patternIndex];
    if (frame < 0)
        return NULL;
    return RKC_UPDIB_GetFrame(self, frame);
}

void RKC_UPDIB_GetPatternOffset(const RKC_UPDIB *self, long patternIndex, long *outOffsetX, long *outOffsetY)
{
    if (patternIndex < 0 || patternIndex >= self->patternCount)
    {
        *outOffsetX = 0;
        *outOffsetY = 0;
        return;
    }
    *outOffsetX = self->patternOffsetX[patternIndex];
    *outOffsetY = self->patternOffsetY[patternIndex];
}

static void CopyPaletteEntries(unsigned char *dst, const unsigned char *filePal, long count)
{
    for (long i = 0; i < count; i++)
    {
        dst[i * 4 + 0] = filePal[i * 4 + 2];
        dst[i * 4 + 1] = filePal[i * 4 + 1];
        dst[i * 4 + 2] = filePal[i * 4 + 0];
        dst[i * 4 + 3] = filePal[i * 4 + 3];
    }
}

static const unsigned char kShadowPalette[8] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xFF,
};

static int ReadFromMemory(RKC_UPDIB *self, const unsigned char *src, unsigned long srcSize)
{
    if (srcSize < 0x18)
        return 0;

    int isShadow = memcmp(src, "ShadowLowPat", 12) == 0;
    if (!isShadow && memcmp(src, "NJudgeUniPat", 12) != 0)
        return 0;

    int version = (src[12] - '0') * 100 + (src[13] - '0') * 10 + (src[14] - '0');
    unsigned long numFrames = *(const unsigned int *)(src + 0x10);
    if (numFrames == 0)
        return 0;

    const unsigned char *p = src + (version > 2 ? 0x18 : 0x14);
    const unsigned char *end = src + srcSize;

    self->frames = calloc((size_t)numFrames, sizeof(RKC_DIB));
    if (!self->frames)
        return 0;
    for (unsigned long j = 0; j < numFrames; j++)
        RKC_DIB_Init(&self->frames[j]);

    unsigned int *palIdx = calloc((size_t)numFrames, sizeof(unsigned int));
    if (!palIdx)
    {
        free(self->frames);
        self->frames = NULL;
        return 0;
    }

    unsigned long i;
    for (i = 0; i < numFrames && p + 16 <= end; i++)
    {
        unsigned int bpp = isShadow ? 1 : *(const unsigned int *)(p + 0);
        long width = (long)*(const unsigned int *)(p + 4);
        long height = (long)*(const unsigned int *)(p + 8);
        unsigned int compressed = *(const unsigned int *)(p + 12);
        p += 16;

        RKC_DIB *frame = &self->frames[i];
        if (!RKC_DIB_Create(frame, width, height, (unsigned short)bpp, 0))
            break;

        unsigned long decompSize = (unsigned long)frame->alignWidth * (unsigned long)frame->height;
        frame->pixels = malloc(decompSize ? decompSize : 1);
        if (!frame->pixels)
            break;

        if (compressed)
        {
            if (p + 0x10 > end)
                break;
            void *decoded = NULL;
            unsigned long outSize = 0;
            if (RK_LzDecodeMemoryToMemory(p, (unsigned long)(end - p), &decoded, &outSize) != 1)
                break;
            memcpy(frame->pixels, decoded, outSize < decompSize ? outSize : decompSize);
            free(decoded);
            unsigned int compSize = *(const unsigned int *)(p + 0x0C);
            p += 0x10 + compSize;
        }
        else
        {
            if (p + decompSize > end)
                break;
            memcpy(frame->pixels, p, decompSize);
            p += decompSize;
        }
    }

    if (i == 0)
    {
        free(palIdx);
        free(self->frames);
        self->frames = NULL;
        return 0;
    }
    self->frameCount = (long)i;
    self->isShadowType = isShadow;

    const unsigned char *palTable = NULL;
    unsigned int numPalettes = 0;

    self->patternCount = 0;
    self->patternFrame = NULL;
    self->patternOffsetX = NULL;
    self->patternOffsetY = NULL;
    if (p + 4 <= end)
    {
        unsigned int numPatterns = *(const unsigned int *)p;
        p += 4;

        if (version > 2 && p + 4 <= end)
            p += 4;

        long *patternFrame = numPatterns ? malloc((size_t)numPatterns * sizeof(long)) : NULL;
        long *patternOffsetX = numPatterns ? malloc((size_t)numPatterns * sizeof(long)) : NULL;
        long *patternOffsetY = numPatterns ? malloc((size_t)numPatterns * sizeof(long)) : NULL;
        if (numPatterns > 0 && (!patternFrame || !patternOffsetX || !patternOffsetY))
        {
            free(patternFrame);
            free(patternOffsetX);
            free(patternOffsetY);
            patternFrame = NULL;
            patternOffsetX = NULL;
            patternOffsetY = NULL;
            numPatterns = 0;
        }

        unsigned int pat;
        for (pat = 0; pat < numPatterns; pat++)
        {
            if (p + 4 + 16 > end)
                break;
            unsigned int frameCount = *(const unsigned int *)p;
            p += 4 + 16;

            unsigned int idx = 0;
            if (version > 0)
            {
                if (p + 4 > end)
                    break;
                idx = *(const unsigned int *)p;
                p += 4;
            }

            unsigned long need = (unsigned long)frameCount * 28;
            if (p + need > end)
                break;

            long resolvedFrame = -1;
            long resolvedOffsetX = 0, resolvedOffsetY = 0;
            for (unsigned int fr = 0; fr < frameCount; fr++)
            {
                int objectRef = *(const int *)(p + fr * 28 + 4);
                if (objectRef >= 0 && objectRef < self->frameCount)
                {
                    palIdx[objectRef] = idx;
                    if (fr == 0)
                    {
                        resolvedFrame = objectRef;
                        resolvedOffsetX = *(const int *)(p + fr * 28 + 8);
                        resolvedOffsetY = *(const int *)(p + fr * 28 + 12);
                    }
                }
            }
            patternFrame[pat] = resolvedFrame;
            patternOffsetX[pat] = resolvedOffsetX;
            patternOffsetY[pat] = resolvedOffsetY;
            p += need;
        }
        self->patternFrame = patternFrame;
        self->patternOffsetX = patternOffsetX;
        self->patternOffsetY = patternOffsetY;
        self->patternCount = (long)pat;
    }

    if (p + 4 <= end)
    {
        numPalettes = *(const unsigned int *)p;
        p += 4;
        if (numPalettes > 0 && p + (unsigned long)numPalettes * 1024 <= end)
            palTable = p;
    }

    for (long f = 0; f < self->frameCount; f++)
    {
        RKC_DIB *frame = &self->frames[f];
        long paletteCount = RKC_DIB_GetPaletteCount(frame);
        if (paletteCount <= 0 || !frame->palette)
            continue;

        if (frame->bpp == 1)
        {
            memcpy(frame->palette, kShadowPalette, sizeof(kShadowPalette));
            continue;
        }

        if (palTable)
        {
            unsigned int idx = palIdx[f] < numPalettes ? palIdx[f] : 0;
            CopyPaletteEntries(frame->palette, palTable + (size_t)idx * 1024, paletteCount);
        }
    }

    free(palIdx);
    return 1;
}

int RKC_UPDIB_Read(RKC_UPDIB *self, const char *path)
{
    RKC_UPDIB_Release(self);

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

    int ok = ReadFromMemory(self, buf, (unsigned long)size);
    free(buf);
    if (!ok)
        RKC_UPDIB_Release(self);
    return ok;
}
