#ifndef SFDE_GROUND_COMBAT_H
#define SFDE_GROUND_COMBAT_H

#include "state.h"


typedef struct
{
    long combatStat[8]; 
    long attackSpeedBonus, moveSpeedBonus; 
    long maxHPBonus, maxShieldBonus;   
    long field10;                     
    long field54, field55, field56, field57; 
} FinalCombatStats;

void ComputeFinalCombatStats(const DemoState *state, FinalCombatStats *out);


void FacingDirectionToDelta(int direction, double *outDX, double *outDY);


double ComputePlayerAttackAnimTicksPerFrame(const DemoState *state);


long ComputePlayerMoveSpeed(const DemoState *state, int running);


void RecomputePlayerMaxHP(DemoState *state);


void RecomputePlayerMaxMP(DemoState *state);
int TrySpendMP(DemoState *state, int spellId);


void InitPlayerProgression(DemoState *state);
void PlayerLevelUp(DemoState *state);


const char *SpellName(int spellId);


void AwardPlayerKillExp(DemoState *state, const LiveSpawn *spawn);


void EvaluatePlayerClassChange(DemoState *state);
void PlayerChangeClass(DemoState *state);


void ComputePlayerCellBlockLayers(const DemoState *state, int isAttacking, long cellBlockCount, unsigned int *mask,
                                  unsigned short *tintR, unsigned short *tintG, unsigned short *tintB);


void TickEnemyAttack(DemoState *state, LiveSpawn *spawn);


void HandleAttack(DemoState *state);


int ApplyPlayerAttack(DemoState *state, LiveSpawn *spawn);


void ArmPlayerSingleSwing(DemoState *state, const LiveSpawn *target);


void TickPlayerPendingHit(DemoState *state);


long ResolvePlayerAttackChart(const DemoState *state);


long ResolveWeaponCategoryChart(const DemoState *state, long defaultChart, long axeBluntChart, long twoHandedChart);


int ResolvePlayerAttackSequence(const DemoState *state, long *chart1, long *chart2);


int ResolvePlayerComboSequence(const DemoState *state, long charts[5], int dealsDamage[5]);


int ResolvePlayerSwingPhases(const DemoState *state, int wantCombo, long charts[5], short frames[5],
                              int dealsDamage[5]);


int ResolvePlayerChartDrawDirection(long chart, int liveFacingDirection);


long ComputeRealPlayerAttackDurationTicks(const DemoState *state, int wantCombo);


long ComputeRealAttackDurationTicks(const DemoState *state, const LiveSpawn *spawn);


void HandleComboAttack(DemoState *state, long mouseWindowX, long mouseWindowY);


void CastTransport(DemoState *state, long mouseWindowX, long mouseWindowY);


void TickTransportCircle(DemoState *state);


int IsPlayerBusyAttacking(const DemoState *state);


void TickPlayerCombo(DemoState *state);

#endif
