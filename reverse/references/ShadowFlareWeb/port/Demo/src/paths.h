#ifndef SFDE_GROUND_PATHS_H
#define SFDE_GROUND_PATHS_H

#include "state.h"

int DeriveCharacterRoot(const char *mapRoot, char *outBuf, size_t outSize);

int DerivePlayerRoot(const char *mapRoot, char *outBuf, size_t outSize);

int DeriveItemIbnPath(const char *mapRoot, char *outBuf, size_t outSize);

int DeriveTablePath(const char *mapRoot, char *outBuf, size_t outSize);

int DeriveControlAidPath(const char *mapRoot, const char *aidPathField, char *outBuf, size_t outSize);

int DeriveScenarioPath(const char *mapRoot, long scenarioId, char *outBuf, size_t outSize);

long ParseScenarioIdFromPath(const char *scenarioDir);

int DeriveHudBarPath(const char *mapRoot, char *outBuf, size_t outSize);

int DeriveBloodDecalPath(const char *mapRoot, char *outBuf, size_t outSize);

int DeriveHukidasiPath(const char *mapRoot, char *outBuf, size_t outSize);

int DeriveFontPath(const char *mapRoot, char *outBuf, size_t outSize);

int DeriveStatusSheetPath(const char *mapRoot, char *outBuf, size_t outSize);

int DeriveStatusIconSheetPath(const char *mapRoot, char *outBuf, size_t outSize);

int DeriveMagicBarIconPath(const char *mapRoot, char *outBuf, size_t outSize);

int DeriveDarknessPath(const char *mapRoot, char *outBuf, size_t outSize);

int DeriveItemIconSheetPath(const char *mapRoot, int sheetIndex, char *outBuf, size_t outSize);

int DeriveCursorSheetPath(const char *mapRoot, char *outBuf, size_t outSize);

int DeriveBgmPath(const char *mapRoot, long bgmIndex, char *outBuf, size_t outSize);

int DeriveWaitingSheetPath(const char *mapRoot, char *outBuf, size_t outSize);

int DeriveMapStem(const char *mapPathField, char *outBuf, size_t outSize);

#endif
