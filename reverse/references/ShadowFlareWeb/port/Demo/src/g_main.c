
#include "state.h"
#include "render.h"
#include "paths.h"
#include "sprites.h"
#include "inventory.h"
#include "script_bridge.h"
#include "combat.h"
#include "movement.h"
#include "scenario.h"


typedef enum
{
    DRAW_KIND_PROP,
    DRAW_KIND_LIVE_SPAWN,
    DRAW_KIND_PLAYER,
    DRAW_KIND_WORLD_ITEM,
    DRAW_KIND_TRANSPORT_CIRCLE 
} DrawItemKind;

typedef struct
{
    long depth;
    DrawItemKind kind;
    
    int isCorpse;
    long liveSpawnIndex; 
    long worldItemIndex; 
    long propDestX, propDestY; 
    const RKC_DIB *propIcon;
    long propTrans;
    
    long propShadowDestX, propShadowDestY;
    const RKC_DIB *propShadowIcon;
} DrawItem;

static int CompareDrawItem(const void *a, const void *b)
{
    const DrawItem *ia = (const DrawItem *)a, *ib = (const DrawItem *)b;
    
    if (ia->isCorpse != ib->isCorpse)
        return ia->isCorpse ? -1 : 1;
    if (ia->depth < ib->depth)
        return -1;
    if (ia->depth > ib->depth)
        return 1;
    return 0;
}


static void DrawPlayer(DemoState *state, const Uint8 *keys)
{
    
    long shakeWorldX = state->playerX, shakeWorldY = state->playerY;
    long shakeElapsed = (long)(state->tick - state->playerShakeTick);
    if ((state->playerShakeDirX != 0.0 || state->playerShakeDirY != 0.0) && shakeElapsed < PLAYER_SHAKE_DURATION_TICKS)
    {
        long remaining = PLAYER_SHAKE_DURATION_TICKS - shakeElapsed;
        long magnitude = PLAYER_SHAKE_MAGNITUDE_WORLD_UNITS * remaining / PLAYER_SHAKE_DURATION_TICKS;
        long sign = (shakeElapsed % 2 == 0) ? 1 : -1;
        shakeWorldX += (long)(state->playerShakeDirX * magnitude * sign);
        shakeWorldY += (long)(state->playerShakeDirY * magnitude * sign);
    }

    long screenX, screenY;
    WorldToScreen(state, shakeWorldX, shakeWorldY, &screenX, &screenY);
    long destX = screenX - state->cameraX;
    long destY = screenY - state->cameraY;
    long hitVfxCenterY = destY; 
    if (state->playerTemplate.kind == LIVE_SPAWN_SPRITE_CAF)
    {
        
        
        long attackFrame = -1;
        long attackChart = -1;
        if (state->playerAttacking)
        {
            long elapsedTicks = (long)(state->tick - state->playerAttackTick);
            
            double ticksPerFrame = ComputePlayerAttackAnimTicksPerFrame(state);
            long seqCharts[5];
            short seqFrames[5];
            int seqDealsDamage[5]; 
            int seqCount =
                ResolvePlayerSwingPhases(state, state->playerSwingIsCombo, seqCharts, seqFrames, seqDealsDamage);
            if (seqCount > 0)
            {
                long phaseStart = 0;
                for (int i = 0; i < seqCount; i++)
                {
                    long phaseDurationTicks = (long)((double)seqFrames[i] * ticksPerFrame);
                    if (elapsedTicks < phaseStart + phaseDurationTicks)
                    {
                        attackChart = seqCharts[i];
                        attackFrame = (long)((double)(elapsedTicks - phaseStart) / ticksPerFrame);
                        break;
                    }
                    phaseStart += phaseDurationTicks;
                }
            }
            else
            {
                long chart = ResolvePlayerAttackChart(state);
                if (chart < state->playerTemplate.caf.chartCount)
                {
                    int chartDir = ResolvePlayerChartDrawDirection(chart, state->playerFacingDirection);
                    const RKC_RPGSCRN_CAF_Direction *dir = &state->playerTemplate.caf.charts[chart].directions[chartDir];
                    if (dir->maxFrameCount > 0 && (double)elapsedTicks < (double)dir->maxFrameCount * ticksPerFrame)
                    {
                        attackChart = chart;
                        attackFrame = (long)((double)elapsedTicks / ticksPerFrame);
                    }
                }
            }
        }

        
        
        long castChart = -1;
        long castFrame = -1;
        if (state->playerCasting)
        {
            long castElapsed = (long)(state->tick - state->playerCastTick);
            int windDir = ResolvePlayerChartDrawDirection(PLAYER_CAST_CHART_WINDUP, state->playerFacingDirection);
            long windFrames =
                PLAYER_CAST_CHART_WINDUP < state->playerTemplate.caf.chartCount
                    ? state->playerTemplate.caf.charts[PLAYER_CAST_CHART_WINDUP].directions[windDir].maxFrameCount
                    : 0;
            long windDurationTicks = windFrames * PLAYER_CAST_ANIM_TICKS_PER_FRAME;
            if (windFrames > 0 && castElapsed < windDurationTicks)
            {
                castChart = PLAYER_CAST_CHART_WINDUP;
                castFrame = castElapsed / PLAYER_CAST_ANIM_TICKS_PER_FRAME;
            }
            else if (castElapsed < windDurationTicks + PLAYER_CAST_RELEASE_HOLD_TICKS &&
                     PLAYER_CAST_CHART_RELEASE < state->playerTemplate.caf.chartCount)
            {
                castChart = PLAYER_CAST_CHART_RELEASE;
                castFrame = 0; 
            }
        }

        int running = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL] || state->runToggled;
        long chart;
        if (attackFrame >= 0)
            chart = attackChart;
        else if (castFrame >= 0)
            chart = castChart;
        else if (state->dialogActive)
            
            chart = PLAYER_ATTACK_CHART_TWO_HANDED;
        else if (!state->playerIsMoving)
            chart = ResolveWeaponCategoryChart(state, 0, PLAYER_IDLE_CHART_AXE_BLUNT, PLAYER_IDLE_CHART_TWO_HANDED);
        else if (running)
            chart = ResolveWeaponCategoryChart(state, PLAYER_RUN_CHART, PLAYER_RUN_CHART_AXE_BLUNT,
                                               PLAYER_RUN_CHART_TWO_HANDED);
        else
            chart = ResolveWeaponCategoryChart(state, 1, PLAYER_WALK_CHART_HEAVY, PLAYER_WALK_CHART_HEAVY);
        if (attackFrame < 0 && castFrame < 0 && chart >= state->playerTemplate.caf.chartCount)
            chart = !state->playerIsMoving ? 0 : (running ? PLAYER_RUN_CHART : 1);
        long frame = attackFrame >= 0   ? attackFrame
                     : castFrame >= 0   ? castFrame
                                        : (long)(state->tick / TICKS_PER_ANIM_FRAME);
        RKC_RPGSCRN_CAF_DrawCmd cmds[8];
        
        int drawDirection = ResolvePlayerChartDrawDirection(chart, state->playerFacingDirection);
        
        unsigned int cellMask[PLAYER_MAX_CELL_BLOCKS];
        unsigned short cellTintR[PLAYER_MAX_CELL_BLOCKS], cellTintG[PLAYER_MAX_CELL_BLOCKS],
            cellTintB[PLAYER_MAX_CELL_BLOCKS];
        long cellBlockCount = state->playerTemplate.caf.charts[chart].directions[drawDirection].cellBlockCount;
        if (cellBlockCount > PLAYER_MAX_CELL_BLOCKS)
            cellBlockCount = PLAYER_MAX_CELL_BLOCKS;
        ComputePlayerCellBlockLayers(state, attackFrame >= 0, cellBlockCount, cellMask, cellTintR, cellTintG,
                                     cellTintB);
        int n = RKC_RPGSCRN_CAF_Resolve(&state->playerTemplate.caf, chart, drawDirection, frame,
                                        &state->playerTemplate.animNjp, &state->playerTemplate.animSdw, cellMask,
                                        cellTintR, cellTintG, cellTintB, cellBlockCount, cmds, 8);
        
        long spriteDestY = destY - CafSpriteYOffsetCached(&state->playerCafHeightCache, &state->playerCafHeightCacheChart,
                                                           &state->playerCafHeightCacheDirection,
                                                           &state->playerCafHeightCacheValid, chart, drawDirection, n,
                                                           cmds);
        hitVfxCenterY = destY - TallestCafIconHeight(n, cmds) / 2;
        
        for (int i = 0; i < n; i++)
        {
            const RKC_DIB *icon = cmds[i].icon;
            if (cmds[i].isAdditive)
                
                RKC_DIB_TransferToDIBAdditiveTint(&state->canvas, destX + cmds[i].offsetX,
                                                  spriteDestY + cmds[i].offsetY, icon->width, icon->height, icon, 0,
                                                  0, 0, cmds[i].trans, cmds[i].tintR, cmds[i].tintG, cmds[i].tintB);
            else
                RKC_DIB_TransferToDIBTint(&state->canvas, destX + cmds[i].offsetX, spriteDestY + cmds[i].offsetY,
                                          icon->width, icon->height, icon, 0, 0, 0, cmds[i].trans, cmds[i].tintR,
                                          cmds[i].tintG, cmds[i].tintB);
        }
    }
    else
        DrawMarker(&state->canvas, destX, destY, 3, 255, 255, 255);

    
    DrawHitVfx(state, destX, hitVfxCenterY, state->playerHitTick, state->playerHitVfxVariant);
}

static void OnRender(void *userData)
{
    DemoState *state = (DemoState *)userData;

    state->tick++;

    
    state->floatingValueCount = 0;

    
    if (state->transitionPending)
    {
        state->transitionPending = 0;
        state->loadingScreenActive = 1;
        state->loadingScreenStartTick = state->tick;
        
        state->loadingScreenSkipHold = state->pendingScenarioId == state->currentScenarioId;

#ifdef GROUNDDEMO_AUDIO
        
        char scenarioPathPeek[1024];
        if (state->dsound.initialized && state->bgmLoaded &&
            DeriveScenarioPath(state->mapRoot, state->pendingScenarioId, scenarioPathPeek, sizeof(scenarioPathPeek)))
        {
            char mctPathPeek[1024];
            
            RKC_RPGSCRN_MCT peekMct;
            RKC_RPGSCRN_MCT_Init(&peekMct);
            if (RKC_RPGSCRN_MCT_Read(&peekMct, mctPathPeek) && peekMct.bgmIndex != state->currentBgmIndex)
                RKC_DSOUND_SetVolume(&state->dsound, 0 , state->bgmHandle, -10000);
            RKC_RPGSCRN_MCT_Release(&peekMct);
        }
#endif
    }

    if (state->loadingScreenActive)
    {
        if (!state->loadingScreenSkipHold && state->tick - state->loadingScreenStartTick < LOADING_SCREEN_MIN_TICKS)
        {
            DrawLoadingScreen(state);
            
            RKC_DBFCONTROL_Present(&state->dbf, App_GetRenderer(), &state->canvas);
            return;
        }

        
        long scenarioId = state->pendingScenarioId, entryPoint = state->pendingEntryPoint;
        state->loadingScreenActive = 0;
        char scenarioPath[1024];
        if (DeriveScenarioPath(state->mapRoot, scenarioId, scenarioPath, sizeof(scenarioPath)))
        {
            
            if (!LoadScenario(state, scenarioPath, entryPoint))
                printf("warning: failed to load scenario %s\n", scenarioPath);
            else if (state->pendingWarpValid)
            {
                
                state->playerX = state->pendingWarpX;
                state->playerY = state->pendingWarpY;
                
            }
        }
        else
            printf("warning: unable to derive scenario %08ld\n", scenarioId);
        state->pendingWarpValid = 0; 
    }

    if (state->mctLoaded)
    {
        TickLiveSpawnsAI(state);
        TickNpcWander(state);
    }

    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    
    int dialogWasActive = state->dialogActive;
    
    int mouseX, mouseY;
    Uint32 buttons = SDL_GetMouseState(&mouseX, &mouseY);
    int leftRawDown = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    int rightRawDown = (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    int escDown = keys[SDL_SCANCODE_ESCAPE];
    state->mouseLeftDown = leftRawDown; 
    if (dialogWasActive)
    {
        int leftPressed = leftRawDown && !state->dialogClickWasDown;
        int rightPressed = rightRawDown && !state->dialogRightClickWasDown;
        int escPressed = escDown && !state->dialogEscKeyWasDown;

        if (rightPressed || escPressed)
            CloseDialog(state); 
        else if (leftPressed)
        {
            if (DialogHasOptions(state))
            {
                
                if (FindDialogOptionAtScreenPoint(state, mouseX, mouseY) >= 0)
                    AdvanceDialog(state);
            }
            else
                AdvanceDialog(state); 
        }

        
        state->dialogHoveredOption = DialogHasOptions(state) ? FindDialogOptionAtScreenPoint(state, mouseX, mouseY) : -1;
    }
    else
    {
        state->dialogHoveredOption = -1;
        
        int escPressed = escDown && !state->dialogEscKeyWasDown;
        if (escPressed)
            CloseAllWindows(state);
    }
    state->dialogClickWasDown = leftRawDown;
    state->dialogRightClickWasDown = rightRawDown;
    state->dialogEscKeyWasDown = escDown;

    
    {
        int mouseX, mouseY;
        Uint32 buttons = SDL_GetMouseState(&mouseX, &mouseY);
        int leftRawDown = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
        int leftDown = leftRawDown && !IsScreenPointOverUI(state, mouseX, mouseY);
        
        int isFreshLeftPress = leftDown && !state->leftClickWasDown;
        if (isFreshLeftPress && state->dragActive)
            state->heldClickIsSpawnTarget = 1;
        else if (leftDown && !dialogWasActive && !state->dragActive)
            HandleClick(state, mouseX, mouseY, isFreshLeftPress);
        state->leftClickWasDown = leftDown;

        
        if (state->inventoryOpen && state->statusSheetLoaded)
        {
            
            if (state->equipGridDebug)
            {
                if (leftRawDown && !state->inventoryEquipClickWasDown)
                {
                    
                    fflush(stdout);
                }
            }
            else
            {
                if (leftRawDown && !state->inventoryEquipClickWasDown)
                {
                    if (!state->dragActive)
                        BeginInventoryDrag(state, mouseX, mouseY);
                    else
                        ResolveInventoryDrag(state, mouseX, mouseY);
                }
                if (state->dragActive)
                {
                    state->dragCurrentMouseX = mouseX;
                    state->dragCurrentMouseY = mouseY;
                }
            }
        }
        else
        {
            state->dragActive = 0;
        }
        state->inventoryEquipClickWasDown = leftRawDown;

        
        int rightDown = (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) && !IsScreenPointOverUI(state, mouseX, mouseY);
        
        if (rightDown && !state->rightClickWasDown && !dialogWasActive)
        {
            
            if (state->selectedMagicSlot != MAGIC_SLOT_ATTACK_INDEX &&
                state->magicBarSlot[state->selectedMagicSlot] >= 0)
            {
                int spellId = state->magicBarSlot[state->selectedMagicSlot];
                if (spellId == SPELL_ID_TRANSPORT)
                {
                    
                    if (state->playerIsDead || state->playerAttackCooldownTicks > 0)
                    {
                        
                    }
                    else if (IsTownScenario(state->currentScenarioId))
                    {
                        
                        fflush(stdout);
                    }
                    else if (TrySpendMP(state, spellId))
                        CastTransport(state, mouseX, mouseY);
                    else
                    {
                        
                        fflush(stdout);
                    }
                }
                else if (TrySpendMP(state, spellId))
                {
                    
                    fflush(stdout);
                }
                else
                {
                    
                    fflush(stdout);
                }
            }
            else
                HandleComboAttack(state, mouseX, mouseY);
        }
        state->rightClickWasDown = rightDown;

        
        {
            int magicSlot = FindMagicSlotAtScreenPoint(state, mouseX, mouseY);
            if (leftRawDown && !state->magicSlotClickWasDown && magicSlot >= 0 && !dialogWasActive)
                state->selectedMagicSlot = magicSlot;
            state->magicSlotClickWasDown = leftRawDown;
        }

        
        state->hoveredSpawnIndex = (state->mctLoaded && !IsScreenPointOverUI(state, mouseX, mouseY))
                                        ? FindHoveredSpawn(state, mouseX + state->cameraX, mouseY + state->cameraY)
                                        : -1;
        
        state->hoveredWorldItemIndex = (state->mctLoaded && state->hoveredSpawnIndex < 0 &&
                                        !IsScreenPointOverUI(state, mouseX, mouseY))
                                           ? FindWorldItemNearScreenPoint(state, mouseX + state->cameraX,
                                                                          mouseY + state->cameraY)
                                           : -1;

        
        state->transportCircleHovered = 0;
        if (!IsScreenPointOverUI(state, mouseX, mouseY))
        {
            long tcHoverX, tcHoverY;
            if (ActiveTransportCircleHere(state, &tcHoverX, &tcHoverY, NULL))
            {
                long tcScreenX, tcScreenY;
                WorldToScreen(state, tcHoverX, tcHoverY, &tcScreenX, &tcScreenY);
                long hdx = mouseX - (tcScreenX - state->cameraX);
                long hdy = mouseY - (tcScreenY - state->cameraY);
                if (hdx * hdx + hdy * hdy <=
                    TRANSPORT_CIRCLE_HOVER_RADIUS_PX * TRANSPORT_CIRCLE_HOVER_RADIUS_PX)
                    state->transportCircleHovered = 1;
            }
        }
    }

    
    
    if (!dialogWasActive && (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_UP] ||
                             keys[SDL_SCANCODE_DOWN]))
    {
        state->hasMoveTarget = 0;
        state->pendingActionKind = PENDING_ACTION_NONE;
        state->slideDir = SLIDE_NONE;
        state->visitedCount = 0; 
        state->pathfindCheckValid = 0;
        state->pathfindActive = 0;
    }
    {
        
        long prevPlayerX = state->playerX, prevPlayerY = state->playerY;
        state->playerHasIntendedDir = 0;
        
        if (!dialogWasActive)
        {
            TickPendingAction(state, keys);
            MovePlayer(state, keys);
        }
        long dx = state->playerX - prevPlayerX, dy = state->playerY - prevPlayerY;
        
        state->playerIsMoving = state->playerHasIntendedDir ? 1 : (dx != 0 || dy != 0);
        
        if (state->playerHasIntendedDir && state->slideDir == SLIDE_NONE)
            state->playerFacingDirection =
                ResolveFacingDirection(state->playerIntendedDX, state->playerIntendedDY, state->playerFacingDirection);
        else
            state->playerFacingDirection = ResolveFacingDirection(dx, dy, state->playerFacingDirection);
    }
    UpdateCameraFromPlayer(state);

    
    if (state->mctLoaded)
        TickOnJudgeTriggers(state);

    
    TickTransportCircle(state);

    
    if (state->mctLoaded)
        TickGateHighlights(state);

    
    if (state->mctLoaded)
        TickExecFunctionTriggers(state);

    
    if (state->mctLoaded)
    {
        int down = keys[SDL_SCANCODE_SPACE];
        if (down && !state->interactKeyWasDown && !dialogWasActive)
            HandleInteract(state);
        state->interactKeyWasDown = down;
    }

    
    if (state->mctLoaded)
    {
        int down = keys[SDL_SCANCODE_F];
        if (down && !state->attackKeyWasDown && !dialogWasActive)
            HandleAttack(state);
        state->attackKeyWasDown = down;
    }

    
    TickPlayerCombo(state);
    
    TickPlayerPendingHit(state);

    
    {
        int down = keys[SDL_SCANCODE_N];
        if (down && !state->minimapKeyWasDown)
        {
            state->minimapOpen = !state->minimapOpen;
            if (state->minimapOpen)
            {
                state->statusMagicOpen = 0;
                state->questWindowOpen = 0; 
                state->questTooltipIndex = -1;
            }
        }
        state->minimapKeyWasDown = down;
    }

    
    {
        int down = keys[SDL_SCANCODE_F7];
        if (down && !state->displayDarknessKeyWasDown)
        {
            state->displayDarknessEnabled = !state->displayDarknessEnabled;
            
        }
        state->displayDarknessKeyWasDown = down;
    }

    
    {
        int down = keys[SDL_SCANCODE_S];
        if (down && !state->statusKeyWasDown)
        {
            if (state->statusMagicOpen && state->statusMagicTab == STATUS_MAGIC_TAB_STATUS)
                state->statusMagicOpen = 0;
            else
            {
                state->statusMagicOpen = 1;
                state->statusMagicTab = STATUS_MAGIC_TAB_STATUS;
                state->minimapOpen = 0;
                state->questWindowOpen = 0; 
                state->questTooltipIndex = -1;
            }
        }
        state->statusKeyWasDown = down;
    }

    
    {
        int down = keys[SDL_SCANCODE_M];
        if (down && !state->magicKeyWasDown)
        {
            if (state->statusMagicOpen && state->statusMagicTab == STATUS_MAGIC_TAB_MAGIC)
                state->statusMagicOpen = 0;
            else
            {
                state->statusMagicOpen = 1;
                state->statusMagicTab = STATUS_MAGIC_TAB_MAGIC;
                state->minimapOpen = 0;
                state->questWindowOpen = 0; 
                state->questTooltipIndex = -1;
            }
        }
        state->magicKeyWasDown = down;
    }

    
    {
        int down = keys[SDL_SCANCODE_R];
        if (down && !state->runToggleKeyWasDown)
            state->runToggled = !state->runToggled;
        state->runToggleKeyWasDown = down;
    }

    
    {
        int down = keys[SDL_SCANCODE_I];
        if (down && !state->inventoryKeyWasDown)
        {
            state->inventoryOpen = !state->inventoryOpen;
            if (state->inventoryOpen)
            {
                state->questWindowOpen = 0; 
                state->questTooltipIndex = -1;
            }
        }
        state->inventoryKeyWasDown = down;
    }

    
    {
        int down = keys[SDL_SCANCODE_Q];
        if (down && !state->questKeyWasDown)
        {
            state->questWindowOpen = !state->questWindowOpen;
            if (!state->questWindowOpen)
                state->questTooltipIndex = -1; 
            if (state->questWindowOpen)
            {
                state->minimapOpen = 0;
                state->statusMagicOpen = 0;
                state->inventoryOpen = 0;
            }
        }
        state->questKeyWasDown = down;
    }

    
    {
        int down = keys[SDL_SCANCODE_F10];
        if (down && !state->levelDebugKeyWasDown && state->progressionInitialized &&
            state->playerLevel < PLAYER_LEVEL_CAP)
        {
            PlayerLevelUp(state);
            
        }
        state->levelDebugKeyWasDown = down;
    }

    
    {
        int down = keys[SDL_SCANCODE_F11];
        if (down && !state->classChangeDebugKeyWasDown)
        {
            if (state->playerChangeToClass == PLAYER_CLASS_WARRIOR)
                state->playerChangeToClass = PLAYER_CLASS_WITCH;
            else if (state->playerChangeToClass == PLAYER_CLASS_WITCH)
                state->playerChangeToClass = PLAYER_CLASS_HUNTER;
            else
                state->playerChangeToClass = PLAYER_CLASS_WARRIOR;
            
        }
        state->classChangeDebugKeyWasDown = down;
    }

    
    {
        int down = keys[SDL_SCANCODE_C];
        if (down && !state->clickRangeSquareKeyWasDown)
        {
            state->clickRangeSquareVisible = !state->clickRangeSquareVisible;
            
            fflush(stdout);
        }
        state->clickRangeSquareKeyWasDown = down;
    }
    {
        int down = keys[SDL_SCANCODE_RIGHTBRACKET];
        if (down && !state->clickRangeIncreaseKeyWasDown)
        {
            state->clickRangeTileHeight += CLICK_RANGE_STEP_TILE_HEIGHT;
            if (state->clickRangeTileHeight > CLICK_RANGE_MAX_TILE_HEIGHT)
                state->clickRangeTileHeight = CLICK_RANGE_MAX_TILE_HEIGHT;
            
            fflush(stdout);
        }
        state->clickRangeIncreaseKeyWasDown = down;
    }
    {
        int down = keys[SDL_SCANCODE_LEFTBRACKET];
        if (down && !state->clickRangeDecreaseKeyWasDown)
        {
            state->clickRangeTileHeight -= CLICK_RANGE_STEP_TILE_HEIGHT;
            if (state->clickRangeTileHeight < CLICK_RANGE_MIN_TILE_HEIGHT)
                state->clickRangeTileHeight = CLICK_RANGE_MIN_TILE_HEIGHT;
            
            fflush(stdout);
        }
        state->clickRangeDecreaseKeyWasDown = down;
    }

    
    {
        int down = keys[SDL_SCANCODE_F10];
        if (down && !state->noclipKeyWasDown)
        {
            state->noclip = !state->noclip;
            
            fflush(stdout);
        }
        state->noclipKeyWasDown = down;
    }

    
    {
        int down = keys[SDL_SCANCODE_F9];
        if (down && !state->judgeOverlayKeyWasDown)
        {
            state->judgeOverlay = !state->judgeOverlay;
            
            fflush(stdout);
        }
        state->judgeOverlayKeyWasDown = down;
    }

    
    {
        int down = keys[SDL_SCANCODE_F8];
        if (down && !state->equipGridDebugKeyWasDown)
        {
            state->equipGridDebug = !state->equipGridDebug;
            
            fflush(stdout);
        }
        state->equipGridDebugKeyWasDown = down;
    }

#ifdef GROUNDDEMO_AUDIO
    
    if (state->sfxLoaded)
    {
        static const SDL_Scancode digitScancodes[10] = {
            SDL_SCANCODE_1,
            SDL_SCANCODE_2,
            SDL_SCANCODE_3,
            SDL_SCANCODE_4,
            SDL_SCANCODE_5,
            SDL_SCANCODE_6,
            SDL_SCANCODE_7,
            SDL_SCANCODE_8,
            SDL_SCANCODE_9,
            SDL_SCANCODE_0,
        };
        for (int d = 0; d < 10; d++)
        {
            int down = keys[digitScancodes[d]];
            if (down && !state->digitWasDown[d])
                RKC_DSOUND_Play(&state->dsound, 1 , d, 0 , 0 ,
                                0 );
            state->digitWasDown[d] = down;
        }
    }

    
    if (state->bgmLoaded)
    {
        int down = keys[SDL_SCANCODE_F11];
        if (down && !state->muteKeyWasDown)
        {
            state->bgmMuted = !state->bgmMuted;
            RKC_DSOUND_SetVolume(&state->dsound, 0 , state->bgmHandle,
                                 state->bgmMuted ? -10000 : 0);
        }
        state->muteKeyWasDown = down;
    }
#endif

    
    {
        struct
        {
            SDL_Scancode key;
            int cycleIndex;
            long kind;
            int slot; 
        } cycles[DEBUG_ITEM_CYCLE_COUNT] = {
            {SDL_SCANCODE_F1, DEBUG_ITEM_CYCLE_KIND_WEAPON, 0, -1},
            {SDL_SCANCODE_F2, DEBUG_ITEM_CYCLE_KIND_HELMET, 1, EQUIPMENT_HELMET_SLOT_INDEX},
            {SDL_SCANCODE_F3, DEBUG_ITEM_CYCLE_KIND_BODY, 1, EQUIPMENT_BODY_SLOT_INDEX},
            {SDL_SCANCODE_F4, DEBUG_ITEM_CYCLE_KIND_BOOTS, 1, EQUIPMENT_BOOTS_SLOT_INDEX},
            {SDL_SCANCODE_F5, DEBUG_ITEM_CYCLE_KIND_SHIELD, 1, EQUIPMENT_SHIELD_SLOT_INDEX},
            {SDL_SCANCODE_F6, DEBUG_ITEM_CYCLE_KIND_ACCESSORY, 2, 0},
        };
        for (int c = 0; c < DEBUG_ITEM_CYCLE_COUNT && state->itemDataLoaded; c++)
        {
            int down = keys[cycles[c].key];
            if (down && !state->debugItemCycleKeyWasDown[c])
            {
                long count = state->itemData.kinds[cycles[c].kind].count;
                long idx = state->debugItemCycleIndex[c];
                for (long tries = 0; tries < count; tries++)
                {
                    idx = (idx + 1) % count;
                    const RKC_RPG_ITEMDATA_Record *rec = &state->itemData.kinds[cycles[c].kind].records[idx];
                    if (cycles[c].kind == 1 && !ArmorNameFitsSlot(rec->name, cycles[c].slot))
                        continue;
                    state->debugItemCycleIndex[c] = idx;
                    
                    EquipItemIntoSlot(state, cycles[c].kind, rec->templateId,
                                      rec->name ? rec->name : "(unnamed item)", cycles[c].slot);
                    break;
                }
            }
            state->debugItemCycleKeyWasDown[c] = down;
        }
    }

    RKC_DIB_FillByte(&state->canvas, 32); 

    
    {
        long chipW = state->ground.chipWidth > 0 ? state->ground.chipWidth : 1;
        long chipH = state->ground.chipHeight > 0 ? state->ground.chipHeight : 1;
        long x0 = state->cameraX >= 0 ? state->cameraX / chipW : (state->cameraX - chipW + 1) / chipW;
        long y0 = state->cameraY >= 0 ? state->cameraY / chipH : (state->cameraY - chipH + 1) / chipH;
        long x1 = (state->cameraX + APP_WIDTH) / chipW;
        long y1 = (state->cameraY + APP_HEIGHT) / chipH;
        if (x0 < 0)
            x0 = 0;
        if (y0 < 0)
            y0 = 0;
        if (x1 >= state->ground.areaWidth)
            x1 = state->ground.areaWidth - 1;
        if (y1 >= state->ground.areaHeight)
            y1 = state->ground.areaHeight - 1;

        for (long y = y0; y <= y1; y++)
        {
            for (long x = x0; x <= x1; x++)
            {
                RKC_DIB *icon = RKC_RPGSCRN_GROUND_GetTileIcon(&state->ground, &state->patternSet, x, y);
                if (!icon)
                {
                    
                    icon = RKC_UPDIB_SET_GetPatternIcon(&state->patternSet, 0, 0);
                    if (!icon)
                        continue;
                }

                long screenX, screenY;
                RKC_RPGSCRN_GROUND_CellToScreen(&state->ground, x, y, &screenX, &screenY);
                RKC_DIB_TransferToDIBFast(&state->canvas, screenX - state->cameraX, screenY - state->cameraY,
                                          icon->width, icon->height, icon, 0, 0);
            }
        }
    }

    
    DrawBloodDecals(state);

    
    long playerScreenX, playerScreenY;
    WorldToScreen(state, state->playerX, state->playerY, &playerScreenX, &playerScreenY);
    long playerDestX = playerScreenX - state->cameraX;
    long playerDestY = playerScreenY - state->cameraY;
    long probeLeft = playerDestX - PLAYER_OCCLUSION_PROBE_HALF_WIDTH;
    long probeRight = playerDestX + PLAYER_OCCLUSION_PROBE_HALF_WIDTH;
    long probeTop = playerDestY - PLAYER_OCCLUSION_PROBE_HEIGHT;
    long probeBottom = playerDestY;
    long playerDepth = playerScreenY;

    
    
    DrawItem *drawItems = calloc((size_t)(state->objectDrawOrderCount + state->liveSpawnCount +
                                          state->worldItemCount + 1  +
                                          1 ),
                                 sizeof(DrawItem));
    if (drawItems)
    {
        long n = 0;

        
        for (long o = 0; o < state->objectDrawOrderCount; o++)
        {
            long i = state->objectDrawOrder[o];
            RKC_DIB *icon = RKC_RPGSCRN_OBJECTBLOCK_GetIcon(&state->objects, &state->patternSet, i);
            if (!icon)
                continue;
            const RKC_RPGSCRN_OBJECT_Entry *entry = RKC_RPGSCRN_OBJECTBLOCK_Get(&state->objects, i);

            
            long screenX, screenY;
            WorldToScreen(state, entry->posX, entry->posY, &screenX, &screenY);
            long offsetX, offsetY;
            RKC_RPGSCRN_OBJECTBLOCK_GetOffset(&state->objects, &state->patternSet, i, &offsetX, &offsetY);
            long destX = screenX - state->cameraX + offsetX;
            long destY = screenY - state->cameraY + offsetY;
            if (destX + icon->width < 0 || destX >= APP_WIDTH || destY + icon->height < 0 || destY >= APP_HEIGHT)
                continue;

            
            int isGroundDecal = (entry->field5 & OBL_FIELD5_GROUND_DECAL_BIT) != 0;
            long objectDepth = isGroundDecal ? GROUND_DECAL_DRAW_DEPTH : screenY + offsetY + icon->height;
            long normalTrans = entry->field4 > 0 ? entry->field4 : 1000;
            int occludesPlayer = !isGroundDecal && objectDepth >= playerDepth && destX < probeRight &&
                                  destX + icon->width > probeLeft && destY < probeBottom &&
                                  destY + icon->height > probeTop;
            if (occludesPlayer)
            {
                long x0 = probeLeft > destX ? probeLeft : destX;
                long x1 = probeRight < destX + icon->width ? probeRight : destX + icon->width;
                long y0 = probeTop > destY ? probeTop : destY;
                long y1 = probeBottom < destY + icon->height ? probeBottom : destY + icon->height;
                occludesPlayer = 0;
                for (long py = y0; py < y1 && !occludesPlayer; py++)
                    for (long px = x0; px < x1; px++)
                        if (RKC_DIB_GetPixelIndex(icon, px - destX, py - destY) > 0)
                        {
                            occludesPlayer = 1;
                            break;
                        }
            }

            drawItems[n].kind = DRAW_KIND_PROP;
            drawItems[n].depth = objectDepth;
            drawItems[n].propDestX = destX;
            drawItems[n].propDestY = destY;
            drawItems[n].propIcon = icon;
            drawItems[n].propTrans = occludesPlayer ? PLAYER_OCCLUSION_TRANS : normalTrans;

            
            drawItems[n].propShadowIcon = RKC_RPGSCRN_OBJECTBLOCK_GetShadowIcon(&state->objects, &state->patternSet, i);
            if (drawItems[n].propShadowIcon)
            {
                long shadowOffsetX, shadowOffsetY;
                RKC_RPGSCRN_OBJECTBLOCK_GetShadowOffset(&state->objects, &state->patternSet, i, &shadowOffsetX,
                                                        &shadowOffsetY);
                drawItems[n].propShadowDestX = screenX - state->cameraX + shadowOffsetX;
                drawItems[n].propShadowDestY = screenY - state->cameraY + shadowOffsetY;
            }
            n++;
        }

        if (state->mctLoaded)
        {
            
            for (long i = 0; i < state->worldItemCount; i++, n++)
            {
                long itemScreenX, itemScreenY;
                WorldToScreen(state, state->worldItems[i].x, state->worldItems[i].y, &itemScreenX, &itemScreenY);
                (void)itemScreenX;
                drawItems[n].kind = DRAW_KIND_WORLD_ITEM;
                drawItems[n].worldItemIndex = i;
                drawItems[n].depth = itemScreenY;
            }

            
            for (long i = 0; i < state->liveSpawnCount; i++, n++)
            {
                drawItems[n].kind = DRAW_KIND_LIVE_SPAWN;
                drawItems[n].liveSpawnIndex = i;
                drawItems[n].depth = GetLiveSpawnDepth(state, &state->liveSpawns[i]);
                
                drawItems[n].isCorpse = state->liveSpawns[i].block == 3 && state->liveSpawns[i].aiState.isDead;
            }
        }

        
        drawItems[n].kind = DRAW_KIND_PLAYER;
        drawItems[n].depth = playerDepth;
        n++;

        
        {
            long tcWorldX, tcWorldY;
            if (ActiveTransportCircleHere(state, &tcWorldX, &tcWorldY, NULL))
            {
                long circleScreenX, circleScreenY;
                WorldToScreen(state, tcWorldX, tcWorldY, &circleScreenX, &circleScreenY);
                (void)circleScreenX;
                drawItems[n].kind = DRAW_KIND_TRANSPORT_CIRCLE;
                drawItems[n].depth = circleScreenY;
                n++;
            }
        }

        
        qsort(drawItems, (size_t)n, sizeof(DrawItem), CompareDrawItem);
        for (long k = 0; k < n; k++)
        {
            switch (drawItems[k].kind)
            {
            case DRAW_KIND_PROP:
                
                if (drawItems[k].propShadowIcon)
                    RKC_DIB_TransferToDIBEx(&state->canvas, drawItems[k].propShadowDestX, drawItems[k].propShadowDestY,
                                            drawItems[k].propShadowIcon->width, drawItems[k].propShadowIcon->height,
                                            drawItems[k].propShadowIcon, 0, 0, 0, 500);
                RKC_DIB_TransferToDIBEx(&state->canvas, drawItems[k].propDestX, drawItems[k].propDestY,
                                        drawItems[k].propIcon->width, drawItems[k].propIcon->height,
                                        drawItems[k].propIcon, 0, 0, 0, drawItems[k].propTrans);
                break;
            case DRAW_KIND_LIVE_SPAWN:
                DrawLiveSpawn(state, &state->liveSpawns[drawItems[k].liveSpawnIndex]);
                break;
            case DRAW_KIND_PLAYER:
                DrawPlayer(state, keys);
                break;
            case DRAW_KIND_WORLD_ITEM:
                DrawWorldItem(state, &state->worldItems[drawItems[k].worldItemIndex]);
                break;
            case DRAW_KIND_TRANSPORT_CIRCLE:
                DrawTransportCircle(state);
                break;
            }
        }
        free(drawItems);
    }

    
    DrawScriptEffects(state);

    
    DrawUnlockSwBubble(state);

    
    DrawDisplayDarkness(state);

    
    DrawFloatingValues(state);

    
    if (state->judgeOverlay)
        DrawJudgeOverlay(state);

    
    if (state->hasMoveTarget)
    {
        long screenX, screenY;
        WorldToScreen(state, state->moveTargetX, state->moveTargetY, &screenX, &screenY);
        DrawMarker(&state->canvas, screenX - state->cameraX, screenY - state->cameraY, 2, 255, 0, 255);
    }

    
    if (state->mctLoaded)
    {
        DrawGateLabels(state);
        DrawGateRings(state);
        
        DrawTransportCircleLabel(state);
    }

    
    DrawHud(state, keys);

    
    DrawCompass(state);

    
    DrawQuestWindow(state);

    
    DrawQuestTooltip(state);

    
    DrawGateWindow(state);

    
    DrawMinimap(state);

    
    DrawInventory(state);

    
    DrawStatusMagic(state);

    
    DrawQuestBanner(state);

    
    DrawHudOverlay(state);

    
    DrawDialog(state);

    
    DrawLevelUpNotice(state);

    

    
    {
        int cursorX, cursorY;
        SDL_GetMouseState(&cursorX, &cursorY);
        
        DrawItemTooltip(state, cursorX, cursorY);
        DrawCursor(state, cursorX, cursorY);
    }

    RKC_DBFCONTROL_Present(&state->dbf, App_GetRenderer(), &state->canvas);
}

int main(int argc, char **argv)
{
    const char *stem = argv[2];
    const char *scenarioDir = argc > 3 ? argv[3] : NULL;
#ifdef GROUNDDEMO_AUDIO
    const char *bgmPath = argc > 4 ? argv[4] : NULL;
    const char *sfxPath = argc > 5 ? argv[5] : NULL;
#endif
    const char *playerGender = (argc > 6 && argv[6][0] != '\0') ? argv[6] : "Female";

    
    static DemoState state;
    memset(&state, 0, sizeof(state));
    
    state.currentEntryPoint = -1; 
    state.currentScenarioId = -1; 
    state.deferredTalkEndCharacterNo = -1; 
#ifdef GROUNDDEMO_AUDIO
    state.currentBgmIndex = -1; 
#endif
    state.playerMaxHP = PLAYER_DEFAULT_MAX_HP;
    state.playerHP = PLAYER_DEFAULT_MAX_HP;
    state.playerLevel = 1;      
    state.questBannerIndex = -1; 
    state.questTooltipIndex = -1;       
    state.hoveredInventorySlot = -1;    
    state.itemTooltipHoverKey = -1;     
    state.playerFacingDirection = 1; 
    state.clickRangeTileHeight = CLICK_RANGE_DEFAULT_TILE_HEIGHT; 
    state.clickRangeSquareVisible = 1;

    RKC_RPGSCRN_GROUND_Init(&state.ground);
    RKC_RPGSCRN_OBJECTBLOCK_Init(&state.objects);
    RKC_UPDIB_SET_Init(&state.patternSet);
    RKC_RPGSCRN_MCT_Init(&state.mct);
    RKC_UPDIB_Init(&state.minimapBg);
    RKC_RPG_SCRIPT_Init(&state.script);
    RKC_RPG_AICONTROL_Init(&state.aiControl);
    RKC_RPG_ITEMDATA_Init(&state.itemData);
    RKC_RPG_TABLE_Init(&state.table);
    RKC_UPDIB_Init(&state.playerTemplate.staticNjp);
    RKC_RPGSCRN_CAF_Init(&state.playerTemplate.caf);
    RKC_UPDIB_Init(&state.playerTemplate.animNjp);
    RKC_UPDIB_Init(&state.playerTemplate.animSdw);
    state.playerTemplate.kind = LIVE_SPAWN_SPRITE_NONE;
    RKC_UPDIB_Init(&state.hudBar);
    RKC_UPDIB_Init(&state.font);
    RKC_UPDIB_Init(&state.hukidasi);
    RKC_UPDIB_Init(&state.statusSheet);
    RKC_UPDIB_Init(&state.statusIconSheet);
    RKC_UPDIB_Init(&state.magicBarIcon);
    for (int s = 0; s < ITEM_ICON_SHEET_COUNT; s++)
        RKC_UPDIB_Init(&state.itemIconSheets[s]);
    for (int s = 0; s < WEAPON_WORLD_SHEET_COUNT; s++)
        RKC_UPDIB_Init(&state.weaponWorldSheets[s]);
    RKC_UPDIB_Init(&state.cursorSheet);
    RKC_UPDIB_Init(&state.waitingSheet);
    RKC_UPDIB_Init(&state.bloodDecalSheet);
    RKC_UPDIB_Init(&state.darknessTemplate);
    state.playerIsFemale = strcmp(playerGender, "Male") != 0; 
    RKC_UPDIB_Init(&state.compassTemplate.staticNjp);
    RKC_RPGSCRN_CAF_Init(&state.compassTemplate.caf);
    RKC_UPDIB_Init(&state.compassTemplate.animNjp);
    RKC_UPDIB_Init(&state.compassTemplate.animSdw);
    state.compassTemplate.kind = LIVE_SPAWN_SPRITE_NONE;
    RKC_UPDIB_Init(&state.gateRingTemplate.staticNjp);
    RKC_RPGSCRN_CAF_Init(&state.gateRingTemplate.caf);
    RKC_UPDIB_Init(&state.gateRingTemplate.animNjp);
    RKC_UPDIB_Init(&state.gateRingTemplate.animSdw);
    state.gateRingTemplate.kind = LIVE_SPAWN_SPRITE_NONE;
    RKC_UPDIB_Init(&state.transportCircleTemplate.staticNjp);
    RKC_RPGSCRN_CAF_Init(&state.transportCircleTemplate.caf);
    RKC_UPDIB_Init(&state.transportCircleTemplate.animNjp);
    RKC_UPDIB_Init(&state.transportCircleTemplate.animSdw);
    state.transportCircleTemplate.kind = LIVE_SPAWN_SPRITE_NONE;
    RKC_UPDIB_Init(&state.unlockSwTemplate.staticNjp);
    RKC_RPGSCRN_CAF_Init(&state.unlockSwTemplate.caf);
    RKC_UPDIB_Init(&state.unlockSwTemplate.animNjp);
    RKC_UPDIB_Init(&state.unlockSwTemplate.animSdw);
    state.unlockSwTemplate.kind = LIVE_SPAWN_SPRITE_NONE;
    for (int i = 0; i < HIT_VFX_VARIANT_COUNT; i++)
    {
        RKC_UPDIB_Init(&state.hitVfxTemplates[i].staticNjp);
        RKC_RPGSCRN_CAF_Init(&state.hitVfxTemplates[i].caf);
        RKC_UPDIB_Init(&state.hitVfxTemplates[i].animNjp);
        RKC_UPDIB_Init(&state.hitVfxTemplates[i].animSdw);
        state.hitVfxTemplates[i].kind = LIVE_SPAWN_SPRITE_NONE;
    }

    
    char hudBarPath[1024];
    if (DeriveHudBarPath(state.mapRoot, hudBarPath, sizeof(hudBarPath)))
    {
        state.hudBarLoaded = RKC_UPDIB_Read(&state.hudBar, hudBarPath) && RKC_UPDIB_GetFrameCount(&state.hudBar) > 0;
        if (state.hudBarLoaded)
            printf("loaded HUD sheet: %s\n", hudBarPath);
        else
            printf("warning: failed to load %s\n", hudBarPath);
    }
    else
        printf("warning: unable to derive the HUD sheet path\n");

    
    char fontPath[1024];
    if (DeriveFontPath(state.mapRoot, fontPath, sizeof(fontPath)))
    {
        state.fontLoaded = RKC_UPDIB_Read(&state.font, fontPath) && RKC_UPDIB_GetFrameCount(&state.font) > 0;
        if (state.fontLoaded)
            printf("loaded font sheet: %s\n", fontPath);
        else
            printf("warning: failed to load %s\n", fontPath);
    }
    else
        printf("warning: unable to derive the font sheet path\n");

    
    char hukidasiPath[1024];
    if (DeriveHukidasiPath(state.mapRoot, hukidasiPath, sizeof(hukidasiPath)))
    {
        state.hukidasiLoaded =
            RKC_UPDIB_Read(&state.hukidasi, hukidasiPath) && RKC_UPDIB_GetFrameCount(&state.hukidasi) > 0;
        if (state.hukidasiLoaded)
            printf("loaded dialogue sheet: %s\n", hukidasiPath);
        else
            printf("warning: failed to load %s\n", hukidasiPath);
    }
    else
        printf("warning: unable to derive the dialogue sheet path\n");

    
    char statusSheetPath[1024];
    if (DeriveStatusSheetPath(state.mapRoot, statusSheetPath, sizeof(statusSheetPath)))
    {
        state.statusSheetLoaded =
            RKC_UPDIB_Read(&state.statusSheet, statusSheetPath) && RKC_UPDIB_GetFrameCount(&state.statusSheet) > 0;
        if (state.statusSheetLoaded)
        {
            
            
            RemapEnclosedColorKeyPixels(RKC_UPDIB_GetFrame(&state.statusSheet, INVENTORY_BG_FRAME_GRID_EXT));
        }
        else
            printf("warning: failed to load %s\n", statusSheetPath);
    }
    else
        printf("warning: unable to derive the status sheet path\n");

    
    char magicBarIconPath[1024];
    if (DeriveMagicBarIconPath(state.mapRoot, magicBarIconPath, sizeof(magicBarIconPath)))
    {
        state.magicBarIconLoaded = RKC_UPDIB_Read(&state.magicBarIcon, magicBarIconPath) &&
                                   RKC_UPDIB_GetFrameCount(&state.magicBarIcon) > 0;
        if (state.magicBarIconLoaded)
            printf("loaded magic slot sheet: %s\n", magicBarIconPath);
        else
            printf("warning: failed to load %s\n", magicBarIconPath);
    }
    else
        printf("warning: unable to derive the magic slot sheet path\n");

    
    char statusIconPath[1024];
    if (DeriveStatusIconSheetPath(state.mapRoot, statusIconPath, sizeof(statusIconPath)))
    {
        state.statusIconSheetLoaded =
            RKC_UPDIB_Read(&state.statusIconSheet, statusIconPath) &&
            RKC_UPDIB_GetFrameCount(&state.statusIconSheet) > 0;
        if (state.statusIconSheetLoaded)
            printf("loaded status icon sheet: %s\n", statusIconPath);
        else
            printf("warning: failed to load %s\n", statusIconPath);
    }

    
    char darknessPath[1024];
    if (DeriveDarknessPath(state.mapRoot, darknessPath, sizeof(darknessPath)))
    {
        state.darknessTemplateLoaded =
            RKC_UPDIB_Read(&state.darknessTemplate, darknessPath) && RKC_UPDIB_GetFrameCount(&state.darknessTemplate) > 0;
        if (state.darknessTemplateLoaded)
            printf("loaded darkness sheet: %s\n", darknessPath);
        else
            printf("warning: failed to load %s\n", darknessPath);
    }
    else
        printf("warning: unable to derive the darkness sheet path\n");

    
    int itemIconSheetsLoadedCount = 0;
    for (int s = 0; s < ITEM_ICON_SHEET_COUNT; s++)
    {
        char itemIconSheetPath[1024];
        if (!DeriveItemIconSheetPath(state.mapRoot, s, itemIconSheetPath, sizeof(itemIconSheetPath)))
        {
            printf("warning: unable to derive item icon sheet %d\n", s);
            break; 
        }
        state.itemIconSheetLoaded[s] = RKC_UPDIB_Read(&state.itemIconSheets[s], itemIconSheetPath) &&
                                       RKC_UPDIB_GetFrameCount(&state.itemIconSheets[s]) > 0;
        if (state.itemIconSheetLoaded[s])
            itemIconSheetsLoadedCount++;
        else
            printf("warning: failed to load %s\n", itemIconSheetPath);
    }
    

    
    int weaponWorldSheetsLoadedCount = 0;
    char characterRootForWeaponSheets[1024];
    if (DeriveCharacterRoot(state.mapRoot, characterRootForWeaponSheets, sizeof(characterRootForWeaponSheets)))
    {
        for (int s = 0; s < WEAPON_WORLD_SHEET_COUNT; s++)
        {
            char weaponWorldSheetPath[1200];
            snprintf(weaponWorldSheetPath, sizeof(weaponWorldSheetPath), "%s/ITEM/%08d/Animation.Njp",
                     characterRootForWeaponSheets, s);
            state.weaponWorldSheetLoaded[s] = RKC_UPDIB_Read(&state.weaponWorldSheets[s], weaponWorldSheetPath) &&
                                              RKC_UPDIB_GetFrameCount(&state.weaponWorldSheets[s]) > 0;
            if (!state.weaponWorldSheetLoaded[s])
            {
                snprintf(weaponWorldSheetPath, sizeof(weaponWorldSheetPath), "%s/ITEM/%08d/Pattern.njp",
                         characterRootForWeaponSheets, s);
                state.weaponWorldSheetLoaded[s] = RKC_UPDIB_Read(&state.weaponWorldSheets[s], weaponWorldSheetPath) &&
                                                  RKC_UPDIB_GetFrameCount(&state.weaponWorldSheets[s]) > 0;
            }
            if (state.weaponWorldSheetLoaded[s])
                weaponWorldSheetsLoadedCount++;
            else
                printf("warning: failed to load weapon sheet %d\n", s);
        }
    }
    else
        printf("warning: unable to derive weapon sprite paths\n");
    

    
    char cursorSheetPath[1024];
    if (DeriveCursorSheetPath(state.mapRoot, cursorSheetPath, sizeof(cursorSheetPath)))
    {
        state.cursorSheetLoaded =
            RKC_UPDIB_Read(&state.cursorSheet, cursorSheetPath) && RKC_UPDIB_GetFrameCount(&state.cursorSheet) > 0;
        if (state.cursorSheetLoaded)
            printf("loaded cursor sheet: %s\n", cursorSheetPath);
        else
            printf("warning: failed to load %s\n", cursorSheetPath);
    }
    else
        printf("warning: unable to derive the cursor sheet path\n");

    
    char bloodDecalPath[1024];
    if (DeriveBloodDecalPath(state.mapRoot, bloodDecalPath, sizeof(bloodDecalPath)))
    {
        state.bloodDecalSheetLoaded =
            RKC_UPDIB_Read(&state.bloodDecalSheet, bloodDecalPath) && RKC_UPDIB_GetFrameCount(&state.bloodDecalSheet) > 0;
        if (state.bloodDecalSheetLoaded)
            printf("loaded blood decal sheet: %s\n", bloodDecalPath);
        else
            printf("warning: failed to load %s\n", bloodDecalPath);
    }
    else
        printf("warning: unable to derive the blood decal path\n");

    
    char waitingSheetPath[1024];
    if (DeriveWaitingSheetPath(state.mapRoot, waitingSheetPath, sizeof(waitingSheetPath)))
    {
        state.waitingSheetLoaded = RKC_UPDIB_Read(&state.waitingSheet, waitingSheetPath) &&
                                   RKC_UPDIB_GetFrameCount(&state.waitingSheet) > 0;
        if (state.waitingSheetLoaded)
            printf("loaded loading-screen sheet: %s\n", waitingSheetPath);
        else
            printf("warning: failed to load %s\n", waitingSheetPath);
    }
    else
        printf("warning: unable to derive the loading-screen sheet path\n");

    
    char playerRoot[1024];
    if (DerivePlayerRoot(state.mapRoot, playerRoot, sizeof(playerRoot)))
    {
        char playerDir[sizeof(playerRoot) + 32];
        snprintf(playerDir, sizeof(playerDir), "%s/%s", playerRoot, playerGender);
        LoadCafTemplate(&state.playerTemplate, playerDir, "Animation00");
        if (state.playerTemplate.kind == LIVE_SPAWN_SPRITE_CAF)
            printf("loaded player sprite: %s\n", playerDir);
        else
            printf("warning: failed to load player sprite from %s\n", playerDir);
    }
    else
        printf("warning: unable to derive the player sprite path\n");

    
    char compassRoot[1024];
    if (DerivePlayerRoot(state.mapRoot, compassRoot, sizeof(compassRoot)))
    {
        char compassDir[sizeof(compassRoot) + 16];
        snprintf(compassDir, sizeof(compassDir), "%s/OPTION/%s", compassRoot, playerGender);
        LoadCafTemplate(&state.compassTemplate, compassDir, "compasses");
        if (state.compassTemplate.kind == LIVE_SPAWN_SPRITE_CAF)
            printf("loaded compass sprite: %s\n", compassDir);
        else
            printf("warning: failed to load compass sprite from %s\n", compassDir);

        
        LoadCafTemplate(&state.unlockSwTemplate, compassDir, "UnlockSW");
        if (state.unlockSwTemplate.kind == LIVE_SPAWN_SPRITE_CAF)
            printf("loaded UnlockSW sprite: %s\n", compassDir);
        else
            printf("warning: failed to load UnlockSW sprite from %s\n", compassDir);
    }

    
    char gateRingRoot[1024];
    if (DeriveCharacterRoot(state.mapRoot, gateRingRoot, sizeof(gateRingRoot)))
    {
        char gateRingDir[sizeof(gateRingRoot) + 32];
        snprintf(gateRingDir, sizeof(gateRingDir), "%s/OBJECT/00000015", gateRingRoot);
        LoadCafTemplate(&state.gateRingTemplate, gateRingDir, "Animation");
        if (state.gateRingTemplate.kind == LIVE_SPAWN_SPRITE_CAF)
            printf("loaded gate ring sprite: %s\n", gateRingDir);
        else
            printf("warning: failed to load gate ring sprite from %s\n", gateRingDir);

        
        char tcDir[sizeof(gateRingRoot) + 32];
        snprintf(tcDir, sizeof(tcDir), "%s/OPTION/10000020", gateRingRoot);
        LoadCafTemplate(&state.transportCircleTemplate, tcDir, "Animation");
        char tcPattern[sizeof(tcDir) + 16];
        snprintf(tcPattern, sizeof(tcPattern), "%s/Pattern.Njp", tcDir);
        if (!RKC_UPDIB_Read(&state.transportCircleTemplate.staticNjp, tcPattern))
        {
            snprintf(tcPattern, sizeof(tcPattern), "%s/Pattern.njp", tcDir);
            RKC_UPDIB_Read(&state.transportCircleTemplate.staticNjp, tcPattern);
        }
        if (RKC_UPDIB_GetFrameCount(&state.transportCircleTemplate.staticNjp) > 0)
            printf("loaded transport circle sprite: %s\n", tcDir);
        else
            printf("warning: failed to load transport circle sprite from %s\n", tcDir);
    }

    
    char hitVfxRoot[1024];
    if (DeriveCharacterRoot(state.mapRoot, hitVfxRoot, sizeof(hitVfxRoot)))
    {
        static const char *const kHitVfxIds[HIT_VFX_VARIANT_COUNT] = {"11000000", "11000001", "11000002"};
        for (int i = 0; i < HIT_VFX_VARIANT_COUNT; i++)
        {
            char hitVfxDir[sizeof(hitVfxRoot) + 32];
            snprintf(hitVfxDir, sizeof(hitVfxDir), "%s/OPTION/%s", hitVfxRoot, kHitVfxIds[i]);
            LoadCafTemplate(&state.hitVfxTemplates[i], hitVfxDir, "Animation");
            if (state.hitVfxTemplates[i].kind == LIVE_SPAWN_SPRITE_CAF)
                printf("loaded hit-effect VFX variant %d: %s\n", i, hitVfxDir);
            else
                printf("warning: failed to load hit-effect VFX variant %d from %s\n", i, hitVfxDir);
        }
    }

    
#ifdef GROUNDDEMO_AUDIO
    
    RKC_DSOUND_Init(&state.dsound);
    if (!RKC_DSOUND_Initialize(&state.dsound, NULL, 0))
        printf("warning: audio initialization failed\n");
#endif

    if (scenarioDir && scenarioDir[0] != '\0')
    {
        
        if (!LoadScenario(&state, scenarioDir, 0))
        {
            printf("failed to load scenario %s\n", scenarioDir);
            return 1;
        }
    }
    else if (!LoadArea(&state, stem))
    {
        printf("failed to load map %s\n", stem);
        return 1;
    }

    
    fflush(stdout); 

    if (!App_Init("ShadowFlare port -- ground tile viewer"))
    {
        printf("failed to initialize SDL\n");
        return 1;
    }

    
    SDL_ShowCursor(SDL_DISABLE);

    RKC_DBFCONTROL_Init(&state.dbf);
    RKC_DIB_Init(&state.canvas);
    RKC_DIB_Create(&state.canvas, APP_WIDTH, APP_HEIGHT, 24, 1);
    RKC_DIB_Init(&state.darknessMask);
    RKC_DIB_Create(&state.darknessMask, APP_WIDTH, APP_HEIGHT, 24, 1);

#ifdef GROUNDDEMO_AUDIO
    if (state.dsound.initialized)
    {
        
        
        
        if (bgmPath && !state.bgmLoaded)
        {
            if (RKC_DSOUND_ReadVocFile(&state.dsound, bgmPath, 0) &&
                (state.bgmHandle = RKC_DSOUND_Play(&state.dsound, 0, 0, 1 , 0, 0)) >= 0)
            {
                state.bgmLoaded = 1;
                
            }
            else
                printf("warning: failed to load BGM %s\n", bgmPath);
        }
        if (sfxPath)
        {
            if (RKC_DSOUND_ReadVocFile(&state.dsound, sfxPath, 1))
            {
                state.sfxLoaded = 1;
                
            }
            else
                printf("warning: failed to load SFX %s\n", sfxPath);
        }
    }
#endif

    App_SetRenderCallback(OnRender, &state);
    App_Run();

    RKC_DIB_Release(&state.canvas);
    RKC_DIB_Release(&state.darknessMask);
    RKC_DBFCONTROL_Release(&state.dbf);
#ifdef GROUNDDEMO_AUDIO
    RKC_DSOUND_Release(&state.dsound);
#endif
    App_Shutdown();
    RKC_RPG_SCRIPT_EXEC_State_Release(&state.execState);
    RKC_RPG_SCRIPT_Release(&state.script);
    RKC_RPGSCRN_MCT_Release(&state.mct);
    RKC_RPG_AICONTROL_Release(&state.aiControl);
    RKC_RPG_ITEMDATA_Release(&state.itemData);
    RKC_RPG_TABLE_Release(&state.table);
    free(state.liveSpawns);
    free(state.worldItems);
    for (int i = 0; i < state.templateCount; i++)
    {
        RKC_UPDIB_Release(&state.templates[i].staticNjp);
        RKC_RPGSCRN_CAF_Release(&state.templates[i].caf);
        RKC_UPDIB_Release(&state.templates[i].animNjp);
        RKC_UPDIB_Release(&state.templates[i].animSdw);
    }
    RKC_UPDIB_Release(&state.playerTemplate.staticNjp);
    RKC_RPGSCRN_CAF_Release(&state.playerTemplate.caf);
    RKC_UPDIB_Release(&state.playerTemplate.animNjp);
    RKC_UPDIB_Release(&state.playerTemplate.animSdw);
    RKC_UPDIB_Release(&state.minimapBg);
    RKC_UPDIB_Release(&state.hudBar);
    RKC_UPDIB_Release(&state.font);
    RKC_UPDIB_Release(&state.hukidasi);
    RKC_UPDIB_Release(&state.statusSheet);
    RKC_UPDIB_Release(&state.statusIconSheet);
    RKC_UPDIB_Release(&state.magicBarIcon);
    for (int s = 0; s < ITEM_ICON_SHEET_COUNT; s++)
        RKC_UPDIB_Release(&state.itemIconSheets[s]);
    for (int s = 0; s < WEAPON_WORLD_SHEET_COUNT; s++)
        RKC_UPDIB_Release(&state.weaponWorldSheets[s]);
    RKC_UPDIB_Release(&state.waitingSheet);
    RKC_UPDIB_Release(&state.bloodDecalSheet);
    RKC_UPDIB_Release(&state.darknessTemplate);
    RKC_UPDIB_Release(&state.compassTemplate.staticNjp);
    RKC_RPGSCRN_CAF_Release(&state.compassTemplate.caf);
    RKC_UPDIB_Release(&state.compassTemplate.animNjp);
    RKC_UPDIB_Release(&state.compassTemplate.animSdw);
    RKC_UPDIB_Release(&state.gateRingTemplate.staticNjp);
    RKC_RPGSCRN_CAF_Release(&state.gateRingTemplate.caf);
    RKC_UPDIB_Release(&state.gateRingTemplate.animNjp);
    RKC_UPDIB_Release(&state.gateRingTemplate.animSdw);
    RKC_UPDIB_Release(&state.transportCircleTemplate.staticNjp);
    RKC_RPGSCRN_CAF_Release(&state.transportCircleTemplate.caf);
    RKC_UPDIB_Release(&state.transportCircleTemplate.animNjp);
    RKC_UPDIB_Release(&state.transportCircleTemplate.animSdw);
    RKC_UPDIB_Release(&state.unlockSwTemplate.staticNjp);
    RKC_RPGSCRN_CAF_Release(&state.unlockSwTemplate.caf);
    RKC_UPDIB_Release(&state.unlockSwTemplate.animNjp);
    RKC_UPDIB_Release(&state.unlockSwTemplate.animSdw);
    for (int i = 0; i < HIT_VFX_VARIANT_COUNT; i++)
    {
        RKC_UPDIB_Release(&state.hitVfxTemplates[i].staticNjp);
        RKC_RPGSCRN_CAF_Release(&state.hitVfxTemplates[i].caf);
        RKC_UPDIB_Release(&state.hitVfxTemplates[i].animNjp);
        RKC_UPDIB_Release(&state.hitVfxTemplates[i].animSdw);
    }
    for (int i = 0; i < state.scriptEffectTemplateCount; i++)
    {
        RKC_UPDIB_Release(&state.scriptEffectTemplates[i].staticNjp);
        RKC_RPGSCRN_CAF_Release(&state.scriptEffectTemplates[i].caf);
        RKC_UPDIB_Release(&state.scriptEffectTemplates[i].animNjp);
        RKC_UPDIB_Release(&state.scriptEffectTemplates[i].animSdw);
    }
    RKC_UPDIB_SET_Release(&state.patternSet);
    RKC_RPGSCRN_OBJECTBLOCK_Release(&state.objects);
    RKC_RPGSCRN_GROUND_Release(&state.ground);
    return 0;
}
