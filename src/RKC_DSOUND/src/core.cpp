/**
 * RKC_DSOUND - DirectSound audio handling (reimplemented with haudio)
 * 
 * This DLL provides audio playback functions for ShadowFlare.
 * We use the haudio library for cross-platform audio output.
 * 
 * VOC File Format (VoiceData V003):
 *   Header (16 bytes): "VoiceData  V003\0"
 *   voice_count (4 bytes): Number of base voices
 *   variant_count (4 bytes): Number of variants per voice (for randomization)
 *   reserved (4 bytes)
 *   
 *   Per voice entry:
 *     flags (4 bytes): bit 0 = reference to another voice
 *     name (256 bytes): Voice name (null-terminated)
 *     [if not reference]:
 *       alt_name (256 bytes): Alternative name
 *       format (18 bytes): WAVEFORMATEX structure
 *       size (4 bytes): PCM data size
 *       data[size]: Raw PCM audio data
 *     [if reference]:
 *       (voice data points to another voice)
 *
 * Class layouts:
 *   RKC_DSOUND (0x10 bytes):
 *     +0x00: void* mixer       - haudio::Mixer* (was IDirectSound*)
 *     +0x04: long vocCount     - Number of VOC slots
 *     +0x08: void* vocs        - Array of RKC_DSOUND_VOC_IMPL*
 *     +0x0c: int initialized   - Initialization flag
 * 
 *   Original RKC_DSOUND_VOC (0x114 bytes) - we use our own internal struct
 *   Original RKC_DSOUND_VOICE (0x124 bytes) - we use haudio::Sound internally
 */

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

#define HAUDIO_IMPLEMENTATION
#include "../../happy/haudio.hpp"

// Forward declarations for original structs (for ABI compatibility)
struct IDirectSound;
struct IDirectSoundBuffer;

#ifndef _WAVEFORMATEX_
typedef struct tWAVEFORMATEX {
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
    WORD  wBitsPerSample;
    WORD  cbSize;
} WAVEFORMATEX;
#endif

// ============================================================================
// Internal Structures
// ============================================================================

// Internal representation of a single voice (sound sample)
struct VoiceData {
    char name[256];
    haudio::Sound sound;
    haudio::AudioFormat format;
    int refIndex;           // -1 if not a reference, otherwise index of source voice
    long volume;            // Current volume in DirectSound dB units (-10000 to 0)
    haudio::Voice* playing; // Currently playing voice (if any)
};

// Internal representation of a VOC container
struct VocContainer {
    bool loaded;
    int voiceCount;
    int variantCount;
    std::vector<VoiceData> voices;
    
    VocContainer() : loaded(false), voiceCount(0), variantCount(0) {}
};

// Global mixer (single instance for the DLL)
static haudio::Mixer* g_mixer = nullptr;

// Helper: Convert DirectSound dB volume (-10000 to 0) to linear (0.0 to 1.0)
static float dsVolumeToLinear(long dsVolume) {
    if (dsVolume <= -10000) return 0.0f;
    if (dsVolume >= 0) return 1.0f;
    // dB to linear: 10^(dB/2000) for DirectSound's hundredths of dB
    return powf(10.0f, dsVolume / 2000.0f);
}

// ============================================================================
// VOC File Loading
// ============================================================================

static bool loadVocFile(const char* filename, VocContainer& voc) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "[RKC_DSOUND] Failed to open: %s\n", filename);
        return false;
    }
    
    // Read header
    char header[16];
    if (fread(header, 1, 16, f) != 16) {
        fclose(f);
        return false;
    }
    
    // Check for "VoiceData  V003" or "VoiceData  V001" (note: two spaces)
    bool isV003 = (memcmp(header, "VoiceData  V003", 15) == 0);
    bool isV001 = (memcmp(header, "VoiceData  V001", 15) == 0);
    
    if (!isV003 && !isV001) {
        fprintf(stderr, "[RKC_DSOUND] Unknown VOC format in: %s\n", filename);
        fclose(f);
        return false;
    }
    
    // Read voice count
    uint32_t voiceCount;
    if (fread(&voiceCount, 4, 1, f) != 1) {
        fclose(f);
        return false;
    }
    
    // Read variant count (V003 only) - no reserved bytes, flags start immediately
    uint32_t variantCount = 0;
    if (isV003) {
        if (fread(&variantCount, 4, 1, f) != 1) {
            fclose(f);
            return false;
        }
    }
    
    voc.voiceCount = voiceCount;
    voc.variantCount = variantCount;
    
    // Total voices = base voices * (variants + 1)
    int totalVoices = voiceCount * (variantCount + 1);
    voc.voices.resize(totalVoices);
    
    // First pass: read all base voices
    for (uint32_t i = 0; i < voiceCount; i++) {
        VoiceData& voice = voc.voices[i];
        voice.refIndex = -1;
        voice.volume = 0;
        voice.playing = nullptr;
        
        // V003 has flags field
        uint32_t flags = 0;
        if (isV003) {
            if (fread(&flags, 4, 1, f) != 1) {
                fclose(f);
                return false;
            }
        }
        
        // Read name (256 bytes)
        if (fread(voice.name, 1, 256, f) != 256) {
            fclose(f);
            return false;
        }
        voice.name[255] = '\0';
        
        if (flags & 1) {
            // Reference voice - data comes from another voice
            // Just mark as reference, we'll resolve later
            voice.refIndex = -2; // Mark as unresolved reference
        } else {
            // Skip alt_name for V003
            if (isV003) {
                fseek(f, 256, SEEK_CUR);
            }
            
            // Read WAVEFORMATEX
            WAVEFORMATEX wfx;
            if (fread(&wfx, 1, 18, f) != 18) {
                fclose(f);
                return false;
            }
            
            voice.format.sampleRate = wfx.nSamplesPerSec;
            voice.format.channels = wfx.nChannels;
            voice.format.bitsPerSample = wfx.wBitsPerSample;
            
            // Read size
            uint32_t size;
            if (fread(&size, 4, 1, f) != 1) {
                fclose(f);
                return false;
            }
            
            if (wfx.wFormatTag == 1 && size > 0) {
                // PCM format - read audio data
                std::vector<uint8_t> pcmData(size);
                if (fread(pcmData.data(), 1, size, f) != size) {
                    fprintf(stderr, "[RKC_DSOUND] Voice %d: failed to read PCM data\n", i);
                    fclose(f);
                    return false;
                }
                
                voice.sound.loadRaw(pcmData.data(), size, voice.format);
            }
        }
    }
    
    // Second pass: resolve references
    for (uint32_t i = 0; i < voiceCount; i++) {
        if (voc.voices[i].refIndex == -2) {
            // Find matching voice by name
            for (uint32_t j = 0; j < voiceCount; j++) {
                if (i != j && voc.voices[j].refIndex == -1 &&
                    strcmp(voc.voices[i].name, voc.voices[j].name) == 0) {
                    voc.voices[i].refIndex = j;
                    voc.voices[i].format = voc.voices[j].format;
                    break;
                }
            }
        }
    }
    
    // Copy base voices to variant slots (they share the same sound data)
    for (int variant = 1; variant <= (int)variantCount; variant++) {
        for (uint32_t i = 0; i < voiceCount; i++) {
            int srcIdx = i;
            int dstIdx = variant * voiceCount + i;
            voc.voices[dstIdx].refIndex = srcIdx; // Point to base voice
            voc.voices[dstIdx].format = voc.voices[srcIdx].format;
            voc.voices[dstIdx].volume = 0;
            voc.voices[dstIdx].playing = nullptr;
            strncpy(voc.voices[dstIdx].name, voc.voices[srcIdx].name, 255);
        }
    }
    
    voc.loaded = true;
    fclose(f);
    return true;
}

// ============================================================================
// RKC_DSOUND Implementation
// ============================================================================

/**
 * RKC_DSOUND::constructor
 * USED BY: ShadowFlare.exe
 */
extern "C" void* __thiscall RKC_DSOUND_constructor(void* self) {
    char* p = (char*)self;
    *(void**)(p + 0x00) = nullptr;
    *(long*)(p + 0x04) = 0;
    *(void**)(p + 0x08) = nullptr;
    *(int*)(p + 0x0c) = 0;
    return self;
}

/**
 * RKC_DSOUND::~RKC_DSOUND (destructor)
 * USED BY: ShadowFlare.exe
 */
extern "C" void __thiscall RKC_DSOUND_destructor(void* self) {
    char* p = (char*)self;
    
    // Release all VOCs
    VocContainer** vocs = *(VocContainer***)(p + 0x08);
    long vocCount = *(long*)(p + 0x04);
    
    if (vocs) {
        for (long i = 0; i < vocCount; i++) {
            delete vocs[i];
        }
        delete[] vocs;
        *(void**)(p + 0x08) = nullptr;
    }
    
    // Shutdown mixer
    if (g_mixer) {
        g_mixer->shutdown();
        delete g_mixer;
        g_mixer = nullptr;
    }
    
    *(void**)(p + 0x00) = nullptr;
    *(int*)(p + 0x0c) = 0;
}

/**
 * RKC_DSOUND::Initialize
 * USED BY: ShadowFlare.exe
 */
extern "C" int __thiscall RKC_DSOUND_Initialize(void* self, void* hwnd, long vocCount) {
    char* p = (char*)self;
    
    // Create and init mixer
    if (!g_mixer) {
        g_mixer = new haudio::Mixer();
        // ShadowFlare uses 22050 Hz mono/stereo 16-bit audio typically
        haudio::AudioFormat fmt;
        fmt.sampleRate = 22050;
        fmt.channels = 2;
        fmt.bitsPerSample = 16;
        
        if (!g_mixer->init(fmt, 100)) {
            fprintf(stderr, "[RKC_DSOUND] Failed to initialize audio mixer\n");
            delete g_mixer;
            g_mixer = nullptr;
            return 0;
        }
    }
    
    *(void**)(p + 0x00) = (void*)g_mixer; // Store mixer pointer where IDirectSound* was
    *(long*)(p + 0x04) = vocCount;
    
    // Allocate VOC container array
    VocContainer** vocs = new VocContainer*[vocCount];
    for (long i = 0; i < vocCount; i++) {
        vocs[i] = new VocContainer();
    }
    *(VocContainer***)(p + 0x08) = vocs;
    *(int*)(p + 0x0c) = 1;
    
    return 1;
}

/**
 * RKC_DSOUND::Release
 * USED BY: ShadowFlare.exe
 */
extern "C" void __thiscall RKC_DSOUND_Release(void* self) {
    char* p = (char*)self;
    
    // IMPORTANT: Reset mixer FIRST - this stops all pending callbacks
    // before we delete the VOC data they might be accessing
    if (g_mixer) {
        g_mixer->reset();
    }
    
    VocContainer** vocs = *(VocContainer***)(p + 0x08);
    long vocCount = *(long*)(p + 0x04);
    
    if (vocs) {
        for (long i = 0; i < vocCount; i++) {
            delete vocs[i];
        }
        delete[] vocs;
        *(void**)(p + 0x08) = nullptr;
    }
    
    *(void**)(p + 0x00) = nullptr;
    *(int*)(p + 0x0c) = 0;
}

/**
 * RKC_DSOUND::ReadVocFile
 * USED BY: ShadowFlare.exe
 */
extern "C" int __thiscall RKC_DSOUND_ReadVocFile(void* self, const char* filename, long vocIndex) {
    char* p = (char*)self;
    
    if (!*(void**)(p + 0x00)) return 0;
    
    VocContainer** vocs = *(VocContainer***)(p + 0x08);
    if (!vocs || vocIndex < 0) return 0;
    
    VocContainer* voc = vocs[vocIndex];
    if (!voc) return 0;
    
    // Release existing data
    voc->voices.clear();
    voc->loaded = false;
    
    // Load new file
    if (!loadVocFile(filename, *voc)) {
        return 0;
    }
    
    return 1;
}

/**
 * RKC_DSOUND::ReleaseVoc
 * USED BY: ShadowFlare.exe
 */
extern "C" void __thiscall RKC_DSOUND_ReleaseVoc(void* self, long vocIndex) {
    char* p = (char*)self;
    
    if (!*(void**)(p + 0x00)) return;
    
    VocContainer** vocs = *(VocContainer***)(p + 0x08);
    long vocCount = *(long*)(p + 0x04);
    
    if (!vocs) return;
    
    // CRITICAL: Reset mixer to stop all callbacks before touching sound data
    // This ensures no callback is accessing sound data while we delete it
    if (g_mixer) {
        g_mixer->reset();
    }
    
    // Helper lambda to stop all playing sounds in a VOC
    auto stopVocSounds = [](VocContainer* voc) {
        if (!voc) return;
        for (auto& voice : voc->voices) {
            if (voice.playing) {
                voice.playing = nullptr;  // haudio Voice already stopped by reset()
            }
        }
    };
    
    if (vocIndex == -1) {
        // Release all
        for (long i = 0; i < vocCount; i++) {
            if (vocs[i]) {
                stopVocSounds(vocs[i]);
                vocs[i]->voices.clear();
                vocs[i]->loaded = false;
            }
        }
    } else if (vocIndex >= 0 && vocIndex < vocCount) {
        if (vocs[vocIndex]) {
            stopVocSounds(vocs[vocIndex]);
            vocs[vocIndex]->voices.clear();
            vocs[vocIndex]->loaded = false;
        }
    }
}

/**
 * RKC_DSOUND::Play
 * USED BY: ShadowFlare.exe
 * 
 * Play(vocIndex, voiceIndex, loop, pan, volume)
 * Returns the variant index used, or -1 on failure
 */
extern "C" long __thiscall RKC_DSOUND_Play(void* self, long vocIndex, long voiceIndex, 
                                            int loop, long pan, long volume) {
    char* p = (char*)self;
    
    if (!*(void**)(p + 0x00)) return -1;
    if (!g_mixer) return -1;
    
    VocContainer** vocs = *(VocContainer***)(p + 0x08);
    if (!vocs) return -1;
    
    VocContainer* voc = vocs[vocIndex];
    if (!voc) return -1;
    if (!voc->loaded) return -1;
    
    // For looping sounds (BGM), stop all audio first using reset()
    if (loop) {
        g_mixer->reset();
    }
    
    // Find a free variant slot
    int totalVoices = voc->voiceCount * (voc->variantCount + 1);
    int variantUsed = 0;
    
    for (int variant = 0; variant <= voc->variantCount; variant++) {
        int idx = variant * voc->voiceCount + voiceIndex;
        if (idx >= totalVoices) break;
        
        VoiceData& voice = voc->voices[idx];
        
        // Check if this slot is free (not playing)
        if (voice.playing && voice.playing->active()) {
            continue; // This variant is busy
        }
        
        // Get the actual sound (may be a reference)
        haudio::Sound* sound = &voice.sound;
        if (voice.refIndex >= 0 && voice.refIndex < (int)voc->voices.size()) {
            sound = &voc->voices[voice.refIndex].sound;
        }
        
        if (!sound->valid()) {
            continue; // No sound data
        }
        
        // Play the sound
        float vol = dsVolumeToLinear(volume);
        voice.playing = g_mixer->play(*sound, vol, loop != 0);
        voice.volume = volume;
        variantUsed = variant;
        
        return variantUsed;
    }
    
    return -1; // No free slots
}

/**
 * RKC_DSOUND::Stop
 * USED BY: ShadowFlare.exe
 */
extern "C" void __thiscall RKC_DSOUND_Stop(void* self, long vocIndex, long voiceIndex) {
    char* p = (char*)self;
    
    if (!*(void**)(p + 0x00)) return;
    
    VocContainer** vocs = *(VocContainer***)(p + 0x08);
    if (!vocs) return;
    
    VocContainer* voc = vocs[vocIndex];
    if (!voc || !voc->loaded) return;
    
    // Stop all variants of this voice
    for (int variant = 0; variant <= voc->variantCount; variant++) {
        int idx = variant * voc->voiceCount + voiceIndex;
        if (idx >= (int)voc->voices.size()) break;
        
        VoiceData& voice = voc->voices[idx];
        if (voice.playing) {
            voice.playing->stop();
            voice.playing = nullptr;
        }
    }
}

/**
 * RKC_DSOUND::GetPlayStatus
 * USED BY: ShadowFlare.exe
 */
extern "C" int __thiscall RKC_DSOUND_GetPlayStatus(void* self, long vocIndex, long voiceIndex) {
    char* p = (char*)self;
    
    if (!*(void**)(p + 0x00)) return 0;
    
    VocContainer** vocs = *(VocContainer***)(p + 0x08);
    if (!vocs) return 0;
    
    VocContainer* voc = vocs[vocIndex];
    if (!voc || !voc->loaded) return 0;
    
    // Check if any variant is playing
    for (int variant = 0; variant <= voc->variantCount; variant++) {
        int idx = variant * voc->voiceCount + voiceIndex;
        if (idx >= (int)voc->voices.size()) break;
        
        VoiceData& voice = voc->voices[idx];
        if (voice.playing && voice.playing->active()) {
            return 1;
        }
    }
    
    return 0;
}

/**
 * RKC_DSOUND::SetVolume
 * USED BY: ShadowFlare.exe
 */
extern "C" void __thiscall RKC_DSOUND_SetVolume(void* self, long vocIndex, long voiceIndex, long volume) {
    char* p = (char*)self;
    
    if (!*(void**)(p + 0x00)) return;
    
    VocContainer** vocs = *(VocContainer***)(p + 0x08);
    if (!vocs) return;
    
    VocContainer* voc = vocs[vocIndex];
    if (!voc || !voc->loaded) return;
    
    // Set volume on all variants
    float vol = dsVolumeToLinear(volume);
    
    for (int variant = 0; variant <= voc->variantCount; variant++) {
        int idx = variant * voc->voiceCount + voiceIndex;
        if (idx >= (int)voc->voices.size()) break;
        
        VoiceData& voice = voc->voices[idx];
        voice.volume = volume;
        if (voice.playing) {
            voice.playing->setVolume(vol);
        }
    }
}

/**
 * RKC_DSOUND::GetVolume
 */
extern "C" long __thiscall RKC_DSOUND_GetVolume(void* self, long vocIndex, long voiceIndex) {
    char* p = (char*)self;
    
    if (!*(void**)(p + 0x00)) return -1;
    
    VocContainer** vocs = *(VocContainer***)(p + 0x08);
    if (!vocs) return -1;
    
    VocContainer* voc = vocs[vocIndex];
    if (!voc || !voc->loaded) return -1;
    
    if (voiceIndex >= 0 && voiceIndex < (int)voc->voices.size()) {
        return voc->voices[voiceIndex].volume;
    }
    
    return -1;
}

/**
 * RKC_DSOUND::GetSoundObject - Returns mixer (for compatibility)
 */
extern "C" void* __thiscall RKC_DSOUND_GetSoundObject(void* self) {
    return *(void**)self;
}

/**
 * RKC_DSOUND::GetVoice - Not really used, return nullptr
 */
extern "C" void* __thiscall RKC_DSOUND_GetVoice(void* self, long vocIndex, long voiceIndex) {
    return nullptr;
}

/**
 * RKC_DSOUND::SetVocCount - Resize VOC array
 */
extern "C" int __thiscall RKC_DSOUND_SetVocCount(void* self, long count) {
    char* p = (char*)self;
    
    if (!*(void**)(p + 0x00)) return 0;
    if (count < 1) return 0;
    
    // Release old VOCs
    VocContainer** oldVocs = *(VocContainer***)(p + 0x08);
    long oldCount = *(long*)(p + 0x04);
    
    if (oldVocs) {
        for (long i = 0; i < oldCount; i++) {
            delete oldVocs[i];
        }
        delete[] oldVocs;
    }
    
    // Allocate new array
    VocContainer** vocs = new VocContainer*[count];
    for (long i = 0; i < count; i++) {
        vocs[i] = new VocContainer();
    }
    
    *(VocContainer***)(p + 0x08) = vocs;
    *(long*)(p + 0x04) = count;
    
    return 1;
}

/**
 * RKC_DSOUND::operator=
 */
extern "C" void* __thiscall RKC_DSOUND_operatorAssign(void* self, const void* other) {
    return self;
}

// ============================================================================
// RKC_DSOUND_VOICE Stubs (not used directly by EXE, but exported)
// ============================================================================

extern "C" void* __thiscall RKC_DSOUND_VOICE_constructor(void* self) {
    memset(self, 0, 0x124);
    *(long*)((char*)self + 0x104) = -1;
    return self;
}

extern "C" void __thiscall RKC_DSOUND_VOICE_destructor(void* self) {}

extern "C" void* __thiscall RKC_DSOUND_VOICE_operatorAssign(void* self, const void* other) {
    return self;
}

extern "C" char* __thiscall RKC_DSOUND_VOICE_GetName(void* self) {
    return (char*)self;
}

extern "C" long __thiscall RKC_DSOUND_VOICE_GetSize(void* self) {
    return *(long*)((char*)self + 0x100);
}

extern "C" WAVEFORMATEX* __thiscall RKC_DSOUND_VOICE_GetFormat(void* self) {
    return (WAVEFORMATEX*)((char*)self + 0x108);
}

extern "C" IDirectSoundBuffer* __thiscall RKC_DSOUND_VOICE_GetBuffer(void* self) {
    return *(IDirectSoundBuffer**)((char*)self + 0x11c);
}

extern "C" int __thiscall RKC_DSOUND_VOICE_GetPlayStatus(void* self) { return 0; }
extern "C" long __thiscall RKC_DSOUND_VOICE_GetVolume(void* self) { return 0; }
extern "C" void __thiscall RKC_DSOUND_VOICE_Play(void* self, int loop, long pan, long volume) {}
extern "C" void __thiscall RKC_DSOUND_VOICE_Release(void* self) {}
extern "C" void __thiscall RKC_DSOUND_VOICE_SetBuffer(void* self, void* buffer) {}
extern "C" void __thiscall RKC_DSOUND_VOICE_SetFormat(void* self, void* format) {}
extern "C" void __thiscall RKC_DSOUND_VOICE_SetImage(void* self, char* data, long size) {}
extern "C" void __thiscall RKC_DSOUND_VOICE_SetName(void* self, char* name) {}
extern "C" void __thiscall RKC_DSOUND_VOICE_SetSize(void* self, long size) {}
extern "C" void __thiscall RKC_DSOUND_VOICE_SetVolume(void* self, long volume) {}
extern "C" void __thiscall RKC_DSOUND_VOICE_Stop(void* self) {}

// ============================================================================
// RKC_DSOUND_VOC Stubs (not used directly by EXE, but exported)
// ============================================================================

extern "C" void* __thiscall RKC_DSOUND_VOC_constructor(void* self) {
    memset(self, 0, 0x114);
    return self;
}

extern "C" void __thiscall RKC_DSOUND_VOC_destructor(void* self) {}

extern "C" void* __thiscall RKC_DSOUND_VOC_operatorAssign(void* self, const void* other) {
    return self;
}

extern "C" int __thiscall RKC_DSOUND_VOC_GetPlayStatus(void* self, long index) { return 0; }
extern "C" long __thiscall RKC_DSOUND_VOC_GetVolume(void* self, long index) { return 0; }
extern "C" long __thiscall RKC_DSOUND_VOC_Play(void* self, long index, int loop, long pan, long volume) { return 0; }
extern "C" int __thiscall RKC_DSOUND_VOC_Read(void* self, void* dsound, char* filename) { return 0; }
extern "C" void __thiscall RKC_DSOUND_VOC_Release(void* self) {}
extern "C" void __thiscall RKC_DSOUND_VOC_SetVolume(void* self, long index, long volume) {}
extern "C" void __thiscall RKC_DSOUND_VOC_Stop(void* self, long index) {}
