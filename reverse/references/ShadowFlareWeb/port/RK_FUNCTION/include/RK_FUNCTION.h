#ifndef SFDE_RK_FUNCTION_H
#define SFDE_RK_FUNCTION_H

int RK_LzDecodeMemoryToMemory(const void *pSrc, unsigned long dwSrcSize,
                              void **ppDst, unsigned long *outSize);

int RK_HuffmanDecodeMemoryToMemory(const void *pSrc, unsigned long dwSrcSize,
                                   void **ppDst, unsigned long *outSize);

#endif
