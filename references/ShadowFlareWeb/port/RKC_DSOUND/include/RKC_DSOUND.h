#ifndef SFDE_RKC_DSOUND_H
#define SFDE_RKC_DSOUND_H

typedef struct RKC_DSOUND_Voc RKC_DSOUND_Voc;

typedef struct RKC_DSOUND
{
    RKC_DSOUND_Voc **voc;
    long vocCount;
    int initialized;
} RKC_DSOUND;

void RKC_DSOUND_Init(RKC_DSOUND *self);
int RKC_DSOUND_Initialize(RKC_DSOUND *self, void *hwnd, long reserved);
int RKC_DSOUND_ReadVocFile(RKC_DSOUND *self, const char *path, long vocId);
void RKC_DSOUND_ReleaseVoc(RKC_DSOUND *self, long vocId);
long RKC_DSOUND_Play(RKC_DSOUND *self, long vocId, long trackIdx, int flag, long volume, long pan);
int RKC_DSOUND_GetPlayStatus(RKC_DSOUND *self, long vocId, long handle);
void RKC_DSOUND_SetVolume(RKC_DSOUND *self, long vocId, long handle, long volume);
void RKC_DSOUND_Release(RKC_DSOUND *self);

#endif
