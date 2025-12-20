/**
 * RKC_NETWORK - Network handling (incremental implementation)
 * 
 * Provides networking functionality for multiplayer.
 */

#include <windows.h>
#include <cstdint>

extern "C" {

// ============================================================================
// RKC_NETWORK_PACKET Class Layout (from decompilation)
// ============================================================================
// Offset  Field
// 0x00    DWORD line (or ID) - initialized to -1
// 0x04    DWORD unknown1 - initialized to 0
// 0x08    DWORD unknown2 - initialized to 0
// 0x0c    DWORD unknown3 - initialized to 0
// 0x10    void* data - initialized to 0
// 0x18    RKC_NETWORK_PACKET* next - initialized to 0
// Total size: ~0x1c bytes

/**
 * RKC_NETWORK_PACKET::constructor - Initialize packet object
 * NOT REFERENCED - internal class
 */
void* __thiscall RKC_NETWORK_PACKET_constructor(void* self) {
    char* p = (char*)self;
    *(long*)(p + 0x00) = -1;  // line/ID
    *(long*)(p + 0x04) = 0;
    *(long*)(p + 0x08) = 0;
    *(long*)(p + 0x0c) = 0;
    *(long*)(p + 0x10) = 0;   // data
    *(long*)(p + 0x18) = 0;   // next
    return self;
}

/**
 * RKC_NETWORK_PACKET::GetNext - Get next packet in list
 * USED BY: ShadowFlare.exe
 */
void* __thiscall RKC_NETWORK_PACKET_GetNext(void* self) {
    return *(void**)((char*)self + 0x18);
}

/**
 * RKC_NETWORK_PACKET::GetLine - Get line/ID value
 * USED BY: ShadowFlare.exe
 * Note: Same code as GetID, GetServer at offset 0
 */
long __thiscall RKC_NETWORK_PACKET_GetLine(void* self) {
    return *(long*)((char*)self);
}

/**
 * RKC_NETWORK_PACKET::GetID - Get ID value
 * USED BY: o_RKC_NETWORK.dll (internal)
 */
long __thiscall RKC_NETWORK_PACKET_GetID(void* self) {
    return *(long*)((char*)self + 0x04);
}

/**
 * RKC_NETWORK_PACKET::GetData - Get data pointer
 * USED BY: o_RKC_NETWORK.dll (internal)
 */
void* __thiscall RKC_NETWORK_PACKET_GetData(void* self) {
    return *(void**)((char*)self + 0x10);
}

// ============================================================================
// RKC_NETWORK_PACKETBLOCK Class Layout
// ============================================================================
// Offset  Field
// 0x00    RKC_NETWORK_PACKET* first - initialized to 0

/**
 * RKC_NETWORK_PACKETBLOCK::constructor - Initialize packet block
 * Also used as RKC_NETWORK_USERINFOBLOCK::constructor
 * NOT REFERENCED - internal class
 */
void* __thiscall RKC_NETWORK_PACKETBLOCK_constructor(void* self) {
    *(long*)self = 0;
    return self;
}

// ============================================================================
// RKC_NETWORK_USERINFO Class Layout
// ============================================================================
// Offset  Field
// 0x00    DWORD id - initialized to -1
// 0x04    char* userName - initialized to 0
// 0x08    DWORD unknown1 - initialized to 0
// 0x0c    DWORD unknown2 - initialized to 0
// 0x10    void* next - initialized to 0

/**
 * RKC_NETWORK_USERINFO::constructor - Initialize user info
 * NOT REFERENCED - internal class
 */
void* __thiscall RKC_NETWORK_USERINFO_constructor(void* self) {
    char* p = (char*)self;
    *(long*)(p + 0x00) = -1;  // id
    *(long*)(p + 0x04) = 0;   // userName
    *(long*)(p + 0x08) = 0;
    *(long*)(p + 0x0c) = 0;
    *(long*)(p + 0x10) = 0;   // next
    return self;
}

/**
 * RKC_NETWORK_USERINFO::GetID - Get user ID
 * USED BY: o_RKC_NETWORK.dll (internal)
 */
long __thiscall RKC_NETWORK_USERINFO_GetID(void* self) {
    return *(long*)((char*)self);
}

/**
 * RKC_NETWORK_USERINFO::GetUserNameA - Get user name string
 * USED BY: o_RKC_NETWORK.dll (internal)
 */
char* __thiscall RKC_NETWORK_USERINFO_GetUserNameA(void* self) {
    return *(char**)((char*)self + 0x04);
}

// ============================================================================
// RKC_NETWORK_TRANSFER Class Layout
// ============================================================================
// Empty - just returns self

/**
 * RKC_NETWORK_TRANSFER::constructor - Initialize transfer object
 * NOT REFERENCED - internal class
 */
void* __thiscall RKC_NETWORK_TRANSFER_constructor(void* self) {
    return self;
}

// ============================================================================
// RKC_NETWORK_SERVER_CONNECTION Class Layout
// ============================================================================
// Offset  Field
// 0x00    DWORD id
// 0x04    DWORD userId
// 0x10    DWORD status

/**
 * RKC_NETWORK_SERVER_CONNECTION::GetID - Get connection ID
 * USED BY: o_RKC_NETWORK.dll (internal)
 */
long __thiscall RKC_NETWORK_SERVER_CONNECTION_GetID(void* self) {
    return *(long*)((char*)self);
}

/**
 * RKC_NETWORK_SERVER_CONNECTION::GetUserID - Get user ID
 * USED BY: o_RKC_NETWORK.dll (internal)
 */
long __thiscall RKC_NETWORK_SERVER_CONNECTION_GetUserID(void* self) {
    return *(long*)((char*)self + 0x04);
}

/**
 * RKC_NETWORK_SERVER_CONNECTION::GetStatus - Get connection status
 * USED BY: o_RKC_NETWORK.dll (internal)
 */
long __thiscall RKC_NETWORK_SERVER_CONNECTION_GetStatus(void* self) {
    return *(long*)((char*)self + 0x10);
}

// ============================================================================
// RKC_NETWORK Class Layout
// ============================================================================
// Offset  Field
// 0x00    RKC_NETWORK_SERVER* server
// 0x04    RKC_NETWORK_CLIENT* client

/**
 * RKC_NETWORK::GetServer - Get server object
 * USED BY: o_RKC_NETWORK.dll (internal)
 */
void* __thiscall RKC_NETWORK_GetServer(void* self) {
    return *(void**)((char*)self);
}

/**
 * RKC_NETWORK::GetClient - Get client object
 * USED BY: o_RKC_NETWORK.dll (internal)
 */
void* __thiscall RKC_NETWORK_GetClient(void* self) {
    return *(void**)((char*)self + 0x04);
}

// ============================================================================
// RKC_NETWORK_CLIENT and RKC_NETWORK_SERVER - Shared getter
// ============================================================================
// Both have ActiveFlag at offset 0x04

/**
 * RKC_NETWORK_CLIENT::GetActiveFlag - Get client active flag
 * USED BY: ShadowFlare.exe
 */
int __thiscall RKC_NETWORK_CLIENT_GetActiveFlag(void* self) {
    return *(int*)((char*)self + 0x04);
}

/**
 * RKC_NETWORK_SERVER::GetActiveFlag - Get server active flag
 * USED BY: ShadowFlare.exe
 */
int __thiscall RKC_NETWORK_SERVER_GetActiveFlag(void* self) {
    return *(int*)((char*)self);
}

} // extern "C"
