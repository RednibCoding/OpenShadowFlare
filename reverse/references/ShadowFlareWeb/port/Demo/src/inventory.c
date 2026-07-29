#include "inventory.h"
#include "render.h"
#include "combat.h"

#include <math.h>


void BuildWorldItems(DemoState *state)
{
    if (state->mct.block4Count <= 0)
        return;

    state->worldItems = malloc(sizeof(WorldItem) * (size_t)state->mct.block4Count);
    long n = 0;

    for (long i = 0; i < state->mct.block4Count; i++)
    {
        RKC_RPGSCRN_MCT_Block4Tail tail;
        if (!RKC_RPGSCRN_MCT_GetBlock4Tail(&state->mct.block4[i], &tail))
            continue;

        WorldItem *item = &state->worldItems[n++];
        item->x = state->mct.block4[i].x;
        item->y = state->mct.block4[i].y;
        item->pickedUp = 0;
        item->kind = tail.kind;
        item->templateId = tail.templateId;
        item->hitTestHalfWidth = 0;
        item->hitTestHalfHeight = 0;
        item->dropTick = 0; 
        item->jumpEffectTick = -1;

        
        if (tail.kind == 4 && tail.templateId == 0)
        {
            item->isGold = 1;
            item->resolved = 1;
            item->amount = tail.randMin + rand() % (tail.randMax - tail.randMin + 1);
            
            continue;
        }

        item->isGold = 0;
        item->amount = 1;
        const RKC_RPG_ITEMDATA_Record *rec =
            state->itemDataLoaded ? RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, (int)tail.kind, tail.templateId)
                                  : NULL;
        item->resolved = rec != NULL;
        
    }
    state->worldItemCount = n;
}


void SnapshotWorldItems(const DemoState *state, WorldItemSaveEntry **outEntries, long *outCount)
{
    long baseCount = state->baseWorldItemCount;
    
    long cap = state->worldItemCount > baseCount ? state->worldItemCount : baseCount;
    if (cap <= 0)
    {
        *outEntries = NULL;
        *outCount = 0;
        return;
    }

    WorldItemSaveEntry *entries = malloc(sizeof(WorldItemSaveEntry) * (size_t)cap);
    long n = 0;

    
    for (long i = 0; i < baseCount; i++, n++)
        entries[n].pickedUp = state->worldItems[i].pickedUp;

    
    for (long i = baseCount; i < state->worldItemCount; i++)
    {
        const WorldItem *item = &state->worldItems[i];
        if (item->pickedUp)
            continue;
        entries[n].pickedUp = 0;
        entries[n].x = item->x;
        entries[n].y = item->y;
        entries[n].isGold = item->isGold;
        entries[n].amount = item->amount;
        
        entries[n].resolved = item->resolved;
        entries[n].kind = item->kind;
        entries[n].templateId = item->templateId;
        n++;
    }

    *outEntries = entries;
    *outCount = n;
}


void RestoreWorldItems(DemoState *state, const WorldItemSaveEntry *entries, long count, long baseCountAtSave)
{
    if (baseCountAtSave != state->baseWorldItemCount || count < baseCountAtSave)
        return;

    for (long i = 0; i < baseCountAtSave; i++)
        state->worldItems[i].pickedUp = entries[i].pickedUp;

    long dynamicCount = count - baseCountAtSave;
    if (dynamicCount <= 0)
        return;

    WorldItem *grown = realloc(state->worldItems, sizeof(WorldItem) * (size_t)(state->worldItemCount + dynamicCount));
    if (!grown)
        return;
    state->worldItems = grown;

    for (long i = 0; i < dynamicCount; i++)
    {
        const WorldItemSaveEntry *saved = &entries[baseCountAtSave + i];
        WorldItem *item = &state->worldItems[state->worldItemCount + i];
        item->x = saved->x;
        item->y = saved->y;
        item->isGold = saved->isGold;
        item->amount = saved->amount;
        
        item->resolved = saved->resolved;
        item->pickedUp = 0;
        item->kind = saved->kind;
        item->templateId = saved->templateId;
        item->hitTestHalfWidth = 0;
        item->hitTestHalfHeight = 0;
        item->dropTick = 0;       
        item->jumpEffectTick = -1;
    }
    state->worldItemCount += dynamicCount;
}




static int IsShieldIncompatibleWeapon(const RKC_RPG_ITEMDATA_Kind0Tail *weaponTail)
{
    if (weaponTail->weaponClass == SHIELD_INCOMPATIBLE_WEAPON_CLASS_AXE_BLUNT ||
        weaponTail->weaponClass == SHIELD_INCOMPATIBLE_WEAPON_CLASS_TWO_HANDED)
        return 1;
    
    return weaponTail->requiresTwoHands != 0;
}


static int IsShieldItemName(const char *name)
{
    return name && strstr(name, "Shield") != NULL;
}


static int IsBucklerItemName(const char *name)
{
    return name && strstr(name, "Buckler") != NULL;
}


static int IsHelmetItemName(const char *name)
{
    return name && (strstr(name, "Helm") || strstr(name, "Hat") || strstr(name, "Circlet") || strstr(name, "Mask"));
}


static int IsBootsItemName(const char *name)
{
    return name && (strstr(name, "Boots") || strstr(name, "Shoes") || strstr(name, "Greaves"));
}


int ArmorNameFitsSlot(const char *name, int slot)
{
    if (slot == EQUIPMENT_HELMET_SLOT_INDEX)
        return IsHelmetItemName(name);
    if (slot == EQUIPMENT_BOOTS_SLOT_INDEX)
        return IsBootsItemName(name);
    if (slot == EQUIPMENT_SHIELD_SLOT_INDEX)
        return IsShieldItemName(name) || IsBucklerItemName(name);
    return !IsHelmetItemName(name) && !IsBootsItemName(name) && !IsShieldItemName(name) && !IsBucklerItemName(name);
}


int IsShieldIneffective(const DemoState *state)
{
    return state->hasArmor[EQUIPMENT_SHIELD_SLOT_INDEX] &&
           IsShieldItemName(state->armorName[EQUIPMENT_SHIELD_SLOT_INDEX]) && state->hasWeapon &&
           IsShieldIncompatibleWeapon(&state->weapon.tail);
}

static void EquipWeapon(DemoState *state, long templateId, const char *name)
{
    const RKC_RPG_ITEMDATA_Record *rec = RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, 0, templateId);
    const RKC_RPG_ITEMDATA_Kind0Tail *tmpl = rec ? RKC_RPG_ITEMDATA_GetKind0Tail(rec) : NULL;
    if (!tmpl)
        return;

    RKC_RPG_ITEMDATA_RollKind0Instance(tmpl, &state->weapon);
    state->hasWeapon = 1;
    
    
    if (state->hasArmor[EQUIPMENT_SHIELD_SLOT_INDEX] &&
        IsShieldItemName(state->armorName[EQUIPMENT_SHIELD_SLOT_INDEX]) && IsShieldIncompatibleWeapon(tmpl))
        
    else
        
}


static void EquipArmorIntoSlot(DemoState *state, int slot, long templateId, const char *name)
{
    const RKC_RPG_ITEMDATA_Record *rec = RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, 1, templateId);
    const RKC_RPG_ITEMDATA_Kind1Tail *tmpl = rec ? RKC_RPG_ITEMDATA_GetKind1Tail(rec) : NULL;
    if (!tmpl)
        return;

    int isShield = IsShieldItemName(name);
    RKC_RPG_ITEMDATA_RollKind1Instance(tmpl, &state->armor[slot]);
    state->hasArmor[slot] = 1;
    
    
    int nowInert = isShield && slot == EQUIPMENT_SHIELD_SLOT_INDEX && state->hasWeapon &&
                   IsShieldIncompatibleWeapon(&state->weapon.tail);
    
}


static void EquipAccessoryIntoSlot(DemoState *state, int slot, long templateId, const char *name)
{
    const RKC_RPG_ITEMDATA_Record *rec = RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, 2, templateId);
    const RKC_RPG_ITEMDATA_Kind2Tail *tmpl = rec ? RKC_RPG_ITEMDATA_GetKind2Tail(rec) : NULL;
    if (!tmpl)
        return;

    RKC_RPG_ITEMDATA_RollKind2Instance(tmpl, &state->accessory[slot]);
    state->hasAccessory[slot] = 1;
    
    
}


void EquipItemIntoSlot(DemoState *state, long kind, long templateId, const char *name, int slot)
{
    if (!state->itemDataLoaded)
        return;
    switch (kind)
    {
    case 0:
        EquipWeapon(state, templateId, name); 
        break;
    case 1:
        EquipArmorIntoSlot(state, slot, templateId, name);
        break;
    case 2:
        EquipAccessoryIntoSlot(state, slot, templateId, name);
        break;
    default:
        break;
    }
}


static long ComputeCounterProcBonus(const DemoState *state)
{
    long chance = 0, bonus = 0;
    if (state->hasWeapon)
    {
        long v = state->weapon.tail.rollTable1[20].value;
        chance += v;
        if (v != 0)
            bonus += state->weapon.tail.rollTable1[21].value;
    }
    int shieldInert = IsShieldIneffective(state);
    for (int s = 0; s < EQUIPMENT_ARMOR_SLOTS; s++)
    {
        if (!state->hasArmor[s] || (s == EQUIPMENT_SHIELD_SLOT_INDEX && shieldInert))
            continue;
        long v = state->armor[s].tail.rollTable1[20].value;
        chance += v;
        if (v != 0)
            bonus += state->armor[s].tail.rollTable1[21].value;
    }
    for (int s = 0; s < EQUIPMENT_ACCESSORY_SLOTS; s++)
    {
        if (!state->hasAccessory[s])
            continue;
        long v = state->accessory[s].tail.rollTable1[20].value;
        chance += v;
        if (v != 0)
            bonus += state->accessory[s].tail.rollTable1[21].value;
    }
    return (rand() % 100 < chance) ? bonus : 0;
}



static int GridRectIsFree(const DemoState *state, long col, long row, long w, long h)
{
    if (col < 0 || row < 0 || w > INVENTORY_REAL_GRID_COLS || h > INVENTORY_REAL_GRID_ROWS ||
        col > INVENTORY_REAL_GRID_COLS - w || row > INVENTORY_REAL_GRID_ROWS - h)
        return 0;
    for (long i = 0; i < state->inventoryCount; i++)
    {
        const InventorySlot *s = &state->inventory[i];
        if (s->gridCol < 0)
            continue; 
        if (col < s->gridCol + s->gridWidth && s->gridCol < col + w && row < s->gridRow + s->gridHeight &&
            s->gridRow < row + h)
            return 0;
    }
    return 1;
}

static int FindFreeGridRect(const DemoState *state, long w, long h, long *outCol, long *outRow)
{
    if (w > INVENTORY_REAL_GRID_COLS || h > INVENTORY_REAL_GRID_ROWS)
        return 0;
    for (long row = 0; row <= INVENTORY_REAL_GRID_ROWS - h; row++)
        for (long col = 0; col <= INVENTORY_REAL_GRID_COLS - w; col++)
            if (GridRectIsFree(state, col, row, w, h))
            {
                *outCol = col;
                *outRow = row;
                return 1;
            }
    return 0;
}


int ClassifyInventoryDropTarget(const DemoState *state, long col, long row, long w, long h, int skipIndex)
{
    if (col < 0 || row < 0 || w > INVENTORY_REAL_GRID_COLS || h > INVENTORY_REAL_GRID_ROWS ||
        col > INVENTORY_REAL_GRID_COLS - w || row > INVENTORY_REAL_GRID_ROWS - h)
        return INVENTORY_DROP_OUT_OF_BOUNDS;
    int single = INVENTORY_DROP_FREE;
    for (long i = 0; i < state->inventoryCount; i++)
    {
        if ((int)i == skipIndex)
            continue;
        const InventorySlot *s = &state->inventory[i];
        if (s->gridCol < 0)
            continue;
        if (col < s->gridCol + s->gridWidth && s->gridCol < col + w && row < s->gridRow + s->gridHeight &&
            s->gridRow < row + h)
        {
            if (single != INVENTORY_DROP_FREE)
                return INVENTORY_DROP_BLOCKED; 
            single = (int)i;
        }
    }
    return single;
}


int AddItemToInventory(DemoState *state, long kind, long templateId, const char *name)
{
    int slot = -1;
    for (int s = 0; s < state->inventoryCount; s++)
        if (strcmp(state->inventory[s].name, name) == 0)
        {
            slot = s;
            break;
        }
    if (slot < 0 && state->inventoryCount < INVENTORY_MAX)
    {
        slot = state->inventoryCount++;
        
        state->inventory[slot].count = 0;
        
        state->inventory[slot].kind = kind;
        state->inventory[slot].templateId = templateId;

        
        state->inventory[slot].gridCol = -1;

        
        const RKC_RPG_ITEMDATA_Record *rec =
            state->itemDataLoaded ? RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, (int)kind, templateId)
                                  : NULL;
        RKC_RPG_ITEMDATA_GetGridSize(rec, &state->inventory[slot].gridWidth, &state->inventory[slot].gridHeight);
        if (!FindFreeGridRect(state, state->inventory[slot].gridWidth, state->inventory[slot].gridHeight,
                              &state->inventory[slot].gridCol, &state->inventory[slot].gridRow))
        {
            state->inventory[slot].gridCol = -1;
            state->inventory[slot].gridRow = -1;
        }
    }
    if (slot >= 0)
        state->inventory[slot].count++;
    return slot;
}


int CreateHeldInventorySlot(DemoState *state, long kind, long templateId, const char *name)
{
    if (state->inventoryCount >= INVENTORY_MAX)
        return -1;
    int slot = state->inventoryCount++;
    
    state->inventory[slot].count = 1;
    state->inventory[slot].kind = kind;
    state->inventory[slot].templateId = templateId;
    const RKC_RPG_ITEMDATA_Record *rec =
        state->itemDataLoaded ? RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, (int)kind, templateId) : NULL;
    RKC_RPG_ITEMDATA_GetGridSize(rec, &state->inventory[slot].gridWidth, &state->inventory[slot].gridHeight);
    state->inventory[slot].gridCol = -1;
    state->inventory[slot].gridRow = -1;
    return slot;
}


static void RemoveInventorySlotEntirely(DemoState *state, int index)
{
    if (index < 0 || index >= state->inventoryCount)
        return;
    for (int i = index; i < state->inventoryCount - 1; i++)
        state->inventory[i] = state->inventory[i + 1];
    state->inventoryCount--;
}


void RemoveOneFromInventorySlot(DemoState *state, int index)
{
    if (index < 0 || index >= state->inventoryCount)
        return;
    if (--state->inventory[index].count > 0)
        return;
    RemoveInventorySlotEntirely(state, index);
}


void RecomputePlayerGold(DemoState *state)
{
    long total = 0;
    for (long i = 0; i < state->inventoryCount; i++)
        if (state->inventory[i].kind == 4)
            total += state->inventory[i].count;
    state->gold = total;
}


long AddGoldToInventory(DemoState *state, long amount)
{
    for (long i = 0; i < state->inventoryCount && amount > 0; i++)
    {
        if (state->inventory[i].kind != 4)
            continue;
        long room = GOLD_PILE_CAP - state->inventory[i].count;
        if (room <= 0)
            continue;
        long take = amount < room ? amount : room;
        state->inventory[i].count += take;
        amount -= take;
    }

    while (amount > 0 && state->inventoryCount < INVENTORY_MAX)
    {
        long pileAmount = amount < GOLD_PILE_CAP ? amount : GOLD_PILE_CAP;
        int slot = state->inventoryCount;
        InventorySlot *inv = &state->inventory[slot];
        
        inv->kind = 4;
        inv->templateId = 0;
        inv->count = pileAmount;
        const RKC_RPG_ITEMDATA_Record *rec =
            state->itemDataLoaded ? RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, 4, 0) : NULL;
        RKC_RPG_ITEMDATA_GetGridSize(rec, &inv->gridWidth, &inv->gridHeight);
        inv->gridCol = -1; 
        state->inventoryCount++;
        if (!FindFreeGridRect(state, inv->gridWidth, inv->gridHeight, &inv->gridCol, &inv->gridRow))
        {
            state->inventoryCount--; 
            break;
        }
        amount -= pileAmount;
    }

    return amount;
}


void DeductGoldFromInventory(DemoState *state, long amount)
{
    for (long i = 0; i < state->inventoryCount && amount > 0;)
    {
        if (state->inventory[i].kind != 4)
        {
            i++;
            continue;
        }
        long take = amount < state->inventory[i].count ? amount : state->inventory[i].count;
        state->inventory[i].count -= take;
        amount -= take;
        if (state->inventory[i].count == 0)
        {
            RemoveInventorySlotEntirely(state, (int)i);
            continue; 
        }
        i++;
    }
    RecomputePlayerGold(state);
}


void ApplyPlayerPickup(DemoState *state, long index)
{
    if (index < 0 || index >= state->worldItemCount)
        return;
    WorldItem *item = &state->worldItems[index];
    if (item->pickedUp)
        return;

    if (item->isGold)
    {
        long leftover = AddGoldToInventory(state, item->amount);
        long picked = item->amount - leftover;
        if (leftover > 0)
        {
            item->amount = leftover; 
            item->jumpEffectTick = (long)state->tick;
            if (picked > 0)
                
            else
                
        }
        else
        {
            item->pickedUp = 1;
            
        }
        RecomputePlayerGold(state);
        
        return;
    }

    if (state->inventoryOpen && !state->dragActive)
    {
        int heldSlot = CreateHeldInventorySlot(state, item->kind, item->templateId, item->name);
        if (heldSlot < 0)
        {
            
            item->jumpEffectTick = (long)state->tick;
            
            return;
        }
        item->pickedUp = 1;
        state->dragActive = 1;
        state->dragSourceKind = DRAG_INVENTORY;
        state->dragSourceIndex = heldSlot;
        
        WorldToScreen(state, item->x, item->y, &state->dragCurrentMouseX, &state->dragCurrentMouseY);
        state->dragCurrentMouseX -= state->cameraX;
        state->dragCurrentMouseY -= state->cameraY;
        
        return;
    }

    
    int slot = AddItemToInventory(state, item->kind, item->templateId, item->name);
    if (slot >= 0 && state->inventory[slot].gridCol < 0)
    {
        
        RemoveOneFromInventorySlot(state, slot);
        slot = -1;
    }
    if (slot >= 0)
    {
        item->pickedUp = 1;
        
    }
    else
    {
        item->jumpEffectTick = (long)state->tick;
        
    }
}


void DropEnemyGold(DemoState *state, LiveSpawn *spawn)
{
    if (rand() % 100 >= ENEMY_GOLD_DROP_CHANCE_PERCENT)
        return;

    long minGold = ENEMY_GOLD_MIN;
    long maxGold = spawn->aiState.maxHP;
    long amount = minGold;
    if (maxGold > minGold)
        amount = minGold + rand() % (maxGold - minGold + 1);
    amount = amount * ENEMY_GOLD_FIND_MULTIPLIER_PERCENT / 100;
    if (amount < 1)
        amount = 1;

    WorldItem *grown = realloc(state->worldItems, sizeof(WorldItem) * (size_t)(state->worldItemCount + 1));
    if (!grown)
        return;
    state->worldItems = grown;

    WorldItem *item = &state->worldItems[state->worldItemCount++];
    item->x = spawn->x;
    item->y = spawn->y;
    item->isGold = 1;
    item->amount = amount;
    item->resolved = 1;
    item->pickedUp = 0;
    item->kind = 4;
    item->templateId = 0;
    item->hitTestHalfWidth = 0;
    item->hitTestHalfHeight = 0;
    item->dropTick = state->tick; 
    item->jumpEffectTick = -1;
    

    
}


void DropInventoryItemToGround(DemoState *state, int index, long clickWorldX, long clickWorldY)
{
    if (index < 0 || index >= state->inventoryCount)
        return;
    const InventorySlot *inv = &state->inventory[index];

    double dirDX = (double)(clickWorldX - state->playerX), dirDY = (double)(clickWorldY - state->playerY);
    double clickDist = sqrt(dirDX * dirDX + dirDY * dirDY);
    if (clickDist > 1.0)
    {
        dirDX /= clickDist;
        dirDY /= clickDist;
    }
    else
        FacingDirectionToDelta(state->playerFacingDirection, &dirDX, &dirDY);
    double dropDist = clickDist < (double)ITEM_DROP_DISTANCE_WORLD_UNITS ? clickDist
                                                                          : (double)ITEM_DROP_DISTANCE_WORLD_UNITS;
    long wantX = state->playerX + (long)(dirDX * dropDist);
    long wantY = state->playerY + (long)(dirDY * dropDist);
    long dropX, dropY;
    RKC_RPGSCRN_GROUND_SweepMove(&state->ground, state->playerX, state->playerY, wantX, wantY, &dropX, &dropY);

    WorldItem *grown = realloc(state->worldItems, sizeof(WorldItem) * (size_t)(state->worldItemCount + 1));
    if (!grown)
        return;
    state->worldItems = grown;

    
    int isGoldSlot = inv->kind == 4;

    WorldItem *item = &state->worldItems[state->worldItemCount++];
    item->x = dropX;
    item->y = dropY;
    item->isGold = isGoldSlot;
    item->amount = isGoldSlot ? inv->count : 1;
    item->resolved = 1;
    item->pickedUp = 0;
    item->kind = inv->kind;
    item->templateId = inv->templateId;
    item->hitTestHalfWidth = 0;
    item->hitTestHalfHeight = 0;
    item->dropTick = state->tick;
    item->jumpEffectTick = -1;
    

    if (isGoldSlot)
    {
        
        RemoveInventorySlotEntirely(state, index);
        RecomputePlayerGold(state);
    }
    else
    {
        
        RemoveOneFromInventorySlot(state, index);
    }
}
