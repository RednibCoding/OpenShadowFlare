#ifndef SFDE_RKC_RPGSCRN_MCT_H
#define SFDE_RKC_RPGSCRN_MCT_H

typedef struct
{
    long id;
    int groupType;
} RKC_RPGSCRN_MCT_TaggedId;


typedef struct
{
    long sortKey;
    long field8;
    char *name;
    long field158;
    long x, y;
    long rectL, rectT, rectR, rectB;
    long field15;
    long *waypoints;
    long waypointCount;
    int hasSubArray;
    long subCount;
    unsigned int *sub17;   /* DWORD[subCount], NULL unless hasSubArray */
    unsigned short *sub18; /* WORD[subCount], NULL unless hasSubArray */
    unsigned short *sub19; /* WORD[subCount], NULL unless hasSubArray */
    unsigned short *sub20; /* WORD[subCount], NULL unless hasSubArray */
    long field160;
    unsigned char *tail;
    long tailSize;
} RKC_RPGSCRN_MCT_Record;

typedef struct
{
    long kind; /* 0-4 */
    long templateId;
    long randMin, randMax;
} RKC_RPGSCRN_MCT_Block4Tail;


typedef struct
{
    long a, b, c, d;
} RKC_RPGSCRN_MCT_Block5Entry;

typedef struct
{
    char aidPath[0x104];
    char mapPath[0x104];
    long fieldC, fieldD, bgmIndex;
    char displayName[0x100];

    RKC_RPGSCRN_MCT_TaggedId *taggedIds;
    long taggedIdCount;

    RKC_RPGSCRN_MCT_Record *block1;
    long block1Count;
    RKC_RPGSCRN_MCT_Record *block2;
    long block2Count;
    RKC_RPGSCRN_MCT_Record *block3;
    long block3Count;
    RKC_RPGSCRN_MCT_Record *block4;
    long block4Count;

    RKC_RPGSCRN_MCT_Block5Entry *block5;
    long block5Count;

    long fieldG, fieldH;
    long darknessIntensity;
} RKC_RPGSCRN_MCT;

void RKC_RPGSCRN_MCT_Init(RKC_RPGSCRN_MCT *self);
void RKC_RPGSCRN_MCT_Release(RKC_RPGSCRN_MCT *self);

int RKC_RPGSCRN_MCT_Read(RKC_RPGSCRN_MCT *self, const char *path);

int RKC_RPGSCRN_MCT_GetBlock4Tail(const RKC_RPGSCRN_MCT_Record *record, RKC_RPGSCRN_MCT_Block4Tail *out);

int RKC_RPGSCRN_MCT_GetBlock3AiName(const RKC_RPGSCRN_MCT_Record *record, char *out);

int RKC_RPGSCRN_MCT_GetBlock3BaseDamage(const RKC_RPGSCRN_MCT_Record *record, int actionIndex, long *out);

int RKC_RPGSCRN_MCT_GetBlock3MaxHP(const RKC_RPGSCRN_MCT_Record *record, long *out);

int RKC_RPGSCRN_MCT_GetBlock3Level(const RKC_RPGSCRN_MCT_Record *record, long *out);

int RKC_RPGSCRN_MCT_GetBlock3LootTableRow(const RKC_RPGSCRN_MCT_Record *record, long *out);

int RKC_RPGSCRN_MCT_GetBlock3SpeedPercent(const RKC_RPGSCRN_MCT_Record *record, long *out);

int RKC_RPGSCRN_MCT_GetBlock3MoveSpeedX1000(const RKC_RPGSCRN_MCT_Record *record, long *out);

int RKC_RPGSCRN_MCT_GetBlock3AttackChart(const RKC_RPGSCRN_MCT_Record *record, int actionIndex, long *out);

int RKC_RPGSCRN_MCT_GetBlock3SpeedIndex(const RKC_RPGSCRN_MCT_Record *record, int actionIndex, long *out);

int RKC_RPGSCRN_MCT_GetBlock1PatternIndex(const RKC_RPGSCRN_MCT_Record *record, long *out);

int RKC_RPGSCRN_MCT_GetBlock1DrawFlag(const RKC_RPGSCRN_MCT_Record *record, long *out);

int RKC_RPGSCRN_MCT_GetBlock1Height(const RKC_RPGSCRN_MCT_Record *record, long *out);

int RKC_RPGSCRN_MCT_GetBlock1WalkableFlag(const RKC_RPGSCRN_MCT_Record *record, long *out);

int RKC_RPGSCRN_MCT_GetBlock2WanderRect(const RKC_RPGSCRN_MCT_Record *record, long *outL, long *outT, long *outR,
                                        long *outB);

int RKC_RPGSCRN_MCT_GetBlock2FacesTalker(const RKC_RPGSCRN_MCT_Record *record);

int RKC_RPGSCRN_MCT_IsBlock2Stationary(const RKC_RPGSCRN_MCT_Record *record);

int RKC_RPGSCRN_MCT_FindEntryPoint(const RKC_RPGSCRN_MCT *self, long playerCharacterNo, long entryPoint,
                                    RKC_RPGSCRN_MCT_Block5Entry *out);

#endif
