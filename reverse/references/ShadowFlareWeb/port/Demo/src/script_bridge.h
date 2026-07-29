#ifndef SFDE_GROUND_SCRIPT_BRIDGE_H
#define SFDE_GROUND_SCRIPT_BRIDGE_H

#include "state.h"

typedef struct
{
    const char *spawnLabel;
    DemoState *state; 
    long characterNo; 
} MessagePrintContext;


int LookupLiveSpawnPos(const DemoState *state, long characterNo, long *outX, long *outY);

void PrintMessage(void *userData, long messageId, const char *text);

void PrintUnhandledOpcode(void *userData, long opcode);

void PrintCompassText(void *userData, long messageId, const char *text);


void PrintPlaySound(void *userData, long soundId, int alwaysAudible, int hasPosition, long posX, long posY);


void PrintChangeScenario(void *userData, long scenarioId, long entryPoint);


int GetEntryPoint(void *userData, long *outValue);


int GetPlayerLife(void *userData, long *outHP, long *outMaxHP);


int GetPlayerGold(void *userData, long *outGold);


void PrintPayGold(void *userData, long amount);


int CheckProximity(void *userData, long characterNo, long *outResult);


int CalcPlayerDist(void *userData, long characterNo, long *outDist);


void SetGateLabel(void *userData, long characterNo, const char *text);


void HealPlayerLife(void *userData);


void HealPlayerMental(void *userData);


void RequestSpawnAction(void *userData, long characterNo, long actionNo, long arg1, long arg2, long arg3, long arg4);


int GetPlayerMental(void *userData, long *outMP, long *outMaxMP);


int GetPlayerLevel(void *userData, long *outLevel);


int CheckItemExistInInventory(void *userData, long kind, long templateId, long *outExists);


void DeleteItemFromInventory(void *userData, long kind, long templateId);


void DrawQuestNameBanner(void *userData, long questIndex);


void MessageBoxValue(void *userData, long characterNo, long offsetX, long offsetY, long value, long r, long g, long b);


void SetUnlockSw(void *userData, long value);


void CreateEffect(void *userData, long effectId, long x, long y, long direction);
void CreateEffectChar(void *userData, long effectId, long characterNo);


void ResurrectSpawn(void *userData, long characterNo, long x, long y, long direction);


RKC_RPG_SCRIPT_EXEC_Context BuildExecContext(MessagePrintContext *ctx);


int GetCharacterPos(void *userData, long characterNo, long *outX, long *outY);


int GetCharacterAlive(void *userData, long characterNo, long *outAlive);


void PrintCreateItem(void *userData, long kind, long templateId, long posX, long posY, long amount);


void PrintCreateItemTable(void *userData, long tableRowIndex, long posX, long posY);


int RunTriggersForCharacter(DemoState *state, long characterNo, const char *label);


void TickOnJudgeTriggers(DemoState *state);


void TickExecFunctionTriggers(DemoState *state);


void TickGateHighlights(DemoState *state);


void ApplyPlayerInteract(DemoState *state, const LiveSpawn *spawn);


void HandleInteract(DemoState *state);


void AdvanceDialog(DemoState *state);


void CloseDialog(DemoState *state);

#endif
