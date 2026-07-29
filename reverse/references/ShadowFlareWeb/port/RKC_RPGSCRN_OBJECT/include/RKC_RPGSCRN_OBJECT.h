#ifndef SFDE_RKC_RPGSCRN_OBJECT_H
#define SFDE_RKC_RPGSCRN_OBJECT_H

#include "RKC_UPDIB_SET.h"


typedef struct
{
    long posX, posY;
    long slotIndex;
    long localPattern;
    long field3;
    long field4;
    long field5; /* unknown */
    long field6;
    long field7, field8, field9;
    long rectL, rectT, rectR, rectB;
} RKC_RPGSCRN_OBJECT_Entry;

typedef struct RKC_RPGSCRN_OBJECTBLOCK
{
    RKC_RPGSCRN_OBJECT_Entry *objects;
    long count;
} RKC_RPGSCRN_OBJECTBLOCK;

void RKC_RPGSCRN_OBJECTBLOCK_Init(RKC_RPGSCRN_OBJECTBLOCK *self);
void RKC_RPGSCRN_OBJECTBLOCK_Release(RKC_RPGSCRN_OBJECTBLOCK *self);


int RKC_RPGSCRN_OBJECTBLOCK_Read(RKC_RPGSCRN_OBJECTBLOCK *self, const char *path, long resourceBase);

long RKC_RPGSCRN_OBJECTBLOCK_GetCount(const RKC_RPGSCRN_OBJECTBLOCK *self);

const RKC_RPGSCRN_OBJECT_Entry *RKC_RPGSCRN_OBJECTBLOCK_Get(const RKC_RPGSCRN_OBJECTBLOCK *self, long index);


RKC_DIB *RKC_RPGSCRN_OBJECTBLOCK_GetIcon(const RKC_RPGSCRN_OBJECTBLOCK *self, RKC_UPDIB_SET *set, long index);


void RKC_RPGSCRN_OBJECTBLOCK_GetOffset(const RKC_RPGSCRN_OBJECTBLOCK *self, RKC_UPDIB_SET *set, long index,
                                        long *outOffsetX, long *outOffsetY);


#define RKC_RPGSCRN_OBJECT_STATUS_SHADOW_BIT 8


RKC_DIB *RKC_RPGSCRN_OBJECTBLOCK_GetShadowIcon(const RKC_RPGSCRN_OBJECTBLOCK *self, RKC_UPDIB_SET *set, long index);
void RKC_RPGSCRN_OBJECTBLOCK_GetShadowOffset(const RKC_RPGSCRN_OBJECTBLOCK *self, RKC_UPDIB_SET *set, long index,
                                              long *outOffsetX, long *outOffsetY);

#endif
