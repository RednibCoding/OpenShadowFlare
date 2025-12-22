/**
 * RKC_DSOUND - DirectSound audio handling
 * 
 * This DLL provides audio playback functions for ShadowFlare.
 * 
 * Class layouts (from reverse engineering):
 * 
 * RKC_DSOUND class (size ~0x10 bytes):
 *   +0x00: IDirectSound* dsound  - DirectSound interface
 *   +0x04: void* unknown1
 *   +0x08: void* unknown2  
 *   +0x0c: void* vocs             - VOC array/list
 * 
 * RKC_DSOUND_VOICE class (size 0x124 bytes):
 *   +0x00:  char[256] name        - Sound name (256 bytes)
 *   +0x100: long size             - Buffer size
 *   +0x104: long unknown          - Set to -1 in constructor
 *   +0x108: WAVEFORMATEX format   - Wave format (embedded, ~20 bytes)
 *   +0x11c: IDirectSoundBuffer* buffer
 *   +0x120: void* unknown2
 */

#include <windows.h>
#include <mmsystem.h>

// Forward declarations for DirectSound types
struct IDirectSound;
struct IDirectSoundBuffer;

// WAVEFORMATEX if not defined
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
// RKC_DSOUND FUNCTIONS
// ============================================================================

/**
 * RKC_DSOUND::constructor - Initialize sound manager
 * USED BY: ShadowFlare.exe
 * 
 * Zeros dsound pointer and vocs pointer.
 */
extern "C" void* __thiscall RKC_DSOUND_constructor(void* self) {
    char* p = (char*)self;
    *(void**)(p + 0x00) = nullptr;  // dsound
    *(void**)(p + 0x0c) = nullptr;  // vocs
    return self;
}

/**
 * RKC_DSOUND::GetSoundObject - Get DirectSound interface
 * NOT REFERENCED - stub only, not imported by any module
 */
extern "C" IDirectSound* __thiscall RKC_DSOUND_GetSoundObject(void* self) {
    return *(IDirectSound**)self;
}

// ============================================================================
// RKC_DSOUND_VOICE FUNCTIONS
// ============================================================================

/**
 * RKC_DSOUND_VOICE::constructor - Initialize voice object
 * USED BY: o_RKC_DSOUND.dll (internal)
 * 
 * Zeros size, buffer, and sets unknown to -1.
 */
extern "C" void* __thiscall RKC_DSOUND_VOICE_constructor(void* self) {
    char* p = (char*)self;
    *(long*)(p + 0x100) = 0;        // size
    *(void**)(p + 0x11c) = nullptr; // buffer
    *(void**)(p + 0x120) = nullptr; // unknown
    *(long*)(p + 0x104) = -1;       // unknown, set to -1
    return self;
}

/**
 * RKC_DSOUND_VOICE::GetName - Get voice name
 * NOT REFERENCED - stub only, not imported by any module
 * 
 * Name is at the start of the struct (offset 0).
 */
extern "C" char* __thiscall RKC_DSOUND_VOICE_GetName(void* self) {
    return (char*)self;
}

/**
 * RKC_DSOUND_VOICE::GetSize - Get buffer size
 * NOT REFERENCED - stub only, not imported by any module
 */
extern "C" long __thiscall RKC_DSOUND_VOICE_GetSize(void* self) {
    return *(long*)((char*)self + 0x100);
}

/**
 * RKC_DSOUND_VOICE::GetFormat - Get wave format
 * NOT REFERENCED - stub only, not imported by any module
 * 
 * Returns pointer to embedded WAVEFORMATEX at offset 0x108.
 */
extern "C" WAVEFORMATEX* __thiscall RKC_DSOUND_VOICE_GetFormat(void* self) {
    return (WAVEFORMATEX*)((char*)self + 0x108);
}

/**
 * RKC_DSOUND_VOICE::GetBuffer - Get DirectSound buffer
 * NOT REFERENCED - stub only, not imported by any module
 */
extern "C" IDirectSoundBuffer* __thiscall RKC_DSOUND_VOICE_GetBuffer(void* self) {
    return *(IDirectSoundBuffer**)((char*)self + 0x11c);
}

// ============================================================================
// RKC_DSOUND_VOC Class
// ============================================================================
// Layout (from decompilation):
//   0x00: DWORD unknown1
//   0x04: BYTE  unknown2 
//   0x05-0x107: possibly char array (name/path?)
//   0x108: DWORD unknown3
//   0x10c: DWORD unknown4
//   0x110: DWORD unknown5
// Total size: ~0x114 bytes

/**
 * RKC_DSOUND_VOC::constructor - Initialize VOC object
 * NOT REFERENCED - internal class, not imported by any module
 */
extern "C" void* __thiscall RKC_DSOUND_VOC_constructor(void* self) {
    char* p = (char*)self;
    *(long*)(p + 0x00) = 0;
    *(char*)(p + 0x04) = 0;
    *(long*)(p + 0x108) = 0;
    *(long*)(p + 0x10c) = 0;
    *(long*)(p + 0x110) = 0;
    return self;
}

// ============================================================================
// STUBS FOR UNUSED FUNCTIONS
// The following functions are exported but NOT imported by ShadowFlare.exe.
// They are only used internally by the original DLL or not at all.
// ============================================================================

// --- RKC_DSOUND_VOICE stubs ---

/**
 * RKC_DSOUND_VOICE::destructor - Destroy voice object
 * NOT REFERENCED - stub only
 */
extern "C" void __thiscall RKC_DSOUND_VOICE_destructor(void* self) {
    // Stub - would normally release DirectSound buffer
}

/**
 * RKC_DSOUND_VOICE::operator= - Copy voice object
 * NOT REFERENCED - stub only
 */
extern "C" void* __thiscall RKC_DSOUND_VOICE_operatorAssign(void* self, const void* other) {
    return self;
}

/**
 * RKC_DSOUND_VOICE::GetPlayStatus - Check if playing
 * NOT REFERENCED - stub only
 */
extern "C" int __thiscall RKC_DSOUND_VOICE_GetPlayStatus(void* self) {
    return 0;  // Not playing
}

/**
 * RKC_DSOUND_VOICE::GetVolume - Get volume level
 * NOT REFERENCED - stub only
 */
extern "C" long __thiscall RKC_DSOUND_VOICE_GetVolume(void* self) {
    return 0;
}

/**
 * RKC_DSOUND_VOICE::Play - Play sound
 * NOT REFERENCED - stub only
 */
extern "C" void __thiscall RKC_DSOUND_VOICE_Play(void* self, int loop, long pan, long volume) {
    // Stub
}

/**
 * RKC_DSOUND_VOICE::Release - Release voice resources
 * NOT REFERENCED - stub only
 */
extern "C" void __thiscall RKC_DSOUND_VOICE_Release(void* self) {
    // Stub
}

/**
 * RKC_DSOUND_VOICE::SetBuffer - Set DirectSound buffer
 * NOT REFERENCED - stub only
 */
extern "C" void __thiscall RKC_DSOUND_VOICE_SetBuffer(void* self, void* buffer) {
    *(void**)((char*)self + 0x11c) = buffer;
}

/**
 * RKC_DSOUND_VOICE::SetFormat - Set wave format
 * NOT REFERENCED - stub only
 */
extern "C" void __thiscall RKC_DSOUND_VOICE_SetFormat(void* self, void* format) {
    // Copy format to embedded WAVEFORMATEX at offset 0x108
    if (format) {
        memcpy((char*)self + 0x108, format, sizeof(WAVEFORMATEX));
    }
}

/**
 * RKC_DSOUND_VOICE::SetImage - Set sound data
 * NOT REFERENCED - stub only
 */
extern "C" void __thiscall RKC_DSOUND_VOICE_SetImage(void* self, char* data, long size) {
    // Stub
}

/**
 * RKC_DSOUND_VOICE::SetName - Set voice name
 * NOT REFERENCED - stub only
 */
extern "C" void __thiscall RKC_DSOUND_VOICE_SetName(void* self, char* name) {
    if (name) {
        strncpy((char*)self, name, 255);
        ((char*)self)[255] = 0;
    }
}

/**
 * RKC_DSOUND_VOICE::SetSize - Set buffer size
 * NOT REFERENCED - stub only
 */
extern "C" void __thiscall RKC_DSOUND_VOICE_SetSize(void* self, long size) {
    *(long*)((char*)self + 0x100) = size;
}

/**
 * RKC_DSOUND_VOICE::SetVolume - Set volume level
 * NOT REFERENCED - stub only
 */
extern "C" void __thiscall RKC_DSOUND_VOICE_SetVolume(void* self, long volume) {
    // Stub
}

/**
 * RKC_DSOUND_VOICE::Stop - Stop playback
 * NOT REFERENCED - stub only
 */
extern "C" void __thiscall RKC_DSOUND_VOICE_Stop(void* self) {
    // Stub
}

// --- RKC_DSOUND_VOC stubs ---

/**
 * RKC_DSOUND_VOC::destructor - Destroy VOC object
 * NOT REFERENCED - stub only
 */
extern "C" void __thiscall RKC_DSOUND_VOC_destructor(void* self) {
    // Stub
}

/**
 * RKC_DSOUND_VOC::operator= - Copy VOC object
 * NOT REFERENCED - stub only
 */
extern "C" void* __thiscall RKC_DSOUND_VOC_operatorAssign(void* self, const void* other) {
    return self;
}

/**
 * RKC_DSOUND_VOC::GetPlayStatus - Check if playing
 * NOT REFERENCED - stub only
 */
extern "C" int __thiscall RKC_DSOUND_VOC_GetPlayStatus(void* self, long index) {
    return 0;  // Not playing
}

/**
 * RKC_DSOUND_VOC::GetVolume - Get volume level
 * NOT REFERENCED - stub only
 */
extern "C" long __thiscall RKC_DSOUND_VOC_GetVolume(void* self, long index) {
    return 0;
}

/**
 * RKC_DSOUND_VOC::Play - Play VOC sound
 * NOT REFERENCED - stub only
 */
extern "C" long __thiscall RKC_DSOUND_VOC_Play(void* self, long index, int loop, long pan, long volume) {
    return 0;
}

/**
 * RKC_DSOUND_VOC::Read - Read VOC file
 * NOT REFERENCED - stub only
 */
extern "C" int __thiscall RKC_DSOUND_VOC_Read(void* self, void* dsound, char* filename) {
    return 0;  // Failure
}

/**
 * RKC_DSOUND_VOC::Release - Release VOC resources
 * NOT REFERENCED - stub only
 */
extern "C" void __thiscall RKC_DSOUND_VOC_Release(void* self) {
    // Stub
}

/**
 * RKC_DSOUND_VOC::SetVolume - Set volume level
 * NOT REFERENCED - stub only
 */
extern "C" void __thiscall RKC_DSOUND_VOC_SetVolume(void* self, long index, long volume) {
    // Stub
}

/**
 * RKC_DSOUND_VOC::Stop - Stop playback
 * NOT REFERENCED - stub only
 */
extern "C" void __thiscall RKC_DSOUND_VOC_Stop(void* self, long index) {
    // Stub
}

// --- RKC_DSOUND stubs (unused functions) ---

/**
 * RKC_DSOUND::operator= - Copy sound manager
 * NOT REFERENCED - stub only
 */
extern "C" void* __thiscall RKC_DSOUND_operatorAssign(void* self, const void* other) {
    return self;
}

/**
 * RKC_DSOUND::GetVoice - Get voice by index
 * NOT REFERENCED - stub only
 */
extern "C" void* __thiscall RKC_DSOUND_GetVoice(void* self, long vocIndex, long voiceIndex) {
    return nullptr;
}

/**
 * RKC_DSOUND::GetVolume - Get volume for voc/voice
 * NOT REFERENCED - stub only
 */
extern "C" long __thiscall RKC_DSOUND_GetVolume(void* self, long vocIndex, long voiceIndex) {
    return 0;
}

/**
 * RKC_DSOUND::SetVocCount - Set number of VOC slots
 * NOT REFERENCED - stub only
 */
extern "C" int __thiscall RKC_DSOUND_SetVocCount(void* self, long count) {
    return 0;
}

/**
 * RKC_DSOUND::Stop - Stop playback
 * NOT REFERENCED - stub only
 */
extern "C" void __thiscall RKC_DSOUND_Stop(void* self, long vocIndex, long voiceIndex) {
    // Stub
}