#include "RKC_RPG_SCRIPT_EXEC.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define RKC_RPG_SCRIPT_OP_IF 0x00
#define RKC_RPG_SCRIPT_OP_SETFLAG 0x01
#define RKC_RPG_SCRIPT_OP_TALK 0x02
#define RKC_RPG_SCRIPT_OP_HEALLIFE 0x07
#define RKC_RPG_SCRIPT_OP_HEALMENTAL 0x08
#define RKC_RPG_SCRIPT_OP_CREATEITEM 0x0a
#define RKC_RPG_SCRIPT_OP_CREATEITEMTABLE 0x18
#define RKC_RPG_SCRIPT_OP_ADD 0x0b
#define RKC_RPG_SCRIPT_OP_SUB 0x0c
#define RKC_RPG_SCRIPT_OP_MUL 0x0d
#define RKC_RPG_SCRIPT_OP_DIV 0x0e
#define RKC_RPG_SCRIPT_OP_REM 0x0f
#define RKC_RPG_SCRIPT_OP_PLAYSOUND 0x10
#define RKC_RPG_SCRIPT_OP_CHANGESCENARIO 0x11
#define RKC_RPG_SCRIPT_OP_C_ACTIONREQ 0x14
#define RKC_RPG_SCRIPT_OP_MESSAGEBOXVALUE 0x1a
#define RKC_RPG_SCRIPT_OP_MESSAGEBOX 0x1b
#define RKC_RPG_SCRIPT_OP_CHECKENEMYLIVE 0x1f
#define RKC_RPG_SCRIPT_OP_CHECKENEMYDEAD 0x20
#define RKC_RPG_SCRIPT_OP_CALCMYPLAYERDIST 0x22
#define RKC_RPG_SCRIPT_OP_WARPGATE 0x25
#define RKC_RPG_SCRIPT_OP_DELETEWARPGATE 0x26
#define RKC_RPG_SCRIPT_OP_GETRANDAM 0x27
#define RKC_RPG_SCRIPT_OP_SETACTIVE 0x16
#define RKC_RPG_SCRIPT_OP_SETINACTIVE 0x17
#define RKC_RPG_SCRIPT_OP_ROUTIN 0x1c
#define RKC_RPG_SCRIPT_OP_ROUTINNET 0x1d
#define RKC_RPG_SCRIPT_OP_SETCOMPASSES 0x31
#define RKC_RPG_SCRIPT_OP_GETENTRYPOINT 0x32
#define RKC_RPG_SCRIPT_OP_GETPLAYERLIFE 0x2a
#define RKC_RPG_SCRIPT_OP_GETTOTALGOLD 0x35
#define RKC_RPG_SCRIPT_OP_PAYGOLD 0x36
#define RKC_RPG_SCRIPT_OP_SETQUESTFLAG 0x3e
#define RKC_RPG_SCRIPT_OP_SETUNLOCKSW 0x3c
#define RKC_RPG_SCRIPT_OP_CREATEEFFECT 0x24
#define RKC_RPG_SCRIPT_OP_CREATEEFFECTCHAR 0x28
#define RKC_RPG_SCRIPT_OP_RESSURECT 0x19
#define RKC_RPG_SCRIPT_OP_DRAWQUESTNAME 0x30
#define RKC_RPG_SCRIPT_OP_ABSOLUTEOBJECTFLAG 0x38
#define RKC_RPG_SCRIPT_OP_CHECKITEMEXIST 0x3a
#define RKC_RPG_SCRIPT_OP_DELETEITEM 0x3b
#define RKC_RPG_SCRIPT_OP_GETPLAYERLEVEL 0x3d
#define RKC_RPG_SCRIPT_OP_GETPLAYERMENTAL 0x2b
#define RKC_RPG_SCRIPT_QUEST_START_SOUND 0x41
#define RKC_RPG_SCRIPT_QUEST_COMPLETE_SOUND 0x42
#define RKC_RPG_SCRIPT_OPERAND_TYPE_ENEMYALIVE 3
#define RKC_RPG_SCRIPT_OPERAND_TYPE_TEMPFLAG 4
#define RKC_RPG_SCRIPT_OPERAND_TYPE_CCHECK 9
#define RKC_RPG_SCRIPT_OPERAND_TYPE_NETFLAG 5
#define RKC_RPG_SCRIPT_OPERAND_TYPE_CPOSX 6
#define RKC_RPG_SCRIPT_OPERAND_TYPE_CPOSY 7
#define RKC_RPG_SCRIPT_OPERAND_TYPE_PLAYMODE 8
#define RKC_RPG_SCRIPT_OPERAND_TYPE_ARRAY_A 0x0a
#define RKC_RPG_SCRIPT_OPERAND_TYPE_ARRAY_B 0x0b
#define RKC_RPG_SCRIPT_OPERAND_TYPE_QUEST 0x0c
#define RKC_RPG_SCRIPT_OPERAND_TYPE_EXSTRG 0x0d
#define RKC_RPG_SCRIPT_ENEMY_ADDEND 14000000
#define RKC_RPG_SCRIPT_EXEC_MAX_DEPTH 32

void RKC_RPG_SCRIPT_EXEC_State_Init(RKC_RPG_SCRIPT_EXEC_State *self, const RKC_RPG_SCRIPT *script)
{
    self->netFlagCount = script->netFlagCount;
    self->netFlagValues = self->netFlagCount > 0 ? malloc(sizeof(long) * (size_t)self->netFlagCount) : NULL;
    for (long i = 0; i < self->netFlagCount; i++)
        self->netFlagValues[i] = script->netFlags[i].field1;

    self->characterFlagKeys = NULL;
    self->characterFlagValues = NULL;
    self->characterFlagCount = 0;

    memset(self->arrayA, 0, sizeof(self->arrayA));
    memset(self->arrayB, 0, sizeof(self->arrayB));
    memset(self->questArray, 0, sizeof(self->questArray));
    memset(self->exStorage, 0, sizeof(self->exStorage));
    memset(self->questCompletedNotified, 0, sizeof(self->questCompletedNotified));
}

void RKC_RPG_SCRIPT_EXEC_State_SaveGlobals(const RKC_RPG_SCRIPT_EXEC_State *self, RKC_RPG_SCRIPT_EXEC_Globals *out)
{
    memcpy(out->arrayA, self->arrayA, sizeof(out->arrayA));
    memcpy(out->arrayB, self->arrayB, sizeof(out->arrayB));
    memcpy(out->questArray, self->questArray, sizeof(out->questArray));
    memcpy(out->exStorage, self->exStorage, sizeof(out->exStorage));
    memcpy(out->questCompletedNotified, self->questCompletedNotified, sizeof(out->questCompletedNotified));
}

void RKC_RPG_SCRIPT_EXEC_State_RestoreGlobals(RKC_RPG_SCRIPT_EXEC_State *self,
                                              const RKC_RPG_SCRIPT_EXEC_Globals *globals)
{
    memcpy(self->arrayA, globals->arrayA, sizeof(self->arrayA));
    memcpy(self->arrayB, globals->arrayB, sizeof(self->arrayB));
    memcpy(self->questArray, globals->questArray, sizeof(self->questArray));
    memcpy(self->exStorage, globals->exStorage, sizeof(self->exStorage));
    memcpy(self->questCompletedNotified, globals->questCompletedNotified, sizeof(self->questCompletedNotified));
}

void RKC_RPG_SCRIPT_EXEC_State_InitCharacterFlags(RKC_RPG_SCRIPT_EXEC_State *self, const RKC_RPG_SCRIPT *script,
                                                   const RKC_RPG_SCRIPT_EXEC_CharacterSeed *characters,
                                                   long characterCount)
{
    long total = script->tempFlagCount + characterCount * 3;
    self->characterFlagCount = total;
    if (total <= 0)
    {
        self->characterFlagKeys = NULL;
        self->characterFlagValues = NULL;
        return;
    }
    self->characterFlagKeys = malloc(sizeof(long) * (size_t)total);
    self->characterFlagValues = malloc(sizeof(long) * (size_t)total);

    long n = 0;
    for (long i = 0; i < script->tempFlagCount; i++)
    {
        self->characterFlagKeys[n] = script->tempFlags[i].field0;
        self->characterFlagValues[n] = script->tempFlags[i].field1;
        n++;
    }
    for (long i = 0; i < characterCount; i++)
    {
        long value = characters[i].initiallyActive ? 1 : 0;
        self->characterFlagKeys[n] = characters[i].characterNo + RKC_RPG_SCRIPT_EXEC_CHARFLAG_VISIBLE_ADDEND;
        self->characterFlagValues[n] = value;
        n++;
        self->characterFlagKeys[n] = characters[i].characterNo + RKC_RPG_SCRIPT_EXEC_CHARFLAG_STATUS_ADDEND;
        self->characterFlagValues[n] = value;
        n++;
        self->characterFlagKeys[n] = characters[i].characterNo + RKC_RPG_SCRIPT_EXEC_CHARFLAG_ACTIVE_ADDEND;
        self->characterFlagValues[n] = value;
        n++;
    }
}

void RKC_RPG_SCRIPT_EXEC_State_Release(RKC_RPG_SCRIPT_EXEC_State *self)
{
    free(self->netFlagValues);
    self->netFlagValues = NULL;
    self->netFlagCount = 0;
    free(self->characterFlagKeys);
    free(self->characterFlagValues);
    self->characterFlagKeys = NULL;
    self->characterFlagValues = NULL;
    self->characterFlagCount = 0;
}

static const RKC_RPG_SCRIPT_Message *FindMessageById(const RKC_RPG_SCRIPT *script, long id)
{
    for (long i = 0; i < script->messageCount; i++)
        if (script->messages[i].id == id)
            return &script->messages[i];
    return NULL;
}

static long FindNetFlagIndex(const RKC_RPG_SCRIPT *script, long key)
{
    for (long i = 0; i < script->netFlagCount; i++)
        if (script->netFlags[i].field0 == key)
            return i;
    return -1;
}

static long FindCharacterFlagIndex(const RKC_RPG_SCRIPT_EXEC_State *state, long key)
{
    for (long i = 0; i < state->characterFlagCount; i++)
        if (state->characterFlagKeys[i] == key)
            return i;
    return -1;
}

int RKC_RPG_SCRIPT_EXEC_IsCharacterActive(const RKC_RPG_SCRIPT_EXEC_State *state, long characterNo)
{
    long idx = FindCharacterFlagIndex(state, characterNo + RKC_RPG_SCRIPT_EXEC_CHARFLAG_VISIBLE_ADDEND);
    return idx < 0 || state->characterFlagValues[idx] != 0;
}

static long EvalOperand(const RKC_RPG_SCRIPT *script, const RKC_RPG_SCRIPT_EXEC_State *state,
                         const RKC_RPG_SCRIPT_EXEC_Context *ctx, const RKC_RPG_SCRIPT_Operand *op)
{
    switch (op->type)
    {
    case 0:
    case 1:
    case 2:
        return op->value;
    case RKC_RPG_SCRIPT_OPERAND_TYPE_ENEMYALIVE:
    {
        long alive;
        if (ctx->getCharacterAlive &&
            ctx->getCharacterAlive(ctx->userData, op->value + RKC_RPG_SCRIPT_ENEMY_ADDEND, &alive))
            return alive;
        return -1;
    }
    case RKC_RPG_SCRIPT_OPERAND_TYPE_TEMPFLAG:
    {
        long idx = FindNetFlagIndex(script, op->value);
        return idx >= 0 ? state->netFlagValues[idx] : -1;
    }
    case RKC_RPG_SCRIPT_OPERAND_TYPE_NETFLAG:
    {
        long idx = FindCharacterFlagIndex(state, op->value);
        return idx >= 0 ? state->characterFlagValues[idx] : -1;
    }
    case RKC_RPG_SCRIPT_OPERAND_TYPE_CPOSX:
    case RKC_RPG_SCRIPT_OPERAND_TYPE_CPOSY:
    {
        long x, y;
        if (ctx->getCharacterPos && ctx->getCharacterPos(ctx->userData, op->value, &x, &y))
            return op->type == RKC_RPG_SCRIPT_OPERAND_TYPE_CPOSX ? x : y;
        return -1;
    }
    case RKC_RPG_SCRIPT_OPERAND_TYPE_PLAYMODE:
        return 0;
    case RKC_RPG_SCRIPT_OPERAND_TYPE_CCHECK:
    {
        long result;
        if (ctx->checkProximity && ctx->checkProximity(ctx->userData, op->value, &result))
            return result;
        return -1;
    }
    case RKC_RPG_SCRIPT_OPERAND_TYPE_ARRAY_A:
        return (op->value >= 0 && op->value < RKC_RPG_SCRIPT_EXEC_ARRAY_A_CAP) ? state->arrayA[op->value] : -1;
    case RKC_RPG_SCRIPT_OPERAND_TYPE_ARRAY_B:
        return (op->value >= 0 && op->value < RKC_RPG_SCRIPT_EXEC_ARRAY_B_CAP) ? state->arrayB[op->value] : -1;
    case RKC_RPG_SCRIPT_OPERAND_TYPE_QUEST:
        return (op->value >= 0 && op->value < RKC_RPG_SCRIPT_EXEC_QUEST_CAP) ? state->questArray[op->value] : -1;
    case RKC_RPG_SCRIPT_OPERAND_TYPE_EXSTRG:
        return (op->value >= 0 && op->value < RKC_RPG_SCRIPT_EXEC_EXSTRG_CAP) ? state->exStorage[op->value] : -1;
    default:
        return -1;
    }
}

static void SetOperand(const RKC_RPG_SCRIPT *script, RKC_RPG_SCRIPT_EXEC_State *state,
                        const RKC_RPG_SCRIPT_Operand *op, long value)
{
    if (op->type == RKC_RPG_SCRIPT_OPERAND_TYPE_TEMPFLAG)
    {
        long idx = FindNetFlagIndex(script, op->value);
        if (idx >= 0)
            state->netFlagValues[idx] = value;
    }
    else if (op->type == RKC_RPG_SCRIPT_OPERAND_TYPE_NETFLAG)
    {
        long idx = FindCharacterFlagIndex(state, op->value);
        if (idx >= 0)
            state->characterFlagValues[idx] = value;
    }
    else if (op->type == RKC_RPG_SCRIPT_OPERAND_TYPE_ARRAY_A)
    {
        if (op->value >= 0 && op->value < RKC_RPG_SCRIPT_EXEC_ARRAY_A_CAP)
            state->arrayA[op->value] = value;
    }
    else if (op->type == RKC_RPG_SCRIPT_OPERAND_TYPE_ARRAY_B)
    {
        if (op->value >= 0 && op->value < RKC_RPG_SCRIPT_EXEC_ARRAY_B_CAP)
            state->arrayB[op->value] = value;
    }
    else if (op->type == RKC_RPG_SCRIPT_OPERAND_TYPE_EXSTRG)
    {
        if (op->value >= 0 && op->value < RKC_RPG_SCRIPT_EXEC_EXSTRG_CAP)
            state->exStorage[op->value] = value;
    }
}

static void RunArithmetic(const RKC_RPG_SCRIPT *script, RKC_RPG_SCRIPT_EXEC_State *state,
                           const RKC_RPG_SCRIPT_EXEC_Context *ctx, const RKC_RPG_SCRIPT_Command *cmd)
{
    if (cmd->operandCount < 2)
    {
        if (ctx->onUnhandledOpcode)
            ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
        return;
    }
    long a = EvalOperand(script, state, ctx, &cmd->operands[0]);
    long b = EvalOperand(script, state, ctx, &cmd->operands[1]);
    long result;
    switch (cmd->opcode)
    {
    case RKC_RPG_SCRIPT_OP_ADD:
        result = a + b;
        break;
    case RKC_RPG_SCRIPT_OP_SUB:
        result = a - b;
        break;
    case RKC_RPG_SCRIPT_OP_MUL:
        result = a * b;
        break;
    case RKC_RPG_SCRIPT_OP_DIV:
        if (b == 0)
            return;
        result = a / b;
        break;
    case RKC_RPG_SCRIPT_OP_REM:
        if (b == 0)
            return;
        result = a % b;
        break;
    default:
        return;
    }
    SetOperand(script, state, &cmd->operands[0], result);
}

static void RunGetRandam(const RKC_RPG_SCRIPT *script, RKC_RPG_SCRIPT_EXEC_State *state,
                          const RKC_RPG_SCRIPT_EXEC_Context *ctx, const RKC_RPG_SCRIPT_Command *cmd)
{
    if (cmd->operandCount < 3)
    {
        if (ctx->onUnhandledOpcode)
            ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
        return;
    }
    long lo = EvalOperand(script, state, ctx, &cmd->operands[0]);
    long hi = EvalOperand(script, state, ctx, &cmd->operands[1]);
    if (hi < lo)
        return;
    long value = rand() % (hi - lo + 1) + lo;
    SetOperand(script, state, &cmd->operands[2], value);
}

static void RunCheckEnemyStatus(const RKC_RPG_SCRIPT *script, RKC_RPG_SCRIPT_EXEC_State *state,
                                 const RKC_RPG_SCRIPT_EXEC_Context *ctx, const RKC_RPG_SCRIPT_Command *cmd,
                                 long wantAlive)
{
    if (cmd->operandCount < 3)
    {
        if (ctx->onUnhandledOpcode)
            ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
        return;
    }
    long lo = EvalOperand(script, state, ctx, &cmd->operands[0]);
    long hi = EvalOperand(script, state, ctx, &cmd->operands[1]);
    if (lo > hi)
        return;
    long found = -1;
    if (ctx->getCharacterAlive)
    {
        long count = hi - lo + 1;
        if (count > RKC_RPG_SCRIPT_EXEC_ENEMY_SCAN_CAP)
            count = RKC_RPG_SCRIPT_EXEC_ENEMY_SCAN_CAP;
        for (long i = 0; i < count; i++)
        {
            long key = lo + i;
            long alive;
            if (ctx->getCharacterAlive(ctx->userData, key, &alive) && alive == wantAlive)
            {
                found = key;
                break;
            }
        }
    }
    SetOperand(script, state, &cmd->operands[2], found);
}

static void RunCreateItem(const RKC_RPG_SCRIPT *script, RKC_RPG_SCRIPT_EXEC_State *state,
                           const RKC_RPG_SCRIPT_EXEC_Context *ctx, const RKC_RPG_SCRIPT_Command *cmd)
{
    if (cmd->operandCount < 4)
    {
        if (ctx->onUnhandledOpcode)
            ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
        return;
    }
    long kind = EvalOperand(script, state, ctx, &cmd->operands[0]);
    long templateId = EvalOperand(script, state, ctx, &cmd->operands[1]);
    long posX = EvalOperand(script, state, ctx, &cmd->operands[2]);
    long posY = EvalOperand(script, state, ctx, &cmd->operands[3]);

    long amount = 1;
    if (kind == 4 && templateId == 0 && cmd->operandCount >= 6)
    {
        long lo = EvalOperand(script, state, ctx, &cmd->operands[4]);
        long hi = EvalOperand(script, state, ctx, &cmd->operands[5]);
        if (hi >= lo)
            amount = rand() % (hi - lo + 1) + lo;
        else
            amount = lo;
    }

    if (ctx->onCreateItem)
        ctx->onCreateItem(ctx->userData, kind, templateId, posX, posY, amount);
}

static void RunPlaySound(const RKC_RPG_SCRIPT *script, RKC_RPG_SCRIPT_EXEC_State *state,
                          const RKC_RPG_SCRIPT_EXEC_Context *ctx, const RKC_RPG_SCRIPT_Command *cmd)
{
    if (cmd->operandCount < 1)
    {
        if (ctx->onUnhandledOpcode)
            ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
        return;
    }
    long soundId = EvalOperand(script, state, ctx, &cmd->operands[0]);

    int alwaysAudible = 0;
    if (cmd->operandCount >= 2)
        alwaysAudible = EvalOperand(script, state, ctx, &cmd->operands[1]) != 0;

    int hasPosition = 0;
    long posX = 0, posY = 0;
    if (cmd->operandCount >= 3)
    {
        long opX = EvalOperand(script, state, ctx, &cmd->operands[2]);
        if (opX != -1)
        {
            hasPosition = 1;
            posX = opX;
            posY = cmd->operandCount >= 4 ? EvalOperand(script, state, ctx, &cmd->operands[3]) : 0;
        }
    }

    if (ctx->onPlaySound)
        ctx->onPlaySound(ctx->userData, soundId, alwaysAudible, hasPosition, posX, posY);
}

static void RunCreateItemTable(const RKC_RPG_SCRIPT *script, RKC_RPG_SCRIPT_EXEC_State *state,
                                 const RKC_RPG_SCRIPT_EXEC_Context *ctx, const RKC_RPG_SCRIPT_Command *cmd)
{
    if (cmd->operandCount < 3)
    {
        if (ctx->onUnhandledOpcode)
            ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
        return;
    }
    long tableRowIndex = EvalOperand(script, state, ctx, &cmd->operands[0]);
    long posX = EvalOperand(script, state, ctx, &cmd->operands[1]);
    long posY = EvalOperand(script, state, ctx, &cmd->operands[2]);

    if (ctx->onCreateItemTable)
        ctx->onCreateItemTable(ctx->userData, tableRowIndex, posX, posY);
}

static void RunSetQuestFlag(const RKC_RPG_SCRIPT *script, RKC_RPG_SCRIPT_EXEC_State *state,
                             const RKC_RPG_SCRIPT_EXEC_Context *ctx, const RKC_RPG_SCRIPT_Command *cmd)
{
    if (cmd->operandCount < 2)
    {
        if (ctx->onUnhandledOpcode)
            ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
        return;
    }
    long questIndex = EvalOperand(script, state, ctx, &cmd->operands[0]);
    long newValue = EvalOperand(script, state, ctx, &cmd->operands[1]);
    if (questIndex < 0 || questIndex >= RKC_RPG_SCRIPT_EXEC_QUEST_CAP)
        return;

    if (newValue != 2)
    {
        state->questArray[questIndex] = newValue;
        if (ctx->onPlaySound)
            ctx->onPlaySound(ctx->userData, RKC_RPG_SCRIPT_QUEST_START_SOUND, 1, 0, 0, 0);
        return;
    }
    if (state->questCompletedNotified[questIndex] != 0)
        return;
    state->questCompletedNotified[questIndex] = 1;
    if (cmd->operandCount >= 3)
        EvalOperand(script, state, ctx, &cmd->operands[2]);
    if (state->questArray[questIndex] == 1)
    {
        state->questArray[questIndex] = 2;
        if (ctx->onPlaySound)
            ctx->onPlaySound(ctx->userData, RKC_RPG_SCRIPT_QUEST_COMPLETE_SOUND, 1, 0, 0, 0);
    }
}

static void RunActionRequest(const RKC_RPG_SCRIPT *script, RKC_RPG_SCRIPT_EXEC_State *state,
                              const RKC_RPG_SCRIPT_EXEC_Context *ctx, const RKC_RPG_SCRIPT_Command *cmd)
{
    if (cmd->operandCount < 2)
    {
        if (ctx->onUnhandledOpcode)
            ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
        return;
    }
    long characterNo = EvalOperand(script, state, ctx, &cmd->operands[0]);
    long args[5] = {-1, -1, -1, -1, -1};
    for (long k = 1; k < cmd->operandCount && k <= 5; k++)
        args[k - 1] = EvalOperand(script, state, ctx, &cmd->operands[k]);
    if (ctx->onActionRequest)
        ctx->onActionRequest(ctx->userData, characterNo, args[0], args[1], args[2], args[3], args[4]);
}

static void RunCalcMyPlayerDist(const RKC_RPG_SCRIPT *script, RKC_RPG_SCRIPT_EXEC_State *state,
                                  const RKC_RPG_SCRIPT_EXEC_Context *ctx, const RKC_RPG_SCRIPT_Command *cmd)
{
    if (cmd->operandCount < 2)
    {
        if (ctx->onUnhandledOpcode)
            ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
        return;
    }
    long characterNo = EvalOperand(script, state, ctx, &cmd->operands[0]);
    long dist = -1;
    if (ctx->calcPlayerDist)
        ctx->calcPlayerDist(ctx->userData, characterNo, &dist);
    SetOperand(script, state, &cmd->operands[1], dist);
}

static void RunSetActive(const RKC_RPG_SCRIPT *script, RKC_RPG_SCRIPT_EXEC_State *state,
                          const RKC_RPG_SCRIPT_EXEC_Context *ctx, const RKC_RPG_SCRIPT_Command *cmd, long active)
{
    if (cmd->operandCount < 1)
    {
        if (ctx->onUnhandledOpcode)
            ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
        return;
    }
    long characterNo = EvalOperand(script, state, ctx, &cmd->operands[0]);
    static const long addends[3] = {RKC_RPG_SCRIPT_EXEC_CHARFLAG_VISIBLE_ADDEND,
                                     RKC_RPG_SCRIPT_EXEC_CHARFLAG_STATUS_ADDEND,
                                     RKC_RPG_SCRIPT_EXEC_CHARFLAG_ACTIVE_ADDEND};
    for (int k = 0; k < 3; k++)
    {
        long idx = FindCharacterFlagIndex(state, characterNo + addends[k]);
        if (idx >= 0)
            state->characterFlagValues[idx] = active;
    }
}

static void RunAbsoluteObjectFlag(const RKC_RPG_SCRIPT *script, RKC_RPG_SCRIPT_EXEC_State *state,
                                   const RKC_RPG_SCRIPT_EXEC_Context *ctx, const RKC_RPG_SCRIPT_Command *cmd)
{
    if (cmd->operandCount < 4)
    {
        if (ctx->onUnhandledOpcode)
            ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
        return;
    }
    long characterNo = EvalOperand(script, state, ctx, &cmd->operands[0]);
    static const long addends[3] = {RKC_RPG_SCRIPT_EXEC_CHARFLAG_VISIBLE_ADDEND,
                                     RKC_RPG_SCRIPT_EXEC_CHARFLAG_STATUS_ADDEND,
                                     RKC_RPG_SCRIPT_EXEC_CHARFLAG_ACTIVE_ADDEND};
    for (int k = 0; k < 3; k++)
    {
        long value = EvalOperand(script, state, ctx, &cmd->operands[1 + k]);
        long idx = FindCharacterFlagIndex(state, characterNo + addends[k]);
        if (idx >= 0)
            state->characterFlagValues[idx] = value;
    }
}

static void RunSentenceDepth(const RKC_RPG_SCRIPT *script, RKC_RPG_SCRIPT_EXEC_State *state, long sentenceIndex,
                              const RKC_RPG_SCRIPT_EXEC_Context *ctx, int depth)
{
    if (sentenceIndex < 0 || sentenceIndex >= script->sentenceCount || depth >= RKC_RPG_SCRIPT_EXEC_MAX_DEPTH)
        return;

    const RKC_RPG_SCRIPT_Sentence *sentence = &script->sentences[sentenceIndex];
    for (long c = 0; c < sentence->commandCount; c++)
    {
        const RKC_RPG_SCRIPT_Command *cmd = &sentence->commands[c];

        switch (cmd->opcode)
        {
        case RKC_RPG_SCRIPT_OP_IF:
        {
            if (cmd->operandCount < 4)
            {
                if (ctx->onUnhandledOpcode)
                    ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
                break;
            }
            long a = EvalOperand(script, state, ctx, &cmd->operands[0]);
            long b = EvalOperand(script, state, ctx, &cmd->operands[2]);
            int taken;
            switch (cmd->operands[1].value)
            {
            case 0:
                taken = a == b;
                break;
            case 1:
                taken = a != b;
                break;
            case 2:
                taken = a > b;
                break;
            case 3:
                taken = a < b;
                break;
            default:
                taken = 0;
                break;
            }
            if (taken)
                RunSentenceDepth(script, state, cmd->operands[3].value, ctx, depth + 1);
            break;
        }
        case RKC_RPG_SCRIPT_OP_SETFLAG:
            if (cmd->operandCount >= 2)
                SetOperand(script, state, &cmd->operands[0], EvalOperand(script, state, ctx, &cmd->operands[1]));
            else if (ctx->onUnhandledOpcode)
                ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
            break;
        case RKC_RPG_SCRIPT_OP_ADD:
        case RKC_RPG_SCRIPT_OP_SUB:
        case RKC_RPG_SCRIPT_OP_MUL:
        case RKC_RPG_SCRIPT_OP_DIV:
        case RKC_RPG_SCRIPT_OP_REM:
            RunArithmetic(script, state, ctx, cmd);
            break;
        case RKC_RPG_SCRIPT_OP_GETRANDAM:
            RunGetRandam(script, state, ctx, cmd);
            break;
        case RKC_RPG_SCRIPT_OP_CREATEITEM:
            RunCreateItem(script, state, ctx, cmd);
            break;
        case RKC_RPG_SCRIPT_OP_CREATEITEMTABLE:
            RunCreateItemTable(script, state, ctx, cmd);
            break;
        case RKC_RPG_SCRIPT_OP_CHECKENEMYLIVE:
            RunCheckEnemyStatus(script, state, ctx, cmd, 1);
            break;
        case RKC_RPG_SCRIPT_OP_CHECKENEMYDEAD:
            RunCheckEnemyStatus(script, state, ctx, cmd, 0);
            break;
        case RKC_RPG_SCRIPT_OP_PLAYSOUND:
            RunPlaySound(script, state, ctx, cmd);
            break;
        case RKC_RPG_SCRIPT_OP_HEALLIFE:
            if (ctx->onHealLife)
                ctx->onHealLife(ctx->userData);
            break;
        case RKC_RPG_SCRIPT_OP_HEALMENTAL:
            if (ctx->onHealMental)
                ctx->onHealMental(ctx->userData);
            break;
        case RKC_RPG_SCRIPT_OP_C_ACTIONREQ:
            RunActionRequest(script, state, ctx, cmd);
            break;
        case RKC_RPG_SCRIPT_OP_CHANGESCENARIO:
            if (cmd->operandCount >= 2)
            {
                if (ctx->onChangeScenario)
                    ctx->onChangeScenario(ctx->userData, EvalOperand(script, state, ctx, &cmd->operands[0]),
                                           EvalOperand(script, state, ctx, &cmd->operands[1]));
            }
            else if (ctx->onUnhandledOpcode)
                ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
            break;
        case RKC_RPG_SCRIPT_OP_TALK:
        {
            const RKC_RPG_SCRIPT_Message *msg =
                cmd->operandCount > 0
                    ? FindMessageById(script, EvalOperand(script, state, ctx, &cmd->operands[0]))
                    : NULL;
            if (msg)
            {
                if (ctx->onMessage)
                    ctx->onMessage(ctx->userData, msg->id, msg->text ? msg->text : "");
            }
            else if (ctx->onUnhandledOpcode)
                ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
            break;
        }
        case RKC_RPG_SCRIPT_OP_MESSAGEBOXVALUE:
        {
            if (cmd->operandCount < 7 || !ctx->onMessageBoxValue)
            {
                if (ctx->onUnhandledOpcode)
                    ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
                break;
            }
            ctx->onMessageBoxValue(ctx->userData, EvalOperand(script, state, ctx, &cmd->operands[0]),
                                   EvalOperand(script, state, ctx, &cmd->operands[1]),
                                   EvalOperand(script, state, ctx, &cmd->operands[2]),
                                   EvalOperand(script, state, ctx, &cmd->operands[3]),
                                   EvalOperand(script, state, ctx, &cmd->operands[4]),
                                   EvalOperand(script, state, ctx, &cmd->operands[5]),
                                   EvalOperand(script, state, ctx, &cmd->operands[6]));
            break;
        }
        case RKC_RPG_SCRIPT_OP_SETUNLOCKSW:
        {
            if (cmd->operandCount < 1 || !ctx->onSetUnlockSw)
            {
                if (ctx->onUnhandledOpcode)
                    ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
                break;
            }
            ctx->onSetUnlockSw(ctx->userData, EvalOperand(script, state, ctx, &cmd->operands[0]));
            break;
        }
        case RKC_RPG_SCRIPT_OP_CREATEEFFECT:
        {
            if (cmd->operandCount < 7 || !ctx->onCreateEffect)
            {
                if (ctx->onUnhandledOpcode)
                    ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
                break;
            }
            long fxDir = EvalOperand(script, state, ctx, &cmd->operands[4]);
            if (fxDir < 0)
                fxDir = 8;
            ctx->onCreateEffect(ctx->userData, EvalOperand(script, state, ctx, &cmd->operands[0]),
                                EvalOperand(script, state, ctx, &cmd->operands[1]),
                                EvalOperand(script, state, ctx, &cmd->operands[2]), fxDir);
            break;
        }
        case RKC_RPG_SCRIPT_OP_CREATEEFFECTCHAR:
        {
            if (cmd->operandCount < 2 || !ctx->onCreateEffectChar)
            {
                if (ctx->onUnhandledOpcode)
                    ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
                break;
            }
            ctx->onCreateEffectChar(ctx->userData, EvalOperand(script, state, ctx, &cmd->operands[0]),
                                    EvalOperand(script, state, ctx, &cmd->operands[1]));
            break;
        }
        case RKC_RPG_SCRIPT_OP_RESSURECT:
        {
            if (cmd->operandCount < 4 || !ctx->onResurrect)
            {
                if (ctx->onUnhandledOpcode)
                    ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
                break;
            }
            ctx->onResurrect(ctx->userData, EvalOperand(script, state, ctx, &cmd->operands[0]),
                             EvalOperand(script, state, ctx, &cmd->operands[1]),
                             EvalOperand(script, state, ctx, &cmd->operands[2]),
                             EvalOperand(script, state, ctx, &cmd->operands[3]));
            break;
        }
        case RKC_RPG_SCRIPT_OP_MESSAGEBOX:
        {
            const RKC_RPG_SCRIPT_Message *msg =
                cmd->operandCount > 3
                    ? FindMessageById(script, EvalOperand(script, state, ctx, &cmd->operands[3]))
                    : NULL;
            if (msg)
            {
                long characterNo = EvalOperand(script, state, ctx, &cmd->operands[0]);
                if (ctx->onGateLabel)
                    ctx->onGateLabel(ctx->userData, characterNo, msg->text ? msg->text : "");
            }
            else if (ctx->onUnhandledOpcode)
                ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
            break;
        }
        case RKC_RPG_SCRIPT_OP_SETCOMPASSES:
        {
            const RKC_RPG_SCRIPT_Message *msg =
                cmd->operandCount > 0 ? FindMessageById(script, cmd->operands[0].value) : NULL;
            if (msg)
            {
                if (ctx->onCompassText)
                    ctx->onCompassText(ctx->userData, msg->id, msg->text ? msg->text : "");
            }
            else if (ctx->onUnhandledOpcode)
                ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
            break;
        }
        case RKC_RPG_SCRIPT_OP_GETENTRYPOINT:
        {
            long value;
            if (!ctx->getEntryPoint || !ctx->getEntryPoint(ctx->userData, &value))
                value = -1;
            if (cmd->operandCount >= 1)
                SetOperand(script, state, &cmd->operands[0], value);
            else if (ctx->onUnhandledOpcode)
                ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
            break;
        }
        case RKC_RPG_SCRIPT_OP_GETPLAYERLIFE:
        {
            long hp = -1, maxHp = -1;
            if (ctx->getPlayerLife)
                ctx->getPlayerLife(ctx->userData, &hp, &maxHp);
            if (cmd->operandCount >= 2)
            {
                SetOperand(script, state, &cmd->operands[0], hp);
                SetOperand(script, state, &cmd->operands[1], maxHp);
            }
            else if (ctx->onUnhandledOpcode)
                ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
            break;
        }
        case RKC_RPG_SCRIPT_OP_GETPLAYERMENTAL:
        {
            long mp = -1, maxMp = -1;
            if (ctx->getPlayerMental)
                ctx->getPlayerMental(ctx->userData, &mp, &maxMp);
            if (cmd->operandCount >= 2)
            {
                SetOperand(script, state, &cmd->operands[0], mp);
                SetOperand(script, state, &cmd->operands[1], maxMp);
            }
            else if (ctx->onUnhandledOpcode)
                ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
            break;
        }
        case RKC_RPG_SCRIPT_OP_GETPLAYERLEVEL:
        {
            long level = -1;
            if (ctx->getPlayerLevel)
                ctx->getPlayerLevel(ctx->userData, &level);
            if (cmd->operandCount >= 1)
                SetOperand(script, state, &cmd->operands[0], level);
            else if (ctx->onUnhandledOpcode)
                ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
            break;
        }
        case RKC_RPG_SCRIPT_OP_DRAWQUESTNAME:
        {
            if (cmd->operandCount >= 1 && ctx->onDrawQuestName)
                ctx->onDrawQuestName(ctx->userData, EvalOperand(script, state, ctx, &cmd->operands[0]));
            else if (ctx->onUnhandledOpcode)
                ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
            break;
        }
        case RKC_RPG_SCRIPT_OP_CHECKITEMEXIST:
        {
            if (cmd->operandCount < 3)
            {
                if (ctx->onUnhandledOpcode)
                    ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
                break;
            }
            long kind = EvalOperand(script, state, ctx, &cmd->operands[0]);
            long templateId = EvalOperand(script, state, ctx, &cmd->operands[1]);
            long exists = -1;
            if (ctx->checkItemExist)
            {
                long got = 0;
                if (ctx->checkItemExist(ctx->userData, kind, templateId, &got))
                    exists = got;
            }
            SetOperand(script, state, &cmd->operands[2], exists);
            break;
        }
        case RKC_RPG_SCRIPT_OP_DELETEITEM:
        {
            if (cmd->operandCount >= 2 && ctx->onDeleteItem)
                ctx->onDeleteItem(ctx->userData, EvalOperand(script, state, ctx, &cmd->operands[0]),
                                  EvalOperand(script, state, ctx, &cmd->operands[1]));
            else if (ctx->onUnhandledOpcode)
                ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
            break;
        }
        case RKC_RPG_SCRIPT_OP_GETTOTALGOLD:
        {
            long gold;
            if (!ctx->getPlayerGold || !ctx->getPlayerGold(ctx->userData, &gold))
                gold = -1;
            if (cmd->operandCount >= 1)
                SetOperand(script, state, &cmd->operands[0], gold);
            else if (ctx->onUnhandledOpcode)
                ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
            break;
        }
        case RKC_RPG_SCRIPT_OP_PAYGOLD:
            if (cmd->operandCount >= 1)
            {
                if (ctx->onPayGold)
                    ctx->onPayGold(ctx->userData, EvalOperand(script, state, ctx, &cmd->operands[0]));
            }
            else if (ctx->onUnhandledOpcode)
                ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
            break;
        case RKC_RPG_SCRIPT_OP_SETQUESTFLAG:
            RunSetQuestFlag(script, state, ctx, cmd);
            break;
        case RKC_RPG_SCRIPT_OP_CALCMYPLAYERDIST:
            RunCalcMyPlayerDist(script, state, ctx, cmd);
            break;
        case RKC_RPG_SCRIPT_OP_SETACTIVE:
            RunSetActive(script, state, ctx, cmd, 1);
            break;
        case RKC_RPG_SCRIPT_OP_SETINACTIVE:
            RunSetActive(script, state, ctx, cmd, 0);
            break;
        case RKC_RPG_SCRIPT_OP_ABSOLUTEOBJECTFLAG:
            RunAbsoluteObjectFlag(script, state, ctx, cmd);
            break;
        case RKC_RPG_SCRIPT_OP_ROUTIN:
        case RKC_RPG_SCRIPT_OP_ROUTINNET:
        {
            if (cmd->operandCount < 1)
            {
                if (ctx->onUnhandledOpcode)
                    ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
                break;
            }
            long routineId = EvalOperand(script, state, ctx, &cmd->operands[0]);
            for (long si = 0; si < script->statusCount; si++)
                if (script->statuses[si].status == RKC_RPG_SCRIPT_STATUS_ROUTIN &&
                    script->statuses[si].characterNo == routineId)
                    RunSentenceDepth(script, state, script->statuses[si].sentence, ctx, depth + 1);
            break;
        }
        case RKC_RPG_SCRIPT_OP_WARPGATE:
            if (ctx->onWarpGate)
                ctx->onWarpGate(ctx->userData);
            break;
        case RKC_RPG_SCRIPT_OP_DELETEWARPGATE:
            if (ctx->onDeleteWarpGate)
                ctx->onDeleteWarpGate(ctx->userData);
            break;
        default:
            if (ctx->onUnhandledOpcode)
                ctx->onUnhandledOpcode(ctx->userData, cmd->opcode);
            break;
        }
    }
}

void RKC_RPG_SCRIPT_EXEC_RunSentence(const RKC_RPG_SCRIPT *script, RKC_RPG_SCRIPT_EXEC_State *state,
                                      long sentenceIndex, const RKC_RPG_SCRIPT_EXEC_Context *ctx)
{
    RunSentenceDepth(script, state, sentenceIndex, ctx, 0);
}
