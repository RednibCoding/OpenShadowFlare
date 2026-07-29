#include "RKC_FILE.h"

void RKC_FILE_Init(RKC_FILE *self)
{
    self->fp = NULL;
    self->size = 0;
}

int RKC_FILE_Create(RKC_FILE *self, const char *path, long mode)
{
    RKC_FILE_Close(self);

    self->fp = fopen(path, mode == 0 ? "rb" : "w+b");
    if (!self->fp)
        return 0;

    fseek(self->fp, 0, SEEK_END);
    self->size = ftell(self->fp);
    fseek(self->fp, 0, SEEK_SET);

    return 1;
}

int RKC_FILE_Close(RKC_FILE *self)
{
    if (self->fp)
    {
        fclose(self->fp);
        self->fp = NULL;
    }
    self->size = 0;
    return 1;
}

int RKC_FILE_Read(RKC_FILE *self, void *buf, long size)
{
    if (!self->fp || size < 0)
        return 0;

    return fread(buf, 1, (size_t)size, self->fp) == (size_t)size;
}

int RKC_FILE_Write(RKC_FILE *self, const void *buf, long size)
{
    if (!self->fp || size < 0)
        return 0;

    return fwrite(buf, 1, (size_t)size, self->fp) == (size_t)size;
}

int RKC_FILE_Seek(RKC_FILE *self, long offset, long whence)
{
    if (!self->fp)
        return 0;

    return fseek(self->fp, offset, (int)whence) == 0;
}

long RKC_FILE_GetSize(RKC_FILE *self)
{
    return self->size;
}

void *RKC_FILE_GetHandle(RKC_FILE *self)
{
    return self->fp;
}
