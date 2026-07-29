#ifndef SFDE_GROUND_MOVEMENT_H
#define SFDE_GROUND_MOVEMENT_H

#include "state.h"

long RectGapDistance(long ax, long ay, long aL, long aT, long aR, long aB, long bx, long by, long bL, long bT,
                      long bR, long bB);

int RectsOverlap(long ax, long ay, long aL, long aT, long aR, long aB, long bx, long by, long bL, long bT, long bR,
                  long bB);

int ResolveFacingDirection(long dx, long dy, int current);

void TickLiveSpawnsAI(DemoState *state);

void TickNpcWander(DemoState *state);

void MovePlayer(DemoState *state, const Uint8 *keys);

void HandleClick(DemoState *state, long mouseWindowX, long mouseWindowY, int isFreshPress);

void TickPendingAction(DemoState *state, const Uint8 *keys);

int PlayerBlockedByCharacterAt(DemoState *state, long x, long y);

#endif
