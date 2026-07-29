#include "RK_FUNCTION_HuffmanTree.h"

#include <stddef.h>

HuffNode *RK_Huffman_BuildTree(HuffNode *table, HuffNode **scratch, HuffNode *internalBuf)
{
    for (int i = 0; i < 256; i++)
        scratch[i] = &table[i];

    for (int pass = 255; pass > 0; pass--)
        for (int j = 0; j < pass; j++)
            if (scratch[j]->freq < scratch[j + 1]->freq)
            {
                HuffNode *t = scratch[j];
                scratch[j] = scratch[j + 1];
                scratch[j + 1] = t;
            }

    HuffNode *work[257];
    for (int i = 0; i < 256; i++)
        work[i + 1] = scratch[i];

    int count = 0;
    while (count < 256 && work[count + 1]->freq != 0)
        count++;

    if (count == 0)
        return NULL;
    if (count == 1)
    {
        work[2]->freq = 1;
        count = 2;
    }

    HuffNode *internal = internalBuf;
    for (int n = count; n > 1; n--)
    {
        internal->left = work[n - 1];
        internal->right = work[n];
        internal->freq = work[n - 1]->freq + work[n]->freq;
        work[n] = NULL;

        int pos = 0;
        while (pos < n - 2 && work[pos + 1]->freq >= internal->freq)
            pos++;

        for (int i = n - 2; i > pos; i--)
            work[i + 1] = work[i];
        work[pos + 1] = internal;

        internal++;
    }

    return work[1];
}
