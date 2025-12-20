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
