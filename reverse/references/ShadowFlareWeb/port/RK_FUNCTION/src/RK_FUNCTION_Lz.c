#include "RK_FUNCTION.h"

#include <stdlib.h>
#include <string.h>

int RK_LzDecodeMemoryToMemory(const void *pSrc, unsigned long dwSrcSize,
                              void **ppDst, unsigned long *outSize)
{
    if (dwSrcSize < 0x11)
        return -1;

    const unsigned char *src = (const unsigned char *)pSrc;
    if (memcmp(src, "RCLIB-L", 7) != 0 || src[7] != 0x1A)
        return 0;

    unsigned long decompSize = *(const unsigned int *)(src + 0x08);
    unsigned long compSize = *(const unsigned int *)(src + 0x0C);

    if (dwSrcSize < 0x10 + compSize)
        return -1;

    unsigned char *out = malloc(decompSize ? decompSize : 1);
    if (!out)
        return -1;

    unsigned char ring[4096] = {0};
    unsigned int ringPos = 0xFEE;

    const unsigned char *bits = src + 0x10;
    const unsigned char *end = bits + compSize;
    unsigned long outOff = 0;

    while (bits < end)
    {
        unsigned char flags = *bits++;

        for (unsigned char mask = 0x80; mask != 0 && bits < end; mask >>= 1)
        {
            if (!(flags & mask))
            {
                unsigned char b = *bits++;
                if (outOff < decompSize)
                    out[outOff++] = b;
                ring[ringPos] = b;
                ringPos = (ringPos + 1) & 0xFFF;
            }
            else
            {
                if (bits + 1 >= end)
                    break;
                unsigned char b1 = *bits++;
                unsigned char b2 = *bits++;

                unsigned int offset = (unsigned int)b1 | ((unsigned int)(b2 & 0xF0) << 4);
                unsigned int length = (unsigned int)(b2 & 0x0F) + 3;

                for (unsigned int i = 0; i < length; i++)
                {
                    unsigned char b = ring[(offset + i) & 0xFFF];
                    if (outOff < decompSize)
                        out[outOff++] = b;
                    ring[ringPos] = b;
                    ringPos = (ringPos + 1) & 0xFFF;
                }
            }
        }
    }

    *ppDst = out;
    if (outSize)
        *outSize = outOff;
    return 1;
}
