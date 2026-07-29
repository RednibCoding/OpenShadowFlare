#ifndef SFDE_GROUND_INVENTORY_H
#define SFDE_GROUND_INVENTORY_H

#include "state.h"


int IsShieldIneffective(const DemoState *state);


int ArmorNameFitsSlot(const char *name, int slot);


void BuildWorldItems(DemoState *state);


void SnapshotWorldItems(const DemoState *state, WorldItemSaveEntry **outEntries, long *outCount);


void RestoreWorldItems(DemoState *state, const WorldItemSaveEntry *entries, long count, long baseCountAtSave);


void ApplyPlayerPickup(DemoState *state, long index);


int AddItemToInventory(DemoState *state, long kind, long templateId, const char *name);


int CreateHeldInventorySlot(DemoState *state, long kind, long templateId, const char *name);


void RemoveOneFromInventorySlot(DemoState *state, int index);


void RecomputePlayerGold(DemoState *state);
long AddGoldToInventory(DemoState *state, long amount);
void DeductGoldFromInventory(DemoState *state, long amount);


void EquipItemIntoSlot(DemoState *state, long kind, long templateId, const char *name, int slot);


#define INVENTORY_DROP_OUT_OF_BOUNDS (-2)
#define INVENTORY_DROP_BLOCKED (-3)
#define INVENTORY_DROP_FREE (-1)
int ClassifyInventoryDropTarget(const DemoState *state, long col, long row, long w, long h, int skipIndex);


void DropEnemyGold(DemoState *state, LiveSpawn *spawn);


void DropInventoryItemToGround(DemoState *state, int index, long clickWorldX, long clickWorldY);

#endif
