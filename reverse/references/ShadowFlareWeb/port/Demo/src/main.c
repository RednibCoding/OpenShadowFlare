
#include "App.h"
#include "RKC_DBFCONTROL.h"
#include "RKC_DIB.h"
#include "RKC_UPDIB.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    RKC_DBFCONTROL dbf;
    RKC_DIB canvas;
    RKC_DIB *sprite;
} DemoState;

static void OnRender(void *userData)
{
    DemoState *state = (DemoState *)userData;

    RKC_DIB_FillByte(&state->canvas, 32); 

    long destX = (state->canvas.width - state->sprite->width) / 2;
    long destY = (state->canvas.height - state->sprite->height) / 2;
    RKC_DIB_TransferToDIBFast(&state->canvas, destX, destY, state->sprite->width, state->sprite->height,
                              state->sprite, 0, 0);

    RKC_DBFCONTROL_Present(&state->dbf, App_GetRenderer(), &state->canvas);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        
        return 1;
    }
    long frameIndex = argc > 2 ? atol(argv[2]) : 0;

    RKC_UPDIB updib;
    RKC_UPDIB_Init(&updib);
    if (!RKC_UPDIB_Read(&updib, argv[1]))
    {
        
        return 1;
    }

    RKC_DIB *sprite = RKC_UPDIB_GetFrame(&updib, frameIndex);
    if (!sprite)
    {
        
        RKC_UPDIB_Release(&updib);
        return 1;
    }
    
    if (sprite->bpp != 4 && sprite->bpp != 8)
        

    if (!App_Init("ShadowFlare port -- sprite viewer"))
    {
        
        RKC_UPDIB_Release(&updib);
        return 1;
    }

    DemoState state;
    RKC_DBFCONTROL_Init(&state.dbf);
    RKC_DIB_Init(&state.canvas);
    RKC_DIB_Create(&state.canvas, APP_WIDTH, APP_HEIGHT, 24, 1);
    state.sprite = sprite;

    App_SetRenderCallback(OnRender, &state);
    App_Run();

    RKC_DIB_Release(&state.canvas);
    RKC_DBFCONTROL_Release(&state.dbf);
    App_Shutdown();
    RKC_UPDIB_Release(&updib);
    return 0;
}
