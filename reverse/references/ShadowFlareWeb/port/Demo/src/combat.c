#include "combat.h"
#include "render.h"
#include "inventory.h"
#include "movement.h"
#include "script_bridge.h"


static void PlayCombatSound(DemoState *state, long track)
{
    if (track < 0)
        return;
#ifdef GROUNDDEMO_AUDIO
    if (state->sfxLoaded)
        RKC_DSOUND_Play(&state->dsound, 1 , track, 0 , 0 ,
                        0 );
#endif
}


static int TryRollHitReaction(const DemoState *state, long damage, long defenderMaxHP, long *outDurationTicks)
{
    if (defenderMaxHP <= 0)
        return 0;
    long row = damage * 50 / defenderMaxHP;
    if (row > 49)
        row = 49;
    const RKC_RPG_TABLEDATA *table19 = RKC_RPG_TABLE_GetFromTableNo(&state->table, 0x19);
    if (!table19)
        return 0;
    long chancePercent = RKC_RPG_TABLEDATA_GetValue(table19, row, 0) + HIT_REACTION_UNRESOLVED_CHANCE_BONUS_PERCENT;
    if (chancePercent > 100)
        chancePercent = 100;
    if ((rand() % 100) >= chancePercent)
        return 0;

    long units = RKC_RPG_TABLEDATA_GetValue(table19, row, 1) + HIT_REACTION_UNRESOLVED_DURATION_BONUS_UNITS;
    long ticks = units * HIT_REACTION_TICKS_PER_TABLE_UNIT;
    if (ticks < HIT_REACTION_MIN_TICKS)
        ticks = HIT_REACTION_MIN_TICKS;
    if (ticks > HIT_REACTION_MAX_TICKS)
        ticks = HIT_REACTION_MAX_TICKS;
    *outDurationTicks = ticks;
    return 1;
}


static void ApplyHitReactionWobble(DemoState *state, long *x, long *y, long attackerX, long attackerY)
{
    double dx = (double)(*x - attackerX);
    double dy = (double)(*y - attackerY);
    double dist = sqrt(dx * dx + dy * dy);
    if (dist < 1.0)
        return;
    long wantX = *x + (long)(dx / dist * HIT_REACTION_WOBBLE_DISTANCE_WORLD_UNITS);
    long wantY = *y + (long)(dy / dist * HIT_REACTION_WOBBLE_DISTANCE_WORLD_UNITS);
    long clampedX, clampedY;
    RKC_RPGSCRN_GROUND_SweepMove(&state->ground, *x, *y, wantX, wantY, &clampedX, &clampedY);
    *x = clampedX;
    *y = clampedY;
}


static const double FACING_DIRECTION_DX[8] = {0.70710678, 1.0, 0.70710678, 0.0,
                                              -0.70710678, -1.0, -0.70710678, 0.0};
static const double FACING_DIRECTION_DY[8] = {0.70710678, 0.0, -0.70710678, -1.0,
                                              -0.70710678, 0.0, 0.70710678, 1.0};


void FacingDirectionToDelta(int direction, double *outDX, double *outDY)
{
    if (direction < 0 || direction > 7)
        direction = 1; 
    *outDX = FACING_DIRECTION_DX[direction];
    *outDY = FACING_DIRECTION_DY[direction];
}


static void ApplyComboLunge(DemoState *state, int phaseIndex, long ticksIntoPhase, long ticksToHit)
{
    if (state->playerComboLungePhase != phaseIndex)
    {
        state->playerComboLungeAnchorX = state->playerX;
        state->playerComboLungeAnchorY = state->playerY;
        state->playerComboLungePhase = phaseIndex;
    }

    double progress = (double)(ticksIntoPhase + 1) / (double)ticksToHit;
    double dist = (double)PLAYER_COMBO_LUNGE_PER_PHASE_WORLD_UNITS * progress;
    double dirDX, dirDY;
    FacingDirectionToDelta(state->playerFacingDirection, &dirDX, &dirDY);
    long wantX = state->playerComboLungeAnchorX + (long)(dirDX * dist);
    long wantY = state->playerComboLungeAnchorY + (long)(dirDY * dist);
    long clampedX, clampedY;
    RKC_RPGSCRN_GROUND_SweepMove(&state->ground, state->playerX, state->playerY, wantX, wantY, &clampedX, &clampedY);
    
    if (!PlayerBlockedByCharacterAt(state, clampedX, clampedY))
    {
        state->playerX = clampedX;
        state->playerY = clampedY;
    }
}


static void ArmPlayerShake(DemoState *state, long attackerX, long attackerY)
{
    double dx = (double)(state->playerX - attackerX);
    double dy = (double)(state->playerY - attackerY);
    double dist = sqrt(dx * dx + dy * dy);
    if (dist < 1.0)
        return;
    state->playerShakeDirX = dx / dist;
    state->playerShakeDirY = dy / dist;
    state->playerShakeTick = state->tick;
}


static long FindNearestLiveEnemy(DemoState *state)
{
    long centerX, centerY;
    WorldToScreen(state, state->playerX, state->playerY, &centerX, &centerY);
    long radius = ATTACK_RADIUS_TILES * state->ground.chipHeight;

    long best = -1;
    long bestDistSq = radius * radius;
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        const LiveSpawn *spawn = &state->liveSpawns[i];
        if (spawn->block != 3 || spawn->aiState.isDead)
            continue;
        long sx, sy;
        WorldToScreen(state, spawn->x, spawn->y, &sx, &sy);
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


static long ComputeRealPlayerHitDamage(const DemoState *state, long attackerPower);


long ResolvePlayerAttackChart(const DemoState *state)
{
    if (!state->hasWeapon)
        return PLAYER_ATTACK_CHART_UNARMED;
    switch (state->weapon.tail.weaponClass)
    {
    case 0:
        return PLAYER_ATTACK_CHART_ONE_HANDED;
    case 1:
        return PLAYER_ATTACK_CHART_AXE_BLUNT;
    case 3:
        return PLAYER_ATTACK_CHART_TWO_HANDED;
    case 4:
    case 5:
        return PLAYER_ATTACK_CHART_RANGED;
    default:
        return PLAYER_ATTACK_CHART_UNARMED;
    }
}


long ResolveWeaponCategoryChart(const DemoState *state, long defaultChart, long axeBluntChart, long twoHandedChart)
{
    if (!state->hasWeapon)
        return defaultChart;
    if (state->weapon.tail.weaponClass == 1)
        return axeBluntChart;
    if (state->weapon.tail.weaponClass == 3)
        return twoHandedChart;
    return defaultChart;
}


int ResolvePlayerAttackSequence(const DemoState *state, long *chart1, long *chart2)
{
    if (!state->hasWeapon)
        return 0;
    if (state->weapon.tail.weaponClass == 0)
    {
        *chart1 = PLAYER_ATTACK_CHART_ONE_HANDED_SWING1;
        *chart2 = PLAYER_ATTACK_CHART_ONE_HANDED_SWING2;
        return 1;
    }
    if (state->weapon.tail.weaponClass == 1)
    {
        *chart1 = PLAYER_ATTACK_CHART_AXE_SWING1;
        *chart2 = PLAYER_ATTACK_CHART_AXE_SWING2;
        return 1;
    }
    if (state->weapon.tail.weaponClass == 3)
    {
        *chart1 = PLAYER_ATTACK_CHART_TWO_HANDED_SWING1;
        *chart2 = PLAYER_ATTACK_CHART_TWO_HANDED_SWING2;
        return 1;
    }
    return 0;
}


int ResolvePlayerChartDrawDirection(long chart, int liveFacingDirection)
{
    if (chart == PLAYER_ATTACK_CHART_AXE_SWING_LOOP)
        return PLAYER_ATTACK_NONDIRECTIONAL_DIRECTION;
    return liveFacingDirection;
}


int ResolvePlayerComboSequence(const DemoState *state, long charts[5], int dealsDamage[5])
{
    if (!state->hasWeapon)
        return 0;
    if (state->weapon.tail.weaponClass == 0)
    {
        charts[0] = PLAYER_ATTACK_CHART_ONE_HANDED_SWING1;
        charts[1] = PLAYER_ATTACK_CHART_ONE_HANDED_COMBO2;
        charts[2] = PLAYER_ATTACK_CHART_ONE_HANDED; 
        dealsDamage[0] = dealsDamage[1] = dealsDamage[2] = 1;
        return 3;
    }
    if (state->weapon.tail.weaponClass == 1)
    {
        
        int i = 0;
        charts[i] = PLAYER_ATTACK_CHART_AXE_COMBO_WINDUP;
        dealsDamage[i] = 0;
        i++;
        for (int r = 0; r < PLAYER_AXE_SPIN_LOOP_REPEAT_COUNT; r++, i++)
        {
            charts[i] = PLAYER_ATTACK_CHART_AXE_SWING_LOOP;
            dealsDamage[i] = 1;
        }
        charts[i] = PLAYER_ATTACK_CHART_AXE_COMBO_RECOVERY;
        dealsDamage[i] = 0;
        i++;
        return i;
    }
    if (state->weapon.tail.weaponClass == 3)
    {
        charts[0] = PLAYER_ATTACK_CHART_TWO_HANDED_SWING1; 
        charts[1] = PLAYER_ATTACK_CHART_TWO_HANDED_COMBO2;
        charts[2] = PLAYER_ATTACK_CHART_TWO_HANDED_COMBO3;
        dealsDamage[0] = dealsDamage[1] = dealsDamage[2] = 1;
        return 3;
    }
    return 0;
}


int ResolvePlayerSwingPhases(const DemoState *state, int wantCombo, long charts[5], short frames[5],
                              int dealsDamage[5])
{
    int direction = state->playerFacingDirection;
    if (direction < 0 || direction >= RKC_RPGSCRN_CAF_NUM_DIRECTIONS)
        direction = 0;

    int count;
    if (wantCombo)
    {
        count = ResolvePlayerComboSequence(state, charts, dealsDamage);
        if (count <= 0)
            return 0;
    }
    else
    {
        long c1, c2;
        if (!ResolvePlayerAttackSequence(state, &c1, &c2))
            return 0;
        charts[0] = c1;
        charts[1] = c2;
        dealsDamage[0] = dealsDamage[1] = 1; 
        count = 2;
    }

    for (int i = 0; i < count; i++)
    {
        if (charts[i] < 0 || charts[i] >= state->playerTemplate.caf.chartCount)
            return 0;
        int dir = ResolvePlayerChartDrawDirection(charts[i], direction);
        frames[i] = state->playerTemplate.caf.charts[charts[i]].directions[dir].maxFrameCount;
        if (frames[i] <= 0)
            return 0;
    }
    return count;
}


long ComputeRealPlayerAttackDurationTicks(const DemoState *state, int wantCombo)
{
    
    long charts[5];
    short frames[5];
    int dealsDamage[5]; 
    int count = ResolvePlayerSwingPhases(state, wantCombo, charts, frames, dealsDamage);
    double ticksPerFrame = ComputePlayerAttackAnimTicksPerFrame(state);
    if (count > 0)
    {
        
        long total = 0;
        for (int i = 0; i < count; i++)
            total += (long)((double)frames[i] * ticksPerFrame);
        return total > 0 ? total : PLAYER_ATTACK_COOLDOWN_TICKS;
    }

    int direction = state->playerFacingDirection;
    if (direction < 0 || direction >= RKC_RPGSCRN_CAF_NUM_DIRECTIONS)
        direction = 0;

    long chart = ResolvePlayerAttackChart(state);
    if (chart < 0 || chart >= state->playerTemplate.caf.chartCount)
        return PLAYER_ATTACK_COOLDOWN_TICKS;

    short frameCount = state->playerTemplate.caf.charts[chart].directions[direction].maxFrameCount;
    long duration = (long)((double)frameCount * ticksPerFrame);
    return duration > 0 ? duration : PLAYER_ATTACK_COOLDOWN_TICKS;
}


int ApplyPlayerAttack(DemoState *state, LiveSpawn *spawn)
{
    const char *label = spawn->name ? spawn->name : "(unnamed enemy)";

    
    long baseAttack = state->progressionInitialized ? state->playerBaseStats[PLAYER_STAT_ATTACK] : ATTACK_DAMAGE;
    long attackerPower = baseAttack + (state->hasWeapon ? state->weapon.tail.rollTable1[0].value : 0);
    long damage = ComputeRealPlayerHitDamage(state, attackerPower);
    int killed = RKC_RPG_AI_EXEC_ApplyDamage(&spawn->aiState, damage);
    
    
    spawn->hitVfxTick = state->tick;
    spawn->hitVfxVariant = rand() % HIT_VFX_VARIANT_COUNT;
    
    SpawnBloodDecal(state, spawn->x, spawn->y);
    
    
    PlayCombatSound(state, COMBAT_HIT_SOUND_TRACK);
    if (killed)
    {
        
        spawn->deathTick = state->tick; 
        if (spawn->field8 >= 0 && spawn->field8 < 25)
            PlayCombatSound(state, ENEMY_DEATH_SOUND[spawn->field8]);
        
        AwardPlayerKillExp(state, spawn);
        
        if (spawn->lootTableRow >= 0)
        {
            MessagePrintContext dropCtx = {label, state, spawn->characterNo};
            PrintCreateItemTable(&dropCtx, spawn->lootTableRow, spawn->x, spawn->y);
        }
        DropEnemyGold(state, spawn);
        if (!RunTriggersForCharacter(state, spawn->characterNo, label))
            printf("%s was defeated\n", label);
    }
    else
    {
        
        long durationTicks;
        if (TryRollHitReaction(state, damage, spawn->aiState.maxHP, &durationTicks))
        {
            spawn->hitStunTick = state->tick;
            spawn->hitStunDurationTicks = durationTicks;
            ApplyHitReactionWobble(state, &spawn->x, &spawn->y, state->playerX, state->playerY);
            spawn->hasPendingHit = 0;
            if (spawn->attackCooldownTicks < durationTicks)
                spawn->attackCooldownTicks = durationTicks;
        }
    }
    return killed;
}


void ArmPlayerSingleSwing(DemoState *state, const LiveSpawn *target)
{
    state->playerSwingIsCombo = 0; 
    state->playerAttacking = 1;
    state->playerAttackTick = state->tick; 
    long duration = ComputeRealPlayerAttackDurationTicks(state, 0);
    state->playerAttackCooldownTicks = duration;
    
    state->playerHasPendingHit = 1;
    state->playerPendingHitCharacterNo = target->characterNo;
    state->playerPendingHitResolveTick = state->tick + (unsigned long)(duration * HIT_CONNECT_FRACTION_PERCENT / 100);
}


void HandleAttack(DemoState *state)
{
    if (state->playerIsDead || state->playerAttackCooldownTicks > 0)
        return;

    long spawnIdx = FindNearestLiveEnemy(state);
    if (spawnIdx < 0)
    {
        
        return;
    }
    
    const LiveSpawn *target = &state->liveSpawns[spawnIdx];
    state->playerFacingDirection =
        ResolveFacingDirection(target->x - state->playerX, target->y - state->playerY, state->playerFacingDirection);
    ArmPlayerSingleSwing(state, target);
}


void TickPlayerPendingHit(DemoState *state)
{
    if (!state->playerHasPendingHit || state->tick < state->playerPendingHitResolveTick)
        return;
    state->playerHasPendingHit = 0;

    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        LiveSpawn *target = &state->liveSpawns[i];
        if (target->characterNo != state->playerPendingHitCharacterNo)
            continue;
        if (target->block == 3 && target->aiState.isDead)
            return; 

        
        long targetScreenX, targetScreenY, playerScreenX, playerScreenY;
        WorldToScreen(state, target->x, target->y, &targetScreenX, &targetScreenY);
        WorldToScreen(state, state->playerX, state->playerY, &playerScreenX, &playerScreenY);
        long rdx = targetScreenX - playerScreenX, rdy = targetScreenY - playerScreenY;
        long rangeRadius = ATTACK_RADIUS_TILES * state->ground.chipHeight;
        if (rdx * rdx + rdy * rdy > rangeRadius * rangeRadius)
            return; 

        ApplyPlayerAttack(state, target);
        return;
    }
    
}


void HandleComboAttack(DemoState *state, long mouseWindowX, long mouseWindowY)
{
    if (state->playerIsDead || state->playerAttackCooldownTicks > 0)
        return;

    state->hasMoveTarget = 0;
    state->pendingActionKind = PENDING_ACTION_NONE;
    state->playerAttacking = 1;
    state->playerAttackTick = state->tick;
    state->playerSwingIsCombo = 1;
    state->playerComboHitsApplied = 0;
    state->playerComboLungePhase = -1; 
    state->playerAttackCooldownTicks = ComputeRealPlayerAttackDurationTicks(state, 1);

    
    long clickScreenX = mouseWindowX + state->cameraX;
    long clickScreenY = mouseWindowY + state->cameraY;
    long clickWorldX, clickWorldY;
    ScreenToWorld(state, clickScreenX, clickScreenY, &clickWorldX, &clickWorldY);
    state->playerFacingDirection = ResolveFacingDirection(clickWorldX - state->playerX, clickWorldY - state->playerY,
                                                            state->playerFacingDirection);
}


static void ChooseTransportCirclePos(DemoState *state, long clickScreenX, long clickScreenY, long *outX, long *outY)
{
    long playerScreenX, playerScreenY;
    WorldToScreen(state, state->playerX, state->playerY, &playerScreenX, &playerScreenY);
    double dx = (double)(clickScreenX - playerScreenX);
    double dy = (double)(clickScreenY - playerScreenY);
    double len = sqrt(dx * dx + dy * dy);
    if (len < 1.0) 
    {
        dx = 0.0;
        dy = 1.0;
        len = 1.0;
    }
    double ux = dx / len, uy = dy / len;

    
    static const double PROBE_ANGLES_DEG[] = {0, 45, -45, 90, -90, 135, -135, 180};
    for (int i = 0; i < (int)(sizeof(PROBE_ANGLES_DEG) / sizeof(PROBE_ANGLES_DEG[0])); i++)
    {
        double a = PROBE_ANGLES_DEG[i] * 3.14159265358979323846 / 180.0;
        double rx = ux * cos(a) - uy * sin(a);
        double ry = ux * sin(a) + uy * cos(a);
        long candScreenX = playerScreenX + (long)(rx * TRANSPORT_CIRCLE_FRONT_DISTANCE_PX);
        long candScreenY = playerScreenY + (long)(ry * TRANSPORT_CIRCLE_FRONT_DISTANCE_PX);
        long wx, wy;
        ScreenToWorld(state, candScreenX, candScreenY, &wx, &wy);
        if (!RKC_RPGSCRN_GROUND_IsBlocked(&state->ground, wx, wy))
        {
            *outX = wx;
            *outY = wy;
            return;
        }
    }
    *outX = state->playerX; 
    *outY = state->playerY;
}


void CastTransport(DemoState *state, long mouseWindowX, long mouseWindowY)
{
    if (state->playerIsDead || state->playerAttackCooldownTicks > 0)
        return;

    state->hasMoveTarget = 0;
    state->pendingActionKind = PENDING_ACTION_NONE;

    
    long clickScreenX = mouseWindowX + state->cameraX;
    long clickScreenY = mouseWindowY + state->cameraY;
    long clickWorldX, clickWorldY;
    ScreenToWorld(state, clickScreenX, clickScreenY, &clickWorldX, &clickWorldY);
    state->playerFacingDirection = ResolveFacingDirection(clickWorldX - state->playerX, clickWorldY - state->playerY,
                                                          state->playerFacingDirection);

    
    long windFrames = 0;
    if (state->playerTemplate.kind == LIVE_SPAWN_SPRITE_CAF && PLAYER_CAST_CHART_WINDUP < state->playerTemplate.caf.chartCount)
    {
        int windDir = ResolvePlayerChartDrawDirection(PLAYER_CAST_CHART_WINDUP, state->playerFacingDirection);
        windFrames = state->playerTemplate.caf.charts[PLAYER_CAST_CHART_WINDUP].directions[windDir].maxFrameCount;
    }
    if (windFrames <= 0)
        windFrames = 10; 
    long castDurationTicks = windFrames * PLAYER_CAST_ANIM_TICKS_PER_FRAME + PLAYER_CAST_RELEASE_HOLD_TICKS;

    state->playerCasting = 1;
    state->playerCastTick = state->tick;
    state->playerCastEndTick = state->tick + (unsigned long)castDurationTicks;
    state->playerAttackCooldownTicks = castDurationTicks;

    
    long fieldX, fieldY;
    ChooseTransportCirclePos(state, clickScreenX, clickScreenY, &fieldX, &fieldY);

    
    long townScenarioId, townX, townY;
    TransportDestTown(state->currentScenarioId, &townScenarioId, &townX, &townY);

    state->transportCircle.active = 1;
    state->transportCircle.fieldScenarioId = state->currentScenarioId;
    state->transportCircle.fieldX = fieldX;
    state->transportCircle.fieldY = fieldY;
    state->transportCircle.townScenarioId = townScenarioId;
    state->transportCircle.townX = townX;
    state->transportCircle.townY = townY;
    state->transportCircle.spawnTick = state->tick;
    
    state->transportCircle.playerInsideLastTick = 1;

    
    fflush(stdout);
}


void TickTransportCircle(DemoState *state)
{
    if (!state->transportCircle.active)
        return;

    long cx, cy;
    int isTownEnd;
    if (!ActiveTransportCircleHere(state, &cx, &cy, &isTownEnd))
    {
        
        state->transportCircle.playerInsideLastTick = 0;
        return;
    }

    
    if (state->transitionPending)
        return;

    
    long playerScreenX, playerScreenY, circleScreenX, circleScreenY;
    WorldToScreen(state, state->playerX, state->playerY, &playerScreenX, &playerScreenY);
    WorldToScreen(state, cx, cy, &circleScreenX, &circleScreenY);
    long ddx = playerScreenX - circleScreenX, ddy = playerScreenY - circleScreenY;
    int inside = (ddx * ddx + ddy * ddy) <=
                 (TRANSPORT_CIRCLE_TRIGGER_RADIUS_PX * TRANSPORT_CIRCLE_TRIGGER_RADIUS_PX);

    if (inside && !state->transportCircle.playerInsideLastTick)
    {
        if (isTownEnd)
        {
            
            state->transitionPending = 1;
            state->pendingScenarioId = state->transportCircle.fieldScenarioId;
            state->pendingEntryPoint = -1;
            state->pendingWarpValid = 1;
            state->pendingWarpX = state->transportCircle.fieldX;
            state->pendingWarpY = state->transportCircle.fieldY;
            
            state->transportCircle.active = 0; 
        }
        else
        {
            
            state->transitionPending = 1;
            state->pendingScenarioId = state->transportCircle.townScenarioId;
            state->pendingEntryPoint = -1;
            state->pendingWarpValid = 1;
            state->pendingWarpX = state->transportCircle.townX;
            state->pendingWarpY = state->transportCircle.townY;
            
        }
        fflush(stdout);
        
        state->transportCircle.playerInsideLastTick = 1;
    }
    else
    {
        state->transportCircle.playerInsideLastTick = inside;
    }
}


int IsPlayerBusyAttacking(const DemoState *state)
{
    return state->playerAttackCooldownTicks > 0;
}


static void ApplyPlayerSpinAttack(DemoState *state)
{
    long centerX, centerY;
    WorldToScreen(state, state->playerX, state->playerY, &centerX, &centerY);
    long radius = ATTACK_RADIUS_TILES * state->ground.chipHeight;
    long radiusSq = radius * radius;

    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        LiveSpawn *spawn = &state->liveSpawns[i];
        if (spawn->block != 3 || spawn->aiState.isDead)
            continue;
        long sx, sy;
        WorldToScreen(state, spawn->x, spawn->y, &sx, &sy);
        long dx = sx - centerX, dy = sy - centerY;
        if (dx * dx + dy * dy <= radiusSq)
            ApplyPlayerAttack(state, spawn);
    }
}


void TickPlayerCombo(DemoState *state)
{
    if (!state->playerAttacking || !state->playerSwingIsCombo)
        return;

    long charts[5];
    short frames[5];
    int dealsDamage[5];
    int count = ResolvePlayerSwingPhases(state, 1, charts, frames, dealsDamage);
    if (count <= 0)
    {
        state->playerSwingIsCombo = 0;
        return;
    }

    long elapsedTicks = (long)(state->tick - state->playerAttackTick);
    double ticksPerFrame = ComputePlayerAttackAnimTicksPerFrame(state);
    long phaseStart = 0;
    for (int i = 0; i < count; i++)
    {
        
        long phaseDurationTicks = (long)((double)frames[i] * ticksPerFrame);
        long phaseHitTick = phaseStart + (long)(phaseDurationTicks * HIT_CONNECT_FRACTION_PERCENT / 100);
        
        if (state->playerComboHitsApplied <= i && elapsedTicks >= phaseStart && elapsedTicks < phaseHitTick)
            ApplyComboLunge(state, i, elapsedTicks - phaseStart, phaseHitTick - phaseStart);
        if (state->playerComboHitsApplied <= i && elapsedTicks >= phaseHitTick)
        {
            
            if (dealsDamage[i])
            {
                if (charts[i] == PLAYER_ATTACK_CHART_AXE_SWING_LOOP)
                    ApplyPlayerSpinAttack(state);
                else
                {
                    long spawnIdx = FindNearestLiveEnemy(state);
                    if (spawnIdx >= 0)
                        ApplyPlayerAttack(state, &state->liveSpawns[spawnIdx]);
                }
            }
            
            state->playerComboHitsApplied = i + 1;
        }
        phaseStart += phaseDurationTicks;
    }
}


static int TryConsumeReviveItem(DemoState *state)
{
    for (int s = 0; s < state->inventoryCount; s++)
    {
        if (state->inventory[s].count < 1 || strcmp(state->inventory[s].name, "Elixir") != 0)
            continue;
        state->inventory[s].count--;
        return 1;
    }
    return 0;
}


static void AccumulateRollTable1(FinalCombatStats *out, const RKC_RPG_ITEMDATA_RollEntry *rollTable1)
{
    long v;
    if ((v = rollTable1[0].value) != 0)
        out->combatStat[0] += v;
    if ((v = rollTable1[1].value) != 0)
        out->combatStat[1] += v;
    if ((v = rollTable1[2].value) != 0)
        out->combatStat[2] += v;
    if ((v = rollTable1[3].value) != 0)
        out->combatStat[3] += v;
    if ((v = rollTable1[4].value) != 0)
        out->combatStat[4] += v;
    if ((v = rollTable1[5].value) != 0)
        out->combatStat[5] += v;
    if ((v = rollTable1[6].value) != 0)
        out->combatStat[6] += v;
    if ((v = rollTable1[7].value) != 0)
        out->combatStat[7] += v;
    if ((v = rollTable1[8].value) != 0)
        out->attackSpeedBonus += v;
    if ((v = rollTable1[9].value) != 0)
        out->moveSpeedBonus += v;
    if ((v = rollTable1[10].value) != 0)
        out->maxHPBonus += v;
    if ((v = rollTable1[11].value) != 0)
        out->maxShieldBonus += v;
    if ((v = rollTable1[12].value) != 0)
        out->field10 += v;
    if ((v = rollTable1[13].value) != 0)
        out->field56 += v;
    if ((v = rollTable1[27].value) != 0)
        out->field55 += v;
    if ((v = rollTable1[28].value) != 0)
        out->field54 += v;
    if ((v = rollTable1[29].value) != 0)
        out->field57 += v;
}

void ComputeFinalCombatStats(const DemoState *state, FinalCombatStats *out)
{
    memset(out, 0, sizeof(*out));
    if (state->hasWeapon && state->weapon.durability > 0)
        AccumulateRollTable1(out, state->weapon.tail.rollTable1);
    
    int shieldInert = IsShieldIneffective(state);
    for (int s = 0; s < EQUIPMENT_ARMOR_SLOTS; s++)
        if (state->hasArmor[s] && state->armor[s].durability > 0 && !(s == EQUIPMENT_SHIELD_SLOT_INDEX && shieldInert))
            AccumulateRollTable1(out, state->armor[s].tail.rollTable1);
    for (int s = 0; s < EQUIPMENT_ACCESSORY_SLOTS; s++)
        if (state->hasAccessory[s])
            AccumulateRollTable1(out, state->accessory[s].tail.rollTable1);
}


double ComputePlayerAttackAnimTicksPerFrame(const DemoState *state)
{
    FinalCombatStats stats;
    ComputeFinalCombatStats(state, &stats);
    double finalAttackSpeed = PLAYER_ATTACK_SPEED_BASE + (double)stats.attackSpeedBonus;
    double multiplier = finalAttackSpeed > 0 ? finalAttackSpeed / PLAYER_ATTACK_SPEED_BASE : 1.0;
    return PLAYER_ATTACK_ANIM_TICKS_PER_FRAME / multiplier;
}


long ComputePlayerMoveSpeed(const DemoState *state, int running)
{
    FinalCombatStats stats;
    ComputeFinalCombatStats(state, &stats);
    double finalWalkSpeed = PLAYER_WALK_SPEED_BASE + (double)stats.moveSpeedBonus;
    double multiplier = finalWalkSpeed > 0 ? finalWalkSpeed / PLAYER_WALK_SPEED_BASE : 1.0;
    long base = running ? PLAYER_RUN_SPEED : PLAYER_WALK_SPEED;
    long scaled = (long)((double)base * multiplier);
    return scaled > 0 ? scaled : base;
}



static int PlayerClassCompactIndex(long cls)
{
    switch (cls)
    {
    case PLAYER_CLASS_MERCENARY:
        return 0;
    case PLAYER_CLASS_WARRIOR:
        return 1;
    case PLAYER_CLASS_WITCH:
        return 2;
    case PLAYER_CLASS_HUNTER:
        return 3;
    default:
        return 0;
    }
}


static const RKC_RPG_TABLEDATA *PlayerGrowthTable(const DemoState *state, long cls)
{
    return RKC_RPG_TABLE_GetFromTableNo(&state->table, TABLE_PLAYER_GROWTH_BASE + PlayerClassCompactIndex(cls) * 2);
}


void InitPlayerProgression(DemoState *state)
{
    const RKC_RPG_TABLEDATA *base = RKC_RPG_TABLE_GetFromTableNo(&state->table, TABLE_PLAYER_BASE_STATS);
    state->playerClass = PLAYER_CLASS_MERCENARY;
    state->playerChangeToClass = PLAYER_CLASS_MERCENARY; 
    memset(state->classKillUsage, 0, sizeof(state->classKillUsage));
    state->classKillTotal = 0;
    state->playerLevel = 1;
    memset(state->playerClassHistory, 0, sizeof(state->playerClassHistory));
    state->playerClassHistory[0] = PLAYER_CLASS_MERCENARY; 
    memset(state->playerBaseStats, 0, sizeof(state->playerBaseStats));
    if (base)
    {
        for (int r = 0; r < PLAYER_STAT_COUNT; r++)
            state->playerBaseStats[r] = RKC_RPG_TABLEDATA_GetValue(base, r, 0);
    }
    else
    {
        
        state->playerBaseStats[PLAYER_STAT_MAX_HP] = PLAYER_DEFAULT_MAX_HP;
    }
    state->progressionInitialized = 1;

    
    for (int i = 0; i < MAGIC_BAR_SLOT_COUNT; i++)
        state->magicBarSlot[i] = i;
    state->selectedMagicSlot = MAGIC_SLOT_ATTACK_INDEX;

    
}


static const char *const SPELL_NAMES[22] = {
    "Transport",       "Fire Ball",     "Ice Bolt",       "Plasma",
    "Hell Fire",       "Ice Blast",     "Heal",           "Moon",
    "Berserker",       "Energy Shield", "Earth Spear",    "Flame Strike",
    "Dread Deathscythe", "Lightning Storm", "Medusa",     "Sonic Blade",
    "Mud Javelin",     "Identify",      "Magic Shield",   "Counter Burst",
    "Explosion",       "Elemental Strike"};

const char *SpellName(int spellId)
{
    return (spellId >= 0 && spellId < 22) ? SPELL_NAMES[spellId] : "?";
}


void PlayerLevelUp(DemoState *state)
{
    if (!state->progressionInitialized || state->playerLevel >= PLAYER_LEVEL_CAP)
        return;
    long cls = state->playerClass;
    long oldLevel = state->playerLevel;
    state->playerClassHistory[oldLevel] = (unsigned char)cls; 
    state->playerLevel = oldLevel + 1;

    long count = 0;
    for (long i = 0; i < state->playerLevel; i++)
        if (state->playerClassHistory[i] == cls)
            count++;
    long column = count - 1;

    long oldClass = state->playerClass;
    const RKC_RPG_TABLEDATA *gt = PlayerGrowthTable(state, cls);
    for (int r = 0; r < PLAYER_STAT_COUNT; r++)
    {
        long delta = gt ? RKC_RPG_TABLEDATA_GetValue(gt, r, column) : 0;
        state->playerBaseStats[r] += delta;
        state->levelUpNoticeGains[r] = delta; 
    }

    
    if (state->playerLevel == 5 && state->playerClass == PLAYER_CLASS_MERCENARY)
        state->playerClass = PLAYER_CLASS_WARRIOR;

    RecomputePlayerMaxHP(state);
    state->playerHP = state->playerMaxHP; 
    RecomputePlayerMaxMP(state);
    state->playerMP = state->playerMaxMP; 

    
    state->levelUpNoticeActive = 1;
    state->levelUpNoticeArmedTick = state->tick;
    state->levelUpNoticeLevel = state->playerLevel;
    state->levelUpNoticeNewClass = (state->playerClass != oldClass) ? state->playerClass : 0;
    state->levelUpNoticeSkill[0] = '\0';

    EvaluatePlayerClassChange(state); 
}


void EvaluatePlayerClassChange(DemoState *state)
{
    if (!state->progressionInitialized || state->playerLevel < 5)
    {
        state->playerChangeToClass = state->playerClass;
        return;
    }
    long total = state->classKillTotal, meleePct = 0, bowPct = 0, spellPct = 0;
    if (total > 0)
    {
        meleePct = (state->classKillUsage[0] + state->classKillUsage[1] + state->classKillUsage[3]) * 100 / total;
        bowPct = state->classKillUsage[8] * 100 / total; 
        
    }
    long cls = state->playerClass, changeTo = cls;
    if (cls != PLAYER_CLASS_WITCH && spellPct > CLASS_WITCH_USAGE_PCT)
        changeTo = PLAYER_CLASS_WITCH;
    else if (cls != PLAYER_CLASS_HUNTER && bowPct > CLASS_HUNTER_USAGE_PCT)
        changeTo = PLAYER_CLASS_HUNTER;
    else if (cls != PLAYER_CLASS_WARRIOR && meleePct >= CLASS_WARRIOR_USAGE_PCT)
        changeTo = PLAYER_CLASS_WARRIOR;
    state->playerChangeToClass = changeTo;
}


void PlayerChangeClass(DemoState *state)
{
    if (state->playerChangeToClass == 0 || state->playerChangeToClass == state->playerClass)
        return;
    
    state->playerClass = state->playerChangeToClass;
    EvaluatePlayerClassChange(state);
}


void AwardPlayerKillExp(DemoState *state, const LiveSpawn *spawn)
{
    if (!state->progressionInitialized)
        return;
    
    {
        long wc = state->hasWeapon ? state->weapon.tail.weaponClass : 0;
        long idx = (wc == 8 || wc == 9) ? 8 : (wc >= 0 && wc < CLASS_KILL_USAGE_SLOTS ? wc : 8);
        state->classKillUsage[idx]++;
        state->classKillTotal++;
        EvaluatePlayerClassChange(state);
    }

    if (spawn->monsterLevel <= 0)
        return;
    const RKC_RPG_TABLEDATA *mult = RKC_RPG_TABLE_GetFromTableNo(&state->table, TABLE_EXP_MULTIPLIER);
    long m = mult ? RKC_RPG_TABLEDATA_GetValue(mult, 10, 0) : 100; 
    long gained = (m * spawn->monsterLevel) / 100;
    if (gained < 0)
        gained = 0;
    state->playerExp += gained;

    const RKC_RPG_TABLEDATA *xp = RKC_RPG_TABLE_GetFromTableNo(&state->table, TABLE_XP_CURVE);
    if (xp && state->playerLevel >= 1 && state->playerLevel < PLAYER_LEVEL_CAP)
    {
        long threshold = RKC_RPG_TABLEDATA_GetValue(xp, state->playerLevel - 1, 0);
        if (threshold > 0 && state->playerExp >= threshold)
        {
            long oldLevel = state->playerLevel;
            PlayerLevelUp(state);
            state->playerExp = 0;
            
            return;
        }
        
    }
}

void RecomputePlayerMaxHP(DemoState *state)
{
    FinalCombatStats stats;
    ComputeFinalCombatStats(state, &stats);
    long base = state->progressionInitialized ? state->playerBaseStats[PLAYER_STAT_MAX_HP] : PLAYER_DEFAULT_MAX_HP;
    long newMax = base + stats.maxHPBonus;
    if (newMax < 1)
        newMax = 1;
    if (newMax != state->playerMaxHP)
        
    state->playerMaxHP = newMax;
    if (state->playerHP > state->playerMaxHP)
        state->playerHP = state->playerMaxHP;
}


void RecomputePlayerMaxMP(DemoState *state)
{
    long newMax = state->progressionInitialized ? state->playerBaseStats[PLAYER_STAT_MAX_MP] : 0;
    if (newMax < 0)
        newMax = 0;
    state->playerMaxMP = newMax;
    if (state->playerMP > state->playerMaxMP)
        state->playerMP = state->playerMaxMP;
}


int TrySpendMP(DemoState *state, int spellId)
{
    long cost = 0;
    if (state->tableLoaded)
    {
        const RKC_RPG_TABLEDATA *t = RKC_RPG_TABLE_GetFromTableNo(&state->table, TABLE_SPELL_MP_COST);
        if (t)
            cost = RKC_RPG_TABLEDATA_GetValue(t, spellId, 0);
    }
    if (cost < 0)
        cost = 0;
    if (state->playerMP < cost)
        return 0;
    state->playerMP -= cost;
    return 1;
}


void ComputePlayerCellBlockLayers(const DemoState *state, int isAttacking, long cellBlockCount, unsigned int *mask,
                                  unsigned short *tintR, unsigned short *tintG, unsigned short *tintB)
{
    for (long i = 0; i < cellBlockCount; i++)
    {
        mask[i] = 0;
        tintR[i] = 1000;
        tintG[i] = 1000;
        tintB[i] = 1000;
    }
    if (cellBlockCount > 0)
        mask[0] = 1;
    if (cellBlockCount > 1)
        mask[1] = 1;

    if (state->hasArmor[EQUIPMENT_BODY_SLOT_INDEX])
    {
        const RKC_RPG_ITEMDATA_Kind1Tail *a = &state->armor[EQUIPMENT_BODY_SLOT_INDEX].tail;
        long idx = a->cellBlockIndex;
        if (idx >= 0 && idx < cellBlockCount)
        {
            mask[idx] = 1;
            tintR[idx] = (unsigned short)a->cellBlockTintR;
            tintG[idx] = (unsigned short)a->cellBlockTintG;
            tintB[idx] = (unsigned short)a->cellBlockTintB;
        }
    }
    if (state->hasArmor[EQUIPMENT_SHIELD_SLOT_INDEX] && !IsShieldIneffective(state))
    {
        const RKC_RPG_ITEMDATA_Kind1Tail *sh = &state->armor[EQUIPMENT_SHIELD_SLOT_INDEX].tail;
        long idx = sh->cellBlockIndex;
        if (idx >= 0 && idx < cellBlockCount)
        {
            mask[idx] = 1;
            tintR[idx] = (unsigned short)sh->cellBlockTintR;
            tintG[idx] = (unsigned short)sh->cellBlockTintG;
            tintB[idx] = (unsigned short)sh->cellBlockTintB;
        }
    }

    if (!state->hasWeapon)
        return;

    const RKC_RPG_ITEMDATA_Kind0Tail *w = &state->weapon.tail;
    long primary = w->cellBlockIndex;
    if (primary >= 0 && primary < cellBlockCount)
    {
        mask[primary] = 1;
        tintR[primary] = (unsigned short)w->cellBlockTintR;
        tintG[primary] = (unsigned short)w->cellBlockTintG;
        tintB[primary] = (unsigned short)w->cellBlockTintB;
    }
    long secondary = w->weaponSecondaryCellBlockIndex;
    if (secondary >= 0 && secondary < cellBlockCount)
    {
        mask[secondary] = 1;
        tintR[secondary] = (unsigned short)w->weaponSecondaryCellBlockTintR;
        tintG[secondary] = (unsigned short)w->weaponSecondaryCellBlockTintG;
        tintB[secondary] = (unsigned short)w->weaponSecondaryCellBlockTintB;
    }
    if (cellBlockCount > 2)
    {
        tintR[2] = (unsigned short)w->weaponGripTintR;
        tintG[2] = (unsigned short)w->weaponGripTintG;
        tintB[2] = (unsigned short)w->weaponGripTintB;
        if (isAttacking)
            mask[2] = 1;
    }
}


static long ComputeRealEnemyHitDamage(const DemoState *state, long baseDamage)
{
    long facingVal = 10;
    if (state->tableLoaded)
    {
        const RKC_RPG_TABLEDATA *tableB = RKC_RPG_TABLE_GetFromTableNo(&state->table, 0xb);
        if (tableB)
            facingVal = RKC_RPG_TABLEDATA_GetValue(tableB, 10, 0);
    }
    FinalCombatStats stats;
    ComputeFinalCombatStats(state, &stats);
    
    long defenderStat =
        stats.combatStat[1] + (state->progressionInitialized ? state->playerBaseStats[PLAYER_STAT_DEFENCE] : 0);
    long roll = ((rand() % 3 + 9) * baseDamage) / 10 - (defenderStat * facingVal) / 10;
    return roll < 1 ? 1 : roll;
}


static long ComputeRealPlayerHitDamage(const DemoState *state, long attackerPower)
{
    long facingVal = 10;
    if (state->tableLoaded)
    {
        const RKC_RPG_TABLEDATA *tableB = RKC_RPG_TABLE_GetFromTableNo(&state->table, 0xb);
        if (tableB)
            facingVal = RKC_RPG_TABLEDATA_GetValue(tableB, 10, 0);
    }
    long defenderMitigation = ENEMY_MITIGATION_PLACEHOLDER;
    long roll = ((rand() % 3 + 9) * attackerPower) / 10 - (defenderMitigation * facingVal) / 10;
    return roll < 1 ? 1 : roll;
}


static void TickPlayerGearDurability(DemoState *state)
{
    static const int BREAK_CHANCE_PERCENT[EQUIPMENT_ARMOR_SLOTS] = {20, 30, 30, 20};
    const int ARMOR_SHIELD_BREAK_SLOT_INDEX = 2;

    int weaponRequiresTwoHands = state->hasWeapon && (state->weapon.tail.weaponClass == 1 ||
                                                       state->weapon.tail.weaponClass == 3 ||
                                                       state->weapon.tail.requiresTwoHands);
    int anyBroke = 0;
    for (int s = 0; s < EQUIPMENT_ARMOR_SLOTS; s++)
    {
        if (!state->hasArmor[s])
            continue;
        if (s == ARMOR_SHIELD_BREAK_SLOT_INDEX && weaponRequiresTwoHands)
            continue;
        if (rand() % 100 >= BREAK_CHANCE_PERCENT[s])
            continue;

        int wasIntact = state->armor[s].durability > 0;
        if (wasIntact)
        {
            state->armor[s].durability--;
            if (state->armor[s].durability <= 0)
                printf("%s broke\n", state->armorName[s]);
        }
        if (state->armor[s].durability <= 0)
            anyBroke = 1;
    }
    if (anyBroke)
        RecomputePlayerMaxHP(state);
}


long ComputeRealAttackDurationTicks(const DemoState *state, const LiveSpawn *spawn)
{
    if (spawn->attackChartIndex < 0 || spawn->templateIndex < 0)
        return ENEMY_ATTACK_COOLDOWN_TICKS;

    const LiveSpawnTemplate *tmpl = &state->templates[spawn->templateIndex];
    if (tmpl->kind != LIVE_SPAWN_SPRITE_CAF || spawn->attackChartIndex >= tmpl->caf.chartCount)
        return ENEMY_ATTACK_COOLDOWN_TICKS;

    int direction = spawn->facingDirection;
    if (direction < 0 || direction >= RKC_RPGSCRN_CAF_NUM_DIRECTIONS)
        direction = 0;

    short frameCount = tmpl->caf.charts[spawn->attackChartIndex].directions[direction].maxFrameCount;
    
    long idx = spawn->attackSpeedIndex;
    double speedMultiplier = (idx >= 0 && idx < 10) ? ENEMY_ATTACK_SPEED_TABLE[idx] : 1.0;
    
    long duration = (long)((double)frameCount * ATTACK_ANIM_TICKS_PER_FRAME / speedMultiplier);
    return duration > 0 ? duration : ENEMY_ATTACK_COOLDOWN_TICKS;
}


void TickEnemyAttack(DemoState *state, LiveSpawn *spawn)
{
    if (spawn->aiState.isDead || state->playerIsDead)
        return;

    if (spawn->hasPendingHit && state->tick >= spawn->pendingHitTick)
    {
        spawn->hasPendingHit = 0;
        
        if (RKC_RPG_AI_EXEC_IsAttacking(&spawn->aiState))
        {
            const char *label = spawn->name ? spawn->name : "(unnamed enemy)";
            long damage = ComputeRealEnemyHitDamage(state, spawn->baseDamage);
            state->playerHP -= damage;
            TickPlayerGearDurability(state);
            
            state->playerHitTick = state->tick;
            state->playerHitVfxVariant = rand() % HIT_VFX_VARIANT_COUNT;
            
            PlayCombatSound(state, COMBAT_HIT_SOUND_TRACK);
            
            if (state->playerHP > 0)
                ArmPlayerShake(state, spawn->x, spawn->y);

            if (state->playerHP > 0)
                printf("%s hit player for %ld damage\n", label, damage);
            else
            {
                state->playerHP = 0;
                if (TryConsumeReviveItem(state))
                {
                    state->playerHP = state->playerMaxHP;
                    
                }
                else
                {
                    state->playerIsDead = 1;
                    
                }
            }
        }
    }

    if (spawn->attackCooldownTicks > 0)
    {
        spawn->attackCooldownTicks--;
        return;
    }

    if (!RKC_RPG_AI_EXEC_IsAttacking(&spawn->aiState))
        return;

    
    spawn->attackCooldownTicks = ComputeRealAttackDurationTicks(state, spawn);
    spawn->attackAnimTick = state->tick; 
    spawn->hasPendingHit = 1;
    spawn->pendingHitTick =
        state->tick + (unsigned long)(spawn->attackCooldownTicks * HIT_CONNECT_FRACTION_PERCENT / 100);
    
    if (spawn->field8 >= 0 && spawn->field8 < 25)
        PlayCombatSound(state, ENEMY_ATTACK_SWING_SOUND[spawn->field8]);
}
