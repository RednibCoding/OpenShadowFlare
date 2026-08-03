#include "sprites.h"
#include "render.h"


void LoadCafTemplate(LiveSpawnTemplate *t, const char *dir, const char *stem)
{
    char path[sizeof(((DemoState *)0)->characterRoot) + 64];
    snprintf(path, sizeof(path), "%s/%s.Caf", dir, stem);
    int ok = RKC_RPGSCRN_CAF_Read(&t->caf, path);
    if (ok)
    {
        snprintf(path, sizeof(path), "%s/%s.Njp", dir, stem);
        ok = RKC_UPDIB_Read(&t->animNjp, path);
    }
    if (ok)
    {
        snprintf(path, sizeof(path), "%s/%s.Sdw", dir, stem);
        RKC_UPDIB_Read(&t->animSdw, path);
    }
    if (ok)
        t->kind = LIVE_SPAWN_SPRITE_CAF;
}


static int FindOrLoadTemplate(DemoState *state, int block, long field8)
{
    if (field8 < 0 || state->characterRoot[0] == '\0')
        return -1;

    for (int i = 0; i < state->templateCount; i++)
        if (state->templates[i].block == block && state->templates[i].field8 == field8)
            return state->templates[i].kind == LIVE_SPAWN_SPRITE_NONE ? -1 : i;

    if (state->templateCount >= LIVE_SPAWN_TEMPLATE_CACHE_MAX)
        return -1;

    int idx = state->templateCount++;
    LiveSpawnTemplate *t = &state->templates[idx];
    memset(t, 0, sizeof(*t));
    t->block = block;
    t->field8 = field8;
    t->kind = LIVE_SPAWN_SPRITE_NONE;
    RKC_UPDIB_Init(&t->staticNjp);
    RKC_RPGSCRN_CAF_Init(&t->caf);
    RKC_UPDIB_Init(&t->animNjp);
    RKC_UPDIB_Init(&t->animSdw);

    char dir[sizeof(((DemoState *)0)->characterRoot) + 32];
    if (block == 1)
    {
        snprintf(dir, sizeof(dir), "%s/OBJECT/%08ld", state->characterRoot, field8);
        char path[sizeof(dir) + 32];
        snprintf(path, sizeof(path), "%s/Pattern.Njp", dir);
        if (!RKC_UPDIB_Read(&t->staticNjp, path))
        {
            snprintf(path, sizeof(path), "%s/Pattern.njp", dir);
            RKC_UPDIB_Read(&t->staticNjp, path);
        }
        if (RKC_UPDIB_GetFrameCount(&t->staticNjp) > 0)
            t->kind = LIVE_SPAWN_SPRITE_STATIC;
        else
            LoadCafTemplate(t, dir, "Animation");
    }
    else if (block == 2)
    {
        if (field8 < LIVE_SPAWN_BLOCK2_PARTNER_OFFSET)
            snprintf(dir, sizeof(dir), "%s/ENEMY/%08ld", state->characterRoot, field8);
        else
            snprintf(dir, sizeof(dir), "%s/PARTNER/%08ld", state->characterRoot,
                     field8 - LIVE_SPAWN_BLOCK2_PARTNER_OFFSET);
        LoadCafTemplate(t, dir, "Animation");
    }
    else 
    {
        snprintf(dir, sizeof(dir), "%s/PEOPLE/%08ld", state->characterRoot, field8);
        LoadCafTemplate(t, dir, "Animation");
    }

    return t->kind == LIVE_SPAWN_SPRITE_NONE ? -1 : idx;
}


void BuildLiveSpawns(DemoState *state)
{
    long total = state->mct.block1Count + state->mct.block2Count + state->mct.block3Count;
    if (total <= 0)
        return;

    state->liveSpawns = malloc(sizeof(LiveSpawn) * (size_t)total);
    long n = 0;

    struct
    {
        const RKC_RPGSCRN_MCT_Record *records;
        long count;
        int block;
        long addend;
    } blocks[3] = {
        {state->mct.block1, state->mct.block1Count, 1, LIVE_SPAWN_BLOCK1_ADDEND},
        {state->mct.block2, state->mct.block2Count, 2, LIVE_SPAWN_BLOCK2_ADDEND},
        {state->mct.block3, state->mct.block3Count, 3, LIVE_SPAWN_BLOCK3_ADDEND},
    };

    for (int b = 0; b < 3; b++)
        for (long i = 0; i < blocks[b].count; i++)
        {
            const RKC_RPGSCRN_MCT_Record *rec = &blocks[b].records[i];
            LiveSpawn *spawn = &state->liveSpawns[n++];
            spawn->x = rec->x;
            spawn->y = rec->y;
            spawn->rectL = rec->rectL;
            spawn->rectT = rec->rectT;
            spawn->rectR = rec->rectR;
            spawn->rectB = rec->rectB;
            spawn->block = blocks[b].block;
            spawn->characterNo = rec->sortKey + blocks[b].addend;
            spawn->field8 = rec->field8;
            spawn->templateIndex = FindOrLoadTemplate(state, blocks[b].block, rec->field8);
            
            spawn->patternIndex = 0;
            if (blocks[b].block == 1 && spawn->templateIndex >= 0)
            {
                long rawPattern;
                if (RKC_RPGSCRN_MCT_GetBlock1PatternIndex(rec, &rawPattern))
                {
                    long patternCount = RKC_UPDIB_GetPatternCount(&state->templates[spawn->templateIndex].staticNjp);
                    if (rawPattern >= 0 && rawPattern < patternCount)
                        spawn->patternIndex = rawPattern;
                }
                
                long drawFlag;
                if (RKC_RPGSCRN_MCT_GetBlock1DrawFlag(rec, &drawFlag) && drawFlag == 0 &&
                    state->templates[spawn->templateIndex].kind == LIVE_SPAWN_SPRITE_STATIC)
                    spawn->templateIndex = -1;
            }
            spawn->name = rec->name;
            spawn->hasCheckTrigger = 0; 
            spawn->isCCheckTarget = 0;  
            spawn->block1Height = 0;
            spawn->block1Walkable = 0;
            spawn->block1InitialVisible = 1;
            if (blocks[b].block == 1)
            {
                long h, w;
                if (RKC_RPGSCRN_MCT_GetBlock1Height(rec, &h) && h > 0)
                    spawn->block1Height = h; 
                if (RKC_RPGSCRN_MCT_GetBlock1WalkableFlag(rec, &w) && w == 1)
                    spawn->block1Walkable = 1; 
                
                if (rec->waypointCount >= 1 && rec->waypoints)
                    spawn->block1InitialVisible = rec->waypoints[0] != 0;
                else
                    spawn->block1InitialVisible = rec->field8 >= 0;
            }
            spawn->aiListNo = -1;
            spawn->aiListName = NULL;
            spawn->baseDamage = ENEMY_ATTACK_BASE_DAMAGE_PLACEHOLDER;
            spawn->attackSpeedIndex = 4; 
            spawn->attackChartIndex = -1;
            
            spawn->facingDirection =
                (blocks[b].block == 2 && rec->field15 >= 0 && rec->field15 <= 7) ? (int)rec->field15 : 1;
            spawn->isMoving = 0;
            spawn->slideDir = SLIDE_NONE;
            spawn->deathTick = 0;
            spawn->hitStunTick = 0;
            spawn->hitStunDurationTicks = 0;
            spawn->hitVfxTick = 0;
            spawn->hitTestOffsetY = 0; 
            spawn->hitTestHalfWidth = 0;
            spawn->hitTestHalfHeight = 0;
            spawn->actionAnimChart = -1; 
            spawn->actionAnimTick = 0;
            spawn->cafHeightCacheValid = 0; 
            
            spawn->cellBlockMask = rec->hasSubArray ? rec->sub17 : NULL;
            spawn->cellBlockMaskCount = rec->hasSubArray ? rec->subCount : 0;
            
            spawn->cellTintR = rec->hasSubArray ? rec->sub18 : NULL;
            spawn->cellTintG = rec->hasSubArray ? rec->sub19 : NULL;
            spawn->cellTintB = rec->hasSubArray ? rec->sub20 : NULL;

            
            spawn->gateHighlightFadeTicks = 0;
            spawn->gateDestinationName[0] = '\0';

            
            spawn->npcWanderL = spawn->npcWanderT = spawn->npcWanderR = spawn->npcWanderB = 0;
            if (blocks[b].block == 2 && !RKC_RPGSCRN_MCT_IsBlock2Stationary(rec))
                RKC_RPGSCRN_MCT_GetBlock2WanderRect(rec, &spawn->npcWanderL, &spawn->npcWanderT, &spawn->npcWanderR,
                                                    &spawn->npcWanderB);
            spawn->npcHasWanderTarget = 0;
            spawn->npcWanderPauseUntilTick = 0;

            
            spawn->npcFacesTalker = blocks[b].block == 2 ? RKC_RPGSCRN_MCT_GetBlock2FacesTalker(rec) : 1;

            
            const RKC_RPG_AILIST *resolvedAiList = NULL;
            if (blocks[b].block == 3 && state->aiControlLoaded)
            {
                char aiName[0x101];
                if (RKC_RPGSCRN_MCT_GetBlock3AiName(rec, aiName) && aiName[0] != '\0')
                {
                    resolvedAiList = RKC_RPG_AICONTROL_GetFromName(&state->aiControl, aiName);
                    if (resolvedAiList)
                    {
                        spawn->aiListNo = RKC_RPG_AICONTROL_GetNo(&state->aiControl, resolvedAiList);
                        spawn->aiListName = resolvedAiList->name;
                    }
                }
            }
            
            long realMaxHP = LIVE_SPAWN_DEFAULT_MAX_HP;
            spawn->lootTableRow = -1; 
            spawn->monsterLevel = 0;
            spawn->speedPercent = 100;
            spawn->moveSpeedPercent = 100;
            if (blocks[b].block == 3)
            {
                long realBaseDamage;
                if (RKC_RPGSCRN_MCT_GetBlock3BaseDamage(rec, 0, &realBaseDamage))
                    spawn->baseDamage = realBaseDamage;

                
                long realSpeedIndex;
                if (RKC_RPGSCRN_MCT_GetBlock3SpeedIndex(rec, 0, &realSpeedIndex) && realSpeedIndex >= 0 &&
                    realSpeedIndex < 10)
                    spawn->attackSpeedIndex = realSpeedIndex;

                
                spawn->attackChartIndex = ENEMY_ATTACK_CHART;

                
                long v;
                if (RKC_RPGSCRN_MCT_GetBlock3MaxHP(rec, &v) && v > 0)
                    realMaxHP = v;
                if (RKC_RPGSCRN_MCT_GetBlock3LootTableRow(rec, &v))
                    spawn->lootTableRow = v;
                if (RKC_RPGSCRN_MCT_GetBlock3Level(rec, &v))
                    spawn->monsterLevel = v;
                
                if (RKC_RPGSCRN_MCT_GetBlock3SpeedPercent(rec, &v) && v > 0)
                    spawn->speedPercent = v;
                if (RKC_RPGSCRN_MCT_GetBlock3MoveSpeedX1000(rec, &v) && v > 0)
                    spawn->moveSpeedPercent = v / 10;
            }
            
            RKC_RPG_AI_EXEC_State_Init(&spawn->aiState, resolvedAiList, spawn->x, spawn->y, realMaxHP);
            spawn->attackCooldownTicks = 0;
            spawn->attackAnimTick = 0;
        }
    state->liveSpawnCount = n;
}


void SnapshotLiveSpawns(const DemoState *state, LiveSpawnSaveEntry **outEntries, long *outCount)
{
    *outCount = state->liveSpawnCount;
    if (state->liveSpawnCount <= 0)
    {
        *outEntries = NULL;
        return;
    }

    LiveSpawnSaveEntry *entries = malloc(sizeof(LiveSpawnSaveEntry) * (size_t)state->liveSpawnCount);
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        const LiveSpawn *spawn = &state->liveSpawns[i];
        entries[i].x = spawn->x;
        entries[i].y = spawn->y;
        entries[i].currentHP = spawn->aiState.currentHP;
        entries[i].isDead = spawn->aiState.isDead;
    }
    *outEntries = entries;
}


void RestoreLiveSpawns(DemoState *state, const LiveSpawnSaveEntry *entries, long count)
{
    for (long i = 0; i < count; i++)
    {
        LiveSpawn *spawn = &state->liveSpawns[i];
        spawn->x = entries[i].x;
        spawn->y = entries[i].y;
        spawn->aiState.currentHP = entries[i].currentHP;
        spawn->aiState.isDead = entries[i].isDead;
    }
}


long FindNearestLiveSpawn(DemoState *state)
{
    long centerX, centerY;
    WorldToScreen(state, state->playerX, state->playerY, &centerX, &centerY);
    long radius = INTERACT_RADIUS_TILES * state->ground.chipHeight;

    long best = -1;
    long bestDistSq = radius * radius;
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        long sx, sy;
        WorldToScreen(state, state->liveSpawns[i].x, state->liveSpawns[i].y, &sx, &sy);
        long dx = sx - centerX, dy = sy - centerY;
        long distSq = dx * dx + dy * dy;
        if (distSq <= bestDistSq)
        {
            bestDistSq = distSq;
            best = i;
        }
    }
    return best;
}


long FindSpawnNearScreenPoint(DemoState *state, long screenX, long screenY)
{
    long radius = (long)(state->clickRangeTileHeight * state->ground.chipHeight);
    long best = -1;
    long bestDistSq = -1;
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        const LiveSpawn *spawn = &state->liveSpawns[i];
        
        
        int bodilessBlock3 =
            spawn->templateIndex < 0 && spawn->block == 3 && spawn->name != NULL && spawn->rectL < spawn->rectR;
        if ((spawn->templateIndex < 0 && !bodilessBlock3) || (spawn->block == 3 && spawn->aiState.isDead) ||
            !IsHoverableLiveSpawn(spawn))
            continue;
        
        if (spawn->block == 1 && state->scriptLoaded &&
            !RKC_RPG_SCRIPT_EXEC_IsCharacterActive(&state->execState, spawn->characterNo))
            continue;
        long sx, sy;
        WorldToScreen(state, spawn->x, spawn->y, &sx, &sy);
        long dx = sx - screenX, dy = (sy - spawn->hitTestOffsetY) - screenY;
        if (bodilessBlock3)
        {
            long wx, wy;
            ScreenToWorld(state, screenX, screenY, &wx, &wy);
            if (wx < spawn->x + spawn->rectL || wx > spawn->x + spawn->rectR || wy < spawn->y + spawn->rectT ||
                wy > spawn->y + spawn->rectB)
                continue;
            dy = sy - screenY; 
        }
        else
        {
            long adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
            if (adx > radius + spawn->hitTestHalfWidth || ady > radius + spawn->hitTestHalfHeight)
                continue;
        }
        long distSq = dx * dx + dy * dy;
        if (best < 0 || distSq <= bestDistSq)
        {
            bestDistSq = distSq;
            best = i;
        }
    }
    return best;
}


long FindWorldItemNearScreenPoint(DemoState *state, long screenX, long screenY)
{
    long radius = (long)(state->clickRangeTileHeight * state->ground.chipHeight);
    long best = -1;
    long bestDistSq = -1;
    for (long i = 0; i < state->worldItemCount; i++)
    {
        const WorldItem *item = &state->worldItems[i];
        if (item->pickedUp)
            continue;
        long sx, sy;
        WorldToScreen(state, item->x, item->y, &sx, &sy);
        long dx = sx - screenX, dy = sy - screenY;
        long adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
        if (adx > radius + item->hitTestHalfWidth || ady > radius + item->hitTestHalfHeight)
            continue;
        long distSq = dx * dx + dy * dy;
        if (best < 0 || distSq <= bestDistSq)
        {
            bestDistSq = distSq;
            best = i;
        }
    }
    return best;
}


long FindHoveredSpawn(DemoState *state, long screenX, long screenY)
{
    long radius = (long)(state->clickRangeTileHeight * state->ground.chipHeight);
    long best = -1;
    long bestDistSq = -1;
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        const LiveSpawn *spawn = &state->liveSpawns[i];
        
        int bodilessBlock3 =
            spawn->templateIndex < 0 && spawn->block == 3 && spawn->name != NULL && spawn->rectL < spawn->rectR;
        if ((spawn->templateIndex < 0 && !bodilessBlock3) || (spawn->block == 3 && spawn->aiState.isDead) ||
            !IsHoverableLiveSpawn(spawn))
            continue;
        
        if (spawn->block == 1 && state->scriptLoaded &&
            !RKC_RPG_SCRIPT_EXEC_IsCharacterActive(&state->execState, spawn->characterNo))
            continue;
        long sx, sy;
        WorldToScreen(state, spawn->x, spawn->y, &sx, &sy);
        long dx = sx - screenX, dy = (sy - spawn->hitTestOffsetY) - screenY;
        if (bodilessBlock3)
        {
            long wx, wy;
            ScreenToWorld(state, screenX, screenY, &wx, &wy);
            if (wx < spawn->x + spawn->rectL || wx > spawn->x + spawn->rectR || wy < spawn->y + spawn->rectT ||
                wy > spawn->y + spawn->rectB)
                continue;
            dy = sy - screenY;
        }
        else
        {
            long adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
            if (adx > radius + spawn->hitTestHalfWidth || ady > radius + spawn->hitTestHalfHeight)
                continue;
        }
        long distSq = dx * dx + dy * dy;
        if (best < 0 || distSq <= bestDistSq)
        {
            bestDistSq = distSq;
            best = i;
        }
    }
    return best;
}


void PrintSpawnNavigationHints(DemoState *state)
{
    long startCameraX, startCameraY;
    RKC_RPGSCRN_GROUND_CellToScreen(&state->ground, state->ground.areaWidth / 2, state->ground.areaHeight / 2,
                                    &startCameraX, &startCameraY);
    startCameraX -= APP_WIDTH / 2;
    startCameraY -= APP_HEIGHT / 2;

    int printed = 0;
    for (long i = 0; i < state->liveSpawnCount && printed < 8; i++)
    {
        const LiveSpawn *spawn = &state->liveSpawns[i];
        if (!spawn->name)
            continue;

        int hasTrigger = 0;
        for (long j = 0; j < state->script.statusCount && !hasTrigger; j++)
            if (state->script.statuses[j].characterNo == spawn->characterNo)
                hasTrigger = 1;
        if (!hasTrigger)
            continue;

        long screenX, screenY;
        WorldToScreen(state, spawn->x, spawn->y, &screenX, &screenY);
        long dx = screenX - APP_WIDTH / 2 - startCameraX;
        long dy = screenY - APP_HEIGHT / 2 - startCameraY;

        if (printed == 0)
            printf("  named scenario actors near the starting view:\n");
        printf("    %s at screen offset (%ld,%ld)\n", spawn->name, dx, dy);
        printed++;
    }
    if (printed == 0)
        printf("  (no nearby named scenario actors with interaction triggers)\n");
}
