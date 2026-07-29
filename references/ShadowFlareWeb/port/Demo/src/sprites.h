#ifndef SFDE_GROUND_SPRITES_H
#define SFDE_GROUND_SPRITES_H

#include "state.h"

void LoadCafTemplate(LiveSpawnTemplate *t, const char *dir, const char *stem);

void BuildLiveSpawns(DemoState *state);

void SnapshotLiveSpawns(const DemoState *state, LiveSpawnSaveEntry **outEntries, long *outCount);

void RestoreLiveSpawns(DemoState *state, const LiveSpawnSaveEntry *entries, long count);

long FindNearestLiveSpawn(DemoState *state);

long FindSpawnNearScreenPoint(DemoState *state, long screenX, long screenY);

long FindHoveredSpawn(DemoState *state, long screenX, long screenY);

long FindWorldItemNearScreenPoint(DemoState *state, long screenX, long screenY);

void PrintSpawnNavigationHints(DemoState *state);

#endif
