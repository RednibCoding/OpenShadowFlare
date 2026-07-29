#include "RKC_RPGSCRN_CAF.h"

#include "RKC_FILE.h"

#include <stdlib.h>
#include <string.h>

void RKC_RPGSCRN_CAF_Init(RKC_RPGSCRN_CAF *self)
{
    memset(self, 0, sizeof(*self));
}

void RKC_RPGSCRN_CAF_Release(RKC_RPGSCRN_CAF *self)
{
    if (self->charts)
    {
        for (long c = 0; c < self->chartCount; c++)
        {
            for (int d = 0; d < RKC_RPGSCRN_CAF_NUM_DIRECTIONS; d++)
            {
                RKC_RPGSCRN_CAF_Direction *dir = &self->charts[c].directions[d];
                if (dir->cellBlocks)
                {
                    for (long b = 0; b < dir->cellBlockCount; b++)
                        free(dir->cellBlocks[b].cells);
                    free(dir->cellBlocks);
                }
            }
        }
        free(self->charts);
    }
    memset(self, 0, sizeof(*self));
}

static int ReadU16(RKC_FILE *f, unsigned short *out)
{
    return RKC_FILE_Read(f, out, 2);
}

static int ReadU32(RKC_FILE *f, unsigned int *out)
{
    return RKC_FILE_Read(f, out, 4);
}

int RKC_RPGSCRN_CAF_Read(RKC_RPGSCRN_CAF *self, const char *path)
{
    RKC_RPGSCRN_CAF_Release(self);

    RKC_FILE file;
    RKC_FILE_Init(&file);
    if (!RKC_FILE_Create(&file, path, 0))
        return 0;

    char magic[16];
    if (!RKC_FILE_Read(&file, magic, 16) || memcmp(magic, "CHRAnimation", 12) != 0)
    {
        RKC_FILE_Close(&file);
        return 0;
    }
    long version = (magic[12] - '0') * 100 + (magic[13] - '0') * 10 + (magic[14] - '0');

    unsigned int numCharts;
    if (!ReadU32(&file, &numCharts))
    {
        RKC_FILE_Close(&file);
        return 0;
    }

    self->charts = numCharts ? calloc(numCharts, sizeof(RKC_RPGSCRN_CAF_Chart)) : NULL;
    if (numCharts && !self->charts)
    {
        RKC_FILE_Close(&file);
        return 0;
    }
    self->chartCount = (long)numCharts;
    self->version = version;

    for (unsigned int c = 0; c < numCharts; c++)
    {
        RKC_RPGSCRN_CAF_Chart *chart = &self->charts[c];
        unsigned short status;
        if (!ReadU16(&file, &status))
            goto fail;
        chart->status = (short)status;

        for (int d = 0; d < RKC_RPGSCRN_CAF_NUM_DIRECTIONS; d++)
        {
            RKC_RPGSCRN_CAF_Direction *dir = &chart->directions[d];
            unsigned int blockCount;
            unsigned short maxFrames;
            if (!ReadU32(&file, &blockCount) || !ReadU16(&file, &maxFrames))
                goto fail;
            dir->cellBlockCount = (long)blockCount;
            dir->maxFrameCount = (short)maxFrames;
            if (blockCount == 0)
                continue;

            dir->cellBlocks = calloc(blockCount, sizeof(RKC_RPGSCRN_CAF_CellBlock));
            if (!dir->cellBlocks)
                goto fail;

            for (unsigned int b = 0; b < blockCount; b++)
            {
                RKC_RPGSCRN_CAF_CellBlock *block = &dir->cellBlocks[b];
                unsigned int cellCount;
                if (!ReadU32(&file, &cellCount))
                    goto fail;
                block->cellCount = (long)cellCount;
                if (cellCount == 0)
                    continue;

                block->cells = calloc(cellCount, sizeof(RKC_RPGSCRN_CAF_Cell));
                if (!block->cells)
                    goto fail;

                for (unsigned int k = 0; k < cellCount; k++)
                {
                    RKC_RPGSCRN_CAF_Cell *cell = &block->cells[k];
                    unsigned short status16, trans16, priority16;
                    if (!ReadU16(&file, &status16) || !ReadU16(&file, &trans16))
                        goto fail;
                    cell->status = (short)status16;
                    cell->trans = (short)trans16;

                    if (version >= 2)
                    {
                        unsigned int patternNo;
                        if (!ReadU32(&file, &patternNo))
                            goto fail;
                        cell->patternNo = (long)(int)patternNo;
                    }
                    else
                    {
                        unsigned short patternNo16;
                        if (!ReadU16(&file, &patternNo16))
                            goto fail;
                        cell->patternNo = (long)patternNo16;
                    }

                    if (!ReadU16(&file, &priority16))
                        goto fail;
                    cell->priority = (short)priority16;
                }
            }
        }
    }

    RKC_FILE_Close(&file);
    return 1;

fail:
    RKC_FILE_Close(&file);
    RKC_RPGSCRN_CAF_Release(self);
    return 0;
}

int RKC_RPGSCRN_CAF_Resolve(const RKC_RPGSCRN_CAF *self, long chart, long direction, long animFrame, RKC_UPDIB *njp,
                            RKC_UPDIB *sdw, const unsigned int *cellBlockMask, const unsigned short *tintR,
                            const unsigned short *tintG, const unsigned short *tintB, long cellBlockMaskCount,
                            RKC_RPGSCRN_CAF_DrawCmd *out, int maxOut)
{
    if (!self || chart < 0 || chart >= self->chartCount || direction < 0 || direction >= RKC_RPGSCRN_CAF_NUM_DIRECTIONS)
        return 0;

    const RKC_RPGSCRN_CAF_Direction *dir = &self->charts[chart].directions[direction];
    if (dir->maxFrameCount <= 0)
        return 0;

    long frame = animFrame % dir->maxFrameCount;
    if (frame < 0)
        frame += dir->maxFrameCount;

    int isShadow[RKC_RPGSCRN_CAF_MAX_DRAW_CMDS];
    int cap = maxOut < RKC_RPGSCRN_CAF_MAX_DRAW_CMDS ? maxOut : RKC_RPGSCRN_CAF_MAX_DRAW_CMDS;

    int n = 0;
    for (long b = 0; b < dir->cellBlockCount && n < cap; b++)
    {
        if (cellBlockMask && b < cellBlockMaskCount && cellBlockMask[b] != 1)
            continue;

        short cellTintR = (tintR && b < cellBlockMaskCount) ? tintR[b] : 1000;
        short cellTintG = (tintG && b < cellBlockMaskCount) ? tintG[b] : 1000;
        short cellTintB = (tintB && b < cellBlockMaskCount) ? tintB[b] : 1000;

        const RKC_RPGSCRN_CAF_CellBlock *block = &dir->cellBlocks[b];
        if (frame >= block->cellCount)
            continue;
        const RKC_RPGSCRN_CAF_Cell *cell = &block->cells[frame];

        if (njp)
        {
            RKC_DIB *icon = RKC_UPDIB_GetPatternIcon(njp, cell->patternNo);
            if (icon && n < cap)
            {
                out[n].icon = icon;
                out[n].trans = cell->trans;
                RKC_UPDIB_GetPatternOffset(njp, cell->patternNo, &out[n].offsetX, &out[n].offsetY);
                out[n].tintR = cellTintR;
                out[n].tintG = cellTintG;
                out[n].tintB = cellTintB;
                out[n].isShadow = 0;
                out[n].isAdditive = (cell->status & 0x10) != 0;
                isShadow[n] = 0;
                n++;
            }
        }
        if ((cell->status & 8) && sdw && n < cap)
        {
            RKC_DIB *icon = RKC_UPDIB_GetPatternIcon(sdw, cell->patternNo);
            if (icon && n < cap)
            {
                out[n].icon = icon;
                out[n].trans = cell->trans;
                RKC_UPDIB_GetPatternOffset(sdw, cell->patternNo, &out[n].offsetX, &out[n].offsetY);
                out[n].tintR = 1000;
                out[n].tintG = 1000;
                out[n].tintB = 1000;
                out[n].isShadow = 1;
                out[n].isAdditive = 0;
                isShadow[n] = 1;
                n++;
            }
        }
    }

    for (int i = 1; i < n; i++)
    {
        if (!isShadow[i])
            continue;
        RKC_RPGSCRN_CAF_DrawCmd key = out[i];
        int j = i - 1;
        while (j >= 0 && !isShadow[j])
        {
            out[j + 1] = out[j];
            isShadow[j + 1] = 0;
            j--;
        }
        out[j + 1] = key;
        isShadow[j + 1] = 1;
    }

    return n;
}
