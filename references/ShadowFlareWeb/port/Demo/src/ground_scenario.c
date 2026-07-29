#include "scenario.h"
#include "paths.h"
#include "sprites.h"
#include "inventory.h"
#include "combat.h"
#include "script_bridge.h"
#include "render.h"


static int ReadPatternListIntoSet(const char *lstPath, const char *mapRoot, RKC_UPDIB_SET *set)
{
    FILE *f = fopen(lstPath, "rb");
    if (!f)
        return -1;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0)
    {
        fclose(f);
        return -1;
    }
    char *buf = malloc((size_t)size + 1);
    if (!buf || fread(buf, 1, (size_t)size, f) != (size_t)size)
    {
        fclose(f);
        free(buf);
        return -1;
    }
    fclose(f);
    buf[size] = '\0';

    
    int lineCount = 0;
    {
        int inLine = 0;
        for (long i = 0; i < size; i++)
        {
            if (buf[i] == '\r' || buf[i] == '\n')
                inLine = 0;
            else if (!inLine)
            {
                inLine = 1;
                lineCount++;
            }
        }
    }
    if (lineCount <= 0)
    {
        free(buf);
        return -1;
    }
    if (!RKC_UPDIB_SET_Create(set, lineCount))
    {
        free(buf);
        return -1;
    }

    
    int slot = 0;
    long lineStart = 0;
    for (long i = 0; i <= size; i++)
    {
        if (i == size || buf[i] == '\r' || buf[i] == '\n')
        {
            long lineLen = i - lineStart;
            if (lineLen > 0)
            {
                char line[512];
                if ((size_t)lineLen >= sizeof(line))
                    lineLen = (long)sizeof(line) - 1;
                memcpy(line, buf + lineStart, (size_t)lineLen);
                line[lineLen] = '\0';

                
                if (strcmp(line, "?") != 0)
                {
                    char path[1024];
                    
                    if (!RKC_UPDIB_SET_ReadSlot(set, slot, path))
                    {
                        free(buf);
                        return -1;
                    }
                }
                slot++;
            }
            lineStart = i + 1;
        }
    }

    free(buf);
    return slot;
}


static void FindOpenStartCell(DemoState *state, long centerCellX, long centerCellY, long *outWorldX, long *outWorldY)
{
    long maxRadius =
        state->ground.areaWidth > state->ground.areaHeight ? state->ground.areaWidth : state->ground.areaHeight;
    for (long radius = 0; radius <= maxRadius; radius++)
        for (long dy = -radius; dy <= radius; dy++)
            for (long dx = -radius; dx <= radius; dx++)
            {
                
                if (radius > 0 && dx > -radius && dx < radius && dy > -radius && dy < radius)
                    continue;
                long cellX = centerCellX + dx, cellY = centerCellY + dy;
                if (cellX < 0 || cellY < 0 || cellX >= state->ground.areaWidth || cellY >= state->ground.areaHeight)
                    continue;
                long screenX, screenY, worldX, worldY;
                RKC_RPGSCRN_GROUND_CellToScreen(&state->ground, cellX, cellY, &screenX, &screenY);
                ScreenToWorld(state, screenX, screenY, &worldX, &worldY);
                if (!RKC_RPGSCRN_GROUND_IsBlocked(&state->ground, worldX, worldY))
                {
                    *outWorldX = worldX;
                    *outWorldY = worldY;
                    return;
                }
            }

    long screenX, screenY;
    RKC_RPGSCRN_GROUND_CellToScreen(&state->ground, centerCellX, centerCellY, &screenX, &screenY);
    ScreenToWorld(state, screenX, screenY, outWorldX, outWorldY);
}


typedef struct
{
    long index;
    long depth;
} ObjectDepthKey;

static int CompareObjectDepth(const void *a, const void *b)
{
    long da = ((const ObjectDepthKey *)a)->depth;
    long db = ((const ObjectDepthKey *)b)->depth;
    if (da < db)
        return -1;
    if (da > db)
        return 1;
    return 0;
}


static void BuildObjectDrawOrder(DemoState *state)
{
    free(state->objectDrawOrder);
    state->objectDrawOrder = NULL;
    state->objectDrawOrderCount = 0;

    long count = RKC_RPGSCRN_OBJECTBLOCK_GetCount(&state->objects);
    if (count <= 0)
        return;

    ObjectDepthKey *keys = malloc(sizeof(ObjectDepthKey) * (size_t)count);
    if (!keys)
        return;

    for (long i = 0; i < count; i++)
    {
        const RKC_RPGSCRN_OBJECT_Entry *entry = RKC_RPGSCRN_OBJECTBLOCK_Get(&state->objects, i);
        keys[i].index = i;
        
        if (entry->field5 & OBL_FIELD5_GROUND_DECAL_BIT)
        {
            keys[i].depth = GROUND_DECAL_DRAW_DEPTH;
            continue;
        }
        RKC_DIB *icon = RKC_RPGSCRN_OBJECTBLOCK_GetIcon(&state->objects, &state->patternSet, i);
        long screenY = (entry->posX + entry->posY) * RKC_RPGSCRN_SCENE_SCALE_Y / 100;
        long offsetX, offsetY;
        RKC_RPGSCRN_OBJECTBLOCK_GetOffset(&state->objects, &state->patternSet, i, &offsetX, &offsetY);
        (void)offsetX; 
        keys[i].depth = screenY + offsetY + (icon ? icon->height : 0);
    }
    qsort(keys, (size_t)count, sizeof(ObjectDepthKey), CompareObjectDepth);

    state->objectDrawOrder = malloc(sizeof(long) * (size_t)count);
    if (!state->objectDrawOrder)
    {
        free(keys);
        return;
    }
    for (long i = 0; i < count; i++)
        state->objectDrawOrder[i] = keys[i].index;
    state->objectDrawOrderCount = count;
    free(keys);
}


int LoadArea(DemoState *state, const char *stem)
{
    char path[1024];
    
    if (!RKC_RPGSCRN_GROUND_Read(&state->ground, path, 0))
    {
        
        return 0;
    }

    
    int slotsLoaded = ReadPatternListIntoSet(path, state->mapRoot, &state->patternSet);
    if (slotsLoaded <= 0)
    {
        
        return 0;
    }

    

    char oblPath[1024];
    
    if (!RKC_RPGSCRN_OBJECTBLOCK_Read(&state->objects, oblPath, 0))
    {
        
        BuildObjectDrawOrder(state); 
    }
    else
    {
        
        BuildObjectDrawOrder(state);
        
    }

    
    FindOpenStartCell(state, state->ground.areaWidth / 2, state->ground.areaHeight / 2, &state->playerX,
                      &state->playerY);

    
    state->hasMoveTarget = 0;
    state->pendingActionKind = PENDING_ACTION_NONE;
    
    state->heldClickIsSpawnTarget = 0;
    
    state->dragActive = 0;
    state->slideDir = SLIDE_NONE;
    state->visitedCount = 0; 
    state->pathfindCheckValid = 0;
    state->pathfindActive = 0; 

    
    UpdateCameraFromPlayer(state);

    return 1;
}


static void RunEntryScenarioTrigger(DemoState *state)
{
    if (!state->scriptLoaded)
        return;
    MessagePrintContext ctx = {"(scenario entry)", state, -1}; 
    
    RKC_RPG_SCRIPT_EXEC_Context execCtx = BuildExecContext(&ctx);
    for (long i = 0; i < state->script.statusCount; i++)
    {
        const RKC_RPG_SCRIPT_Status *st = &state->script.statuses[i];
        if (st->status == RKC_RPG_SCRIPT_STATUS_ENTRYSCENARIO)
            RKC_RPG_SCRIPT_EXEC_RunSentence(&state->script, &state->execState, st->sentence, &execCtx);
    }
}


static void ResolveGateDestinationNames(DemoState *state)
{
    if (!state->scriptLoaded)
        return;

    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        LiveSpawn *spawn = &state->liveSpawns[i];
        spawn->gateDestinationName[0] = '\0';
        if (spawn->block != 1 || spawn->field8 != GATE_RUNE_CIRCLE_FIELD8)
            continue;

        long scenarioId = -1;
        for (long delta = -4; delta <= 1 && scenarioId < 0; delta++)
        {
            long candidateCharacterNo = spawn->characterNo + delta;
            for (long s = 0; s < state->script.statusCount && scenarioId < 0; s++)
            {
                const RKC_RPG_SCRIPT_Status *st = &state->script.statuses[s];
                if (st->characterNo != candidateCharacterNo || st->sentence < 0 ||
                    st->sentence >= state->script.sentenceCount)
                    continue;
                const RKC_RPG_SCRIPT_Sentence *sentence = &state->script.sentences[st->sentence];
                for (long c = 0; c < sentence->commandCount; c++)
                {
                    const RKC_RPG_SCRIPT_Command *cmd = &sentence->commands[c];
                    if (cmd->opcode == 0x11  && cmd->operandCount >= 1 &&
                        cmd->operands[0].type <= 2 )
                    {
                        scenarioId = cmd->operands[0].value;
                        break;
                    }
                }
            }
        }
        if (scenarioId < 0)
            continue;

        char scenarioPath[1024];
        if (!DeriveScenarioPath(state->mapRoot, scenarioId, scenarioPath, sizeof(scenarioPath)))
            continue;
        char mctPath[1024];
        
        RKC_RPGSCRN_MCT peekMct;
        RKC_RPGSCRN_MCT_Init(&peekMct);
        if (RKC_RPGSCRN_MCT_Read(&peekMct, mctPath))
            
        RKC_RPGSCRN_MCT_Release(&peekMct);
    }
}


static void SuppressTriggerOnlyChestVisuals(DemoState *state)
{
    if (!state->scriptLoaded)
        return;

    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        LiveSpawn *spawn = &state->liveSpawns[i];
        if (spawn->block != 1 || spawn->field8 != 0 || spawn->templateIndex < 0)
            continue;

        for (long s = 0; s < state->script.statusCount; s++)
        {
            const RKC_RPG_SCRIPT_Status *st = &state->script.statuses[s];
            if (st->characterNo == spawn->characterNo && st->status == RKC_RPG_SCRIPT_STATUS_ONJUDGE)
            {
                spawn->templateIndex = -1;
                break;
            }
        }
    }
}


static void ReleaseScenarioSave(ScenarioSaveState *save)
{
    free(save->liveSpawnSaves);
    free(save->worldItemSaves);
    free(save->netFlagValues);
    free(save->characterFlagValues);
    memset(save, 0, sizeof(*save));
}


static ScenarioSaveState *FindOrCreateScenarioSave(DemoState *state, long scenarioId)
{
    for (long i = 0; i < state->scenarioSaveCount; i++)
        if (state->scenarioSaves[i].scenarioId == scenarioId)
        {
            ReleaseScenarioSave(&state->scenarioSaves[i]);
            state->scenarioSaves[i].scenarioId = scenarioId;
            return &state->scenarioSaves[i];
        }

    ScenarioSaveState *grown =
        realloc(state->scenarioSaves, sizeof(ScenarioSaveState) * (size_t)(state->scenarioSaveCount + 1));
    if (!grown)
        return NULL;
    state->scenarioSaves = grown;

    ScenarioSaveState *save = &state->scenarioSaves[state->scenarioSaveCount++];
    memset(save, 0, sizeof(*save));
    save->scenarioId = scenarioId;
    return save;
}


static void SnapshotScenarioState(DemoState *state)
{
    if (!state->mctLoaded || state->currentScenarioId < 0)
        return;

    ScenarioSaveState *save = FindOrCreateScenarioSave(state, state->currentScenarioId);
    if (!save)
        return;

    SnapshotLiveSpawns(state, &save->liveSpawnSaves, &save->liveSpawnSaveCount);
    SnapshotWorldItems(state, &save->worldItemSaves, &save->worldItemSaveCount);
    save->worldItemBaseCountAtSave = state->baseWorldItemCount;

    save->netFlagCount = state->execState.netFlagCount;
    if (save->netFlagCount > 0)
    {
        save->netFlagValues = malloc(sizeof(long) * (size_t)save->netFlagCount);
        memcpy(save->netFlagValues, state->execState.netFlagValues, sizeof(long) * (size_t)save->netFlagCount);
    }

    save->characterFlagCount = state->execState.characterFlagCount;
    if (save->characterFlagCount > 0)
    {
        save->characterFlagValues = malloc(sizeof(long) * (size_t)save->characterFlagCount);
        memcpy(save->characterFlagValues, state->execState.characterFlagValues,
               sizeof(long) * (size_t)save->characterFlagCount);
    }
}


static void RestoreScenarioState(DemoState *state)
{
    if (state->currentScenarioId < 0)
        return;

    const ScenarioSaveState *save = NULL;
    for (long i = 0; i < state->scenarioSaveCount; i++)
        if (state->scenarioSaves[i].scenarioId == state->currentScenarioId)
        {
            save = &state->scenarioSaves[i];
            break;
        }
    if (!save)
        return;

    if (save->liveSpawnSaveCount == state->liveSpawnCount)
        RestoreLiveSpawns(state, save->liveSpawnSaves, save->liveSpawnSaveCount);

    if (save->worldItemBaseCountAtSave == state->baseWorldItemCount)
        RestoreWorldItems(state, save->worldItemSaves, save->worldItemSaveCount, save->worldItemBaseCountAtSave);

    if (save->netFlagCount == state->execState.netFlagCount && save->netFlagCount > 0)
        memcpy(state->execState.netFlagValues, save->netFlagValues, sizeof(long) * (size_t)save->netFlagCount);

    if (save->characterFlagCount == state->execState.characterFlagCount && save->characterFlagCount > 0)
        memcpy(state->execState.characterFlagValues, save->characterFlagValues,
               sizeof(long) * (size_t)save->characterFlagCount);
}


static int SeedInitiallyActive(const DemoState *state, long spawnIndex)
{
    const LiveSpawn *s = &state->liveSpawns[spawnIndex];
    if (s->block != 1)
        return 1;
    return s->block1InitialVisible;
}


int LoadScenario(DemoState *state, const char *scenarioDir, long entryPoint)
{
    
    SnapshotScenarioState(state);

    
    if (state->scriptLoaded)
    {
        RKC_RPG_SCRIPT_EXEC_State_SaveGlobals(&state->execState, &state->scriptGlobals);
        state->scriptGlobalsValid = 1;
    }

    free(state->liveSpawns);
    state->liveSpawns = NULL;
    state->liveSpawnCount = 0;
    free(state->worldItems);
    state->worldItems = NULL;
    state->worldItemCount = 0;
    RKC_RPG_SCRIPT_EXEC_State_Release(&state->execState);
    state->mctLoaded = state->scriptLoaded = state->aiControlLoaded = 0;
    
    for (int i = 0; i < SCRIPT_EFFECT_MAX; i++)
        state->scriptEffects[i].active = 0;

    
    state->playerCasting = 0;

    
    RKC_UPDIB_Release(&state->minimapBg);
    RKC_UPDIB_Init(&state->minimapBg);
    state->minimapBgLoaded = 0;

    char mctPath[1024], scsPath[1024], minimapBgPath[1024];
    
    
    
    state->minimapBgLoaded = RKC_UPDIB_Read(&state->minimapBg, minimapBgPath) &&
                              RKC_UPDIB_GetFrameCount(&state->minimapBg) > 0;
    if (!state->minimapBgLoaded)
        

    state->mctLoaded = RKC_RPGSCRN_MCT_Read(&state->mct, mctPath);
    if (!state->mctLoaded)
    {
        
        return 0;
    }
    
    state->currentScenarioId = ParseScenarioIdFromPath(scenarioDir);
    

#ifdef GROUNDDEMO_AUDIO
    
    if (state->dsound.initialized && state->mct.bgmIndex != state->currentBgmIndex)
    {
        char bgmPath[1024];
        if (DeriveBgmPath(state->mapRoot, state->mct.bgmIndex, bgmPath, sizeof(bgmPath)))
        {
            
            if (RKC_DSOUND_ReadVocFile(&state->dsound, bgmPath, 0) &&
                (state->bgmHandle = RKC_DSOUND_Play(&state->dsound, 0, 0, 1 ,
                                                     state->bgmMuted ? -10000 : 0, 0)) >= 0)
            {
                state->bgmLoaded = 1;
                state->currentBgmIndex = state->mct.bgmIndex;
                
            }
            else
            {
                state->bgmLoaded = 0;
                state->currentBgmIndex = -1;
                
            }
        }
    }
#endif

    char stem[0x104];
    if (!DeriveMapStem(state->mct.mapPath, stem, sizeof(stem)) || !LoadArea(state, stem))
    {
        
        return 0;
    }

    
    RKC_RPGSCRN_MCT_Block5Entry entryPointSpawn;
    if (RKC_RPGSCRN_MCT_FindEntryPoint(&state->mct, 0, entryPoint, &entryPointSpawn))
    {
        state->playerX = entryPointSpawn.b;
        state->playerY = entryPointSpawn.c;
        state->playerFacingDirection = (int)entryPointSpawn.d;

        
        UpdateCameraFromPlayer(state);
    }

    
    if (!DeriveCharacterRoot(state->mapRoot, state->characterRoot, sizeof(state->characterRoot)))
        

    
    char aidFilePath[1024];
    if (DeriveControlAidPath(state->mapRoot, state->mct.aidPath, aidFilePath, sizeof(aidFilePath)))
    {
        state->aiControlLoaded = RKC_RPG_AICONTROL_Read(&state->aiControl, aidFilePath);
        if (state->aiControlLoaded)
            
        else
            
    }
    else
        

    BuildLiveSpawns(state);
    long resolved = 0, aiResolved = 0, block3Total = 0;
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        if (state->liveSpawns[i].templateIndex >= 0)
            resolved++;
        if (state->liveSpawns[i].block == 3)
        {
            block3Total++;
            if (state->liveSpawns[i].aiListNo >= 0)
                aiResolved++;
        }
    }
    
    
    
    long shown = 0;
    for (long i = 0; i < state->liveSpawnCount && shown < 3; i++)
        if (state->liveSpawns[i].block == 3 && state->liveSpawns[i].aiListNo >= 0)
        {
            
            shown++;
        }

    
    if (!state->itemDataLoaded)
    {
        char itemFilePath[1024];
        if (DeriveItemIbnPath(state->mapRoot, itemFilePath, sizeof(itemFilePath)))
        {
            state->itemDataLoaded = RKC_RPG_ITEMDATA_Read(&state->itemData, itemFilePath);
            if (state->itemDataLoaded)
                
            else
                
        }
        else
            
    }

    
    if (state->itemDataLoaded && !state->hasWeapon)
    {
        const RKC_RPG_ITEMDATA_Record *rec = RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, 0, 80000047);
        const RKC_RPG_ITEMDATA_Kind0Tail *tmpl = rec ? RKC_RPG_ITEMDATA_GetKind0Tail(rec) : NULL;
        if (tmpl)
        {
            RKC_RPG_ITEMDATA_RollKind0Instance(tmpl, &state->weapon);
            state->hasWeapon = 1;
            
            
            
        }

        
        const RKC_RPG_ITEMDATA_Record *axeRec = RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, 0, 4000000);
        if (axeRec)
        {
            AddItemToInventory(state, 0, 4000000, axeRec->name ? axeRec->name : "(unnamed weapon)");
            
        }

        
        const RKC_RPG_ITEMDATA_Record *efreetRec = RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, 0, 80000003);
        if (efreetRec)
        {
            AddItemToInventory(state, 0, 80000003, efreetRec->name ? efreetRec->name : "(unnamed weapon)");
            
        }
        const RKC_RPG_ITEMDATA_Record *blizzardRec =
            RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, 0, 80000037);
        if (blizzardRec)
        {
            AddItemToInventory(state, 0, 80000037, blizzardRec->name ? blizzardRec->name : "(unnamed weapon)");
            
        }
    }

    
    for (int s = 0; state->itemDataLoaded && s < EQUIPMENT_ARMOR_SLOTS; s++)
    {
        if (s == EQUIPMENT_SHIELD_SLOT_INDEX || state->hasArmor[s])
            continue;
        long armorTemplateId =
            s == EQUIPMENT_HELMET_SLOT_INDEX ? 2000000 : s == EQUIPMENT_BOOTS_SLOT_INDEX ? 3000000 : 80000000;
        const RKC_RPG_ITEMDATA_Record *rec = RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, 1, armorTemplateId);
        const RKC_RPG_ITEMDATA_Kind1Tail *tmpl = rec ? RKC_RPG_ITEMDATA_GetKind1Tail(rec) : NULL;
        if (!tmpl)
            continue;
        RKC_RPG_ITEMDATA_RollKind1Instance(tmpl, &state->armor[s]);
        state->hasArmor[s] = 1;
        
        
        
    }
    for (int s = 0; state->itemDataLoaded && s < EQUIPMENT_ACCESSORY_SLOTS; s++)
    {
        if (state->hasAccessory[s])
            continue;
        const RKC_RPG_ITEMDATA_Record *rec = RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, 2, 1000000);
        const RKC_RPG_ITEMDATA_Kind2Tail *tmpl = rec ? RKC_RPG_ITEMDATA_GetKind2Tail(rec) : NULL;
        if (!tmpl)
            continue;
        RKC_RPG_ITEMDATA_RollKind2Instance(tmpl, &state->accessory[s]);
        state->hasAccessory[s] = 1;
        
        
        
    }
    int hasAnyEquipment = state->hasWeapon;
    for (int s = 0; !hasAnyEquipment && s < EQUIPMENT_ARMOR_SLOTS; s++)
        hasAnyEquipment = state->hasArmor[s];
    for (int s = 0; !hasAnyEquipment && s < EQUIPMENT_ACCESSORY_SLOTS; s++)
        hasAnyEquipment = state->hasAccessory[s];
    if (hasAnyEquipment)
    {
        FinalCombatStats stats;
        ComputeFinalCombatStats(state, &stats);
        
        RecomputePlayerMaxHP(state);
    }

    
    if (!state->tableLoaded)
    {
        char tableFilePath[1024];
        if (DeriveTablePath(state->mapRoot, tableFilePath, sizeof(tableFilePath)))
        {
            state->tableLoaded = RKC_RPG_TABLE_Read(&state->table, tableFilePath);
            if (state->tableLoaded)
                
            else
                
        }
        else
            
    }

    
    if (state->tableLoaded && !state->progressionInitialized)
    {
        InitPlayerProgression(state);
        RecomputePlayerMaxHP(state);
        state->playerHP = state->playerMaxHP;
        RecomputePlayerMaxMP(state);
        state->playerMP = state->playerMaxMP;
    }

    BuildWorldItems(state);
    
    state->baseWorldItemCount = state->worldItemCount;
    {
        long itemsResolved = 0;
        shown = 0;
        for (long i = 0; i < state->worldItemCount; i++)
        {
            if (!state->worldItems[i].resolved)
                continue;
            itemsResolved++;
            if (shown < 3)
            {
                if (state->worldItems[i].isGold)
                    
                else
                    
                shown++;
            }
        }
        
    }

    state->scriptLoaded = RKC_RPG_SCRIPT_Read(&state->script, scsPath);
    if (state->scriptLoaded)
    {
        RKC_RPG_SCRIPT_EXEC_State_Init(&state->execState, &state->script);
        
        if (state->scriptGlobalsValid)
            RKC_RPG_SCRIPT_EXEC_State_RestoreGlobals(&state->execState, &state->scriptGlobals);

        
        for (long i = 0; i < state->liveSpawnCount; i++)
        {
            LiveSpawn *spawn = &state->liveSpawns[i];
            if (spawn->block != 1)
                continue;
            for (long j = 0; j < state->script.statusCount; j++)
                if (state->script.statuses[j].characterNo == spawn->characterNo &&
                    state->script.statuses[j].status == RKC_RPG_SCRIPT_STATUS_CHECK)
                {
                    spawn->hasCheckTrigger = 1;
                    break;
                }
        }
        for (long s = 0; s < state->script.sentenceCount; s++)
        {
            const RKC_RPG_SCRIPT_Sentence *sen = &state->script.sentences[s];
            for (long c = 0; c < sen->commandCount; c++)
            {
                const RKC_RPG_SCRIPT_Command *cmd = &sen->commands[c];
                
                for (long o = 0; o < cmd->operandCount; o++)
                    if (cmd->operands[o].type == 9)
                        for (long i = 0; i < state->liveSpawnCount; i++)
                            if (state->liveSpawns[i].block == 1 &&
                                state->liveSpawns[i].characterNo == cmd->operands[o].value)
                            {
                                state->liveSpawns[i].isCCheckTarget = 1;
                                break;
                            }
            }
        }

        
        if (state->liveSpawnCount > 0)
        {
            RKC_RPG_SCRIPT_EXEC_CharacterSeed *seeds =
                malloc(sizeof(RKC_RPG_SCRIPT_EXEC_CharacterSeed) * (size_t)state->liveSpawnCount);
            for (long i = 0; i < state->liveSpawnCount; i++)
            {
                seeds[i].characterNo = state->liveSpawns[i].characterNo;
                seeds[i].initiallyActive = SeedInitiallyActive(state, i);
            }
            RKC_RPG_SCRIPT_EXEC_State_InitCharacterFlags(&state->execState, &state->script, seeds,
                                                          state->liveSpawnCount);
            free(seeds);
        }
        else
            RKC_RPG_SCRIPT_EXEC_State_InitCharacterFlags(&state->execState, &state->script, NULL, 0);
        
        
        for (long i = 0; i < state->script.messageCount && i < 3; i++)
            if (state->script.messages[i].textLen > 0)
                
    }
    else
        

    
    ResolveGateDestinationNames(state);
    SuppressTriggerOnlyChestVisuals(state);

    
    RestoreScenarioState(state);

    PrintSpawnNavigationHints(state);

    state->currentEntryPoint = entryPoint;
    RunEntryScenarioTrigger(state);

    return 1;
}
