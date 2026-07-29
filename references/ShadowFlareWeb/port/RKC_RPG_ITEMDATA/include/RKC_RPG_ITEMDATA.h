#ifndef SFDE_RKC_RPG_ITEMDATA_H
#define SFDE_RKC_RPG_ITEMDATA_H

typedef struct
{
    long templateId;
    char *name;
    char *description;
    unsigned char *tail;
    long tailSize;
} RKC_RPG_ITEMDATA_Record;

typedef struct
{
    long value;
    long max;
    long chancePercent;
} RKC_RPG_ITEMDATA_RollEntry;

typedef struct
{
    long weaponClass;
    long templateId;
    long affixTier;
    long bitmask;
    long weight;
    long field14, field18;
    long gridWidth, gridHeight;
    long field24;
    long iconSheet, iconPattern;
    long groundSpriteFolder;
    long groundSpriteFrame;
    long field38, field3C, field40, field44;
    long weaponSecondaryCellBlockIndex;
    long weaponSecondaryCellBlockTintR, weaponSecondaryCellBlockTintG, weaponSecondaryCellBlockTintB;
    long field58;
    long field5C[2];
    long durabilityMax;
    long field68[10];
    long field90, field94, field98;
    long weaponGripTintR, weaponGripTintG, weaponGripTintB;
    long cellBlockIndex;
    long cellBlockTintR, cellBlockTintG, cellBlockTintB;
    long fieldB8, fieldBC, fieldC0, fieldC4, fieldC8;
    long requiresTwoHands;
    long combatModifier[8];
    RKC_RPG_ITEMDATA_RollEntry rollTable1[39]; /* +0xF0, "39-triple" table */
    RKC_RPG_ITEMDATA_RollEntry rollTable2[8]; /* +0x2C4, "8-triple" table */
} RKC_RPG_ITEMDATA_Kind0Tail;
typedef struct
{
    long weaponClass;
    long templateId;
    long affixTier;
    long bitmask;
    long field10;
    long field14, field18;
    long gridWidth, gridHeight;
    long tooltipWeight;
    long iconSheet, iconPattern;
    long groundSpriteFolder;
    long groundSpriteFrame;
    long field38, field3C, field40, field44, field48, field4C, field50, field54, field58;
    long field5C[2];
    long durabilityMax;
    long attack;
    long field6C;
    long defense;
    long evasionRate;
    long magicalAttack;
    long magicalHitRate;
    long magicalDefense;
    long magicalEvasionRate;
    long field88, field8C;
    long requiredLevel;
    long field94;
    long cellBlockIndex;
    long cellBlockTintR, cellBlockTintG, cellBlockTintB;
    long combatModifier[8];
    RKC_RPG_ITEMDATA_RollEntry rollTable1[39];
    RKC_RPG_ITEMDATA_RollEntry rollTable2[8];
} RKC_RPG_ITEMDATA_Kind1Tail;

typedef struct
{
    long weaponClass;
    long templateId;
    long affixTier;
    long bitmask;
    long weight;
    long field14, field18;
    long gridWidth, gridHeight;
    long field24;
    long iconSheet, iconPattern;
    long groundSpriteFolder;
    long groundSpriteFrame;
    long field38, field3C, field40, field44, field48, field4C, field50, field54, field58;
    long field5C[2];
    long field64;
    long field68;
    RKC_RPG_ITEMDATA_RollEntry rollTable1[39];
    RKC_RPG_ITEMDATA_RollEntry rollTable2[8];
} RKC_RPG_ITEMDATA_Kind2Tail;

typedef struct
{
    RKC_RPG_ITEMDATA_Kind1Tail tail;
    long durability;
} RKC_RPG_ITEMDATA_Kind1Instance;

typedef struct
{
    RKC_RPG_ITEMDATA_Kind2Tail tail;
} RKC_RPG_ITEMDATA_Kind2Instance;

typedef struct
{
    RKC_RPG_ITEMDATA_Record *records;
    long count;
} RKC_RPG_ITEMDATA_Kind;

typedef struct
{
    RKC_RPG_ITEMDATA_Kind kinds[5];
} RKC_RPG_ITEMDATA;

void RKC_RPG_ITEMDATA_Init(RKC_RPG_ITEMDATA *self);
void RKC_RPG_ITEMDATA_Release(RKC_RPG_ITEMDATA *self);

int RKC_RPG_ITEMDATA_Read(RKC_RPG_ITEMDATA *self, const char *path);

const RKC_RPG_ITEMDATA_Record *RKC_RPG_ITEMDATA_GetFromTemplateId(const RKC_RPG_ITEMDATA *self, int kind,
                                                                  long templateId);

void RKC_RPG_ITEMDATA_GetGridSize(const RKC_RPG_ITEMDATA_Record *record, long *outWidth, long *outHeight);

long RKC_RPG_ITEMDATA_GetAffixTier(const RKC_RPG_ITEMDATA_Record *record);

int RKC_RPG_ITEMDATA_GetGroundSprite(const RKC_RPG_ITEMDATA_Record *record, long *outFolder, long *outFrame);

int RKC_RPG_ITEMDATA_GetBagIcon(const RKC_RPG_ITEMDATA_Record *record, long *outSheet, long *outPattern);

typedef struct
{
    long weight, durabilityMax, requiredLevel;
    long attack, hitRate, defense, evasionRate, magicalAttack, magicalHitRate, magicalDefense, magicalEvasionRate,
        speedOfAttack;
} RKC_RPG_ITEMDATA_TooltipStats;
int RKC_RPG_ITEMDATA_GetTooltipStats(const RKC_RPG_ITEMDATA_Record *record, RKC_RPG_ITEMDATA_TooltipStats *out);
int RKC_RPG_ITEMDATA_GetTooltipStatsFromTail(const void *tail, long tailSize, RKC_RPG_ITEMDATA_TooltipStats *out);

int RKC_RPG_ITEMDATA_GetResistancesFromTail(const void *tail, long tailSize, long out[8]);

long RKC_RPG_ITEMDATA_GetBonusFromTail(const void *tail, long tailSize, int index);

const RKC_RPG_ITEMDATA_Kind0Tail *RKC_RPG_ITEMDATA_GetKind0Tail(const RKC_RPG_ITEMDATA_Record *record);

typedef struct
{
    RKC_RPG_ITEMDATA_Kind0Tail tail;
    long durability;
    int identified;
} RKC_RPG_ITEMDATA_Kind0Instance;

void RKC_RPG_ITEMDATA_RollKind0Instance(const RKC_RPG_ITEMDATA_Kind0Tail *tmpl, RKC_RPG_ITEMDATA_Kind0Instance *out);

const RKC_RPG_ITEMDATA_Kind1Tail *RKC_RPG_ITEMDATA_GetKind1Tail(const RKC_RPG_ITEMDATA_Record *record);
const RKC_RPG_ITEMDATA_Kind2Tail *RKC_RPG_ITEMDATA_GetKind2Tail(const RKC_RPG_ITEMDATA_Record *record);

void RKC_RPG_ITEMDATA_RollKind1Instance(const RKC_RPG_ITEMDATA_Kind1Tail *tmpl, RKC_RPG_ITEMDATA_Kind1Instance *out);
void RKC_RPG_ITEMDATA_RollKind2Instance(const RKC_RPG_ITEMDATA_Kind2Tail *tmpl, RKC_RPG_ITEMDATA_Kind2Instance *out);

#endif
