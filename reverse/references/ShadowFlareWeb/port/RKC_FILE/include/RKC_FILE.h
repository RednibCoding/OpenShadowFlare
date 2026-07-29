#ifndef SFDE_RKC_FILE_H
#define SFDE_RKC_FILE_H

#include <stdio.h>

typedef struct RKC_FILE
{
    FILE *fp;
    long size;
} RKC_FILE;

void RKC_FILE_Init(RKC_FILE *self);
int RKC_FILE_Create(RKC_FILE *self, const char *path, long mode);
int RKC_FILE_Close(RKC_FILE *self);
int RKC_FILE_Read(RKC_FILE *self, void *buf, long size);
int RKC_FILE_Write(RKC_FILE *self, const void *buf, long size);
int RKC_FILE_Seek(RKC_FILE *self, long offset, long whence);
long RKC_FILE_GetSize(RKC_FILE *self);
void *RKC_FILE_GetHandle(RKC_FILE *self);

#endif
