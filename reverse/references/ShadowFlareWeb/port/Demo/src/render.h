#ifndef SFDE_GROUND_RENDER_H
#define SFDE_GROUND_RENDER_H

#include "state.h"


void DrawMarker(RKC_DIB *canvas, long cx, long cy, long half, unsigned char b, unsigned char g, unsigned char r);


void DrawHud(DemoState *state, const Uint8 *keys);


void DrawHudOverlay(DemoState *state);


void DrawLevelUpNotice(DemoState *state);


void DrawCompass(DemoState *state);


void DrawFloatingValues(DemoState *state);


void DrawUnlockSwBubble(DemoState *state);


void DrawScriptEffects(DemoState *state);


void DrawMinimap(DemoState *state);


void DrawInventory(DemoState *state);


void RemapEnclosedColorKeyPixels(RKC_DIB *frame);


void DrawStatusMagic(DemoState *state);


int IsScreenPointOverUI(DemoState *state, long x, long y);


void CloseAllWindows(DemoState *state);


const char *QuestTitleForIndex(long questIndex);


void DrawQuestWindow(DemoState *state);
void DrawQuestTooltip(DemoState *state);
void DrawItemTooltip(DemoState *state, long cursorX, long cursorY);


void DrawQuestBanner(DemoState *state);


void DrawGateWindow(DemoState *state);


void BeginInventoryDrag(DemoState *state, long x, long y);
void ResolveInventoryDrag(DemoState *state, long x, long y);


void DrawDialog(DemoState *state);


int FindDialogOptionAtScreenPoint(DemoState *state, long x, long y);


int DialogHasOptions(DemoState *state);


void DrawGateLabels(DemoState *state);


void DrawGateRings(DemoState *state);


void DrawTransportCircle(DemoState *state);


void DrawTransportCircleLabel(DemoState *state);


int IsTownScenario(long scenarioId);
void TransportDestTown(long fromScenarioId, long *outScenarioId, long *outX, long *outY);
int ActiveTransportCircleHere(const DemoState *state, long *outX, long *outY, int *outIsTownEnd);


void DrawDisplayDarkness(DemoState *state);


void WorldToScreen(const DemoState *state, long worldX, long worldY, long *outScreenX, long *outScreenY);


void UpdateCameraFromPlayer(DemoState *state);


void ScreenToWorld(const DemoState *state, long screenX, long screenY, long *outWorldX, long *outWorldY);


void DrawWorldItem(DemoState *state, WorldItem *item);


void DrawLiveSpawn(DemoState *state, LiveSpawn *spawn);


void DrawHitVfx(DemoState *state, long centerX, long centerY, unsigned long vfxTick, int variant);


void SpawnBloodDecal(DemoState *state, long x, long y);


void DrawBloodDecals(DemoState *state);


long GetLiveSpawnDepth(DemoState *state, const LiveSpawn *spawn);


int IsHoverableLiveSpawn(const LiveSpawn *spawn);


long CafSpriteYOffset(int n, const RKC_RPGSCRN_CAF_DrawCmd *cmds);


long CafSpriteYOffsetCached(long *cacheValue, int *cacheChart, int *cacheDirection, int *cacheValid, long chart,
                            long direction, int n, const RKC_RPGSCRN_CAF_DrawCmd *cmds);


long TallestCafIconHeight(int n, const RKC_RPGSCRN_CAF_DrawCmd *cmds);


long TallestCafIconWidth(int n, const RKC_RPGSCRN_CAF_DrawCmd *cmds);


void DrawLoadingScreen(DemoState *state);


void DrawJudgeOverlay(DemoState *state);


void DrawCursor(DemoState *state, long mouseX, long mouseY);


int FindMagicSlotAtScreenPoint(const DemoState *state, long x, long y);

#endif
