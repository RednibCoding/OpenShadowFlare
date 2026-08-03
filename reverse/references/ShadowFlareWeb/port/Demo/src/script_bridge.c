#include "script_bridge.h"
#include "inventory.h"
#include "movement.h"
#include "render.h"
#include "sprites.h"

#include <stdarg.h>


int LookupLiveSpawnPos(const DemoState *state, long characterNo, long *outX, long *outY)
{
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        if (state->liveSpawns[i].characterNo == characterNo)
        {
            *outX = state->liveSpawns[i].x;
            *outY = state->liveSpawns[i].y;
            return 1;
        }
    }
    return 0;
}


static void PushDialogLine(DemoState *state, long characterNo, const char *text)
{
    if (state->dialogQueueCount > 0)
    {
        const DialogQueueEntry *tail = &state->dialogQueue[state->dialogQueueCount - 1];
        if (tail->characterNo == characterNo && strcmp(tail->text, text) == 0)
            return;
    }
    if (state->dialogQueueCount >= DIALOG_QUEUE_MAX)
        return;
    
    int isNewSession = state->dialogQueueCount == 0;
    DialogQueueEntry *entry = &state->dialogQueue[state->dialogQueueCount];
    
    entry->characterNo = characterNo;
    if (isNewSession)
    {
        state->dialogActive = 1;
        
        CloseAllWindows(state);
    }
    state->dialogQueueCount++;

    if (characterNo >= 0)
        for (long i = 0; i < state->liveSpawnCount; i++)
            if (state->liveSpawns[i].characterNo == characterNo)
            {
                
                if (state->liveSpawns[i].npcFacesTalker)
                    state->liveSpawns[i].facingDirection = ResolveFacingDirection(
                        state->playerX - state->liveSpawns[i].x, state->playerY - state->liveSpawns[i].y,
                        state->liveSpawns[i].facingDirection);
                state->liveSpawns[i].isMoving = 0;
                state->liveSpawns[i].npcHasWanderTarget = 0;
                state->liveSpawns[i].npcWanderPauseUntilTick =
                    state->tick + NPC_WANDER_PAUSE_TICKS_MIN + (unsigned long)(rand() % NPC_WANDER_PAUSE_TICKS_RANGE);
                if (isNewSession)
                    state->liveSpawns[i].talkAnimTick = state->tick;
                break;
            }
}

static void RunTalkEndTriggersForCharacter(DemoState *state, long characterNo, const char *label);


static void FlushDeferredTalkEnd(DemoState *state)
{
    if (state->deferredTalkEndCharacterNo < 0)
        return;
    long characterNo = state->deferredTalkEndCharacterNo;
    char label[sizeof(state->deferredTalkEndLabel)];
    
    state->deferredTalkEndCharacterNo = -1;
    RunTalkEndTriggersForCharacter(state, characterNo, label);
}


void AdvanceDialog(DemoState *state)
{
    if (state->dialogQueueCount <= 0)
        return;
    memmove(&state->dialogQueue[0], &state->dialogQueue[1],
            sizeof(state->dialogQueue[0]) * (size_t)(state->dialogQueueCount - 1));
    state->dialogQueueCount--;
    if (state->dialogQueueCount == 0)
        state->dialogActive = 0;
    state->dialogHoveredOption = -1;
    if (state->dialogQueueCount == 0)
        FlushDeferredTalkEnd(state); 
}


void CloseDialog(DemoState *state)
{
    state->dialogQueueCount = 0;
    state->dialogActive = 0;
    state->dialogHoveredOption = -1;
    FlushDeferredTalkEnd(state); 
}


typedef struct
{
    char **messages;
    int count, capacity;
} LogDedupCache;

static void LogIfChanged(LogDedupCache *cache, const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    for (int i = 0; i < cache->count; i++)
        if (strcmp(cache->messages[i], buf) == 0)
            return;

    if (cache->count == cache->capacity)
    {
        cache->capacity = cache->capacity ? cache->capacity * 2 : 16;
        cache->messages = realloc(cache->messages, sizeof(char *) * (size_t)cache->capacity);
    }
    char *copy = malloc(strlen(buf) + 1);
    memcpy(copy, buf, strlen(buf) + 1);
    cache->messages[cache->count++] = copy;

    fputs(buf, stdout);
}

void PrintMessage(void *userData, long messageId, const char *text)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    
    PushDialogLine(ctx->state, ctx->characterNo, text);
}


void PrintUnhandledOpcode(void *userData, long opcode)
{
    (void)userData;
    static int seen[128];
    if (opcode < 0 || opcode >= (long)(sizeof(seen) / sizeof(seen[0])))
    {
        
        return;
    }
    if (seen[opcode])
        return;
    seen[opcode] = 1;
    
}

void PrintCompassText(void *userData, long messageId, const char *text)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    
    ctx->state->compassActive = 1;
    ctx->state->compassActivatedTick = ctx->state->tick;
    
    
    
}


void PrintPlaySound(void *userData, long soundId, int alwaysAudible, int hasPosition, long posX, long posY)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;

    long fromX = posX, fromY = posY;
    if (!hasPosition)
    {
        if (!LookupLiveSpawnPos(ctx->state, ctx->characterNo, &fromX, &fromY))
        {
            fromX = ctx->state->playerX;
            fromY = ctx->state->playerY;
        }
    }

    long dx = fromX - ctx->state->playerX, dy = fromY - ctx->state->playerY;
    long distSq = dx * dx + dy * dy;
    static LogDedupCache dedup;
    if (!alwaysAudible && distSq > (long)PLAYSOUND_AUDIBLE_RANGE_WORLD_UNITS * PLAYSOUND_AUDIBLE_RANGE_WORLD_UNITS)
    {
        LogIfChanged(&dedup, "[sound] %s triggers track %ld (too far away, %s%ld,%ld -- not played)\n",
                     ctx->spawnLabel, soundId, hasPosition ? "explicit pos " : "", fromX, fromY);
        return;
    }

    LogIfChanged(&dedup, "[sound] %s triggers track %ld\n", ctx->spawnLabel, soundId);
#ifdef GROUNDDEMO_AUDIO
    if (ctx->state->sfxLoaded)
        RKC_DSOUND_Play(&ctx->state->dsound, 1 , soundId, 0 , 0 ,
                        0 );
#endif
}


void PrintChangeScenario(void *userData, long scenarioId, long entryPoint)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    if (ctx->state->transitionPending)
        return;
    ctx->state->transitionPending = 1;
    ctx->state->pendingScenarioId = scenarioId;
    ctx->state->pendingEntryPoint = entryPoint;
    
}


int GetEntryPoint(void *userData, long *outValue)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    *outValue = ctx->state->currentEntryPoint;
    return 1;
}


int GetPlayerLife(void *userData, long *outHP, long *outMaxHP)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    *outHP = ctx->state->playerHP;
    *outMaxHP = ctx->state->playerMaxHP;
    return 1;
}


int GetPlayerMental(void *userData, long *outMP, long *outMaxMP)
{
    (void)userData;
    *outMP = 100; 
    *outMaxMP = 100;
    return 1;
}


int GetPlayerLevel(void *userData, long *outLevel)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    *outLevel = ctx->state->playerLevel;
    return 1;
}


static int FindInventorySlotByItem(const DemoState *state, long kind, long templateId)
{
    for (int i = 0; i < state->inventoryCount; i++)
    {
        const InventorySlot *slot = &state->inventory[i];
        if (slot->kind == kind && slot->templateId == templateId && slot->count > 0)
            return i;
    }
    return -1;
}


int CheckItemExistInInventory(void *userData, long kind, long templateId, long *outExists)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    const DemoState *state = ctx->state;
    *outExists = 0;
    if (FindInventorySlotByItem(state, kind, templateId) >= 0)
        *outExists = 1;
    else if (kind == 0 && state->hasWeapon && state->weapon.tail.templateId == templateId)
        *outExists = 1;
    else if (kind == 1)
    {
        for (int s = 0; s < EQUIPMENT_ARMOR_SLOTS; s++)
            if (state->hasArmor[s] && state->armor[s].tail.templateId == templateId)
            {
                *outExists = 1;
                break;
            }
    }
    else if (kind == 2)
    {
        for (int s = 0; s < EQUIPMENT_ACCESSORY_SLOTS; s++)
            if (state->hasAccessory[s] && state->accessory[s].tail.templateId == templateId)
            {
                *outExists = 1;
                break;
            }
    }
    return 1;
}


void DeleteItemFromInventory(void *userData, long kind, long templateId)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    DemoState *state = ctx->state;

    int bagIndex = FindInventorySlotByItem(state, kind, templateId);
    if (bagIndex >= 0)
    {
        
        RemoveOneFromInventorySlot(state, bagIndex);
        return;
    }
    if (kind == 0 && state->hasWeapon && state->weapon.tail.templateId == templateId)
    {
        
        state->hasWeapon = 0;
        state->weaponName[0] = 0;
        return;
    }
    if (kind == 1)
    {
        for (int s = 0; s < EQUIPMENT_ARMOR_SLOTS; s++)
            if (state->hasArmor[s] && state->armor[s].tail.templateId == templateId)
            {
                
                state->hasArmor[s] = 0;
                state->armorName[s][0] = 0;
                return;
            }
    }
    if (kind == 2)
    {
        for (int s = 0; s < EQUIPMENT_ACCESSORY_SLOTS; s++)
            if (state->hasAccessory[s] && state->accessory[s].tail.templateId == templateId)
            {
                
                state->hasAccessory[s] = 0;
                state->accessoryName[s][0] = 0;
                return;
            }
    }
    
}


static void OpenGateWindow(void *userData)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    DemoState *state = ctx->state;
    if (state->gateWindowOpen)
        return;
    CloseAllWindows(state);
    state->gateWindowOpen = 1;
    state->gateWindowPage = 0; 
    state->gateHoveredRow = -1;
    state->gateTeleportPendingRow = -1; 
    state->gateOpenClickLatch = 1;      
    
}


static void CloseGateWindow(void *userData)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    ctx->state->gateWindowOpen = 0;
}


void DrawQuestNameBanner(void *userData, long questIndex)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    DemoState *state = ctx->state;
    state->questBannerIndex = questIndex;
    state->questBannerUntilTick = state->tick + QUEST_BANNER_TICKS;
    
}


int GetPlayerGold(void *userData, long *outGold)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    *outGold = ctx->state->gold;
    return 1;
}


void PrintPayGold(void *userData, long amount)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    DeductGoldFromInventory(ctx->state, amount);
    
}


int CheckProximity(void *userData, long characterNo, long *outResult)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    const DemoState *state = ctx->state;
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        const LiveSpawn *spawn = &state->liveSpawns[i];
        if (spawn->characterNo != characterNo)
            continue;
        if (!state->mouseLeftDown || state->hoveredSpawnIndex != i)
        {
            *outResult = 0;
            return 1;
        }
        long dist = RectGapDistance(spawn->x, spawn->y, spawn->rectL, spawn->rectT, spawn->rectR, spawn->rectB,
                                     state->playerX, state->playerY, -PLAYER_FOOTPRINT_HALF_WIDTH,
                                     -PLAYER_FOOTPRINT_HALF_WIDTH, PLAYER_FOOTPRINT_HALF_WIDTH,
                                     PLAYER_FOOTPRINT_HALF_WIDTH);
        *outResult = dist <= CCHECK_RANGE_WORLD_UNITS ? 1 : 0;
        return 1;
    }
    return 0;
}


int CalcPlayerDist(void *userData, long characterNo, long *outDist)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    const DemoState *state = ctx->state;
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        const LiveSpawn *spawn = &state->liveSpawns[i];
        if (spawn->characterNo != characterNo)
            continue;
        *outDist = RectGapDistance(spawn->x, spawn->y, spawn->rectL, spawn->rectT, spawn->rectR, spawn->rectB,
                                    state->playerX, state->playerY, -PLAYER_FOOTPRINT_HALF_WIDTH,
                                    -PLAYER_FOOTPRINT_HALF_WIDTH, PLAYER_FOOTPRINT_HALF_WIDTH,
                                    PLAYER_FOOTPRINT_HALF_WIDTH);
        return 1;
    }
    return 0;
}


void SetGateLabel(void *userData, long characterNo, const char *text)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    DemoState *state = ctx->state;
    
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        LiveSpawn *spawn = &state->liveSpawns[i];
        if (spawn->block != 1 || spawn->field8 != GATE_RUNE_CIRCLE_FIELD8)
            continue;
        int matched = 0;
        for (long delta = -4; delta <= 1 && !matched; delta++)
            if (spawn->characterNo + delta == characterNo)
                matched = 1;
        if (!matched)
            continue;
        
        long len = (long)strlen(spawn->gateDestinationName);
        while (len > 0 && (spawn->gateDestinationName[len - 1] == '\n' || spawn->gateDestinationName[len - 1] == '\r' ||
                           spawn->gateDestinationName[len - 1] == ' '))
            spawn->gateDestinationName[--len] = '\0';
        return;
    }
}


void HealPlayerLife(void *userData)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    DemoState *state = ctx->state;
    
    state->playerHP = state->playerMaxHP;
}


void HealPlayerMental(void *userData)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    
}


void RequestSpawnAction(void *userData, long characterNo, long actionNo, long arg1, long arg2, long arg3, long arg4)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    DemoState *state = ctx->state;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    long chart = actionNo - 1;
    if (chart < 0)
        return;
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        LiveSpawn *spawn = &state->liveSpawns[i];
        if (spawn->characterNo != characterNo)
            continue;
        if (spawn->templateIndex < 0)
            return;
        const LiveSpawnTemplate *tmpl = &state->templates[spawn->templateIndex];
        if (tmpl->kind != LIVE_SPAWN_SPRITE_CAF || chart >= tmpl->caf.chartCount ||
            tmpl->caf.charts[chart].directions[spawn->facingDirection].maxFrameCount <= 0)
        {
            
            return;
        }
        spawn->actionAnimChart = chart;
        spawn->actionAnimTick = state->tick;
        
        return;
    }
    
}


void MessageBoxValue(void *userData, long characterNo, long offsetX, long offsetY, long value, long r, long g, long b)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    DemoState *state = ctx->state;
    if (state->floatingValueCount >= FLOATING_VALUE_MAX)
        return;
    state->floatingValues[state->floatingValueCount].characterNo = characterNo;
    state->floatingValues[state->floatingValueCount].offsetX = offsetX;
    state->floatingValues[state->floatingValueCount].offsetY = offsetY;
    state->floatingValues[state->floatingValueCount].value = value;
    state->floatingValues[state->floatingValueCount].r = r;
    state->floatingValues[state->floatingValueCount].g = g;
    state->floatingValues[state->floatingValueCount].b = b;
    state->floatingValueCount++;
}


void SetUnlockSw(void *userData, long value)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    ctx->state->unlockSwValue = value;
    ctx->state->unlockSwSetTick = ctx->state->tick;
}


static int FindOrLoadScriptEffectTemplate(DemoState *state, long effectId)
{
    if (effectId < 20000 || effectId > 29999 || state->characterRoot[0] == '\0')
        return -1;
    for (int i = 0; i < state->scriptEffectTemplateCount; i++)
        if (state->scriptEffectTemplateIds[i] == effectId)
            return state->scriptEffectTemplates[i].kind == LIVE_SPAWN_SPRITE_CAF ? i : -1;
    if (state->scriptEffectTemplateCount >= SCRIPT_EFFECT_TEMPLATE_CACHE_MAX)
        return -1;

    int idx = state->scriptEffectTemplateCount++;
    LiveSpawnTemplate *t = &state->scriptEffectTemplates[idx];
    memset(t, 0, sizeof(*t));
    RKC_UPDIB_Init(&t->staticNjp);
    RKC_RPGSCRN_CAF_Init(&t->caf);
    RKC_UPDIB_Init(&t->animNjp);
    RKC_UPDIB_Init(&t->animSdw);
    t->kind = LIVE_SPAWN_SPRITE_NONE;
    state->scriptEffectTemplateIds[idx] = effectId;

    char dir[sizeof(state->characterRoot) + 32];
    
    LoadCafTemplate(t, dir, "Animation");
    if (t->kind != LIVE_SPAWN_SPRITE_CAF)
    {
        
        return -1;
    }
    return idx;
}


static void CreateScriptEffect(DemoState *state, long effectId, long x, long y, long followCharacterNo, long direction)
{
    int tmplIdx = FindOrLoadScriptEffectTemplate(state, effectId);
    if (tmplIdx < 0)
        return;
    for (int i = 0; i < SCRIPT_EFFECT_MAX; i++)
    {
        ScriptEffect *fx = &state->scriptEffects[i];
        if (fx->active)
            continue;
        fx->active = 1;
        fx->effectId = effectId;
        fx->templateIndex = tmplIdx;
        fx->x = x;
        fx->y = y;
        fx->followCharacterNo = followCharacterNo;
        fx->direction = direction >= 0 && direction < RKC_RPGSCRN_CAF_NUM_DIRECTIONS
                            ? direction
                            : RKC_RPGSCRN_CAF_NUM_DIRECTIONS - 1;
        fx->spawnTick = state->tick;
        return;
    }
}


void CreateEffect(void *userData, long effectId, long x, long y, long direction)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    if (direction >= 0 && direction <= 7)
        direction = (direction + 4) & 7;
    CreateScriptEffect(ctx->state, effectId, x, y, -1, direction);
}


void CreateEffectChar(void *userData, long effectId, long characterNo)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    long x = 0, y = 0;
    if (!LookupLiveSpawnPos(ctx->state, characterNo, &x, &y))
    {
        
        return;
    }
    CreateScriptEffect(ctx->state, effectId, x, y, characterNo, RKC_RPGSCRN_CAF_NUM_DIRECTIONS - 1);
}


void ResurrectSpawn(void *userData, long characterNo, long x, long y, long direction)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    DemoState *state = ctx->state;
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        LiveSpawn *spawn = &state->liveSpawns[i];
        if (spawn->characterNo != characterNo)
            continue;
        if (spawn->block != 3 || !spawn->aiState.isDead)
        {
            
            return;
        }
        spawn->x = x;
        spawn->y = y;
        RKC_RPG_AI_EXEC_State_Init(&spawn->aiState, spawn->aiState.list, x, y, spawn->aiState.maxHP);
        spawn->attackCooldownTicks = 0;
        spawn->attackAnimTick = 0;
        spawn->deathTick = 0;
        spawn->hitStunTick = 0;
        spawn->hitStunDurationTicks = 0;
        spawn->hitVfxTick = 0;
        spawn->isMoving = 0;
        spawn->slideDir = SLIDE_NONE;
        if (direction >= 0 && direction <= 7)
            spawn->facingDirection = (int)direction;
        
        return;
    }
    
}


RKC_RPG_SCRIPT_EXEC_Context BuildExecContext(MessagePrintContext *ctx)
{
    RKC_RPG_SCRIPT_EXEC_Context execCtx;
    memset(&execCtx, 0, sizeof(execCtx));
    execCtx.onMessage = PrintMessage;
    execCtx.onUnhandledOpcode = PrintUnhandledOpcode;
    execCtx.onCompassText = PrintCompassText;
    execCtx.getCharacterPos = GetCharacterPos;
    execCtx.getCharacterAlive = GetCharacterAlive;
    execCtx.onPlaySound = PrintPlaySound;
    execCtx.onChangeScenario = PrintChangeScenario;
    execCtx.getEntryPoint = GetEntryPoint;
    execCtx.onCreateItem = PrintCreateItem;
    execCtx.onCreateItemTable = PrintCreateItemTable;
    execCtx.userData = ctx;
    execCtx.getPlayerGold = GetPlayerGold;
    execCtx.onPayGold = PrintPayGold;
    execCtx.getPlayerLife = GetPlayerLife;
    execCtx.checkProximity = CheckProximity;
    execCtx.calcPlayerDist = CalcPlayerDist;
    execCtx.onGateLabel = SetGateLabel;
    execCtx.onHealLife = HealPlayerLife;
    execCtx.onHealMental = HealPlayerMental;
    execCtx.onActionRequest = RequestSpawnAction;
    execCtx.onMessageBoxValue = MessageBoxValue;
    execCtx.onSetUnlockSw = SetUnlockSw;
    execCtx.onCreateEffect = CreateEffect;
    execCtx.onCreateEffectChar = CreateEffectChar;
    execCtx.onResurrect = ResurrectSpawn;
    execCtx.onDrawQuestName = DrawQuestNameBanner;
    execCtx.checkItemExist = CheckItemExistInInventory;
    execCtx.onDeleteItem = DeleteItemFromInventory;
    execCtx.getPlayerLevel = GetPlayerLevel;
    execCtx.getPlayerMental = GetPlayerMental;
    execCtx.onWarpGate = OpenGateWindow;
    execCtx.onDeleteWarpGate = CloseGateWindow;
    return execCtx;
}


int GetCharacterPos(void *userData, long characterNo, long *outX, long *outY)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    return LookupLiveSpawnPos(ctx->state, characterNo, outX, outY);
}


int GetCharacterAlive(void *userData, long characterNo, long *outAlive)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    for (long i = 0; i < ctx->state->liveSpawnCount; i++)
    {
        const LiveSpawn *spawn = &ctx->state->liveSpawns[i];
        if (spawn->characterNo == characterNo)
        {
            *outAlive = (spawn->block == 3 && spawn->aiState.isDead) ? 0 : 1;
            return 1;
        }
    }
    return 0;
}


void PrintCreateItem(void *userData, long kind, long templateId, long posX, long posY, long amount)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    DemoState *state = ctx->state;

    WorldItem *grown = realloc(state->worldItems, sizeof(WorldItem) * (size_t)(state->worldItemCount + 1));
    if (!grown)
        return;
    state->worldItems = grown;

    WorldItem *item = &state->worldItems[state->worldItemCount++];
    item->x = posX;
    item->y = posY;
    item->pickedUp = 0;
    item->kind = kind;
    item->templateId = templateId;
    item->hitTestHalfWidth = 0;
    item->hitTestHalfHeight = 0;
    item->dropTick = state->tick; 
    item->jumpEffectTick = -1;

    if (kind == 4 && templateId == 0)
    {
        if (amount <= 0)
        {
            
            state->worldItemCount--;
            
            return;
        }
        item->isGold = 1;
        item->amount = amount;
        item->resolved = 1;
        
        
        return;
    }

    item->isGold = 0;
    item->amount = 1;
    const RKC_RPG_ITEMDATA_Record *rec =
        state->itemDataLoaded ? RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, (int)kind, (int)templateId)
                              : NULL;
    item->resolved = rec != NULL;
    
    
}


static long GetTailLong(const RKC_RPG_ITEMDATA_Record *rec, long offset)
{
    if (offset < 0 || offset + 4 > rec->tailSize)
        return 0;
    long value;
    memcpy(&value, rec->tail + offset, 4);
    return value;
}


static long TierFieldOffset(long kind)
{
    (void)kind;
    return 0x08;
}

static long BitmaskFieldOffset(long kind)
{
    (void)kind;
    return 0x0c;
}

static long WeightFieldOffset(long kind)
{
    (void)kind;
    return 0x10;
}


static long LevelFieldOffset(long kind)
{
    if (kind == 0 || kind == 1)
        return 0x90;
    if (kind == 2 || kind == 3)
        return 0x64;
    return -1;
}


static int RecordMatches(const RKC_RPG_ITEMDATA_Record *rec, long kind, long col1, long col2, int tier0Allowed,
                         int tier1Allowed, int tier2Allowed, long bitmask, long levelOffset)
{
    long tier = GetTailLong(rec, TierFieldOffset(kind));
    if (!((tier == 0 && tier0Allowed) || (tier == 1 && tier1Allowed) || (tier == 2 && tier2Allowed) || tier == 3))
        return 0;
    long recBitmask = GetTailLong(rec, BitmaskFieldOffset(kind));
    if ((bitmask & recBitmask) == 0)
        return 0;
    if (levelOffset >= 0 && levelOffset + 4 <= rec->tailSize)
    {
        long recLevel = GetTailLong(rec, levelOffset);
        if (col1 < recLevel && col1 != -1)
            return 0;
        if (recLevel < col2 && col2 != -1)
            return 0;
    }
    return 1;
}


static long SumMatchingWeight(const RKC_RPG_ITEMDATA *itemData, long kind, long col1, long col2, int tier0Allowed,
                              int tier1Allowed, int tier2Allowed, long bitmask)
{
    if (kind < 0 || kind >= 5)
        return 0;
    long levelOffset = LevelFieldOffset(kind);
    long total = 0;
    const RKC_RPG_ITEMDATA_Kind *k = &itemData->kinds[kind];
    for (long i = 0; i < k->count; i++)
        if (RecordMatches(&k->records[i], kind, col1, col2, tier0Allowed, tier1Allowed, tier2Allowed, bitmask,
                          levelOffset))
            total += GetTailLong(&k->records[i], WeightFieldOffset(kind));
    return total;
}


static long PickWeighted(const RKC_RPG_ITEMDATA *itemData, long kind, long col1, long col2, int tier0Allowed,
                         int tier1Allowed, int tier2Allowed, long roll, long bitmask)
{
    if (kind < 0 || kind >= 5)
        return -1;
    long levelOffset = LevelFieldOffset(kind);
    long cumulative = 0;
    const RKC_RPG_ITEMDATA_Kind *k = &itemData->kinds[kind];
    for (long i = 0; i < k->count; i++)
    {
        const RKC_RPG_ITEMDATA_Record *rec = &k->records[i];
        if (!RecordMatches(rec, kind, col1, col2, tier0Allowed, tier1Allowed, tier2Allowed, bitmask, levelOffset))
            continue;
        cumulative += GetTailLong(rec, WeightFieldOffset(kind));
        if (roll < cumulative)
            return rec->templateId;
    }
    return -1;
}


static void ScatterAroundCenter(long centerX, long centerY, long index, long count, long *outX, long *outY)
{
    if (count <= 1)
    {
        *outX = centerX;
        *outY = centerY;
        return;
    }
    double angle = 2.0 * 3.14159265358979323846 * (double)index / (double)count;
    *outX = centerX + (long)(cos(angle) * 200.0);
    *outY = centerY + (long)(sin(angle) * 200.0);
}


void PrintCreateItemTable(void *userData, long tableRowIndex, long posX, long posY)
{
    const MessagePrintContext *ctx = (const MessagePrintContext *)userData;
    DemoState *state = ctx->state;
    if (tableRowIndex < 0 || !state->tableLoaded)
        return;

    const RKC_RPG_TABLEDATA *table1e = RKC_RPG_TABLE_GetFromTableNo(&state->table, 0x1e);
    if (!table1e)
        return;

    long repeatCount = RKC_RPG_TABLEDATA_GetValue(table1e, tableRowIndex, 0);
    if (repeatCount == 0)
        repeatCount = 1; 
    if (repeatCount <= 0)
        return;

    long chancePercent = RKC_RPG_TABLEDATA_GetValue(table1e, tableRowIndex, 1);

    for (long i = 0; i < repeatCount; i++)
    {
        if (rand() % 100 > chancePercent)
            continue;

        const RKC_RPG_TABLEDATA *table1f = RKC_RPG_TABLE_GetFromTableNo(&state->table, 0x1f);
        if (!table1f)
            continue;

        long slot = rand() % 10;
        long famIdx = RKC_RPG_TABLEDATA_GetValue(table1e, tableRowIndex, 2 + slot * 2);
        if (famIdx == -1)
            continue;

        long kind = RKC_RPG_TABLEDATA_GetValue(table1f, famIdx, 0);
        long col1 = RKC_RPG_TABLEDATA_GetValue(table1f, famIdx, 1);
        long col2 = RKC_RPG_TABLEDATA_GetValue(table1f, famIdx, 2);
        long templateId = RKC_RPG_TABLEDATA_GetValue(table1f, famIdx, 3);
        long bitmask = RKC_RPG_TABLEDATA_GetValue(table1f, famIdx, 4);

        long digitField = RKC_RPG_TABLEDATA_GetValue(table1e, tableRowIndex, 3 + slot * 2);
        int tier0Allowed = (digitField % 10) != 0;
        int tier1Allowed = ((digitField % 100) / 10) != 0;
        int tier2Allowed = ((digitField % 1000) / 100) != 0;

        long resolvedTemplateId = templateId;
        if (templateId == -1)
        {
            if (!state->itemDataLoaded)
                continue;
            long totalWeight =
                SumMatchingWeight(&state->itemData, kind, col1, col2, tier0Allowed, tier1Allowed, tier2Allowed, bitmask);
            if (totalWeight <= 0)
                continue;
            long roll = rand() % totalWeight;
            resolvedTemplateId =
                PickWeighted(&state->itemData, kind, col1, col2, tier0Allowed, tier1Allowed, tier2Allowed, roll, bitmask);
            if (resolvedTemplateId == -1)
                continue;
        }

        long scatterX, scatterY;
        ScatterAroundCenter(posX, posY, i, repeatCount, &scatterX, &scatterY);
        PrintCreateItem(userData, kind, resolvedTemplateId, scatterX, scatterY, 1);
    }
}

static const char *StatusKindName(long status)
{
    switch (status)
    {
    case RKC_RPG_SCRIPT_STATUS_CHECK:
        return "CHECK";
    case RKC_RPG_SCRIPT_STATUS_TALKEND:
        return "TALKEND";
    case RKC_RPG_SCRIPT_STATUS_ONJUDGE:
        return "ONJUDGE";
    case RKC_RPG_SCRIPT_STATUS_ENEMYDEAD:
        return "ENEMYDEAD";
    default:
        return "other";
    }
}


int RunTriggersForCharacter(DemoState *state, long characterNo, const char *label)
{
    MessagePrintContext ctx = {label, state, characterNo};
    RKC_RPG_SCRIPT_EXEC_Context execCtx = BuildExecContext(&ctx);

    int ranAny = 0;
    int hasTalkEnd = 0;
    for (long i = 0; i < state->script.statusCount; i++)
    {
        const RKC_RPG_SCRIPT_Status *st = &state->script.statuses[i];
        if (st->characterNo != characterNo)
            continue;
        
        if (st->status == RKC_RPG_SCRIPT_STATUS_ROUTIN || st->status == RKC_RPG_SCRIPT_STATUS_RECVNETFLAG)
            continue;
        ranAny = 1;
        if (st->status == RKC_RPG_SCRIPT_STATUS_TALKEND)
        {
            hasTalkEnd = 1; 
            continue;
        }
        
        RKC_RPG_SCRIPT_EXEC_RunSentence(&state->script, &state->execState, st->sentence, &execCtx);
    }

    if (hasTalkEnd)
    {
        state->talkEndChainDepth = 0; 
        if (state->dialogQueueCount > 0)
        {
            if (state->deferredTalkEndCharacterNo >= 0 && state->deferredTalkEndCharacterNo != characterNo)
                FlushDeferredTalkEnd(state);
            state->deferredTalkEndCharacterNo = characterNo;
            
            
        }
        else
            RunTalkEndTriggersForCharacter(state, characterNo, label);
    }
    return ranAny;
}


static void RunTalkEndTriggersForCharacter(DemoState *state, long characterNo, const char *label)
{
    MessagePrintContext ctx = {label, state, characterNo};
    RKC_RPG_SCRIPT_EXEC_Context execCtx = BuildExecContext(&ctx);

    for (long i = 0; i < state->script.statusCount; i++)
    {
        const RKC_RPG_SCRIPT_Status *st = &state->script.statuses[i];
        if (st->characterNo != characterNo || st->status != RKC_RPG_SCRIPT_STATUS_TALKEND)
            continue;
        
        RKC_RPG_SCRIPT_EXEC_RunSentence(&state->script, &state->execState, st->sentence, &execCtx);
    }

    if (state->dialogQueueCount > 0)
    {
        if (state->talkEndChainDepth >= TALKEND_CHAIN_MAX)
        {
            
            return;
        }
        state->talkEndChainDepth++;
        state->deferredTalkEndCharacterNo = characterNo;
        
        
    }
}


void TickOnJudgeTriggers(DemoState *state)
{
    for (long i = 0; i < state->script.statusCount; i++)
    {
        const RKC_RPG_SCRIPT_Status *st = &state->script.statuses[i];
        if (st->status != RKC_RPG_SCRIPT_STATUS_ONJUDGE)
            continue;

        const LiveSpawn *spawn = NULL;
        for (long s = 0; s < state->liveSpawnCount; s++)
        {
            if (state->liveSpawns[s].characterNo == st->characterNo)
            {
                spawn = &state->liveSpawns[s];
                break;
            }
        }
        if (!spawn)
            continue;

        if (!RectsOverlap(spawn->x, spawn->y, spawn->rectL, spawn->rectT, spawn->rectR, spawn->rectB, state->playerX,
                           state->playerY, -PLAYER_FOOTPRINT_HALF_WIDTH, -PLAYER_FOOTPRINT_HALF_WIDTH,
                           PLAYER_FOOTPRINT_HALF_WIDTH, PLAYER_FOOTPRINT_HALF_WIDTH))
            continue;

        const char *label = spawn->name ? spawn->name : (spawn->block == 1 ? "(unnamed object)" : "(unnamed enemy)");
        MessagePrintContext ctx = {label, state, st->characterNo};
        RKC_RPG_SCRIPT_EXEC_Context execCtx = BuildExecContext(&ctx);
        RKC_RPG_SCRIPT_EXEC_RunSentence(&state->script, &state->execState, st->sentence, &execCtx);
    }
}


void TickExecFunctionTriggers(DemoState *state)
{
    for (long i = 0; i < state->script.statusCount; i++)
    {
        const RKC_RPG_SCRIPT_Status *st = &state->script.statuses[i];
        if (st->status != RKC_RPG_SCRIPT_STATUS_EXECFUNCTION)
            continue;

        MessagePrintContext ctx = {"(scenario)", state, -1};
        RKC_RPG_SCRIPT_EXEC_Context execCtx = BuildExecContext(&ctx);
        RKC_RPG_SCRIPT_EXEC_RunSentence(&state->script, &state->execState, st->sentence, &execCtx);
    }
}


void TickGateHighlights(DemoState *state)
{
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        LiveSpawn *spawn = &state->liveSpawns[i];
        if (spawn->block != 1 || spawn->field8 != GATE_RUNE_CIRCLE_FIELD8 || spawn->templateIndex < 0)
            continue;

        const LiveSpawn *triggerZone = NULL;
        long bestDistSq = GATE_TRIGGER_ZONE_MAX_DISTANCE * GATE_TRIGGER_ZONE_MAX_DISTANCE;
        for (long z = 0; z < state->liveSpawnCount; z++)
        {
            const LiveSpawn *candidate = &state->liveSpawns[z];
            if (candidate->block != 1 || candidate->field8 != -1)
                continue;
            long dx = candidate->x - spawn->x, dy = candidate->y - spawn->y;
            long distSq = dx * dx + dy * dy;
            if (distSq < bestDistSq)
            {
                bestDistSq = distSq;
                triggerZone = candidate;
            }
        }

        int inside = 0;
        if (triggerZone)
            inside = RectsOverlap(triggerZone->x, triggerZone->y, triggerZone->rectL, triggerZone->rectT,
                                  triggerZone->rectR, triggerZone->rectB, state->playerX, state->playerY,
                                  -PLAYER_FOOTPRINT_HALF_WIDTH, -PLAYER_FOOTPRINT_HALF_WIDTH,
                                  PLAYER_FOOTPRINT_HALF_WIDTH, PLAYER_FOOTPRINT_HALF_WIDTH);

        if (inside && spawn->gateHighlightFadeTicks < GATE_HIGHLIGHT_FADE_TICKS)
            spawn->gateHighlightFadeTicks++;
        else if (!inside && spawn->gateHighlightFadeTicks > 0)
            spawn->gateHighlightFadeTicks--;
    }
}


void ApplyPlayerInteract(DemoState *state, const LiveSpawn *spawn)
{
    const char *label = spawn->name ? spawn->name : (spawn->block == 1 ? "(unnamed object)" : "(unnamed enemy)");
    if (!RunTriggersForCharacter(state, spawn->characterNo, label))
        printf("%s has no interaction trigger\n", label);
}


void HandleInteract(DemoState *state)
{
    long spawnIdx = FindNearestLiveSpawn(state);
    if (spawnIdx < 0)
    {
        
        return;
    }
    ApplyPlayerInteract(state, &state->liveSpawns[spawnIdx]);
}
