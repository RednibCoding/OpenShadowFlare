#include "RKC_DSOUND.h"

#include "RKC_FILE.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <stdlib.h>
#include <string.h>

#define VOC_MAGIC "VoiceData"
#define VOC_NUMTRACKS_OFF 0x10
#define VOC_RECORDS_OFF 0x1C
#define VOC_LABEL_SIZE 512
#define VOC_FMT_FIXED_SIZE 16
#define VOC_UNKNOWN2_SIZE 2
#define VOC_SIZE_FIELD_SIZE 4
#define VOC_RECORD_HEADER_SIZE (VOC_LABEL_SIZE + VOC_FMT_FIXED_SIZE + VOC_UNKNOWN2_SIZE + VOC_SIZE_FIELD_SIZE)
#define VOC_INTER_RECORD_GAP 4

struct RKC_DSOUND_Voc
{
    Mix_Chunk **tracks;
    long trackCount;
};

static unsigned short GetU16(const unsigned char *p) { return (unsigned short)(p[0] | (p[1] << 8)); }
static unsigned int GetU32(const unsigned char *p)
{
    return (unsigned int)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}
static void PutU16(unsigned char *p, unsigned short v)
{
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)(v >> 8);
}
static void PutU32(unsigned char *p, unsigned int v)
{
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
    p[2] = (unsigned char)((v >> 16) & 0xFF);
    p[3] = (unsigned char)((v >> 24) & 0xFF);
}

void RKC_DSOUND_Init(RKC_DSOUND *self)
{
    memset(self, 0, sizeof(*self));
}

int RKC_DSOUND_Initialize(RKC_DSOUND *self, void *hwnd, long reserved)
{
    (void)hwnd;
    (void)reserved;

    if (self->initialized)
        return 1;

    if (SDL_WasInit(SDL_INIT_AUDIO) == 0 && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
        return 0;

    if (Mix_OpenAudio(22050, AUDIO_U8, 2, 2048) != 0)
        return 0;

    self->initialized = 1;
    return 1;
}

static int GrowVocSlots(RKC_DSOUND *self, long vocId)
{
    if (vocId < 0)
        return 0;
    if (vocId < self->vocCount)
        return 1;

    long newCount = vocId + 1;
    RKC_DSOUND_Voc **grown = (RKC_DSOUND_Voc **)realloc(self->voc, (size_t)newCount * sizeof(*grown));
    if (!grown)
        return 0;
    for (long i = self->vocCount; i < newCount; i++)
        grown[i] = NULL;
    self->voc = grown;
    self->vocCount = newCount;
    return 1;
}

static void FreeVocTracks(Mix_Chunk **tracks, long count)
{
    if (!tracks)
        return;
    for (long i = 0; i < count; i++)
    {
        if (tracks[i])
            Mix_FreeChunk(tracks[i]);
    }
    free(tracks);
}

static Mix_Chunk *LoadTrackChunk(unsigned short channels, unsigned int sampleRate, unsigned short bitsPerSample,
                                  const unsigned char *pcm, unsigned int pcmSize)
{
    unsigned char *wav = (unsigned char *)malloc(44 + (size_t)pcmSize);
    if (!wav)
        return NULL;

    memcpy(wav + 0, "RIFF", 4);
    PutU32(wav + 4, 36 + pcmSize);
    memcpy(wav + 8, "WAVE", 4);
    memcpy(wav + 12, "fmt ", 4);
    PutU32(wav + 16, 16);
    PutU16(wav + 20, 1); /* wFormatTag = PCM */
    PutU16(wav + 22, channels);
    PutU32(wav + 24, sampleRate);
    PutU32(wav + 28, sampleRate * channels * (bitsPerSample / 8));
    PutU16(wav + 32, (unsigned short)(channels * (bitsPerSample / 8)));
    PutU16(wav + 34, bitsPerSample);
    memcpy(wav + 36, "data", 4);
    PutU32(wav + 40, pcmSize);
    memcpy(wav + 44, pcm, pcmSize);

    SDL_RWops *rw = SDL_RWFromConstMem(wav, (int)(44 + pcmSize));
    Mix_Chunk *chunk = rw ? Mix_LoadWAV_RW(rw, 1) : NULL;
    free(wav);
    return chunk;
}

int RKC_DSOUND_ReadVocFile(RKC_DSOUND *self, const char *path, long vocId)
{
    if (!self || !self->initialized || !path || vocId < 0)
        return 0;

    RKC_FILE file;
    RKC_FILE_Init(&file);
    if (!RKC_FILE_Create(&file, path, 0))
        return 0;

    long size = RKC_FILE_GetSize(&file);
    unsigned char *buf = size >= VOC_RECORDS_OFF ? (unsigned char *)malloc((size_t)size) : NULL;
    if (!buf || !RKC_FILE_Read(&file, buf, size))
    {
        free(buf);
        RKC_FILE_Close(&file);
        return 0;
    }
    RKC_FILE_Close(&file);

    if (memcmp(buf, VOC_MAGIC, 9) != 0)
    {
        free(buf);
        return 0;
    }

    long numTracks = (long)GetU32(buf + VOC_NUMTRACKS_OFF);
    if (numTracks <= 0)
    {
        free(buf);
        return 0;
    }

    Mix_Chunk **tracks = (Mix_Chunk **)calloc((size_t)numTracks, sizeof(*tracks));
    if (!tracks)
    {
        free(buf);
        return 0;
    }

    long off = VOC_RECORDS_OFF;
    long loaded = 0;
    for (; loaded < numTracks; loaded++)
    {
        if (off + VOC_RECORD_HEADER_SIZE > size)
            break;

        const unsigned char *rec = buf + off;
        unsigned short channels = GetU16(rec + VOC_LABEL_SIZE + 2);
        unsigned int sampleRate = GetU32(rec + VOC_LABEL_SIZE + 4);
        unsigned short bitsPerSample = GetU16(rec + VOC_LABEL_SIZE + 14);
        unsigned int pcmSize = GetU32(rec + VOC_LABEL_SIZE + VOC_FMT_FIXED_SIZE + VOC_UNKNOWN2_SIZE);

        long pcmOff = off + VOC_RECORD_HEADER_SIZE;
        if (pcmOff + (long)pcmSize > size)
            break;

        tracks[loaded] = LoadTrackChunk(channels, sampleRate, bitsPerSample, buf + pcmOff, pcmSize);
        if (!tracks[loaded])
            break;

        off = pcmOff + (long)pcmSize;
        if (loaded + 1 < numTracks)
            off += VOC_INTER_RECORD_GAP;
    }
    free(buf);

    if (loaded != numTracks)
    {
        FreeVocTracks(tracks, loaded);
        return 0;
    }

    if (!GrowVocSlots(self, vocId))
    {
        FreeVocTracks(tracks, numTracks);
        return 0;
    }
    if (self->voc[vocId])
        RKC_DSOUND_ReleaseVoc(self, vocId);

    RKC_DSOUND_Voc *slot = (RKC_DSOUND_Voc *)malloc(sizeof(*slot));
    if (!slot)
    {
        FreeVocTracks(tracks, numTracks);
        return 0;
    }
    slot->tracks = tracks;
    slot->trackCount = numTracks;
    self->voc[vocId] = slot;
    return 1;
}

void RKC_DSOUND_ReleaseVoc(RKC_DSOUND *self, long vocId)
{
    if (!self || vocId < 0 || vocId >= self->vocCount || !self->voc[vocId])
        return;

    RKC_DSOUND_Voc *voc = self->voc[vocId];
    int allocated = Mix_AllocateChannels(-1);
    for (int ch = 0; ch < allocated; ch++)
    {
        if (!Mix_Playing(ch))
            continue;
        Mix_Chunk *playing = Mix_GetChunk(ch);
        for (long t = 0; t < voc->trackCount; t++)
        {
            if (voc->tracks[t] == playing)
            {
                Mix_HaltChannel(ch);
                break;
            }
        }
    }

    FreeVocTracks(voc->tracks, voc->trackCount);
    free(voc);
    self->voc[vocId] = NULL;
}

long RKC_DSOUND_Play(RKC_DSOUND *self, long vocId, long trackIdx, int flag, long volume, long pan)
{
    if (!self || vocId < 0 || vocId >= self->vocCount || !self->voc[vocId])
        return -1;

    RKC_DSOUND_Voc *voc = self->voc[vocId];
    if (trackIdx < 0 || trackIdx >= voc->trackCount)
        return -1;

    int loops = flag != 0 ? -1 : 0;
    int channel = Mix_PlayChannel(-1, voc->tracks[trackIdx], loops);
    if (channel < 0)
        return -1;

    RKC_DSOUND_SetVolume(self, vocId, channel, volume);

    if (pan < -10000)
        pan = -10000;
    if (pan > 10000)
        pan = 10000;
    Uint8 left = (Uint8)(pan <= 0 ? 255 : 255 - (255 * pan) / 10000);
    Uint8 right = (Uint8)(pan >= 0 ? 255 : 255 + (255 * pan) / 10000);
    Mix_SetPanning(channel, left, right);

    return channel;
}

int RKC_DSOUND_GetPlayStatus(RKC_DSOUND *self, long vocId, long handle)
{
    (void)self;
    (void)vocId;
    if (handle < 0)
        return 0;
    return Mix_Playing((int)handle) ? 1 : 0;
}

void RKC_DSOUND_SetVolume(RKC_DSOUND *self, long vocId, long handle, long volume)
{
    (void)self;
    (void)vocId;
    if (handle < 0)
        return;

    if (volume > 0)
        volume = 0;
    if (volume < -10000)
        volume = -10000;
    double linear = 1.0;
    if (volume < 0)
        linear = SDL_pow(10.0, (double)volume / 2000.0);
    Mix_Volume((int)handle, (int)(linear * MIX_MAX_VOLUME));
}

void RKC_DSOUND_Release(RKC_DSOUND *self)
{
    if (!self)
        return;

    if (self->voc)
    {
        for (long i = 0; i < self->vocCount; i++)
        {
            if (self->voc[i])
                RKC_DSOUND_ReleaseVoc(self, i);
        }
        free(self->voc);
    }

    if (self->initialized)
    {
        Mix_CloseAudio();
        self->initialized = 0;
    }

    memset(self, 0, sizeof(*self));
}
