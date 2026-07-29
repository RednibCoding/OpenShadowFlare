#include "RKC_DBFCONTROL.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <string.h>

void RKC_DBFCONTROL_Init(RKC_DBFCONTROL *self)
{
    memset(self, 0, sizeof(*self));
}

void RKC_DBFCONTROL_Release(RKC_DBFCONTROL *self)
{
    if (self->texture)
        SDL_DestroyTexture(self->texture);
    memset(self, 0, sizeof(*self));
}

int RKC_DBFCONTROL_Present(RKC_DBFCONTROL *self, SDL_Renderer *renderer, const RKC_DIB *frame)
{
    if (!self || !renderer || !frame || !frame->pixels)
        return 0;
    if (frame->bpp != 24 || frame->width <= 0 || frame->height <= 0)
        return 0;

    if (!self->texture || self->textureWidth != frame->width || self->textureHeight != frame->height)
    {
        if (self->texture)
            SDL_DestroyTexture(self->texture);
        self->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRX32, SDL_TEXTUREACCESS_STREAMING,
                                          (int)frame->width, (int)frame->height);
        if (!self->texture)
        {
            self->textureWidth = 0;
            self->textureHeight = 0;
            return 0;
        }
        self->textureWidth = frame->width;
        self->textureHeight = frame->height;
    }

    void *pixels;
    int pitch;
    if (SDL_LockTexture(self->texture, NULL, &pixels, &pitch) != 0)
        return 0;

    for (long y = 0; y < frame->height; y++)
    {
        const unsigned char *srcRow = frame->pixels + (size_t)(frame->height - 1 - y) * (size_t)frame->alignWidth;
        unsigned int *dstRow = (unsigned int *)((unsigned char *)pixels + (size_t)y * (size_t)pitch);
        for (long x = 0; x < frame->width; x++)
        {
            const unsigned char *s = srcRow + x * 3;
            dstRow[x] = (unsigned int)s[0] | ((unsigned int)s[1] << 8) | ((unsigned int)s[2] << 16) | 0xFF000000u;
        }
    }
    SDL_UnlockTexture(self->texture);

    return SDL_RenderCopy(renderer, self->texture, NULL, NULL) == 0;
}
