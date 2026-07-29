#include "pathfind.h"
#include <math.h>
#include <stdlib.h>

static long FloorDivCell(long a, long b)
{
    long q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0)))
        q--;
    return q;
}

typedef struct
{
    int cx, cy;
    float f;
} PathHeapNode;

typedef struct
{
    PathHeapNode *nodes;
    int count, capacity;
} PathMinHeap;

static void HeapPush(PathMinHeap *h, PathHeapNode n)
{
    if (h->count >= h->capacity)
        return;
    int i = h->count++;
    h->nodes[i] = n;
    while (i > 0)
    {
        int parent = (i - 1) / 2;
        if (h->nodes[parent].f <= h->nodes[i].f)
            break;
        PathHeapNode tmp = h->nodes[parent];
        h->nodes[parent] = h->nodes[i];
        h->nodes[i] = tmp;
        i = parent;
    }
}

static PathHeapNode HeapPop(PathMinHeap *h)
{
    PathHeapNode top = h->nodes[0];
    h->nodes[0] = h->nodes[--h->count];
    int i = 0;
    for (;;)
    {
        int left = i * 2 + 1, right = i * 2 + 2, smallest = i;
        if (left < h->count && h->nodes[left].f < h->nodes[smallest].f)
            smallest = left;
        if (right < h->count && h->nodes[right].f < h->nodes[smallest].f)
            smallest = right;
        if (smallest == i)
            break;
        PathHeapNode tmp = h->nodes[smallest];
        h->nodes[smallest] = h->nodes[i];
        h->nodes[i] = tmp;
        i = smallest;
    }
    return top;
}

#define PATHFIND_BLOCKED_TARGET_SEARCH_RADIUS 8

static int FindNearestOpenCell(const RKC_RPGSCRN_GROUND *ground, long cx, long cy, long *outCx, long *outCy)
{
    for (long r = 1; r <= PATHFIND_BLOCKED_TARGET_SEARCH_RADIUS; r++)
    {
        long bestCx = 0, bestCy = 0;
        double bestDistSq = -1.0;
        for (long dy = -r; dy <= r; dy++)
        {
            for (long dx = -r; dx <= r; dx++)
            {
                if (dx > -r && dx < r && dy > -r && dy < r)
                    continue;
                long ccx = cx + dx, ccy = cy + dy;
                long worldX = ccx * ground->baseMagX + ground->baseMagX / 2;
                long worldY = ccy * ground->baseMagY + ground->baseMagY / 2;
                if (RKC_RPGSCRN_GROUND_IsBlocked(ground, worldX, worldY))
                    continue;
                double distSq = (double)(dx * dx + dy * dy);
                if (bestDistSq < 0.0 || distSq < bestDistSq)
                {
                    bestDistSq = distSq;
                    bestCx = ccx;
                    bestCy = ccy;
                }
            }
        }
        if (bestDistSq >= 0.0)
        {
            *outCx = bestCx;
            *outCy = bestCy;
            return 1;
        }
    }
    return 0;
}

int GroundPathfind_FindPath(const RKC_RPGSCRN_GROUND *ground, long startX, long startY, long targetX, long targetY,
                            long *waypointsX, long *waypointsY, int maxWaypoints, int *outCount)
{
    if (!ground->judge || ground->judgeWidth <= 0 || ground->judgeHeight <= 0)
        return 0;

    long startCx = FloorDivCell(startX, ground->baseMagX), startCy = FloorDivCell(startY, ground->baseMagY);
    long targetCx, targetCy;
    int usedSubstitute = 0;
    if (RKC_RPGSCRN_GROUND_IsBlocked(ground, targetX, targetY))
    {
        if (!FindNearestOpenCell(ground, FloorDivCell(targetX, ground->baseMagX),
                                 FloorDivCell(targetY, ground->baseMagY), &targetCx, &targetCy))
            return 0;
        usedSubstitute = 1;
    }
    else
    {
        targetCx = FloorDivCell(targetX, ground->baseMagX);
        targetCy = FloorDivCell(targetY, ground->baseMagY);
    }

    long minCx = (startCx < targetCx ? startCx : targetCx) - PATHFIND_SEARCH_MARGIN_CELLS;
    long maxCx = (startCx > targetCx ? startCx : targetCx) + PATHFIND_SEARCH_MARGIN_CELLS;
    long minCy = (startCy < targetCy ? startCy : targetCy) - PATHFIND_SEARCH_MARGIN_CELLS;
    long maxCy = (startCy > targetCy ? startCy : targetCy) + PATHFIND_SEARCH_MARGIN_CELLS;

    if (minCx < ground->judgeOffsetX)
        minCx = ground->judgeOffsetX;
    if (minCy < ground->judgeOffsetY)
        minCy = ground->judgeOffsetY;
    if (maxCx >= ground->judgeOffsetX + ground->judgeWidth)
        maxCx = ground->judgeOffsetX + ground->judgeWidth - 1;
    if (maxCy >= ground->judgeOffsetY + ground->judgeHeight)
        maxCy = ground->judgeOffsetY + ground->judgeHeight - 1;

    if (startCx < minCx || startCx > maxCx || startCy < minCy || startCy > maxCy || targetCx < minCx ||
        targetCx > maxCx || targetCy < minCy || targetCy > maxCy)
        return 0;

    long boxW = maxCx - minCx + 1, boxH = maxCy - minCy + 1;
    if (boxW <= 0 || boxH <= 0 || boxW * boxH > PATHFIND_MAX_BOX_CELLS)
        return 0;

    size_t cellCount = (size_t)boxW * (size_t)boxH;
    unsigned char *closed = calloc(cellCount, 1);
    float *gScore = malloc(cellCount * sizeof(float));
    signed char *parentDx = malloc(cellCount);
    signed char *parentDy = malloc(cellCount);
    PathMinHeap heap;
    heap.capacity = 8 * PATHFIND_MAX_EXPANSIONS + 16;
    heap.nodes = malloc((size_t)heap.capacity * sizeof(PathHeapNode));
    heap.count = 0;
    if (!closed || !gScore || !parentDx || !parentDy || !heap.nodes)
    {
        free(closed);
        free(gScore);
        free(parentDx);
        free(parentDy);
        free(heap.nodes);
        return 0;
    }
    for (size_t i = 0; i < cellCount; i++)
        gScore[i] = -1.0f;

    int startLx = (int)(startCx - minCx), startLy = (int)(startCy - minCy);
    int targetLx = (int)(targetCx - minCx), targetLy = (int)(targetCy - minCy);

    gScore[(size_t)startLy * boxW + startLx] = 0.0f;
    PathHeapNode startNode;
    startNode.cx = startLx;
    startNode.cy = startLy;
    startNode.f = (float)hypot((double)(targetLx - startLx), (double)(targetLy - startLy));
    HeapPush(&heap, startNode);

    static const int NBR_DX[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    static const int NBR_DY[8] = {0, 0, 1, -1, 1, -1, 1, -1};

    int found = 0, expansions = 0;
    while (heap.count > 0 && expansions < PATHFIND_MAX_EXPANSIONS)
    {
        PathHeapNode cur = HeapPop(&heap);
        size_t curIdx = (size_t)cur.cy * boxW + cur.cx;
        if (closed[curIdx])
            continue;
        closed[curIdx] = 1;
        expansions++;

        if (cur.cx == targetLx && cur.cy == targetLy)
        {
            found = 1;
            break;
        }

        for (int n = 0; n < 8; n++)
        {
            int nx = cur.cx + NBR_DX[n], ny = cur.cy + NBR_DY[n];
            if (nx < 0 || nx >= boxW || ny < 0 || ny >= boxH)
                continue;
            size_t nIdx = (size_t)ny * boxW + nx;
            if (closed[nIdx])
                continue;

            long worldNx = (minCx + nx) * ground->baseMagX + ground->baseMagX / 2;
            long worldNy = (minCy + ny) * ground->baseMagY + ground->baseMagY / 2;
            if (RKC_RPGSCRN_GROUND_IsBlocked(ground, worldNx, worldNy))
                continue;

            if (NBR_DX[n] != 0 && NBR_DY[n] != 0)
            {
                long worldAx = (minCx + cur.cx + NBR_DX[n]) * ground->baseMagX + ground->baseMagX / 2;
                long worldAy = (minCy + cur.cy) * ground->baseMagY + ground->baseMagY / 2;
                long worldBx = (minCx + cur.cx) * ground->baseMagX + ground->baseMagX / 2;
                long worldBy = (minCy + cur.cy + NBR_DY[n]) * ground->baseMagY + ground->baseMagY / 2;
                if (RKC_RPGSCRN_GROUND_IsBlocked(ground, worldAx, worldAy) &&
                    RKC_RPGSCRN_GROUND_IsBlocked(ground, worldBx, worldBy))
                    continue;
            }

            float stepCost = (NBR_DX[n] != 0 && NBR_DY[n] != 0) ? 1.41421356f : 1.0f;
            float tentativeG = gScore[curIdx] + stepCost;
            float existingG = gScore[nIdx];
            if (existingG >= 0.0f && tentativeG >= existingG)
                continue;

            gScore[nIdx] = tentativeG;
            parentDx[nIdx] = (signed char)(-NBR_DX[n]);
            parentDy[nIdx] = (signed char)(-NBR_DY[n]);

            PathHeapNode node;
            node.cx = nx;
            node.cy = ny;
            node.f = tentativeG + (float)hypot((double)(targetLx - nx), (double)(targetLy - ny));
            HeapPush(&heap, node);
        }
    }

    int rawCount = 0;
    long rawX[PATHFIND_MAX_RAW_WAYPOINTS], rawY[PATHFIND_MAX_RAW_WAYPOINTS];
    if (found)
    {
        int cx = targetLx, cy = targetLy;
        while (!(cx == startLx && cy == startLy) && rawCount < PATHFIND_MAX_RAW_WAYPOINTS)
        {
            rawX[rawCount] = (minCx + cx) * ground->baseMagX + ground->baseMagX / 2;
            rawY[rawCount] = (minCy + cy) * ground->baseMagY + ground->baseMagY / 2;
            rawCount++;
            size_t idx = (size_t)cy * boxW + cx;
            cx += parentDx[idx];
            cy += parentDy[idx];
        }
        if (rawCount == 0)
        {
            rawX[0] = usedSubstitute ? (minCx + targetLx) * ground->baseMagX + ground->baseMagX / 2 : targetX;
            rawY[0] = usedSubstitute ? (minCy + targetLy) * ground->baseMagY + ground->baseMagY / 2 : targetY;
            rawCount = 1;
        }
        for (int i = 0; i < rawCount / 2; i++)
        {
            long tx = rawX[i];
            rawX[i] = rawX[rawCount - 1 - i];
            rawX[rawCount - 1 - i] = tx;
            long ty = rawY[i];
            rawY[i] = rawY[rawCount - 1 - i];
            rawY[rawCount - 1 - i] = ty;
        }
        if (rawCount > 0 && !usedSubstitute)
        {
            rawX[rawCount - 1] = targetX;
            rawY[rawCount - 1] = targetY;
        }
    }

    free(closed);
    free(gScore);
    free(parentDx);
    free(parentDy);
    free(heap.nodes);

    if (!found || rawCount == 0)
        return 0;

    int outCountLocal = 0;
    long curX = startX, curY = startY;
    int i = 0;
    while (i < rawCount && outCountLocal < maxWaypoints)
    {
        int farthest = i;
        for (int j = rawCount - 1; j > i; j--)
        {
            long clampedX, clampedY;
            if (RKC_RPGSCRN_GROUND_SweepMove(ground, curX, curY, rawX[j], rawY[j], &clampedX, &clampedY))
            {
                farthest = j;
                break;
            }
        }
        waypointsX[outCountLocal] = rawX[farthest];
        waypointsY[outCountLocal] = rawY[farthest];
        outCountLocal++;
        curX = rawX[farthest];
        curY = rawY[farthest];
        i = farthest + 1;
    }

    *outCount = outCountLocal;
    return outCountLocal > 0;
}
