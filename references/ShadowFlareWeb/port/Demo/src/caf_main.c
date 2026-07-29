#include "App.h"
#include "RKC_DBFCONTROL.h"
#include "RKC_DIB.h"
#include "RKC_RPGSCRN_CAF.h"
#include "RKC_UPDIB.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TICKS_PER_ANIM_FRAME 4

typedef struct
{
    RKC_DBFCONTROL dbf;
    RKC_DIB canvas;
    RKC_RPGSCRN_CAF caf;
    RKC_UPDIB njp;
    RKC_UPDIB sdw;
    long chart;
    long direction;
    long animFrame;
    unsigned long tick;
    int paused;
    Uint8 prevKeys[SDL_NUM_SCANCODES];
} DemoState;

static void PrintChartInfo(const DemoState *state)
{
    
}

static void OnRender(void *userData)
{
    DemoState *state = (DemoState *)userData;

    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    int changed = 0;
    if (keys[SDL_SCANCODE_RIGHT] && !state->prevKeys[SDL_SCANCODE_RIGHT])
    {
        state->chart = (state->chart + 1) % state->caf.chartCount;
        changed = 1;
    }
    if (keys[SDL_SCANCODE_LEFT] && !state->prevKeys[SDL_SCANCODE_LEFT])
    {
        state->chart = (state->chart - 1 + state->caf.chartCount) % state->caf.chartCount;
        changed = 1;
    }
    if (keys[SDL_SCANCODE_UP] && !state->prevKeys[SDL_SCANCODE_UP])
    {
        state->direction = (state->direction + 1) % RKC_RPGSCRN_CAF_NUM_DIRECTIONS;
        changed = 1;
    }
    if (keys[SDL_SCANCODE_DOWN] && !state->prevKeys[SDL_SCANCODE_DOWN])
    {
        state->direction = (state->direction - 1 + RKC_RPGSCRN_CAF_NUM_DIRECTIONS) % RKC_RPGSCRN_CAF_NUM_DIRECTIONS;
        changed = 1;
    }
    if (keys[SDL_SCANCODE_SPACE] && !state->prevKeys[SDL_SCANCODE_SPACE])
    {
        state->paused = !state->paused;
        changed = 1;
    }
    if (keys[SDL_SCANCODE_R] && !state->prevKeys[SDL_SCANCODE_R])
    {
        state->animFrame = 0;
        changed = 1;
    }
    memcpy(state->prevKeys, keys, sizeof(state->prevKeys));
    if (changed)
    {
        state->animFrame = 0;
        state->tick = 0;
        PrintChartInfo(state);
    }

    state->tick++;
    if (!state->paused && state->tick % TICKS_PER_ANIM_FRAME == 0)
        state->animFrame++;

    RKC_DIB_FillByte(&state->canvas, 32);

    RKC_RPGSCRN_CAF_DrawCmd cmds[8];
    int n = RKC_RPGSCRN_CAF_Resolve(&state->caf, state->chart, state->direction, state->animFrame, &state->njp,
                                     &state->sdw, NULL, NULL, NULL, NULL, 0, cmds, 8);
    long anchorX = state->canvas.width / 2;
    long anchorY = state->canvas.height / 2;
    for (int i = 0; i < n; i++)
    {
        const RKC_DIB *icon = cmds[i].icon;
        RKC_DIB_TransferToDIBEx(&state->canvas, anchorX + cmds[i].offsetX, anchorY + cmds[i].offsetY, icon->width,
                                 icon->height, icon, 0, 0, 0, cmds[i].trans);
    }

    RKC_DBFCONTROL_Present(&state->dbf, App_GetRenderer(), &state->canvas);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        
        
        return 1;
    }
    long chart = argc > 2 ? atol(argv[2]) : 0;
    long direction = argc > 3 ? atol(argv[3]) : 0;

    char path[1024];
    DemoState state;
    memset(&state, 0, sizeof(state));
    state.chart = chart;
    state.direction = direction;

    RKC_RPGSCRN_CAF_Init(&state.caf);
    RKC_UPDIB_Init(&state.njp);
    RKC_UPDIB_Init(&state.sdw);

    
    if (!RKC_RPGSCRN_CAF_Read(&state.caf, path))
    {
        
        return 1;
    }
    
    if (!RKC_UPDIB_Read(&state.njp, path))
    {
        
        return 1;
    }
    
    if (!RKC_UPDIB_Read(&state.sdw, path))
    {
        
        return 1;
    }

    if (chart < 0 || chart >= state.caf.chartCount)
    {
        
        return 1;
    }
    if (direction < 0 || direction >= RKC_RPGSCRN_CAF_NUM_DIRECTIONS)
    {
        
        return 1;
    }
    
    
    PrintChartInfo(&state);

    if (!App_Init("ShadowFlare port -- CAF animation viewer"))
    {
        
        return 1;
    }

    RKC_DBFCONTROL_Init(&state.dbf);
    RKC_DIB_Init(&state.canvas);
    RKC_DIB_Create(&state.canvas, APP_WIDTH, APP_HEIGHT, 24, 1);

    App_SetRenderCallback(OnRender, &state);
    App_Run();

    RKC_DIB_Release(&state.canvas);
    RKC_DBFCONTROL_Release(&state.dbf);
    App_Shutdown();
    RKC_UPDIB_Release(&state.njp);
    RKC_UPDIB_Release(&state.sdw);
    RKC_RPGSCRN_CAF_Release(&state.caf);
    return 0;
}
