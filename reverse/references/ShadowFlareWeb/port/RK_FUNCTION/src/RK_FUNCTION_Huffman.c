#include "RK_FUNCTION.h"
#include "RK_FUNCTION_HuffmanTree.h"

#include <stdlib.h>
#include <string.h>

int RK_HuffmanDecodeMemoryToMemory(const void *pSrc, unsigned long dwSrcSize,
                                   void **ppDst, unsigned long *outSize)
{
    if (dwSrcSize < 0x14)
        return -1;

    const unsigned char *p = (const unsigned char *)pSrc;
    if (memcmp(p, "RCLIB-H", 7) != 0 || p[7] != '\0')
        return 0;

    unsigned long decompressedSize = *(const unsigned int *)(p + 0x08);
    unsigned long compressedBytes = *(const unsigned int *)(p + 0x0C);
    unsigned short bitsInLastByte = *(const unsigned short *)(p + 0x10);
    unsigned short tableEntryCount = *(const unsigned short *)(p + 0x12);

    unsigned long tableBytes = (unsigned long)tableEntryCount * 5;
    if (dwSrcSize < 0x14 + tableBytes + compressedBytes)
        return -1;

    HuffNode table[256];
    for (int i = 0; i < 256; i++)
    {
        table[i].symbol = (unsigned char)i;
        table[i].freq = 0;
        table[i].left = table[i].right = NULL;
    }

    const unsigned char *entry = p + 0x14;
    for (unsigned short i = 0; i < tableEntryCount; i++, entry += 5)
        table[entry[0]].freq = *(const unsigned int *)(entry + 1);

    HuffNode *scratch[256];
    HuffNode internalBuf[255];
    HuffNode *root = RK_Huffman_BuildTree(table, scratch, internalBuf);
    if (!root)
        return -1;

    unsigned char *out = malloc(decompressedSize ? decompressedSize : 1);
    if (!out)
        return -1;

    const unsigned char *bits = entry;
    unsigned long outIdx = 0;
    HuffNode *node = root;

    for (unsigned long i = 0; i < compressedBytes; i++)
    {
        int nbits = (i == compressedBytes - 1 && bitsInLastByte) ? bitsInLastByte : 8;
        for (int b = 7; b > 7 - nbits; b--)
        {
            node = ((bits[i] >> b) & 1) ? node->right : node->left;
            if (!node)
            {
                free(out);
                return -1;
            }
            if (!node->left && !node->right)
            {
                if (outIdx < decompressedSize)
                    out[outIdx++] = node->symbol;
                node = root;
            }
        }
    }

    *ppDst = out;
    if (outSize)
        *outSize = outIdx;
    return 1;
}
