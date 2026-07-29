#ifndef SFDE_RKC_DBFCONTROL_H
#define SFDE_RKC_DBFCONTROL_H

#include "RKC_DIB.h"

typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;

typedef struct RKC_DBFCONTROL
{
    SDL_Texture *texture;
    long textureWidth;
    long textureHeight;
} RKC_DBFCONTROL;

void RKC_DBFCONTROL_Init(RKC_DBFCONTROL *self);
void RKC_DBFCONTROL_Release(RKC_DBFCONTROL *self);

int RKC_DBFCONTROL_Present(RKC_DBFCONTROL *self, SDL_Renderer *renderer, const RKC_DIB *frame);

#endif
