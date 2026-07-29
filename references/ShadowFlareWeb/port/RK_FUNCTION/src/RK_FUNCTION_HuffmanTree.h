#ifndef SFDE_RK_FUNCTION_HUFFMAN_TREE_H
#define SFDE_RK_FUNCTION_HUFFMAN_TREE_H

typedef struct HuffNode
{
    unsigned char symbol;
    unsigned long freq;
    struct HuffNode *left, *right;
} HuffNode;

HuffNode *RK_Huffman_BuildTree(HuffNode *table, HuffNode **scratch, HuffNode *internalBuf);

#endif
