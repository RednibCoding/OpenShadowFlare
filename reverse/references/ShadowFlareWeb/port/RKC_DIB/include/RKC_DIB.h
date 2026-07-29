#ifndef SFDE_RKC_DIB_H
#define SFDE_RKC_DIB_H


typedef struct RKC_DIB
{
    long width;
    long height;
    unsigned short bpp;
    long alignWidth;
    unsigned char *palette;
    unsigned char *pixels;
} RKC_DIB;

void RKC_DIB_Init(RKC_DIB *self);
void RKC_DIB_Release(RKC_DIB *self);

int RKC_DIB_Create(RKC_DIB *self, long width, long height, unsigned short bpp, int allocPixels);

int RKC_DIB_ReadFile(RKC_DIB *self, const char *path, unsigned short allowedBppMask);

int RKC_DIB_FillByte(RKC_DIB *self, unsigned char value);

long RKC_DIB_GetAlignWidth(const RKC_DIB *self);
long RKC_DIB_GetPaletteCount(const RKC_DIB *self);
int RKC_DIB_TransferToDIBFast(RKC_DIB *self, long destX, long destY, long width, long height,
                               const RKC_DIB *src, long srcX, long srcY);

int RKC_DIB_TransferToDIB(RKC_DIB *self, long destX, long destY, long width, long height,
                          const RKC_DIB *src, long srcX, long srcY, long colorKey);

int RKC_DIB_TransferToDIBEx(RKC_DIB *self, long destX, long destY, long width, long height,
                            const RKC_DIB *src, long srcX, long srcY, long colorKey, long opacity);

int RKC_DIB_TransferToDIBTint(RKC_DIB *self, long destX, long destY, long width, long height,
                              const RKC_DIB *src, long srcX, long srcY, long colorKey, long opacity,
                              long tintR, long tintG, long tintB);

unsigned char RKC_DIB_TintChannel(unsigned char channel, long tint);

int RKC_DIB_TransferToDIBAdditive(RKC_DIB *self, long destX, long destY, long width, long height,
                                  const RKC_DIB *src, long srcX, long srcY, long colorKey, long opacity);

int RKC_DIB_TransferToDIBAdditiveTint(RKC_DIB *self, long destX, long destY, long width, long height,
                                      const RKC_DIB *src, long srcX, long srcY, long colorKey, long opacity,
                                      long tintR, long tintG, long tintB);

long RKC_DIB_GetPixelIndex(const RKC_DIB *self, long x, long y);

#endif
