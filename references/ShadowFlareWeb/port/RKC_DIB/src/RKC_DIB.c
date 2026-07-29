#include "RKC_DIB.h"

#include "RKC_FILE.h"

#include <stdlib.h>
#include <string.h>

static long PaletteCountForBpp(unsigned short bpp)
{
    switch (bpp)
    {
    case 1:
        return 2;
    case 4:
        return 16;
    case 8:
        return 256;
    case 16:
    case 24:
        return 0;
    default:
        return -1;
    }
}

static long AlignWidthForBpp(long width, unsigned short bpp)
{
    switch (bpp)
    {
    case 1:
    case 4:
    case 8:
    case 16:
    case 24:
        return (((long)width * bpp + 31) / 32) * 4;
    default:
        return -1;
    }
}

static int BppAllowed(unsigned short bpp, unsigned short mask)
{
    switch (bpp)
    {
    case 1:
        return (mask & 0x01) != 0;
    case 4:
        return (mask & 0x02) != 0;
    case 8:
        return (mask & 0x04) != 0;
    case 16:
        return (mask & 0x08) != 0;
    case 24:
        return (mask & 0x10) != 0;
    default:
        return 0;
    }
}

void RKC_DIB_Init(RKC_DIB *self)
{
    memset(self, 0, sizeof(*self));
}

void RKC_DIB_Release(RKC_DIB *self)
{
    free(self->palette);
    free(self->pixels);
    memset(self, 0, sizeof(*self));
}

int RKC_DIB_Create(RKC_DIB *self, long width, long height, unsigned short bpp, int allocPixels)
{
    RKC_DIB_Release(self);

    if ((width == 0 || height == 0) && allocPixels)
        return 0;

    long paletteCount = PaletteCountForBpp(bpp);
    if (paletteCount < 0)
        return 0;

    long alignWidth = AlignWidthForBpp(width, bpp);
    if (alignWidth < 0)
        return 0;

    if (paletteCount > 0)
    {
        self->palette = calloc((size_t)paletteCount, 4);
        if (!self->palette)
        {
            RKC_DIB_Release(self);
            return 0;
        }
    }

    self->width = width;
    self->height = height;
    self->bpp = bpp;
    self->alignWidth = alignWidth;

    if (allocPixels)
    {
        self->pixels = malloc((size_t)alignWidth * (size_t)height);
        if (!self->pixels)
        {
            RKC_DIB_Release(self);
            return 0;
        }
    }

    return 1;
}

int RKC_DIB_ReadFile(RKC_DIB *self, const char *path, unsigned short allowedBppMask)
{
    RKC_DIB_Release(self);

    RKC_FILE file;
    RKC_FILE_Init(&file);
    if (!RKC_FILE_Create(&file, path, 0))
        return 0;

    unsigned char fileHeader[14];
    unsigned char infoHeader[40];
    if (!RKC_FILE_Read(&file, fileHeader, 14) || !RKC_FILE_Read(&file, infoHeader, 40))
    {
        RKC_FILE_Close(&file);
        return 0;
    }

    long pixelDataOffset = (long)*(unsigned int *)(fileHeader + 10); /* bfOffBits */
    long width = (long)*(int *)(infoHeader + 4);
    long height = (long)*(int *)(infoHeader + 8);
    unsigned short bpp = *(unsigned short *)(infoHeader + 14);

    if (!BppAllowed(bpp, allowedBppMask))
    {
        RKC_FILE_Close(&file);
        return 0;
    }

    long paletteCount = PaletteCountForBpp(bpp);
    long alignWidth = AlignWidthForBpp(width, bpp);
    if (paletteCount < 0 || alignWidth < 0)
    {
        RKC_FILE_Close(&file);
        return 0;
    }

    if (paletteCount > 0)
    {
        self->palette = malloc((size_t)paletteCount * 4);
        if (!self->palette || !RKC_FILE_Read(&file, self->palette, paletteCount * 4))
        {
            RKC_FILE_Close(&file);
            RKC_DIB_Release(self);
            return 0;
        }
    }

    if (!RKC_FILE_Seek(&file, pixelDataOffset, 0))
    {
        RKC_FILE_Close(&file);
        RKC_DIB_Release(self);
        return 0;
    }

    long pixelSize = alignWidth * height;
    self->pixels = malloc((size_t)pixelSize);
    if (!self->pixels || !RKC_FILE_Read(&file, self->pixels, pixelSize))
    {
        RKC_FILE_Close(&file);
        RKC_DIB_Release(self);
        return 0;
    }

    self->width = width;
    self->height = height;
    self->bpp = bpp;
    self->alignWidth = alignWidth;

    RKC_FILE_Close(&file);
    return 1;
}

int RKC_DIB_FillByte(RKC_DIB *self, unsigned char value)
{
    if (!self->pixels)
        return 0;

    memset(self->pixels, value, (size_t)self->alignWidth * (size_t)self->height);
    return 1;
}

long RKC_DIB_GetAlignWidth(const RKC_DIB *self)
{
    return AlignWidthForBpp(self->width, self->bpp);
}

long RKC_DIB_GetPaletteCount(const RKC_DIB *self)
{
    return PaletteCountForBpp(self->bpp);
}

static int SrcBppOk(unsigned short bpp)
{
    return bpp == 1 || bpp == 4 || bpp == 8;
}

static int DstBppOk(unsigned short bpp)
{
    return bpp == 8 || bpp == 24;
}

static unsigned char *RowPtr(const RKC_DIB *dib, long y)
{
    return dib->pixels + (size_t)(dib->height - 1 - y) * (size_t)dib->alignWidth;
}

static unsigned IndexAt(const RKC_DIB *dib, long x, long y)
{
    const unsigned char *row = RowPtr(dib, y);
    if (dib->bpp == 8)
        return row[x];

    if (dib->bpp == 1)
    {
        unsigned char b = row[x >> 3];
        return (b >> (7 - (x & 7))) & 1;
    }

    unsigned char b = row[x >> 1];
    return (x & 1) == 0 ? (unsigned)(b >> 4) : (unsigned)(b & 0x0F);
}

unsigned char RKC_DIB_TintChannel(unsigned char channel, long tint)
{
    long delta = tint - 1000;
    long adjust = delta <= 0 ? (channel * delta) / 1000 : ((255 - channel) * delta) / 1000;
    long result = (long)channel + adjust;
    if (result < 0)
        result = 0;
    if (result > 255)
        result = 255;
    return (unsigned char)result;
}

static void PutPixel(RKC_DIB *dst, long x, long y, const RKC_DIB *src, unsigned index, long opacity,
                      long tintR, long tintG, long tintB)
{
    unsigned char *row = RowPtr(dst, y);

    if (dst->bpp == 8)
    {
        row[x] = (unsigned char)index;
        return;
    }

    unsigned char *dstPixel = row + x * 3;
    const unsigned char *srcColor = src->palette + (size_t)index * 4;
    unsigned char color[3];
    color[0] = tintB == 1000 ? srcColor[0] : RKC_DIB_TintChannel(srcColor[0], tintB);
    color[1] = tintG == 1000 ? srcColor[1] : RKC_DIB_TintChannel(srcColor[1], tintG);
    color[2] = tintR == 1000 ? srcColor[2] : RKC_DIB_TintChannel(srcColor[2], tintR);

    if (opacity >= 1000)
    {
        dstPixel[0] = color[0];
        dstPixel[1] = color[1];
        dstPixel[2] = color[2];
        return;
    }

    for (int c = 0; c < 3; c++)
        dstPixel[c] = (unsigned char)(dstPixel[c] + (color[c] - dstPixel[c]) * opacity / 1000);
}

static int ClipRegion(long dstW, long dstH, long srcW, long srcH,
                       long *destX, long *destY, long *width, long *height,
                       long *srcX, long *srcY)
{
    if (*destX < 0)
    {
        *srcX -= *destX;
        *destX = 0;
    }
    if (*destY < 0)
    {
        *srcY -= *destY;
        *destY = 0;
    }
    if (*srcX < 0)
    {
        *destX -= *srcX;
        *srcX = 0;
    }
    if (*srcY < 0)
    {
        *destY -= *srcY;
        *srcY = 0;
    }

    if (*destX < 0 || *destX >= dstW || *srcX < 0 || *srcX >= srcW)
        return 0;
    if (*destY < 0 || *destY >= dstH || *srcY < 0 || *srcY >= srcH)
        return 0;

    if (dstW < *destX + *width)
        *width = dstW - *destX;
    if (srcW < *srcX + *width)
        *width = srcW - *srcX;
    if (dstH < *destY + *height)
        *height = dstH - *destY;
    if (srcH < *srcY + *height)
        *height = srcH - *srcY;

    return *width > 0 && *height > 0;
}

static int TransferRegion(RKC_DIB *self, long destX, long destY, long width, long height,
                           const RKC_DIB *src, long srcX, long srcY, long colorKey, long opacity,
                           long tintR, long tintG, long tintB)
{
    if (!self->pixels || !src->pixels)
        return 0;
    if (!SrcBppOk(src->bpp) || !DstBppOk(self->bpp))
        return 0;
    if (self->bpp == 8 && src->bpp != 8)
        return 0;

    if (!ClipRegion(self->width, self->height, src->width, src->height,
                     &destX, &destY, &width, &height, &srcX, &srcY))
        return 0;

    int tinted = tintR != 1000 || tintG != 1000 || tintB != 1000;

    if (self->bpp == 8)
    {
        for (long row = 0; row < height; row++)
        {
            const unsigned char *s = RowPtr(src, srcY + row) + srcX;
            unsigned char *d = RowPtr(self, destY + row) + destX;
            if (colorKey < 0)
                memcpy(d, s, (size_t)width);
            else
                for (long col = 0; col < width; col++)
                    if ((long)s[col] != colorKey)
                        d[col] = s[col];
        }
        return 1;
    }

    if (!tinted && src->bpp == 8)
    {
        const unsigned char *pal = src->palette;
        for (long row = 0; row < height; row++)
        {
            const unsigned char *s = RowPtr(src, srcY + row) + srcX;
            unsigned char *d = RowPtr(self, destY + row) + destX * 3;
            if (opacity >= 1000)
            {
                for (long col = 0; col < width; col++, d += 3)
                {
                    unsigned index = s[col];
                    if ((long)index == colorKey)
                        continue;
                    const unsigned char *c = pal + (size_t)index * 4;
                    d[0] = c[0];
                    d[1] = c[1];
                    d[2] = c[2];
                }
            }
            else
            {
                for (long col = 0; col < width; col++, d += 3)
                {
                    unsigned index = s[col];
                    if ((long)index == colorKey)
                        continue;
                    const unsigned char *c = pal + (size_t)index * 4;
                    d[0] = (unsigned char)(d[0] + (c[0] - d[0]) * opacity / 1000);
                    d[1] = (unsigned char)(d[1] + (c[1] - d[1]) * opacity / 1000);
                    d[2] = (unsigned char)(d[2] + (c[2] - d[2]) * opacity / 1000);
                }
            }
        }
        return 1;
    }

    if (!tinted && src->bpp == 1)
    {
        const unsigned char *pal = src->palette;
        for (long row = 0; row < height; row++)
        {
            const unsigned char *srcRow = RowPtr(src, srcY + row);
            unsigned char *d = RowPtr(self, destY + row) + destX * 3;
            for (long col = 0; col < width; col++, d += 3)
            {
                long sx = srcX + col;
                unsigned index = (unsigned)((srcRow[sx >> 3] >> (7 - (sx & 7))) & 1);
                if ((long)index == colorKey)
                    continue;
                const unsigned char *c = pal + (size_t)index * 4;
                if (opacity >= 1000)
                {
                    d[0] = c[0];
                    d[1] = c[1];
                    d[2] = c[2];
                }
                else
                {
                    d[0] = (unsigned char)(d[0] + (c[0] - d[0]) * opacity / 1000);
                    d[1] = (unsigned char)(d[1] + (c[1] - d[1]) * opacity / 1000);
                    d[2] = (unsigned char)(d[2] + (c[2] - d[2]) * opacity / 1000);
                }
            }
        }
        return 1;
    }

    if (!tinted && src->bpp == 4)
    {
        const unsigned char *pal = src->palette;
        for (long row = 0; row < height; row++)
        {
            const unsigned char *srcRow = RowPtr(src, srcY + row);
            unsigned char *d = RowPtr(self, destY + row) + destX * 3;
            for (long col = 0; col < width; col++, d += 3)
            {
                long sx = srcX + col;
                unsigned char b = srcRow[sx >> 1];
                unsigned index = (sx & 1) == 0 ? (unsigned)(b >> 4) : (unsigned)(b & 0x0F);
                if ((long)index == colorKey)
                    continue;
                const unsigned char *c = pal + (size_t)index * 4;
                if (opacity >= 1000)
                {
                    d[0] = c[0];
                    d[1] = c[1];
                    d[2] = c[2];
                }
                else
                {
                    d[0] = (unsigned char)(d[0] + (c[0] - d[0]) * opacity / 1000);
                    d[1] = (unsigned char)(d[1] + (c[1] - d[1]) * opacity / 1000);
                    d[2] = (unsigned char)(d[2] + (c[2] - d[2]) * opacity / 1000);
                }
            }
        }
        return 1;
    }

    for (long row = 0; row < height; row++)
    {
        for (long col = 0; col < width; col++)
        {
            unsigned index = IndexAt(src, srcX + col, srcY + row);
            if (colorKey >= 0 && (long)index == colorKey)
                continue;
            PutPixel(self, destX + col, destY + row, src, index, opacity, tintR, tintG, tintB);
        }
    }

    return 1;
}

static void PutPixelAdditive(RKC_DIB *dst, long x, long y, const RKC_DIB *src, unsigned index, long opacity)
{
    unsigned char *row = RowPtr(dst, y);
    unsigned char *dstPixel = row + x * 3;
    const unsigned char *srcColor = src->palette + (size_t)index * 4;

    for (int c = 0; c < 3; c++)
    {
        long added = (long)dstPixel[c] + srcColor[c] * opacity / 1000;
        dstPixel[c] = (unsigned char)(added > 255 ? 255 : added);
    }
}

static void PutPixelAdditiveTint(RKC_DIB *dst, long x, long y, const RKC_DIB *src, unsigned index, long opacity,
                                 long tintR, long tintG, long tintB)
{
    unsigned char *row = RowPtr(dst, y);
    unsigned char *dstPixel = row + x * 3;
    const unsigned char *srcColor = src->palette + (size_t)index * 4;
    unsigned char color[3];
    color[0] = tintB == 1000 ? srcColor[0] : RKC_DIB_TintChannel(srcColor[0], tintB);
    color[1] = tintG == 1000 ? srcColor[1] : RKC_DIB_TintChannel(srcColor[1], tintG);
    color[2] = tintR == 1000 ? srcColor[2] : RKC_DIB_TintChannel(srcColor[2], tintR);

    for (int c = 0; c < 3; c++)
    {
        long added = (long)dstPixel[c] + color[c] * opacity / 1000;
        dstPixel[c] = (unsigned char)(added > 255 ? 255 : added);
    }
}

static int TransferRegionAdditive(RKC_DIB *self, long destX, long destY, long width, long height,
                                   const RKC_DIB *src, long srcX, long srcY, long colorKey, long opacity,
                                   long tintR, long tintG, long tintB)
{
    if (!self->pixels || !src->pixels)
        return 0;
    if (!SrcBppOk(src->bpp) || self->bpp != 24)
        return 0;

    if (!ClipRegion(self->width, self->height, src->width, src->height, &destX, &destY, &width, &height, &srcX,
                     &srcY))
        return 0;

    int tinted = tintR != 1000 || tintG != 1000 || tintB != 1000;

    if (!tinted && src->bpp == 8)
    {
        const unsigned char *pal = src->palette;
        for (long row = 0; row < height; row++)
        {
            const unsigned char *s = RowPtr(src, srcY + row) + srcX;
            unsigned char *d = RowPtr(self, destY + row) + destX * 3;
            for (long col = 0; col < width; col++, d += 3)
            {
                unsigned index = s[col];
                if ((long)index == colorKey)
                    continue;
                const unsigned char *c = pal + (size_t)index * 4;
                long a0 = (long)d[0] + c[0] * opacity / 1000;
                long a1 = (long)d[1] + c[1] * opacity / 1000;
                long a2 = (long)d[2] + c[2] * opacity / 1000;
                d[0] = (unsigned char)(a0 > 255 ? 255 : a0);
                d[1] = (unsigned char)(a1 > 255 ? 255 : a1);
                d[2] = (unsigned char)(a2 > 255 ? 255 : a2);
            }
        }
        return 1;
    }

    for (long row = 0; row < height; row++)
    {
        for (long col = 0; col < width; col++)
        {
            unsigned index = IndexAt(src, srcX + col, srcY + row);
            if (colorKey >= 0 && (long)index == colorKey)
                continue;
            if (tinted)
                PutPixelAdditiveTint(self, destX + col, destY + row, src, index, opacity, tintR, tintG, tintB);
            else
                PutPixelAdditive(self, destX + col, destY + row, src, index, opacity);
        }
    }

    return 1;
}

int RKC_DIB_TransferToDIBAdditive(RKC_DIB *self, long destX, long destY, long width, long height,
                                  const RKC_DIB *src, long srcX, long srcY, long colorKey, long opacity)
{
    if (opacity < 0 || opacity > 1000)
        return 0;
    return TransferRegionAdditive(self, destX, destY, width, height, src, srcX, srcY, colorKey, opacity, 1000, 1000,
                                  1000);
}

int RKC_DIB_TransferToDIBAdditiveTint(RKC_DIB *self, long destX, long destY, long width, long height,
                                      const RKC_DIB *src, long srcX, long srcY, long colorKey, long opacity,
                                      long tintR, long tintG, long tintB)
{
    if (opacity < 0 || opacity > 1000)
        return 0;
    return TransferRegionAdditive(self, destX, destY, width, height, src, srcX, srcY, colorKey, opacity, tintR,
                                  tintG, tintB);
}

int RKC_DIB_TransferToDIBFast(RKC_DIB *self, long destX, long destY, long width, long height,
                               const RKC_DIB *src, long srcX, long srcY)
{
    return TransferRegion(self, destX, destY, width, height, src, srcX, srcY, -1, 1000, 1000, 1000, 1000);
}

int RKC_DIB_TransferToDIB(RKC_DIB *self, long destX, long destY, long width, long height,
                          const RKC_DIB *src, long srcX, long srcY, long colorKey)
{
    return TransferRegion(self, destX, destY, width, height, src, srcX, srcY, colorKey, 1000, 1000, 1000, 1000);
}

int RKC_DIB_TransferToDIBEx(RKC_DIB *self, long destX, long destY, long width, long height,
                            const RKC_DIB *src, long srcX, long srcY, long colorKey, long opacity)
{
    if (opacity < 0 || opacity > 1000)
        return 0;
    return TransferRegion(self, destX, destY, width, height, src, srcX, srcY, colorKey, opacity, 1000, 1000, 1000);
}

int RKC_DIB_TransferToDIBTint(RKC_DIB *self, long destX, long destY, long width, long height,
                              const RKC_DIB *src, long srcX, long srcY, long colorKey, long opacity,
                              long tintR, long tintG, long tintB)
{
    if (opacity < 0 || opacity > 1000)
        return 0;
    return TransferRegion(self, destX, destY, width, height, src, srcX, srcY, colorKey, opacity, tintR, tintG, tintB);
}

long RKC_DIB_GetPixelIndex(const RKC_DIB *self, long x, long y)
{
    if (!self->pixels || !SrcBppOk(self->bpp))
        return -1;
    if (x < 0 || x >= self->width || y < 0 || y >= self->height)
        return -1;
    return (long)IndexAt(self, x, y);
}
