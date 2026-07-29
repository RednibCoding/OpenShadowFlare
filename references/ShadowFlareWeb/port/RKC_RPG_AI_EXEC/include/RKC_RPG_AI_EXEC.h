#ifndef SFDE_RKC_RPG_AI_EXEC_H
#define SFDE_RKC_RPG_AI_EXEC_H

#include "RKC_RPG_AICONTROL.h"

#define RKC_RPG_AI_EXEC_SPEED_SCALE 35.0
#define RKC_RPG_AI_EXEC_EVENT_AGGRO 11
#define RKC_RPG_AI_EXEC_EVENT_HIT_AND_AWAY 17

typedef enum
{
    RKC_RPG_AI_EXEC_ACTION_IDLE = 0,
    RKC_RPG_AI_EXEC_ACTION_WANDER = 1,
    RKC_RPG_AI_EXEC_ACTION_ATTACK_MIN = 2,
    RKC_RPG_AI_EXEC_ACTION_ATTACK_MAX = 4,
} RKC_RPG_AI_EXEC_KnownAction;

typedef struct
{
    const RKC_RPG_AILIST *list;
    const RKC_RPG_AIDATA *activeData;
    long actionNo;
    long elapsedTicks;
    long homeX, homeY;
    long wanderTargetX, wanderTargetY;
    int hasWanderTarget;
    double posX, posY;
    long currentHP, maxHP;
    int isDead;
    long standoffMinDistance;
    int hasHitAndAway;
    int isRetreating;
} RKC_RPG_AI_EXEC_State;

typedef struct
{
    long currentX, currentY;
    int hasTarget;
    long targetX, targetY;
    long frameDeltaMs;
    int attackLocked;
    long speedPercent;
} RKC_RPG_AI_EXEC_TickInput;

typedef struct
{
    long newX, newY;
    int moved;
    double dirX, dirY;
} RKC_RPG_AI_EXEC_TickResult;

void RKC_RPG_AI_EXEC_State_Init(RKC_RPG_AI_EXEC_State *state, const RKC_RPG_AILIST *list, long homeX, long homeY,
                                 long maxHP);

void RKC_RPG_AI_EXEC_Tick(RKC_RPG_AI_EXEC_State *state, const RKC_RPG_AI_EXEC_TickInput *in,
                          RKC_RPG_AI_EXEC_TickResult *out);

int RKC_RPG_AI_EXEC_IsAttacking(const RKC_RPG_AI_EXEC_State *state);

int RKC_RPG_AI_EXEC_ApplyDamage(RKC_RPG_AI_EXEC_State *state, long amount);

#endif
