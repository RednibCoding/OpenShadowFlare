#include "RKC_RPG_AI_EXEC.h"

#include <math.h>
#include <stdlib.h>

void RKC_RPG_AI_EXEC_State_Init(RKC_RPG_AI_EXEC_State *state, const RKC_RPG_AILIST *list, long homeX, long homeY,
                                 long maxHP)
{
    state->list = list;
    state->activeData = NULL;
    state->actionNo = -1;
    state->elapsedTicks = 0;
    state->homeX = homeX;
    state->homeY = homeY;
    state->wanderTargetX = homeX;
    state->wanderTargetY = homeY;
    state->hasWanderTarget = 0;
    state->posX = (double)homeX;
    state->posY = (double)homeY;
    state->maxHP = maxHP;
    state->currentHP = maxHP;
    state->isDead = 0;
    state->standoffMinDistance = -1;
    if (list)
    {
        for (long e = 0; e < list->eventCount && state->standoffMinDistance < 0; e++)
        {
            const RKC_RPG_AIEVENT *event = &list->events[e];
            for (long i = 0; i < event->dataCount; i++)
            {
                const RKC_RPG_AIDATA *data = &event->data[i];
                if (data->actionNo < RKC_RPG_AI_EXEC_ACTION_ATTACK_MIN || data->actionNo > RKC_RPG_AI_EXEC_ACTION_ATTACK_MAX)
                    continue;
                if (data->condition.targetSearchEnabled && data->condition.targetMinDistance > 0)
                {
                    state->standoffMinDistance = data->condition.targetMinDistance;
                    break;
                }
            }
        }
    }

    state->hasHitAndAway = 0;
    state->isRetreating = 0;
    if (list)
    {
        for (long e = 0; e < list->eventCount && !state->hasHitAndAway; e++)
        {
            const RKC_RPG_AIEVENT *event = &list->events[e];
            for (long i = 0; i < event->dataCount; i++)
            {
                const RKC_RPG_AIDATA *data = &event->data[i];
                if (data->actionNo == 9 && !data->condition.targetSearchEnabled)
                {
                    state->hasHitAndAway = 1;
                    break;
                }
            }
        }
    }
}

int RKC_RPG_AI_EXEC_IsAttacking(const RKC_RPG_AI_EXEC_State *state)
{
    return state->actionNo >= RKC_RPG_AI_EXEC_ACTION_ATTACK_MIN &&
           state->actionNo <= RKC_RPG_AI_EXEC_ACTION_ATTACK_MAX;
}

int RKC_RPG_AI_EXEC_ApplyDamage(RKC_RPG_AI_EXEC_State *state, long amount)
{
    if (state->isDead)
        return 0;
    state->currentHP -= amount;
    if (state->currentHP > 0)
        return 0;
    state->currentHP = 0;
    state->isDead = 1;
    return 1;
}

static const RKC_RPG_AIDATA *FindBestCandidate(const RKC_RPG_AI_EXEC_State *state, long eventNo,
                                                const RKC_RPG_AI_EXEC_TickInput *in)
{
    if (!state->list || eventNo < 0 || eventNo >= state->list->eventCount)
        return NULL;

    const RKC_RPG_AIEVENT *event = &state->list->events[eventNo];
    const RKC_RPG_AIDATA *best = NULL;
    long bestPriority = 0;

    for (long i = 0; i < event->dataCount; i++)
    {
        const RKC_RPG_AIDATA *data = &event->data[i];
        const RKC_RPG_AICONDITION *cond = &data->condition;

        if (cond->hpPercentCheckEnabled && state->maxHP > 0)
        {
            long hpPercent = state->currentHP * 100 / state->maxHP;
            if ((cond->hpPercentMin >= 0 && hpPercent < cond->hpPercentMin) ||
                (cond->hpPercentMax >= 0 && hpPercent > cond->hpPercentMax))
                continue;
        }

        if (cond->targetSearchEnabled)
        {
            if (!in->hasTarget)
                continue;
            double dx = (double)in->targetX - state->posX;
            double dy = (double)in->targetY - state->posY;
            double distSq = dx * dx + dy * dy;
            double minSq = cond->targetMinDistance < 0 ? -1 : (double)cond->targetMinDistance * cond->targetMinDistance;
            double maxSq = cond->targetMaxDistance < 0 ? -1 : (double)cond->targetMaxDistance * cond->targetMaxDistance;
            if ((minSq >= 0 && distSq < minSq) || (maxSq >= 0 && distSq > maxSq))
                continue;
        }

        if (!best || data->parameter.priority > bestPriority)
        {
            best = data;
            bestPriority = data->parameter.priority;
        }
    }
    return best;
}

static void SelectAction(RKC_RPG_AI_EXEC_State *state, long eventNo, const RKC_RPG_AI_EXEC_TickInput *in)
{
    const RKC_RPG_AIDATA *best = FindBestCandidate(state, eventNo, in);
    if (!best)
    {
        if (eventNo != 0)
            SelectAction(state, 0, in);
        return;
    }

    const RKC_RPG_AIEVENT *event = &state->list->events[eventNo];
    long bestPriority = best->parameter.priority;
    long totalWeight = 0;

    for (long i = 0; i < event->dataCount; i++)
        if (event->data[i].parameter.priority == bestPriority)
            totalWeight += event->data[i].parameter.weight;

    long pick = totalWeight > 0 ? rand() % totalWeight : 0;
    long accum = 0;
    for (long i = 0; i < event->dataCount; i++)
    {
        const RKC_RPG_AIDATA *data = &event->data[i];
        if (data->parameter.priority != bestPriority)
            continue;
        accum += data->parameter.weight;
        if (pick < accum || (totalWeight == 0 && data == best))
        {
            state->activeData = data;
            state->actionNo = data->actionNo;
            state->elapsedTicks = 0;
            state->hasWanderTarget = 0;
            return;
        }
    }
    state->activeData = best;
    state->actionNo = best->actionNo;
    state->elapsedTicks = 0;
    state->hasWanderTarget = 0;
}

static int StepToward(RKC_RPG_AI_EXEC_State *state, double targetX, double targetY, double step)
{
    double dx = targetX - state->posX;
    double dy = targetY - state->posY;
    double dist = sqrt(dx * dx + dy * dy);
    if (dist <= step || dist <= 0.0)
    {
        state->posX = targetX;
        state->posY = targetY;
        return dist > 0.0;
    }
    state->posX += dx / dist * step;
    state->posY += dy / dist * step;
    return 1;
}

static void TickWander(RKC_RPG_AI_EXEC_State *state, const RKC_RPG_AI_EXEC_TickInput *in, RKC_RPG_AI_EXEC_TickResult *out)
{
    const RKC_RPG_AIPARAM *param = &state->activeData->parameter;
    long range = param->wanderRangeA + param->wanderRangeB;
    if (range <= 0)
        return;

    if (!state->hasWanderTarget)
    {
        state->wanderTargetX = state->homeX + (rand() % (2 * range + 1)) - range;
        state->wanderTargetY = state->homeY + (rand() % (2 * range + 1)) - range;
        state->hasWanderTarget = 1;
    }

    double step = (double)param->speed * RKC_RPG_AI_EXEC_SPEED_SCALE * (double)in->frameDeltaMs / 1000.0;
    if (in->speedPercent > 0)
        step = step * (double)in->speedPercent / 100.0;
    if (step <= 0.0)
        return;

    double dirX = (double)state->wanderTargetX - state->posX;
    double dirY = (double)state->wanderTargetY - state->posY;
    if (!StepToward(state, (double)state->wanderTargetX, (double)state->wanderTargetY, step))
        return;
    if (state->posX == (double)state->wanderTargetX && state->posY == (double)state->wanderTargetY)
        state->hasWanderTarget = 0;

    out->newX = (long)llround(state->posX);
    out->newY = (long)llround(state->posY);
    out->moved = 1;
    out->dirX = dirX;
    out->dirY = dirY;
}

static int StepAway(RKC_RPG_AI_EXEC_State *state, double fromX, double fromY, double step)
{
    double dx = state->posX - fromX;
    double dy = state->posY - fromY;
    double dist = sqrt(dx * dx + dy * dy);
    if (dist <= 0.0)
        return 0;
    state->posX += dx / dist * step;
    state->posY += dy / dist * step;
    return 1;
}

static void TickApproach(RKC_RPG_AI_EXEC_State *state, const RKC_RPG_AI_EXEC_TickInput *in, RKC_RPG_AI_EXEC_TickResult *out)
{
    if (!in->hasTarget)
        return;
    const RKC_RPG_AIPARAM *param = &state->activeData->parameter;
    double step = (double)param->speed * RKC_RPG_AI_EXEC_SPEED_SCALE * (double)in->frameDeltaMs / 1000.0;
    if (in->speedPercent > 0)
        step = step * (double)in->speedPercent / 100.0;
    if (step <= 0.0)
        return;

    double dirX = (double)in->targetX - state->posX;
    double dirY = (double)in->targetY - state->posY;

    if (state->isRetreating ||
        (state->standoffMinDistance > 0 &&
         dirX * dirX + dirY * dirY < (double)state->standoffMinDistance * (double)state->standoffMinDistance))
    {
        if (!StepAway(state, (double)in->targetX, (double)in->targetY, step))
            return;
        out->newX = (long)llround(state->posX);
        out->newY = (long)llround(state->posY);
        out->moved = 1;
        out->dirX = -dirX;
        out->dirY = -dirY;
        return;
    }

    if (!StepToward(state, (double)in->targetX, (double)in->targetY, step))
        return;
    out->newX = (long)llround(state->posX);
    out->newY = (long)llround(state->posY);
    out->moved = 1;
    out->dirX = dirX;
    out->dirY = dirY;
}

void RKC_RPG_AI_EXEC_Tick(RKC_RPG_AI_EXEC_State *state, const RKC_RPG_AI_EXEC_TickInput *in, RKC_RPG_AI_EXEC_TickResult *out)
{

    if (llround(state->posX) != in->currentX)
        state->posX = (double)in->currentX;
    if (llround(state->posY) != in->currentY)
        state->posY = (double)in->currentY;

    out->newX = (long)llround(state->posX);
    out->newY = (long)llround(state->posY);
    out->moved = 0;
    out->dirX = 0.0;
    out->dirY = 0.0;

    if (!state->list || state->isDead)
        return;

    int wasAttacking = RKC_RPG_AI_EXEC_IsAttacking(state);

    long aggroEvent = state->list->eventCount > RKC_RPG_AI_EXEC_EVENT_AGGRO ? RKC_RPG_AI_EXEC_EVENT_AGGRO : -1;
    int expired = !state->activeData || state->activeData->parameter.durationTicks <= 0 ||
                  state->elapsedTicks >= state->activeData->parameter.durationTicks;

    int interrupted = 0;
    if (!expired && aggroEvent >= 0)
    {
        const RKC_RPG_AIDATA *aggroCandidate = FindBestCandidate(state, aggroEvent, in);
        if (aggroCandidate && (state->actionNo == RKC_RPG_AI_EXEC_ACTION_IDLE ||
                                state->actionNo == RKC_RPG_AI_EXEC_ACTION_WANDER ||
                                aggroCandidate->parameter.priority > state->activeData->parameter.priority))
            interrupted = 1;
    }

    if (state->hasHitAndAway && wasAttacking && !in->attackLocked)
    {
        SelectAction(state, RKC_RPG_AI_EXEC_EVENT_HIT_AND_AWAY, in);
        state->isRetreating = 1;
    }
    else if (expired || interrupted)
    {
        SelectAction(state, aggroEvent >= 0 ? aggroEvent : 0, in);
        state->isRetreating = 0;
    }
    if (!state->activeData)
        return;

    state->elapsedTicks++;

    if (state->actionNo == RKC_RPG_AI_EXEC_ACTION_IDLE)
        return; /* no movement */
    if (state->actionNo == RKC_RPG_AI_EXEC_ACTION_WANDER)
        TickWander(state, in, out);
    else if (RKC_RPG_AI_EXEC_IsAttacking(state))
        return;
    else
        TickApproach(state, in, out);
}
