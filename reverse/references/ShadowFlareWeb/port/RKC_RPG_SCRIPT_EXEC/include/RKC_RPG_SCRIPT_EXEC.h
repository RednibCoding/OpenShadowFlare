#ifndef SFDE_RKC_RPG_SCRIPT_EXEC_H
#define SFDE_RKC_RPG_SCRIPT_EXEC_H

#include "RKC_RPG_SCRIPT.h"


typedef struct
{
    void (*onMessage)(void *userData, long messageId, const char *text);
    void (*onUnhandledOpcode)(void *userData, long opcode);
    void (*onCompassText)(void *userData, long messageId, const char *text);
    int (*getCharacterPos)(void *userData, long characterNo, long *outX, long *outY);
    int (*getCharacterAlive)(void *userData, long characterNo, long *outAlive);
    void (*onPlaySound)(void *userData, long soundId, int alwaysAudible, int hasPosition, long posX, long posY);
    void (*onChangeScenario)(void *userData, long scenarioId, long entryPoint);
    int (*getEntryPoint)(void *userData, long *outValue);
    void (*onCreateItem)(void *userData, long kind, long templateId, long posX, long posY, long amount);
    void (*onCreateItemTable)(void *userData, long tableRowIndex, long posX, long posY);
    void *userData;
    int (*getPlayerGold)(void *userData, long *outGold);
    void (*onPayGold)(void *userData, long amount);
    int (*getPlayerLife)(void *userData, long *outHP, long *outMaxHP);
    int (*checkProximity)(void *userData, long characterNo, long *outResult);
    int (*calcPlayerDist)(void *userData, long characterNo, long *outDist);
    void (*onGateLabel)(void *userData, long characterNo, const char *text);
    void (*onHealLife)(void *userData);
    void (*onHealMental)(void *userData);
    void (*onActionRequest)(void *userData, long characterNo, long actionNo, long arg1, long arg2, long arg3,
                            long arg4);
    void (*onMessageBoxValue)(void *userData, long characterNo, long offsetX, long offsetY, long value, long r, long g,
                              long b);
    void (*onSetUnlockSw)(void *userData, long value);
    void (*onCreateEffect)(void *userData, long effectId, long x, long y, long direction);
    void (*onCreateEffectChar)(void *userData, long effectId, long characterNo);
    void (*onResurrect)(void *userData, long characterNo, long x, long y, long direction);
    void (*onDrawQuestName)(void *userData, long questIndex);
    int (*checkItemExist)(void *userData, long kind, long templateId, long *outExists);
    void (*onDeleteItem)(void *userData, long kind, long templateId);
    int (*getPlayerLevel)(void *userData, long *outLevel);
    int (*getPlayerMental)(void *userData, long *outMP, long *outMaxMP);
    void (*onWarpGate)(void *userData);
    void (*onDeleteWarpGate)(void *userData);
} RKC_RPG_SCRIPT_EXEC_Context;

#define RKC_RPG_SCRIPT_EXEC_ARRAY_A_CAP 64
#define RKC_RPG_SCRIPT_EXEC_ARRAY_B_CAP 128
#define RKC_RPG_SCRIPT_EXEC_QUEST_CAP 64
#define RKC_RPG_SCRIPT_EXEC_EXSTRG_CAP 16

#define RKC_RPG_SCRIPT_EXEC_ENEMY_SCAN_CAP 100000
#define RKC_RPG_SCRIPT_EXEC_CHARFLAG_VISIBLE_ADDEND 100000000
#define RKC_RPG_SCRIPT_EXEC_CHARFLAG_STATUS_ADDEND 200000000
#define RKC_RPG_SCRIPT_EXEC_CHARFLAG_ACTIVE_ADDEND 300000000
typedef struct
{
    long characterNo;
    int initiallyActive;
} RKC_RPG_SCRIPT_EXEC_CharacterSeed;

typedef struct
{
    long *netFlagValues;
    long netFlagCount;
    long *characterFlagKeys;
    long *characterFlagValues;
    long characterFlagCount;
    long arrayA[RKC_RPG_SCRIPT_EXEC_ARRAY_A_CAP]; /* type 0xa */
    long arrayB[RKC_RPG_SCRIPT_EXEC_ARRAY_B_CAP]; /* type 0xb */
    long questArray[RKC_RPG_SCRIPT_EXEC_QUEST_CAP];
    long exStorage[RKC_RPG_SCRIPT_EXEC_EXSTRG_CAP];
    long questCompletedNotified[RKC_RPG_SCRIPT_EXEC_QUEST_CAP];
} RKC_RPG_SCRIPT_EXEC_State;

void RKC_RPG_SCRIPT_EXEC_State_Init(RKC_RPG_SCRIPT_EXEC_State *self, const RKC_RPG_SCRIPT *script);
void RKC_RPG_SCRIPT_EXEC_State_Release(RKC_RPG_SCRIPT_EXEC_State *self);

typedef struct
{
    long arrayA[RKC_RPG_SCRIPT_EXEC_ARRAY_A_CAP];
    long arrayB[RKC_RPG_SCRIPT_EXEC_ARRAY_B_CAP];
    long questArray[RKC_RPG_SCRIPT_EXEC_QUEST_CAP];
    long exStorage[RKC_RPG_SCRIPT_EXEC_EXSTRG_CAP];
    long questCompletedNotified[RKC_RPG_SCRIPT_EXEC_QUEST_CAP];
} RKC_RPG_SCRIPT_EXEC_Globals;

void RKC_RPG_SCRIPT_EXEC_State_SaveGlobals(const RKC_RPG_SCRIPT_EXEC_State *self, RKC_RPG_SCRIPT_EXEC_Globals *out);
void RKC_RPG_SCRIPT_EXEC_State_RestoreGlobals(RKC_RPG_SCRIPT_EXEC_State *self,
                                              const RKC_RPG_SCRIPT_EXEC_Globals *globals);

void RKC_RPG_SCRIPT_EXEC_State_InitCharacterFlags(RKC_RPG_SCRIPT_EXEC_State *self, const RKC_RPG_SCRIPT *script,
                                                   const RKC_RPG_SCRIPT_EXEC_CharacterSeed *characters,
                                                   long characterCount);

int RKC_RPG_SCRIPT_EXEC_IsCharacterActive(const RKC_RPG_SCRIPT_EXEC_State *state, long characterNo);

void RKC_RPG_SCRIPT_EXEC_RunSentence(const RKC_RPG_SCRIPT *script, RKC_RPG_SCRIPT_EXEC_State *state,
                                     long sentenceIndex, const RKC_RPG_SCRIPT_EXEC_Context *ctx);

#endif
