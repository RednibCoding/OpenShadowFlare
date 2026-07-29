#ifndef SFDE_RKC_RPGSCRN_CAF_H
#define SFDE_RKC_RPGSCRN_CAF_H

#include "RKC_UPDIB.h"

#define RKC_RPGSCRN_CAF_NUM_DIRECTIONS 9

#define RKC_RPGSCRN_CAF_MAX_DRAW_CMDS 64

typedef struct
{
    short status;
    short trans; /* 0-1000 fixed-point opacity */
    long patternNo;
    short priority;
} RKC_RPGSCRN_CAF_Cell;

typedef struct
{
    long cellCount;
    RKC_RPGSCRN_CAF_Cell *cells;
} RKC_RPGSCRN_CAF_CellBlock;

typedef struct
{
    long cellBlockCount;
    short maxFrameCount;
    RKC_RPGSCRN_CAF_CellBlock *cellBlocks;
} RKC_RPGSCRN_CAF_Direction;

typedef struct
{
    short status;
    RKC_RPGSCRN_CAF_Direction directions[RKC_RPGSCRN_CAF_NUM_DIRECTIONS];
} RKC_RPGSCRN_CAF_Chart;

typedef struct RKC_RPGSCRN_CAF
{
    long version;
    long chartCount;
    RKC_RPGSCRN_CAF_Chart *charts;
} RKC_RPGSCRN_CAF;

void RKC_RPGSCRN_CAF_Init(RKC_RPGSCRN_CAF *self);
void RKC_RPGSCRN_CAF_Release(RKC_RPGSCRN_CAF *self);

int RKC_RPGSCRN_CAF_Read(RKC_RPGSCRN_CAF *self, const char *path);

typedef struct
{
    const RKC_DIB *icon;
    short trans;
    long offsetX;
    long offsetY;
    short tintR, tintG, tintB;
    int isShadow;
    int isAdditive;
} RKC_RPGSCRN_CAF_DrawCmd;

int RKC_RPGSCRN_CAF_Resolve(const RKC_RPGSCRN_CAF *self, long chart, long direction, long animFrame, RKC_UPDIB *njp,
                            RKC_UPDIB *sdw, const unsigned int *cellBlockMask, const unsigned short *tintR,
                            const unsigned short *tintG, const unsigned short *tintB, long cellBlockMaskCount,
                            RKC_RPGSCRN_CAF_DrawCmd *out, int maxOut);

#endif
