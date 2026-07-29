#include "RKC_RPGSCRN_GROUND.h"

#include "RKC_FILE.h"
#include "RK_FUNCTION.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void RKC_RPGSCRN_GROUND_Init(RKC_RPGSCRN_GROUND *self)
{
    memset(self, 0, sizeof(*self));
}

void RKC_RPGSCRN_GROUND_Release(RKC_RPGSCRN_GROUND *self)
{
    free(self->cells);
    free(self->judge);
    memset(self, 0, sizeof(*self));
}

static unsigned char *ReadBlock(const unsigned char **p, const unsigned char *end, unsigned long expectedSize)
{
    if (*p + 1 > end)
        return NULL;
    unsigned char flag = **p;
    (*p)++;

    unsigned char *out = calloc(expectedSize ? expectedSize : 1, 1);
    if (!out)
        return NULL;

    if (flag == 0)
    {
        if (*p + expectedSize > end)
        {
            free(out);
            return NULL;
        }
        memcpy(out, *p, expectedSize);
        *p += expectedSize;
        return out;
    }

    if (*p + 0x10 > end || memcmp(*p, "RCLIB-L", 7) != 0)
    {
        free(out);
        return NULL;
    }
    unsigned int compSize = *(const unsigned int *)(*p + 0x0C);
    unsigned long blockLen = 0x10 + (unsigned long)compSize;
    if (*p + blockLen > end)
    {
        free(out);
        return NULL;
    }
    void *decoded = NULL;
    unsigned long outSize = 0;
    if (RK_LzDecodeMemoryToMemory(*p, blockLen, &decoded, &outSize) != 1)
    {
        free(out);
        return NULL;
    }
    memcpy(out, decoded, outSize < expectedSize ? outSize : expectedSize);
    free(decoded);
    *p += blockLen;
    return out;
}

static void CalcGroundPos(long groundScaleX, long groundScaleY, long x, long y, long *outX, long *outY)
{
    long v1 = groundScaleX * y + groundScaleY * x;
    long v2 = groundScaleX * y - groundScaleY * x;
    long v4 = groundScaleX * groundScaleY * 2;
    long r1 = (v1 >= 0) ? v1 : (v1 + 1);
    long r2 = (v2 >= 0) ? v2 : (v2 + 1);
    *outX = r1 / v4;
    *outY = r2 / v4;
    if (v1 < 0)
        *outX -= 1;
    if (v2 < 0)
        *outY -= 1;
}

static void CalcAreaJudgeFromMap(long chipWidth, long chipHeight, long areaWidth, long areaHeight, long groundScaleX,
                                   long groundScaleY, long *outW, long *outH, long *outOffX, long *outOffY)
{
    long x00, y00, xW0, yW0, x0H, y0H, xWH, yWH;
    CalcGroundPos(groundScaleX, groundScaleY, 0, 0, &x00, &y00);
    CalcGroundPos(groundScaleX, groundScaleY, chipWidth * areaWidth, 0, &xW0, &yW0);
    CalcGroundPos(groundScaleX, groundScaleY, 0, chipHeight * areaHeight, &x0H, &y0H);
    CalcGroundPos(groundScaleX, groundScaleY, chipWidth * areaWidth, chipHeight * areaHeight, &xWH, &yWH);
    *outW = (xWH - (x00 - 1)) + 1;
    *outH = (y0H - (yW0 - 1)) + 1;
    *outOffX = x00 - 1;
    *outOffY = yW0 - 1;
}

static long FloorDiv(long a, long b)
{
    long q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0)))
        q--;
    return q;
}

static int ReadFromMemory(RKC_RPGSCRN_GROUND *self, const unsigned char *src, unsigned long srcSize, long resourceBase)
{
    if (srcSize < 0x29 || memcmp(src, "RPGSCRN_GNDv", 12) != 0)
        return 0;

    const unsigned char *p = src + 0x10;
    const unsigned char *end = src + srcSize;

    long areaW = (long)*(const unsigned int *)(p + 0);
    long areaH = (long)*(const unsigned int *)(p + 4);
    long chipW = (long)*(const unsigned int *)(p + 8);
    long chipH = (long)*(const unsigned int *)(p + 12);
    long baseMagX = (long)*(const unsigned int *)(p + 16);
    long baseMagY = (long)*(const unsigned int *)(p + 20);
    p += 24;

    if (areaW <= 0 || areaH <= 0)
        return 0;

    unsigned long cellCount = (unsigned long)areaW * (unsigned long)areaH;
    unsigned long groundSize = cellCount * 6;
    unsigned char *ground = ReadBlock(&p, end, groundSize);
    if (!ground)
        return 0;

    RKC_RPGSCRN_GROUND_Cell *cells = calloc(cellCount, sizeof(RKC_RPGSCRN_GROUND_Cell));
    if (!cells)
    {
        free(ground);
        return 0;
    }

    const short *plane = (const short *)ground;
    for (unsigned long i = 0; i < cellCount; i++)
        cells[i].fieldA = plane[i];
    for (unsigned long i = 0; i < cellCount; i++)
        cells[i].slotIndex = plane[cellCount + i] + (short)resourceBase;
    for (unsigned long i = 0; i < cellCount; i++)
        cells[i].localPattern = plane[cellCount * 2 + i];
    free(ground);

    long groundScaleX = (RKC_RPGSCRN_SCENE_SCALE_X * baseMagX) / 100;
    long groundScaleY = (RKC_RPGSCRN_SCENE_SCALE_Y * baseMagY) / 100;
    long judgeW = 0, judgeH = 0, judgeOffX = 0, judgeOffY = 0;
    unsigned short *judge = NULL;
    if (groundScaleX > 0 && groundScaleY > 0)
    {
        CalcAreaJudgeFromMap(chipW, chipH, areaW, areaH, groundScaleX, groundScaleY, &judgeW, &judgeH, &judgeOffX,
                              &judgeOffY);
        if (judgeW > 0 && judgeH > 0)
        {
            unsigned long judgeCellCount = (unsigned long)judgeW * (unsigned long)judgeH;
            unsigned char *judgeBytes = ReadBlock(&p, end, judgeCellCount * 2);
            if (judgeBytes)
            {
                judge = malloc(judgeCellCount * sizeof(unsigned short));
                if (judge)
                    memcpy(judge, judgeBytes, judgeCellCount * sizeof(unsigned short));
                free(judgeBytes);
            }
        }
    }

    self->areaWidth = areaW;
    self->areaHeight = areaH;
    self->chipWidth = chipW;
    self->chipHeight = chipH;
    self->baseMagX = baseMagX;
    self->baseMagY = baseMagY;
    self->cells = cells;
    self->judgeWidth = judge ? judgeW : 0;
    self->judgeHeight = judge ? judgeH : 0;
    self->judgeOffsetX = judgeOffX;
    self->judgeOffsetY = judgeOffY;
    self->judge = judge;
    return 1;
}

int RKC_RPGSCRN_GROUND_Read(RKC_RPGSCRN_GROUND *self, const char *path, long resourceBase)
{
    RKC_RPGSCRN_GROUND_Release(self);

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
        RKC_RPGSCRN_GROUND_Release(self);
    return ok;
}

const RKC_RPGSCRN_GROUND_Cell *RKC_RPGSCRN_GROUND_GetCell(const RKC_RPGSCRN_GROUND *self, long x, long y)
{
    if (!self->cells || x < 0 || x >= self->areaWidth || y < 0 || y >= self->areaHeight)
        return NULL;
    return &self->cells[(unsigned long)y * (unsigned long)self->areaWidth + (unsigned long)x];
}

RKC_DIB *RKC_RPGSCRN_GROUND_GetTileIcon(const RKC_RPGSCRN_GROUND *self, RKC_UPDIB_SET *set, long x, long y)
{
    const RKC_RPGSCRN_GROUND_Cell *cell = RKC_RPGSCRN_GROUND_GetCell(self, x, y);
    if (!cell)
        return NULL;
    return RKC_UPDIB_SET_GetPatternIcon(set, cell->slotIndex, cell->localPattern);
}

void RKC_RPGSCRN_GROUND_CellToScreen(const RKC_RPGSCRN_GROUND *self, long x, long y, long *screenX, long *screenY)
{
    *screenX = x * self->chipWidth;
    *screenY = y * self->chipHeight;
}

int RKC_RPGSCRN_GROUND_IsBlocked(const RKC_RPGSCRN_GROUND *self, long worldX, long worldY)
{
    if (!self->judge || self->judgeWidth <= 0 || self->judgeHeight <= 0)
        return 0;

    long cellX = FloorDiv(worldX, self->baseMagX);
    long cellY = FloorDiv(worldY, self->baseMagY);
    if (cellX < self->judgeOffsetX || cellX >= self->judgeOffsetX + self->judgeWidth || cellY < self->judgeOffsetY ||
        cellY >= self->judgeOffsetY + self->judgeHeight)
        return 1;

    unsigned long idx = (unsigned long)(cellY - self->judgeOffsetY) * (unsigned long)self->judgeWidth +
                         (unsigned long)(cellX - self->judgeOffsetX);
    return (self->judge[idx] & 1) != 0;
}

int RKC_RPGSCRN_GROUND_IsRectBlocked(const RKC_RPGSCRN_GROUND *self, long worldX, long worldY, long rectL,
                                     long rectT, long rectR, long rectB)
{
    if (!self->judge || self->judgeWidth <= 0 || self->judgeHeight <= 0)
        return 0;

    long cellX0 = FloorDiv(worldX + rectL, self->baseMagX);
    long cellX1 = FloorDiv(worldX + rectR, self->baseMagX);
    long cellY0 = FloorDiv(worldY + rectT, self->baseMagY);
    long cellY1 = FloorDiv(worldY + rectB, self->baseMagY);

    for (long cy = cellY0; cy <= cellY1; cy++)
    {
        for (long cx = cellX0; cx <= cellX1; cx++)
        {
            if (cx < self->judgeOffsetX || cx >= self->judgeOffsetX + self->judgeWidth || cy < self->judgeOffsetY ||
                cy >= self->judgeOffsetY + self->judgeHeight)
                return 1;
            unsigned long idx = (unsigned long)(cy - self->judgeOffsetY) * (unsigned long)self->judgeWidth +
                                 (unsigned long)(cx - self->judgeOffsetX);
            if (self->judge[idx] & 1)
                return 1;
        }
    }
    return 0;
}

int RKC_RPGSCRN_GROUND_SweepMove(const RKC_RPGSCRN_GROUND *self, long fromX, long fromY, long toX, long toY,
                                  long *outX, long *outY)
{
    long dx = toX - fromX, dy = toY - fromY;
    if (dx == 0 && dy == 0)
    {
        *outX = fromX;
        *outY = fromY;
        return 1;
    }

    long cellStep = (self->baseMagX > 0 && self->baseMagX < self->baseMagY) ? self->baseMagX : self->baseMagY;
    if (cellStep <= 0)
        cellStep = 1;
    double euclidDist = sqrt((double)dx * (double)dx + (double)dy * (double)dy);
    long steps = (long)(euclidDist / (double)cellStep) + 1;

    long lastX = fromX, lastY = fromY;
    for (long i = 1; i <= steps; i++)
    {
        long x = fromX + dx * i / steps;
        long y = fromY + dy * i / steps;
        if (RKC_RPGSCRN_GROUND_IsBlocked(self, x, y))
        {
            long clearX = lastX, clearY = lastY;
            long blockedX = x, blockedY = y;
            for (int iter = 0; iter < 12; iter++)
            {
                long midX = clearX + (blockedX - clearX) / 2;
                long midY = clearY + (blockedY - clearY) / 2;
                if (midX == clearX && midY == clearY)
                    break;
                if (RKC_RPGSCRN_GROUND_IsBlocked(self, midX, midY))
                {
                    blockedX = midX;
                    blockedY = midY;
                }
                else
                {
                    clearX = midX;
                    clearY = midY;
                }
            }
            *outX = clearX;
            *outY = clearY;
            return 0;
        }
        lastX = x;
        lastY = y;
    }
    *outX = toX;
    *outY = toY;
    return 1;
}
