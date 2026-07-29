#ifndef SFDE_RKC_UPDIB_H
#define SFDE_RKC_UPDIB_H

#include "RKC_DIB.h"


typedef struct RKC_UPDIB
{
    RKC_DIB *frames;
    long frameCount;
    long *patternFrame;
    long *patternOffsetX;
    long *patternOffsetY;
    long patternCount;
    int isShadowType;
} RKC_UPDIB;

void RKC_UPDIB_Init(RKC_UPDIB *self);
void RKC_UPDIB_Release(RKC_UPDIB *self);

int RKC_UPDIB_Read(RKC_UPDIB *self, const char *path);

long RKC_UPDIB_GetFrameCount(const RKC_UPDIB *self);

RKC_DIB *RKC_UPDIB_GetFrame(RKC_UPDIB *self, long index);

long RKC_UPDIB_GetPatternCount(const RKC_UPDIB *self);

RKC_DIB *RKC_UPDIB_GetPatternIcon(RKC_UPDIB *self, long patternIndex);

void RKC_UPDIB_GetPatternOffset(const RKC_UPDIB *self, long patternIndex, long *outOffsetX, long *outOffsetY);

#endif
