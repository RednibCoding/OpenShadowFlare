#ifndef SFDE_GROUND_SCENARIO_H
#define SFDE_GROUND_SCENARIO_H

#include "state.h"

int LoadArea(DemoState *state, const char *stem);

int LoadScenario(DemoState *state, const char *scenarioDir, long entryPoint);

#endif
