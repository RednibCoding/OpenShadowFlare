#ifndef SFDE_RKC_RPGSCRN_GROUND_H
#define SFDE_RKC_RPGSCRN_GROUND_H

#include "RKC_UPDIB_SET.h"

#define RKC_RPGSCRN_SCENE_SCALE_X 15
#define RKC_RPGSCRN_SCENE_SCALE_Y 10

typedef struct
{
    short fieldA;       /* unknown */
    short slotIndex;
    short localPattern;
} RKC_RPGSCRN_GROUND_Cell;

typedef struct RKC_RPGSCRN_GROUND
{
    long areaWidth, areaHeight;
    long chipWidth, chipHeight;
    long baseMagX, baseMagY;
    RKC_RPGSCRN_GROUND_Cell *cells;
    long judgeWidth, judgeHeight, judgeOffsetX, judgeOffsetY;
    unsigned short *judge; /* judgeWidth*judgeHeight entries, row-major; bit0=blocked */
} RKC_RPGSCRN_GROUND;

void RKC_RPGSCRN_GROUND_Init(RKC_RPGSCRN_GROUND *self);
void RKC_RPGSCRN_GROUND_Release(RKC_RPGSCRN_GROUND *self);

int RKC_RPGSCRN_GROUND_Read(RKC_RPGSCRN_GROUND *self, const char *path, long resourceBase);

const RKC_RPGSCRN_GROUND_Cell *RKC_RPGSCRN_GROUND_GetCell(const RKC_RPGSCRN_GROUND *self, long x, long y);

RKC_DIB *RKC_RPGSCRN_GROUND_GetTileIcon(const RKC_RPGSCRN_GROUND *self, RKC_UPDIB_SET *set, long x, long y);

void RKC_RPGSCRN_GROUND_CellToScreen(const RKC_RPGSCRN_GROUND *self, long x, long y, long *screenX, long *screenY);

int RKC_RPGSCRN_GROUND_IsBlocked(const RKC_RPGSCRN_GROUND *self, long worldX, long worldY);

int RKC_RPGSCRN_GROUND_IsRectBlocked(const RKC_RPGSCRN_GROUND *self, long worldX, long worldY, long rectL,
                                     long rectT, long rectR, long rectB);

int RKC_RPGSCRN_GROUND_SweepMove(const RKC_RPGSCRN_GROUND *self, long fromX, long fromY, long toX, long toY,
                                 long *outX, long *outY);

#endif
