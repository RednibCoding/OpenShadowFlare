#ifndef SFDE_GROUND_PATHFIND_H
#define SFDE_GROUND_PATHFIND_H

#include "RKC_RPGSCRN_GROUND.h"

#define PATHFIND_SEARCH_MARGIN_CELLS 24
#define PATHFIND_MAX_BOX_CELLS 40000
#define PATHFIND_MAX_EXPANSIONS 20000
#define PATHFIND_MAX_RAW_WAYPOINTS 512

int GroundPathfind_FindPath(const RKC_RPGSCRN_GROUND *ground, long startX, long startY, long targetX, long targetY,
                            long *waypointsX, long *waypointsY, int maxWaypoints, int *outCount);

#endif
