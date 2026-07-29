#ifndef SFDE_GROUND_STATE_H
#define SFDE_GROUND_STATE_H

#include "App.h"
#include "RKC_DBFCONTROL.h"
#include "RKC_DIB.h"
#include "RKC_RPGSCRN_CAF.h"
#include "RKC_RPGSCRN_GROUND.h"
#include "RKC_RPGSCRN_MCT.h"
#include "RKC_RPGSCRN_OBJECT.h"
#include "RKC_RPG_AICONTROL.h"
#include "RKC_RPG_AI_EXEC.h"
#include "RKC_RPG_ITEMDATA.h"
#include "RKC_RPG_SCRIPT.h"
#include "RKC_RPG_SCRIPT_EXEC.h"
#include "RKC_RPG_TABLE.h"
#include "RKC_UPDIB.h"
#include "RKC_UPDIB_SET.h"


#include "RKC_DSOUND.h"
#define GROUNDDEMO_AUDIO 1

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "config/ai_tuning.h"
#include "config/anim_tuning.h"
#include "config/combat_tuning.h"
#include "config/hud_tuning.h"
#include "config/movement_tuning.h"
#include "config/progression_tuning.h"
#include "config/ui_world_tuning.h"


static const double ENEMY_ATTACK_SPEED_TABLE[10] = {0.3, 0.4, 0.6, 0.8, 1.0, 1.5, 2.0, 2.5, 3.0, 4.0};


enum
{
    PLAYER_STAT_ATK_SPEED = 0,      
    PLAYER_STAT_WALK_SPEED = 1,     
    PLAYER_STAT_MAX_HP = 2,         
    PLAYER_STAT_MAX_MP = 3,         
    PLAYER_STAT_STRENGTH = 4,       
    PLAYER_STAT_ATTACK = 5,         
    PLAYER_STAT_DEFENCE = 6,        
    PLAYER_STAT_MAGIC_ATTACK = 7,   
    PLAYER_STAT_MAGIC_DEFENCE = 8,  
    PLAYER_STAT_HIT_RATE = 9,       
    PLAYER_STAT_EVASION = 10,       
    PLAYER_STAT_MAGIC_HIT = 11,     
    PLAYER_STAT_MAGIC_EVASION = 12, 
    PLAYER_STAT_COUNT = 13
};

static const long ENEMY_ATTACK_SWING_SOUND[25] = {-1, -1, -1, -1, -1, -1, -1, -1, 113,
                                                  -1, -1, -1, 116, 113, 131, 129, 146, 123,
                                                  140, 3, 3, 155, -1, 147, 134};
static const long ENEMY_DEATH_SOUND[25] = {-1, 105, 104, 107, 106, 104, -1, -1, 114,
                                           111, 118, 118, 117, 114, 133, 130, 148, 148,
                                           142, -1, -1, 151, 111, 114, 136};

typedef struct
{
    char text[DIALOG_LINE_MAX];
    long characterNo;
} DialogQueueEntry;

typedef struct
{
    int active;
    long effectId;
    int templateIndex; 
    long x, y;         
    long followCharacterNo;
    long direction; 
    unsigned long spawnTick;
} ScriptEffect;


typedef struct
{
    int active;
    long fieldScenarioId, fieldX, fieldY; 
    long townScenarioId, townX, townY;    
    unsigned long spawnTick;              
    int playerInsideLastTick;             
} TransportCircle;

typedef enum
{
    STATUS_MAGIC_TAB_STATUS,
    STATUS_MAGIC_TAB_MAGIC,
} StatusMagicTab;




typedef enum
{
    DRAG_NONE = 0,
    DRAG_INVENTORY,
    DRAG_EQUIP_WEAPON,
    DRAG_EQUIP_ARMOR,
    DRAG_EQUIP_ACCESSORY
} DragSourceKind;

typedef enum
{
    LIVE_SPAWN_SPRITE_NONE,   
    LIVE_SPAWN_SPRITE_STATIC, 
    LIVE_SPAWN_SPRITE_CAF,    
} LiveSpawnSpriteKind;


typedef struct
{
    int block;   
    long field8; 
    LiveSpawnSpriteKind kind;
    RKC_UPDIB staticNjp;        
    RKC_RPGSCRN_CAF caf;        
    RKC_UPDIB animNjp, animSdw; 
} LiveSpawnTemplate;


typedef struct
{
    long x, y;                         
    int block;                         
    long characterNo;                  
    long field8;                       
    int templateIndex;                 
    long patternIndex;                 
    const char *name;                  
    int hasCheckTrigger;               
    int isCCheckTarget;                
    long block1Height;                 
    int block1Walkable;                
    int block1InitialVisible;          
    long aiListNo;                     
    const char *aiListName;            
    RKC_RPG_AI_EXEC_State aiState;     
    long attackCooldownTicks;          
    long rectL, rectT, rectR, rectB;   
    long baseDamage;                   
    long lootTableRow;                 
    long monsterLevel;                 
    long speedPercent;                 
    long moveSpeedPercent;             
    long attackSpeedIndex;             
    long attackChartIndex;             
    unsigned long attackAnimTick;      
    int hasPendingHit;                 
    unsigned long pendingHitTick;      
    int facingDirection;               
    int isMoving;                      
    int slideDir;                      
    unsigned long deathTick;           
    unsigned long hitStunTick;         
    long hitStunDurationTicks;         
    unsigned long hitVfxTick;          
    int hitVfxVariant;                 
    long hitTestOffsetY;               
    long hitTestHalfWidth;             
    long hitTestHalfHeight;            
    const unsigned int *cellBlockMask; 
    long cellBlockMaskCount;           
    const unsigned short *cellTintR;   
    const unsigned short *cellTintG;   
    const unsigned short *cellTintB;   
    long actionAnimChart;              
    unsigned long actionAnimTick;      
    long cafHeightCache;               
    int cafHeightCacheChart;           
    int cafHeightCacheDirection;
    int cafHeightCacheValid;         
    long gateHighlightFadeTicks;     
    char gateDestinationName[0x100]; 
    
    long npcWanderL, npcWanderT, npcWanderR, npcWanderB;
    long npcWanderTargetX, npcWanderTargetY; 
    int npcHasWanderTarget;
    unsigned long npcWanderPauseUntilTick; 
    
    double npcWanderBestDist;
    unsigned long npcWanderBestDistTick;
    
    int npcFacesTalker;
    unsigned long talkAnimTick; 
} LiveSpawn;


typedef struct
{
    long x, y;      
    int isGold;     
    long amount;    
    char name[128]; 
    int resolved;   
    int pickedUp;
    long kind, templateId; 
    
    long hitTestHalfWidth, hitTestHalfHeight;
    
    unsigned long dropTick;
    long jumpEffectTick;
} WorldItem;


typedef struct
{
    long x, y;
    long currentHP;
    int isDead;
} LiveSpawnSaveEntry;


typedef struct
{
    int pickedUp;
    long x, y; 
    int isGold;
    long amount;
    char name[128];
    int resolved;
    long kind, templateId;
} WorldItemSaveEntry;


typedef struct
{
    long x, y;   
    int variant; 
    unsigned long spawnTick;
    int active;
} BloodDecal;


typedef struct
{
    long scenarioId;

    LiveSpawnSaveEntry *liveSpawnSaves;
    long liveSpawnSaveCount;

    WorldItemSaveEntry *worldItemSaves;
    long worldItemSaveCount;
    
    long worldItemBaseCountAtSave;

    
    long *netFlagValues;
    long netFlagCount;
    
    long *characterFlagValues;
    long characterFlagCount;
} ScenarioSaveState;


typedef enum
{
    PENDING_ACTION_NONE,
    PENDING_ACTION_TALK,
    PENDING_ACTION_ATTACK,
    PENDING_ACTION_PICKUP,
} PendingActionKind;


typedef enum
{
    SLIDE_NONE = 0,
    SLIDE_NORTH = 1, 
    SLIDE_SOUTH = 2, 
    SLIDE_WEST = 3,  
    SLIDE_EAST = 4,  
} SlideDir;


typedef struct
{
    char name[128];
    long count;
    long kind, templateId;
    long gridWidth, gridHeight;
    long gridCol, gridRow;
} InventorySlot;

typedef struct
{
    char mapRoot[1024]; 
    RKC_DBFCONTROL dbf;
    RKC_DIB canvas;
    
    RKC_DIB darknessMask;
    int displayDarknessEnabled; 
    RKC_RPGSCRN_GROUND ground;
    RKC_RPGSCRN_OBJECTBLOCK objects;
    long *objectDrawOrder; 
    long objectDrawOrderCount;
    RKC_UPDIB_SET patternSet;  
    long cameraX, cameraY;     
    long playerX, playerY;     
    int playerFacingDirection; 
    int playerIsMoving;        
    long playerCafHeightCache; 
    int playerCafHeightCacheChart;
    int playerCafHeightCacheDirection;
    int playerCafHeightCacheValid;                     
    long playerIntendedDX, playerIntendedDY;           
    int playerHasIntendedDir;                          
    double playerVirtualScreenX, playerVirtualScreenY; 
    int playerVirtualScreenValid;                      
    int playerAttacking;                               
    unsigned long playerAttackTick;                    
    int playerCasting;                                 
    unsigned long playerCastTick;                      
    unsigned long playerCastEndTick;                   
    unsigned long playerHitTick;                       
    int playerHitVfxVariant;                           
    unsigned long playerShakeTick;                     
    double playerShakeDirX, playerShakeDirY;           
    long playerHP, playerMaxHP;                        
    
    long playerMP, playerMaxMP;
    
    long playerLevel;
    
    long playerClass;
    long playerBaseStats[PLAYER_STAT_COUNT];
    unsigned char playerClassHistory[PLAYER_LEVEL_CAP];
    int progressionInitialized;
    
    long playerExp;
    
    int levelUpNoticeActive;
    long levelUpNoticeArmedTick;
    long levelUpNoticeLevel;
    long levelUpNoticeGains[PLAYER_STAT_COUNT];
    long levelUpNoticeNewClass;
    char levelUpNoticeSkill[48];
    
    long playerChangeToClass;
    long classKillUsage[CLASS_KILL_USAGE_SLOTS];
    long classKillTotal;
    int classChangeClickLatch;
    int classChangeDebugKeyWasDown; 
    
    long questBannerIndex;
    unsigned long questBannerUntilTick;
    
    int questWindowOpen;
    int questWindowTab; 
    
    long questTooltipIndex;
    int questTooltipClickLatch; 
    
    long hoveredInventorySlot;      
    long itemTooltipHoverKey;       
    unsigned long itemTooltipHoverStartTick;
    
    int gateWindowOpen;
    int gateWindowPage;     
    int gatePageClickLatch; 
    int gateOpenClickLatch; 
    long gateHoveredRow;    
    
    long gateTeleportPendingRow;
    unsigned long gateTeleportAtTick;
    
    int pendingWarpValid;
    long pendingWarpX, pendingWarpY;
    int playerIsDead;                                  
    RKC_RPGSCRN_MCT mct;
    int mctLoaded;
    
    RKC_UPDIB minimapBg;
    int minimapBgLoaded;
    RKC_RPG_SCRIPT script;
    int scriptLoaded;
    RKC_RPG_SCRIPT_EXEC_State execState; 
    RKC_RPG_AICONTROL aiControl;         
    int aiControlLoaded;
    RKC_RPG_ITEMDATA itemData; 
    int itemDataLoaded;
    RKC_RPG_TABLE table; 
    int tableLoaded;
    char characterRoot[1024]; 
    LiveSpawnTemplate templates[LIVE_SPAWN_TEMPLATE_CACHE_MAX];
    int templateCount;
    LiveSpawnTemplate playerTemplate; 
    int playerIsFemale;               
    RKC_UPDIB hudBar;                 
    int hudBarLoaded;
    RKC_UPDIB font; 
    int fontLoaded;
    RKC_UPDIB hukidasi; 
    int hukidasiLoaded;
    RKC_UPDIB statusSheet; 
    int statusSheetLoaded;
    RKC_UPDIB statusIconSheet; 
    int statusIconSheetLoaded;
    RKC_UPDIB magicBarIcon; 
    int magicBarIconLoaded;
    RKC_UPDIB darknessTemplate; 
    int darknessTemplateLoaded;
    
    RKC_UPDIB itemIconSheets[ITEM_ICON_SHEET_COUNT];
    int itemIconSheetLoaded[ITEM_ICON_SHEET_COUNT];
    
    RKC_UPDIB weaponWorldSheets[WEAPON_WORLD_SHEET_COUNT];
    int weaponWorldSheetLoaded[WEAPON_WORLD_SHEET_COUNT];
    RKC_UPDIB cursorSheet; 
    int cursorSheetLoaded;
    RKC_UPDIB waitingSheet; 
    int waitingSheetLoaded;
    int loadingScreenActive;                                  
    unsigned long loadingScreenStartTick;                     
    int loadingScreenSkipHold;                                
    LiveSpawnTemplate compassTemplate;                        
    LiveSpawnTemplate unlockSwTemplate;                       
    LiveSpawnTemplate hitVfxTemplates[HIT_VFX_VARIANT_COUNT]; 
    LiveSpawnTemplate gateRingTemplate;                       
    LiveSpawnTemplate transportCircleTemplate;                
    TransportCircle transportCircle;                          
    int transportCircleHovered;                               
    int compassActive;                                        
    unsigned long compassActivatedTick;                       
    char compassText[256];                                    
    
    DialogQueueEntry dialogQueue[DIALOG_QUEUE_MAX];
    int dialogQueueCount;
    int dialogActive;
    int dialogHoveredOption;
    int dialogClickWasDown, dialogRightClickWasDown, dialogEscKeyWasDown;
    
    long deferredTalkEndCharacterNo;
    char deferredTalkEndLabel[256];
    
    RKC_RPG_SCRIPT_EXEC_Globals scriptGlobals;
    int scriptGlobalsValid;
    
    long talkEndChainDepth;
    
    struct
    {
        long characterNo, offsetX, offsetY, value, r, g, b;
    } floatingValues[FLOATING_VALUE_MAX];
    int floatingValueCount;
    
    ScriptEffect scriptEffects[SCRIPT_EFFECT_MAX];
    LiveSpawnTemplate scriptEffectTemplates[SCRIPT_EFFECT_TEMPLATE_CACHE_MAX];
    long scriptEffectTemplateIds[SCRIPT_EFFECT_TEMPLATE_CACHE_MAX];
    int scriptEffectTemplateCount;
    
    long unlockSwValue;
    unsigned long unlockSwSetTick;
    LiveSpawn *liveSpawns; 
    long liveSpawnCount;
    long hoveredSpawnIndex;     
    int mouseLeftDown;          
    long hoveredWorldItemIndex; 
    WorldItem *worldItems;      
    long worldItemCount;
    
    BloodDecal bloodDecals[BLOOD_DECAL_MAX];
    int bloodDecalNextSlot;
    RKC_UPDIB bloodDecalSheet;
    int bloodDecalSheetLoaded;
    
    long baseWorldItemCount;
    
    ScenarioSaveState *scenarioSaves;
    long scenarioSaveCount;
    InventorySlot inventory[INVENTORY_MAX];
    int inventoryCount;
    long gold;
    
    int hasWeapon;
    RKC_RPG_ITEMDATA_Kind0Instance weapon; 
    char weaponName[128];
    int hasArmor[EQUIPMENT_ARMOR_SLOTS];
    RKC_RPG_ITEMDATA_Kind1Instance armor[EQUIPMENT_ARMOR_SLOTS]; 
    char armorName[EQUIPMENT_ARMOR_SLOTS][128];
    int hasAccessory[EQUIPMENT_ACCESSORY_SLOTS];
    RKC_RPG_ITEMDATA_Kind2Instance accessory[EQUIPMENT_ACCESSORY_SLOTS]; 
    char accessoryName[EQUIPMENT_ACCESSORY_SLOTS][128];
    long currentEntryPoint; 
    long currentScenarioId; 
    int transitionPending;  
    long pendingScenarioId;
    long pendingEntryPoint;
    int interactKeyWasDown;                                
    int attackKeyWasDown;                                  
    int rightClickWasDown;                                 
    int leftClickWasDown;                                  
    int heldClickIsSpawnTarget;                            
    int playerSwingIsCombo;                                
    int playerComboHitsApplied;                            
    int playerComboLungePhase;                             
    long playerComboLungeAnchorX, playerComboLungeAnchorY; 
    int minimapOpen;                                       
    int minimapKeyWasDown;                                 
    int displayDarknessKeyWasDown;                         
    
    double clickRangeTileHeight;
    int clickRangeSquareVisible;      
    int clickRangeSquareKeyWasDown;   
    int clickRangeIncreaseKeyWasDown; 
    int clickRangeDecreaseKeyWasDown; 
    
    int runToggled;
    int runToggleKeyWasDown; 
    int inventoryOpen;       
    int inventoryKeyWasDown; 
    int questKeyWasDown;     
    int levelDebugKeyWasDown; 
    
    int statusMagicOpen;
    StatusMagicTab statusMagicTab;
    int statusKeyWasDown;
    int magicKeyWasDown;
    
    int noclip;
    int noclipKeyWasDown; 
    
    int judgeOverlay;
    int judgeOverlayKeyWasDown; 
    
    long debugItemCycleIndex[DEBUG_ITEM_CYCLE_COUNT];
    int debugItemCycleKeyWasDown[DEBUG_ITEM_CYCLE_COUNT];
    
    int equipGridDebug;
    int equipGridDebugKeyWasDown; 
    
    int inventoryEquipClickWasDown;
    
    int dragActive;
    DragSourceKind dragSourceKind;
    int dragSourceIndex;
    long dragCurrentMouseX, dragCurrentMouseY;
    
    int hasMoveTarget;
    long moveTargetX, moveTargetY; 
    PendingActionKind pendingActionKind;
    long pendingActionCharacterNo;             
    long pendingActionWorldItemIndex;          
    long playerAttackCooldownTicks;            
    int playerHasPendingHit;                   
    long playerPendingHitCharacterNo;          
    unsigned long playerPendingHitResolveTick; 
    
    int slideDir, slideFallback, slidePrimary;
    
    long visitedX[VISITED_HISTORY_SIZE], visitedY[VISITED_HISTORY_SIZE];
    int visitedHead, visitedCount;
    
    int pathfindCheckValid;
    double pathfindBestDist;
    long pathfindBestDistTick;
    int pathfindActive;
    long pathWaypointsX[PATHFIND_MAX_WAYPOINTS], pathWaypointsY[PATHFIND_MAX_WAYPOINTS];
    int pathWaypointCount, pathWaypointIndex;
    
    long pathfindTargetX, pathfindTargetY;
    unsigned long tick; 

    
    int magicBarSlot[MAGIC_BAR_SLOT_COUNT];
    int selectedMagicSlot;
    int magicSlotClickWasDown;
#ifdef GROUNDDEMO_AUDIO
    RKC_DSOUND dsound;
    int bgmLoaded;
    long bgmHandle;
    
    long currentBgmIndex;
    int bgmMuted;
    int sfxLoaded;
    int digitWasDown[10]; 
    int muteKeyWasDown;
#endif
} DemoState;

#endif

