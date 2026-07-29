#include "movement.h"
#include "render.h"
#include "combat.h"
#include "inventory.h"
#include "pathfind.h"
#include "script_bridge.h"
#include "sprites.h"

#include <math.h>
#include <stdlib.h>


int RectsOverlap(long ax, long ay, long aL, long aT, long aR, long aB, long bx, long by, long bL, long bT, long bR,
                  long bB)
{
    return ax + aL < bx + bR && bx + bL < ax + aR && ay + aT < by + bB && by + bT < ay + aB;
}

long RectGapDistance(long ax, long ay, long aL, long aT, long aR, long aB, long bx, long by, long bL, long bT,
                      long bR, long bB)
{
    long aLeft = ax + aL, aRight = ax + aR, aTop = ay + aT, aBottom = ay + aB;
    long bLeft = bx + bL, bRight = bx + bR, bTop = by + bT, bBottom = by + bB;

    if (bRight < aLeft || aRight < bLeft)
    {
        if (aTop <= bBottom && bTop <= aBottom)
        {
            long d = labs(aLeft - bLeft) - 1;
            long d2 = labs(aRight - bLeft) - 1;
            if (d2 < d)
                d = d2;
            d2 = labs(aLeft - bRight) - 1;
            if (d2 < d)
                d = d2;
            d2 = labs(aRight - bRight) - 1;
            if (d2 < d)
                d = d2;
            return d;
        }
        long dx, dy;
        if (aLeft < bLeft)
        {
            dy = (aTop < bTop) ? (aBottom - bTop) : (aTop - bBottom);
            dx = aRight - bLeft;
        }
        else if (aTop < bTop)
        {
            dy = aBottom - bTop;
            dx = aLeft - bRight;
        }
        else
        {
            dy = aTop - bBottom;
            dx = aLeft - bRight;
        }
        long d = (long)sqrt((double)dx * (double)dx + (double)dy * (double)dy) - 1;
        return d < 0 ? 0 : d;
    }
    if (bBottom < aTop || aBottom < bTop)
    {
        long d = labs(aTop - bTop) - 1;
        long d2 = labs(aBottom - bTop) - 1;
        if (d2 < d)
            d = d2;
        d2 = labs(aTop - bBottom) - 1;
        if (d2 < d)
            d = d2;
        d2 = labs(aBottom - bBottom) - 1;
        if (d2 < d)
            d = d2;
        return d;
    }
    return 0;
}


static int OverlapsAnyCharacterAt(DemoState *state, long x, long y, long moverL, long moverT, long moverR,
                                  long moverB, long excludeSpawnIndex, int isPlayerMover)
{
    if (!isPlayerMover)
    {
        
        const long approachFootprint = ENEMY_ATTACK_APPROACH_PLAYER_HALF_WIDTH;
        if (RectsOverlap(x, y, moverL, moverT, moverR, moverB, state->playerX, state->playerY, -approachFootprint,
                         -approachFootprint, approachFootprint, approachFootprint))
            return 1;
    }

    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        if (i == excludeSpawnIndex)
            continue;
        const LiveSpawn *other = &state->liveSpawns[i];
        if (other->block < 1 || other->block > 3)
            continue;
        if (other->block == 3 && other->aiState.isDead)
            continue;
        
        if (other->block == 1 && other->templateIndex < 0)
            continue;
        
        if (other->block == 1 && other->block1Walkable)
            continue;
        
        if (other->block == 3 && other->templateIndex < 0)
            continue;
        
        if (other->block == 1 && state->scriptLoaded &&
            !RKC_RPG_SCRIPT_EXEC_IsCharacterActive(&state->execState, other->characterNo))
            continue;
        if (RectsOverlap(x, y, moverL, moverT, moverR, moverB, other->x, other->y, other->rectL, other->rectT,
                         other->rectR, other->rectB))
        {
            
            long curX = isPlayerMover ? state->playerX
                        : excludeSpawnIndex >= 0 ? state->liveSpawns[excludeSpawnIndex].x
                                                 : x;
            long curY = isPlayerMover ? state->playerY
                        : excludeSpawnIndex >= 0 ? state->liveSpawns[excludeSpawnIndex].y
                                                 : y;
            if (RectsOverlap(curX, curY, moverL, moverT, moverR, moverB, other->x, other->y, other->rectL,
                             other->rectT, other->rectR, other->rectB))
                continue;
            return 1;
        }
    }

    long objectCount = RKC_RPGSCRN_OBJECTBLOCK_GetCount(&state->objects);
    for (long i = 0; i < objectCount; i++)
    {
        const RKC_RPGSCRN_OBJECT_Entry *object = RKC_RPGSCRN_OBJECTBLOCK_Get(&state->objects, i);
        if (object->slotIndex < 0 || object->rectL >= object->rectR || object->rectT >= object->rectB)
            continue;
        if (object->field5 & OBL_FIELD5_GROUND_DECAL_BIT)
            continue;
        if (RectsOverlap(x, y, moverL, moverT, moverR, moverB, object->posX, object->posY, object->rectL,
                         object->rectT, object->rectR, object->rectB))
        {
            
            if (isPlayerMover && state->judgeOverlay)
                
            return 1;
        }
    }
    return 0;
}


int PlayerBlockedByCharacterAt(DemoState *state, long x, long y)
{
    const long playerFootprint = PLAYER_FOOTPRINT_HALF_WIDTH;
    return OverlapsAnyCharacterAt(state, x, y, -playerFootprint, -playerFootprint, playerFootprint, playerFootprint,
                                  -1, 1);
}


int ResolveFacingDirection(long dx, long dy, int current)
{
    if (dx == 0 && dy == 0)
        return current;

    double angleDeg = atan2((double)(-dy), (double)dx) * (180.0 / 3.14159265358979323846);
    if (angleDeg < 0.0)
        angleDeg += 360.0;
    long deci = (long)(angleDeg * 10.0);
    if (deci < 0)
        deci = 0;
    if (deci >= 3600)
        deci -= 3600;

    if (deci < 0xe2)
        return 1;
    if (deci < 0x2a4)
        return 2;
    if (deci < 0x466)
        return 3;
    if (deci < 0x628)
        return 4;
    if (deci < 0x7ea)
        return 5;
    if (deci < 0x9ac)
        return 6;
    if (deci < 0xb6e)
        return 7;
    if (deci < 0xd2f)
        return 0;
    return 1;
}


static int StepSpawnToward(DemoState *state, LiveSpawn *spawn, long wantX, long wantY, long spawnIndex)
{
    long curX = spawn->x, curY = spawn->y;
    long stepDX = wantX - curX, stepDY = wantY - curY;
    if (stepDX == 0 && stepDY == 0)
        return 0;

    long speed = (long)llround(sqrt((double)stepDX * (double)stepDX + (double)stepDY * (double)stepDY));
    if (speed <= 0)
        speed = 1;

    SlideDir priorSlide = (SlideDir)spawn->slideDir;
    long clampedX, clampedY;

    
    int unobstructed = RKC_RPGSCRN_GROUND_SweepMove(&state->ground, curX, curY, wantX, wantY, &clampedX, &clampedY);
    if (unobstructed && !OverlapsAnyCharacterAt(state, clampedX, clampedY, spawn->rectL, spawn->rectT, spawn->rectR,
                                                 spawn->rectB, spawnIndex, 0))
    {
        spawn->x = clampedX;
        spawn->y = clampedY;
        spawn->slideDir = SLIDE_NONE;
        return 1;
    }

    
    if (priorSlide != SLIDE_NONE)
    {
        long slideX = curX, slideY = curY;
        switch (priorSlide)
        {
        case SLIDE_NORTH:
            slideY -= speed;
            break;
        case SLIDE_SOUTH:
            slideY += speed;
            break;
        case SLIDE_WEST:
            slideX -= speed;
            break;
        case SLIDE_EAST:
            slideX += speed;
            break;
        default:
            break;
        }
        RKC_RPGSCRN_GROUND_SweepMove(&state->ground, curX, curY, slideX, slideY, &clampedX, &clampedY);
        if ((clampedX != curX || clampedY != curY) &&
            !OverlapsAnyCharacterAt(state, clampedX, clampedY, spawn->rectL, spawn->rectT, spawn->rectR,
                                     spawn->rectB, spawnIndex, 0))
        {
            spawn->x = clampedX;
            spawn->y = clampedY;
            return 1;
        }
        
    }

    
    SlideDir wantedXDir = stepDX > 0 ? SLIDE_EAST : stepDX < 0 ? SLIDE_WEST : SLIDE_NONE;
    SlideDir wantedYDir = stepDY > 0 ? SLIDE_SOUTH : stepDY < 0 ? SLIDE_NORTH : SLIDE_NONE;

    SlideDir probes[4];
    int numProbes;
    if (wantedXDir != SLIDE_NONE && wantedYDir != SLIDE_NONE)
    {
        probes[0] = wantedXDir;
        probes[1] = wantedYDir;
        probes[2] = (wantedXDir == SLIDE_EAST) ? SLIDE_WEST : SLIDE_EAST;
        probes[3] = (wantedYDir == SLIDE_SOUTH) ? SLIDE_NORTH : SLIDE_SOUTH;
        numProbes = 4;
    }
    else if (wantedXDir != SLIDE_NONE)
    {
        probes[0] = SLIDE_SOUTH;
        probes[1] = SLIDE_NORTH;
        numProbes = 2;
    }
    else if (wantedYDir != SLIDE_NONE)
    {
        probes[0] = SLIDE_EAST;
        probes[1] = SLIDE_WEST;
        numProbes = 2;
    }
    else
        numProbes = 0; 

    
    SlideDir avoidReverse;
    switch (priorSlide)
    {
    case SLIDE_EAST:
        avoidReverse = SLIDE_WEST;
        break;
    case SLIDE_WEST:
        avoidReverse = SLIDE_EAST;
        break;
    case SLIDE_NORTH:
        avoidReverse = SLIDE_SOUTH;
        break;
    case SLIDE_SOUTH:
        avoidReverse = SLIDE_NORTH;
        break;
    default:
        avoidReverse = SLIDE_NONE;
        break;
    }

    for (int pass = 0; pass < 2; pass++)
    {
        for (int i = 0; i < numProbes; i++)
        {
            if (pass < 1 && probes[i] == avoidReverse)
                continue;

            long probeX = curX, probeY = curY;
            switch (probes[i])
            {
            case SLIDE_NORTH:
                probeY -= speed;
                break;
            case SLIDE_SOUTH:
                probeY += speed;
                break;
            case SLIDE_WEST:
                probeX -= speed;
                break;
            case SLIDE_EAST:
                probeX += speed;
                break;
            default:
                break;
            }

            if (!RKC_RPGSCRN_GROUND_IsBlocked(&state->ground, probeX, probeY) &&
                !OverlapsAnyCharacterAt(state, probeX, probeY, spawn->rectL, spawn->rectT, spawn->rectR,
                                        spawn->rectB, spawnIndex, 0))
            {
                spawn->slideDir = probes[i];
                return 0; 
            }
        }
    }

    
    spawn->slideDir = SLIDE_NONE;
    return 0;
}


void TickLiveSpawnsAI(DemoState *state)
{
    long playerWorldX = state->playerX, playerWorldY = state->playerY;

    int debugPrinted = 0;
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        LiveSpawn *spawn = &state->liveSpawns[i];
        if (spawn->block != 3)
            continue;

        
        if (spawn->aiState.isDead)
            continue;

        
        RKC_RPG_AI_EXEC_TickInput in = {spawn->x,          spawn->y,          1, playerWorldX, playerWorldY,
                                        AI_TICK_DELTA_MS, spawn->attackCooldownTicks > 0,
                                        spawn->moveSpeedPercent };
        RKC_RPG_AI_EXEC_TickResult out;
        RKC_RPG_AI_EXEC_Tick(&spawn->aiState, &in, &out);
        
        spawn->isMoving = 0;
        
        if (out.moved && spawn->attackCooldownTicks == 0)
        {
            
            long prevSpawnX = spawn->x, prevSpawnY = spawn->y;
            if (StepSpawnToward(state, spawn, out.newX, out.newY, i))
            {
                spawn->isMoving = 1;
                
                if (spawn->slideDir != SLIDE_NONE)
                    spawn->facingDirection =
                        ResolveFacingDirection(spawn->x - prevSpawnX, spawn->y - prevSpawnY, spawn->facingDirection);
                else
                    spawn->facingDirection = ResolveFacingDirection((long)llround(out.dirX * 1000.0),
                                                                     (long)llround(out.dirY * 1000.0),
                                                                     spawn->facingDirection);
            }
        }
        else if (RKC_RPG_AI_EXEC_IsAttacking(&spawn->aiState))
        {
            
            spawn->facingDirection =
                ResolveFacingDirection(playerWorldX - spawn->x, playerWorldY - spawn->y, spawn->facingDirection);
        }

        TickEnemyAttack(state, spawn);

        
        if (spawn->aiState.list && !debugPrinted && state->tick % 120 == 0)
        {
            
            fflush(stdout);
            debugPrinted = 1;
        }
    }
}


void TickNpcWander(DemoState *state)
{
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        LiveSpawn *spawn = &state->liveSpawns[i];
        if (spawn->block != 2 || spawn->npcWanderL >= spawn->npcWanderR || spawn->npcWanderT >= spawn->npcWanderB)
            continue;

        
        if (state->dialogActive && state->dialogQueueCount > 0 && spawn->characterNo == state->dialogQueue[0].characterNo)
        {
            spawn->isMoving = 0;
            continue;
        }

        if (state->tick < spawn->npcWanderPauseUntilTick)
        {
            spawn->isMoving = 0;
            continue;
        }

        if (!spawn->npcHasWanderTarget)
        {
            long rangeX = spawn->npcWanderR - spawn->npcWanderL;
            long rangeY = spawn->npcWanderB - spawn->npcWanderT;
            spawn->npcWanderTargetX = spawn->npcWanderL + (rangeX > 0 ? rand() % (rangeX + 1) : 0);
            spawn->npcWanderTargetY = spawn->npcWanderT + (rangeY > 0 ? rand() % (rangeY + 1) : 0);
            spawn->npcHasWanderTarget = 1;
            
            spawn->npcWanderBestDist = -1.0;
            spawn->npcWanderBestDistTick = state->tick;
        }

        double dx = (double)spawn->npcWanderTargetX - (double)spawn->x;
        double dy = (double)spawn->npcWanderTargetY - (double)spawn->y;
        double dist = sqrt(dx * dx + dy * dy);
        double step = (double)NPC_WANDER_SPEED_WORLD_UNITS_PER_SEC * (double)AI_TICK_DELTA_MS / 1000.0;

        if (dist <= step || dist <= 0.0)
        {
            
            spawn->npcHasWanderTarget = 0;
            spawn->npcWanderPauseUntilTick =
                state->tick + NPC_WANDER_PAUSE_TICKS_MIN + (unsigned long)(rand() % NPC_WANDER_PAUSE_TICKS_RANGE);
            spawn->isMoving = 0;
            continue;
        }

        
        if (spawn->npcWanderBestDist < 0.0 || dist < spawn->npcWanderBestDist - NPC_WANDER_STUCK_MIN_PROGRESS_WORLD_UNITS)
        {
            spawn->npcWanderBestDist = dist;
            spawn->npcWanderBestDistTick = state->tick;
        }
        else if (state->tick - spawn->npcWanderBestDistTick >= NPC_WANDER_STUCK_CHECK_TICKS)
        {
            spawn->npcHasWanderTarget = 0;
            spawn->isMoving = 0;
            continue;
        }

        long stepX = spawn->x + (long)llround(dx / dist * step);
        long stepY = spawn->y + (long)llround(dy / dist * step);
        long prevX = spawn->x, prevY = spawn->y;
        if (StepSpawnToward(state, spawn, stepX, stepY, i))
        {
            spawn->isMoving = 1;
            
            if (spawn->slideDir != SLIDE_NONE)
                spawn->facingDirection =
                    ResolveFacingDirection(spawn->x - prevX, spawn->y - prevY, spawn->facingDirection);
            else
                spawn->facingDirection =
                    ResolveFacingDirection((long)llround(dx * 1000.0), (long)llround(dy * 1000.0), spawn->facingDirection);

            
            if (spawn->x < spawn->npcWanderL || spawn->x > spawn->npcWanderR || spawn->y < spawn->npcWanderT ||
                spawn->y > spawn->npcWanderB)
            {
                if (spawn->x < spawn->npcWanderL) spawn->x = spawn->npcWanderL;
                if (spawn->x > spawn->npcWanderR) spawn->x = spawn->npcWanderR;
                if (spawn->y < spawn->npcWanderT) spawn->y = spawn->npcWanderT;
                if (spawn->y > spawn->npcWanderB) spawn->y = spawn->npcWanderB;
                spawn->npcHasWanderTarget = 0;
            }
        }
        else
        {
            spawn->isMoving = 0;
        }
    }
}


void MovePlayer(DemoState *state, const Uint8 *keys)
{
    
    if (state->playerIsDead || IsPlayerBusyAttacking(state))
        return;

    long dx = 0, dy = 0;
    if (keys[SDL_SCANCODE_LEFT])
        dx -= CAMERA_SPEED;
    if (keys[SDL_SCANCODE_RIGHT])
        dx += CAMERA_SPEED;
    if (keys[SDL_SCANCODE_UP])
        dy -= CAMERA_SPEED;
    if (keys[SDL_SCANCODE_DOWN])
        dy += CAMERA_SPEED;
    if (dx == 0 && dy == 0)
        return;

    long screenX, screenY;
    WorldToScreen(state, state->playerX, state->playerY, &screenX, &screenY);
    long newWorldX, newWorldY;
    ScreenToWorld(state, screenX + dx, screenY + dy, &newWorldX, &newWorldY);

    
    if (state->noclip)
    {
        state->playerX = newWorldX;
        state->playerY = newWorldY;
        return;
    }

    long clampedX, clampedY;
    RKC_RPGSCRN_GROUND_SweepMove(&state->ground, state->playerX, state->playerY, newWorldX, newWorldY, &clampedX,
                                 &clampedY);
    
    const long playerFootprint = PLAYER_FOOTPRINT_HALF_WIDTH;
    if (!OverlapsAnyCharacterAt(state, clampedX, clampedY, -playerFootprint, -playerFootprint, playerFootprint,
                                playerFootprint, -1, 1))
    {
        state->playerX = clampedX;
        state->playerY = clampedY;
    }
}



static void PushVisited(DemoState *state, long x, long y)
{
    state->visitedX[state->visitedHead] = x;
    state->visitedY[state->visitedHead] = y;
    state->visitedHead = (state->visitedHead + 1) % VISITED_HISTORY_SIZE;
    if (state->visitedCount < VISITED_HISTORY_SIZE)
        state->visitedCount++;
}


static int IsRecentlyVisited(DemoState *state, long x, long y, long speed)
{
    long radius = speed > 1 ? speed - 1 : 0;
    long radiusSq = radius * radius;
    for (int i = 0; i < state->visitedCount; i++)
    {
        long dx = x - state->visitedX[i], dy = y - state->visitedY[i];
        if (dx * dx + dy * dy <= radiusSq)
            return 1;
    }
    return 0;
}

static int StepPlayerToward(DemoState *state, long targetWorldX, long targetWorldY, long speed)
{
    long curX = state->playerX, curY = state->playerY;
    const long footprint = PLAYER_FOOTPRINT_HALF_WIDTH;

    
    const double sceneScaleX = (double)RKC_RPGSCRN_SCENE_SCALE_X / 100.0;
    const double sceneScaleY = (double)RKC_RPGSCRN_SCENE_SCALE_Y / 100.0;
    const long slideSpeed = (long)llround((double)speed / sqrt(sceneScaleX * sceneScaleX + sceneScaleY * sceneScaleY));

    
    state->playerIntendedDX = targetWorldX - curX;
    state->playerIntendedDY = targetWorldY - curY;
    state->playerHasIntendedDir = 1;

    
    long checkWorldX, checkWorldY;
    ScreenToWorld(state, (long)llround(state->playerVirtualScreenX), (long)llround(state->playerVirtualScreenY),
                  &checkWorldX, &checkWorldY);
    if (!state->playerVirtualScreenValid || checkWorldX != curX || checkWorldY != curY)
    {
        long screenXNow, screenYNow;
        WorldToScreen(state, curX, curY, &screenXNow, &screenYNow);
        state->playerVirtualScreenX = (double)screenXNow;
        state->playerVirtualScreenY = (double)screenYNow;
        state->playerVirtualScreenValid = 1;
    }

    long targetScreenX, targetScreenY;
    WorldToScreen(state, targetWorldX, targetWorldY, &targetScreenX, &targetScreenY);
    double dx = (double)targetScreenX - state->playerVirtualScreenX;
    double dy = (double)targetScreenY - state->playerVirtualScreenY;
    double distSq = dx * dx + dy * dy;
    if (distSq <= (double)speed * (double)speed)
    {
        state->slideDir = SLIDE_NONE;
        state->playerVirtualScreenValid = 0;
        return 1; 
    }

    double dist = sqrt(distSq);
    double newVsx = state->playerVirtualScreenX + dx * (double)speed / dist;
    double newVsy = state->playerVirtualScreenY + dy * (double)speed / dist;
    long stepWorldX, stepWorldY;
    ScreenToWorld(state, (long)llround(newVsx), (long)llround(newVsy), &stepWorldX, &stepWorldY);

    
    if (state->noclip)
    {
        state->playerX = stepWorldX;
        state->playerY = stepWorldY;
        state->playerVirtualScreenX = newVsx;
        state->playerVirtualScreenY = newVsy;
        state->slideDir = SLIDE_NONE;
        return 0;
    }

    
    long clampedX, clampedY;
    int unobstructed =
        RKC_RPGSCRN_GROUND_SweepMove(&state->ground, curX, curY, stepWorldX, stepWorldY, &clampedX, &clampedY);
    if (unobstructed &&
        !OverlapsAnyCharacterAt(state, clampedX, clampedY, -footprint, -footprint, footprint, footprint, -1, 1))
    {
        state->playerX = clampedX;
        state->playerY = clampedY;
        
        state->playerVirtualScreenX = newVsx;
        state->playerVirtualScreenY = newVsy;
        state->slideDir = SLIDE_NONE;
        PushVisited(state, clampedX, clampedY);
        return 0;
    }

    
    if (state->slideDir != SLIDE_NONE)
    {
        int overshot = 0;
        if (state->slidePrimary)
        {
            switch (state->slideDir)
            {
            case SLIDE_EAST:
                overshot = (curX - targetWorldX) > slideSpeed;
                break;
            case SLIDE_WEST:
                overshot = (targetWorldX - curX) > slideSpeed;
                break;
            case SLIDE_NORTH:
                overshot = (targetWorldY - curY) > slideSpeed;
                break;
            case SLIDE_SOUTH:
                overshot = (curY - targetWorldY) > slideSpeed;
                break;
            default:
                break;
            }
        }
        if (!overshot)
        {
            long slideX = curX, slideY = curY;
            switch (state->slideDir)
            {
            case SLIDE_NORTH:
                slideY -= slideSpeed;
                break;
            case SLIDE_SOUTH:
                slideY += slideSpeed;
                break;
            case SLIDE_WEST:
                slideX -= slideSpeed;
                break;
            case SLIDE_EAST:
                slideX += slideSpeed;
                break;
            default:
                break;
            }
            RKC_RPGSCRN_GROUND_SweepMove(&state->ground, curX, curY, slideX, slideY, &clampedX, &clampedY);
            if ((clampedX != curX || clampedY != curY) &&
                !OverlapsAnyCharacterAt(state, clampedX, clampedY, -footprint, -footprint, footprint, footprint, -1,
                                        1))
            {
                state->playerX = clampedX;
                state->playerY = clampedY;
                PushVisited(state, clampedX, clampedY);
                return 0;
            }
            
        }
        
    }

    
    long wantX = targetWorldX - curX, wantY = targetWorldY - curY;
    SlideDir wantedXDir = wantX > 0 ? SLIDE_EAST : wantX < 0 ? SLIDE_WEST : SLIDE_NONE;
    SlideDir wantedYDir = wantY > 0 ? SLIDE_SOUTH : wantY < 0 ? SLIDE_NORTH : SLIDE_NONE;

    SlideDir probes[4];
    int numProbes;
    if (wantedXDir != SLIDE_NONE && wantedYDir != SLIDE_NONE)
    {
        probes[0] = wantedXDir;
        probes[1] = wantedYDir;
        probes[2] = (wantedXDir == SLIDE_EAST) ? SLIDE_WEST : SLIDE_EAST;
        probes[3] = (wantedYDir == SLIDE_SOUTH) ? SLIDE_NORTH : SLIDE_SOUTH;
        numProbes = 4;
    }
    else if (wantedXDir != SLIDE_NONE)
    {
        
        probes[0] = SLIDE_SOUTH;
        probes[1] = SLIDE_NORTH;
        numProbes = 2;
    }
    else if (wantedYDir != SLIDE_NONE)
    {
        
        probes[0] = SLIDE_EAST;
        probes[1] = SLIDE_WEST;
        numProbes = 2;
    }
    else
        numProbes = 0; 

    
    SlideDir avoidReverse;
    switch (state->slideDir)
    {
    case SLIDE_EAST:
        avoidReverse = SLIDE_WEST;
        break;
    case SLIDE_WEST:
        avoidReverse = SLIDE_EAST;
        break;
    case SLIDE_NORTH:
        avoidReverse = SLIDE_SOUTH;
        break;
    case SLIDE_SOUTH:
        avoidReverse = SLIDE_NORTH;
        break;
    default:
        avoidReverse = SLIDE_NONE;
        break;
    }

    
    for (int pass = 0; pass < 3; pass++)
    {
        for (int i = 0; i < numProbes; i++)
        {
            long probeX = curX, probeY = curY;
            switch (probes[i])
            {
            case SLIDE_NORTH:
                probeY -= slideSpeed;
                break;
            case SLIDE_SOUTH:
                probeY += slideSpeed;
                break;
            case SLIDE_WEST:
                probeX -= slideSpeed;
                break;
            case SLIDE_EAST:
                probeX += slideSpeed;
                break;
            default:
                break;
            }

            int isReverse = probes[i] == avoidReverse;
            if (pass < 2 && isReverse)
                continue;
            if (pass < 1 && IsRecentlyVisited(state, probeX, probeY, slideSpeed))
                continue;

            if (!RKC_RPGSCRN_GROUND_IsBlocked(&state->ground, probeX, probeY) &&
                !OverlapsAnyCharacterAt(state, probeX, probeY, -footprint, -footprint, footprint, footprint, -1, 1))
            {
                state->slideDir = probes[i];
                state->slideFallback = (probes[i] == SLIDE_WEST || probes[i] == SLIDE_EAST) ? wantedYDir : wantedXDir;
                state->slidePrimary = (numProbes == 4 && i < 2) ? 1 : 0;
                return 0; 
            }
        }
    }

    
    state->slideDir = SLIDE_NONE;
    return 1;
}


void HandleClick(DemoState *state, long mouseWindowX, long mouseWindowY, int isFreshPress)
{
    if (state->playerIsDead || IsPlayerBusyAttacking(state))
        return;

    long clickScreenX = mouseWindowX + state->cameraX;
    long clickScreenY = mouseWindowY + state->cameraY;

    long spawnIdx = FindSpawnNearScreenPoint(state, clickScreenX, clickScreenY);
    
    long itemIdx = spawnIdx < 0 ? FindWorldItemNearScreenPoint(state, clickScreenX, clickScreenY) : -1;
    if (isFreshPress)
        state->heldClickIsSpawnTarget = spawnIdx >= 0 || itemIdx >= 0;

    
    if (itemIdx >= 0 && isFreshPress)
    {
        const WorldItem *item = &state->worldItems[itemIdx];
        state->pendingActionKind = PENDING_ACTION_PICKUP;
        state->pendingActionWorldItemIndex = itemIdx;
        state->hasMoveTarget = 1;
        state->moveTargetX = item->x;
        state->moveTargetY = item->y;
        state->pathfindCheckValid = 0; 
        state->pathfindActive = 0;
        return;
    }

    if (spawnIdx >= 0 && isFreshPress)
    {
        const LiveSpawn *spawn = &state->liveSpawns[spawnIdx];
        
        state->pendingActionKind = spawn->block == 3 ? PENDING_ACTION_ATTACK : PENDING_ACTION_TALK;
        state->pendingActionCharacterNo = spawn->characterNo;
        state->hasMoveTarget = 1;
        state->moveTargetX = spawn->x;
        state->moveTargetY = spawn->y;
        state->pathfindCheckValid = 0;
        state->pathfindActive = 0;
        return;
    }

    if (!isFreshPress && state->heldClickIsSpawnTarget)
        return;

    long clickWorldX, clickWorldY;
    ScreenToWorld(state, clickScreenX, clickScreenY, &clickWorldX, &clickWorldY);
    state->pendingActionKind = PENDING_ACTION_NONE;
    state->hasMoveTarget = 1;
    state->moveTargetX = clickWorldX;
    state->moveTargetY = clickWorldY;
    state->pathfindCheckValid = 0;
    state->pathfindActive = 0;
}


static void ResetPathfindCheckpoint(DemoState *state, long targetX, long targetY)
{
    double dx = (double)(targetX - state->playerX), dy = (double)(targetY - state->playerY);
    state->pathfindCheckValid = 1;
    state->pathfindBestDist = sqrt(dx * dx + dy * dy);
    state->pathfindBestDistTick = (long)state->tick;
}


static void TickPathfindStuckDetection(DemoState *state, long targetX, long targetY)
{
    if (state->pathfindActive)
        return;

    double dx = (double)(targetX - state->playerX), dy = (double)(targetY - state->playerY);
    double currentDist = sqrt(dx * dx + dy * dy);

    if (!state->pathfindCheckValid || currentDist < state->pathfindBestDist - PATHFIND_STUCK_MIN_PROGRESS_WORLD_UNITS)
    {
        ResetPathfindCheckpoint(state, targetX, targetY);
        return;
    }

    if ((long)state->tick - state->pathfindBestDistTick < PATHFIND_STUCK_CHECK_TICKS)
        return;

    if (GroundPathfind_FindPath(&state->ground, state->playerX, state->playerY, targetX, targetY,
                                state->pathWaypointsX, state->pathWaypointsY, PATHFIND_MAX_WAYPOINTS,
                                &state->pathWaypointCount))
    {
        state->pathfindActive = 1;
        state->pathWaypointIndex = 0;
        state->pathfindTargetX = targetX;
        state->pathfindTargetY = targetY;
        return;
    }

    state->pathfindBestDistTick = (long)state->tick;
}


void TickPendingAction(DemoState *state, const Uint8 *keys)
{
    
    int wasBusy = IsPlayerBusyAttacking(state);

    if (state->playerAttackCooldownTicks > 0)
        state->playerAttackCooldownTicks--;

    
    if (state->playerCasting && state->tick >= state->playerCastEndTick)
        state->playerCasting = 0;

    if (wasBusy)
        return;

    if (!state->hasMoveTarget)
        return;
    if (state->playerIsDead)
    {
        state->hasMoveTarget = 0;
        state->pendingActionKind = PENDING_ACTION_NONE;
        return;
    }

    long targetX = state->moveTargetX, targetY = state->moveTargetY;
    LiveSpawn *target = NULL;
    WorldItem *pickupTarget = NULL; 
    if (state->pendingActionKind == PENDING_ACTION_PICKUP)
    {
        long idx = state->pendingActionWorldItemIndex;
        if (idx < 0 || idx >= state->worldItemCount || state->worldItems[idx].pickedUp)
        {
            state->hasMoveTarget = 0;
            state->pendingActionKind = PENDING_ACTION_NONE;
            return;
        }
        pickupTarget = &state->worldItems[idx];
        targetX = pickupTarget->x;
        targetY = pickupTarget->y;
    }
    else if (state->pendingActionKind != PENDING_ACTION_NONE)
    {
        for (long i = 0; i < state->liveSpawnCount; i++)
        {
            if (state->liveSpawns[i].characterNo == state->pendingActionCharacterNo)
            {
                target = &state->liveSpawns[i];
                break;
            }
        }
        if (!target || (target->block == 3 && target->aiState.isDead))
        {
            state->hasMoveTarget = 0;
            state->pendingActionKind = PENDING_ACTION_NONE;
            return;
        }
        targetX = target->x;
        targetY = target->y;
    }

    if (target || pickupTarget)
    {
        long realTargetX = target ? target->x : pickupTarget->x;
        long realTargetY = target ? target->y : pickupTarget->y;
        
        long targetScreenX, targetScreenY, playerScreenX, playerScreenY;
        WorldToScreen(state, realTargetX, realTargetY, &targetScreenX, &targetScreenY);
        WorldToScreen(state, state->playerX, state->playerY, &playerScreenX, &playerScreenY);
        long rdx = targetScreenX - playerScreenX, rdy = targetScreenY - playerScreenY;
        long rangeRadius = (state->pendingActionKind == PENDING_ACTION_ATTACK  ? ATTACK_RADIUS_TILES
                            : state->pendingActionKind == PENDING_ACTION_PICKUP ? ITEM_PICKUP_RADIUS_TILES
                                                                                : INTERACT_RADIUS_TILES) *
                           state->ground.chipHeight;
        int inRange = rdx * rdx + rdy * rdy <= rangeRadius * rangeRadius;
        
        if (inRange && state->pendingActionKind == PENDING_ACTION_TALK && target && target->isCCheckTarget)
            inRange = RectGapDistance(target->x, target->y, target->rectL, target->rectT, target->rectR, target->rectB,
                                      state->playerX, state->playerY, -PLAYER_FOOTPRINT_HALF_WIDTH,
                                      -PLAYER_FOOTPRINT_HALF_WIDTH, PLAYER_FOOTPRINT_HALF_WIDTH,
                                      PLAYER_FOOTPRINT_HALF_WIDTH) <= CCHECK_RANGE_WORLD_UNITS;
        if (inRange)
        {
            
            state->playerIntendedDX = realTargetX - state->playerX;
            state->playerIntendedDY = realTargetY - state->playerY;
            state->playerHasIntendedDir = 1;

            if (state->pendingActionKind == PENDING_ACTION_TALK)
            {
                ApplyPlayerInteract(state, target);
                state->hasMoveTarget = 0;
                state->pendingActionKind = PENDING_ACTION_NONE;
            }
            else if (state->pendingActionKind == PENDING_ACTION_PICKUP)
            {
                ApplyPlayerPickup(state, state->pendingActionWorldItemIndex);
                state->hasMoveTarget = 0;
                state->pendingActionKind = PENDING_ACTION_NONE;
            }
            else if (state->playerAttackCooldownTicks == 0)
            {
                
                state->hasMoveTarget = 0;
                state->pendingActionKind = PENDING_ACTION_NONE;
                ArmPlayerSingleSwing(state, target);
            }
            return;
        }
    }

    
    int run = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL] || state->runToggled;
    
    long speed = ComputePlayerMoveSpeed(state, run);

    TickPathfindStuckDetection(state, targetX, targetY);

    
    if (state->pathfindActive)
    {
        double driftDx = (double)(targetX - state->pathfindTargetX);
        double driftDy = (double)(targetY - state->pathfindTargetY);
        if (driftDx * driftDx + driftDy * driftDy >
            (double)PATHFIND_TARGET_DRIFT_TOLERANCE_WORLD_UNITS * (double)PATHFIND_TARGET_DRIFT_TOLERANCE_WORLD_UNITS)
        {
            state->pathfindActive = 0;
            ResetPathfindCheckpoint(state, targetX, targetY);
        }
    }

    long stepTargetX = targetX, stepTargetY = targetY;
    if (state->pathfindActive)
    {
        stepTargetX = state->pathWaypointsX[state->pathWaypointIndex];
        stepTargetY = state->pathWaypointsY[state->pathWaypointIndex];
    }

    int done = StepPlayerToward(state, stepTargetX, stepTargetY, speed);

    if (state->pathfindActive)
    {
        
        if (done)
        {
            state->pathWaypointIndex++;
            if (state->pathWaypointIndex >= state->pathWaypointCount)
            {
                state->pathfindActive = 0;
                if (!target && !pickupTarget)
                {
                    
                    state->hasMoveTarget = 0;
                    state->pendingActionKind = PENDING_ACTION_NONE;
                }
                else
                {
                    
                    ResetPathfindCheckpoint(state, targetX, targetY);
                }
            }
            else
            {
                ResetPathfindCheckpoint(state, targetX, targetY);
            }
        }
        return;
    }

    if (done && !target && !pickupTarget)
    {
        
        state->hasMoveTarget = 0;
        state->pendingActionKind = PENDING_ACTION_NONE;
    }
}
