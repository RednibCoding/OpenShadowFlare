#ifndef SFDE_RKC_RPG_AICONTROL_H
#define SFDE_RKC_RPG_AICONTROL_H

typedef struct
{
    long priority;
    long durationTicks;
    long weight;
    long speed;
    long wanderRangeA;
    long wanderRangeB;
    long animFlag;
    long field1c, field20;
} RKC_RPG_AIPARAM;

typedef struct
{
    long hpPercentCheckEnabled;
    long hpPercentMin;
    long hpPercentMax;
    long targetSearchEnabled;
    long targetMinDistance;
    long targetMaxDistance;
} RKC_RPG_AICONDITION;

typedef struct
{
    long actionNo;
    long eventNo;
    RKC_RPG_AIPARAM parameter;
    RKC_RPG_AICONDITION condition;
} RKC_RPG_AIDATA;

typedef struct
{
    RKC_RPG_AIDATA *data;
    long dataCount;
} RKC_RPG_AIEVENT;

typedef struct
{
    char *name;
    long walkPointSpeed;
    RKC_RPG_AIEVENT *events;
    long eventCount;
} RKC_RPG_AILIST;

typedef struct
{
    RKC_RPG_AILIST *lists;
    long listCount;
    int version;
} RKC_RPG_AICONTROL;

void RKC_RPG_AICONTROL_Init(RKC_RPG_AICONTROL *self);
void RKC_RPG_AICONTROL_Release(RKC_RPG_AICONTROL *self);

int RKC_RPG_AICONTROL_Read(RKC_RPG_AICONTROL *self, const char *path);

const RKC_RPG_AILIST *RKC_RPG_AICONTROL_GetFromName(const RKC_RPG_AICONTROL *self, const char *name);

long RKC_RPG_AICONTROL_GetNo(const RKC_RPG_AICONTROL *self, const RKC_RPG_AILIST *list);

#endif
