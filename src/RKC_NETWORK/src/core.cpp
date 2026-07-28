/**
 * RKC_NETWORK - Network handling (incremental implementation)
 * 
 * Provides networking functionality for multiplayer.
 */

#include <winsock2.h>
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <new>

namespace {
HANDLE gNetworkMutex = nullptr;

HANDLE NetworkMutex() {
    if (!gNetworkMutex)
        gNetworkMutex = CreateMutexA(nullptr, FALSE, nullptr);
    return gNetworkMutex;
}

struct NetworkPacket {
    long line;
    long id;
    long size;
    long infoId;
    void* data;
    long disc;
    NetworkPacket* next;
};

struct NetworkUserInfo {
    long id;
    char* userName;
    char* password;
    void* userData;
    NetworkUserInfo* next;
};

char* CopyGlobalString(const char* source) {
    if (!source)
        return nullptr;
    const SIZE_T size = std::strlen(source) + 1;
    char* result = static_cast<char*>(GlobalAlloc(GPTR, size));
    if (result)
        std::memcpy(result, source, size);
    return result;
}
}

extern "C" {

// ============================================================================
// RKC_NETWORK_PACKET Class Layout (from decompilation)
// ============================================================================
// Offset  Field
// 0x00    long line              - packet line/index, initialized to -1
// 0x04    long id                - packet ID, initialized to 0
// 0x08    long size              - data size, initialized to 0
// 0x0c    long infoId            - info ID, initialized to 0
// 0x10    void* data             - data pointer, initialized to 0
// 0x14    long disc              - disconnect flag? (constructor leaves unchanged)
// 0x18    RKC_NETWORK_PACKET* next - next packet in list, initialized to 0
// Total size: 0x1c bytes

/**
 * RKC_NETWORK_PACKET::constructor - Initialize packet object
 * NOT REFERENCED - internal class
 */
void* __thiscall RKC_NETWORK_PACKET_constructor(void* self) {
    char* p = (char*)self;
    *(long*)(p + 0x00) = -1;       // line
    *(long*)(p + 0x04) = 0;        // id
    *(long*)(p + 0x08) = 0;        // size
    *(long*)(p + 0x0c) = 0;        // infoId
    *(void**)(p + 0x10) = nullptr; // data
    *(void**)(p + 0x18) = nullptr; // next
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
 * USED BY: ShadowFlare.exe
 */
void* __thiscall RKC_NETWORK_PACKET_GetData(void* self) {
    return *(void**)((char*)self + 0x10);
}

/**
 * RKC_NETWORK_PACKET::GetSize - Get data size
 * USED BY: ShadowFlare.exe
 */
long __thiscall RKC_NETWORK_PACKET_GetSize(void* self) {
    return *(long*)((char*)self + 0x08);
}

/**
 * RKC_NETWORK_PACKET::GetInfoID - Get info ID
 * USED BY: ShadowFlare.exe
 */
long __thiscall RKC_NETWORK_PACKET_GetInfoID(void* self) {
    return *(long*)((char*)self + 0x0c);
}

/**
 * RKC_NETWORK_PACKET::GetDisc - Get disconnect flag
 * USED BY: ShadowFlare.exe
 */
long __thiscall RKC_NETWORK_PACKET_GetDisc(void* self) {
    return *(long*)((char*)self + 0x14);
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

// ============================================================================
// FOUNDATIONAL CLASSES AND REMAINING TRANSPORT STUBS
// Packet ownership, user information, containers, and transfer framing below
// are reconstructed. The client/server transport classes remain incremental.
// ============================================================================

// --- RKC_NETWORK stubs ---

void* __thiscall RKC_NETWORK_operatorAssign(void* self, const void* other) { return self; }

// --- RKC_NETWORK_PACKET ---

void __thiscall RKC_NETWORK_PACKET_Release(void* self) {
    auto* packet = static_cast<NetworkPacket*>(self);
    WaitForSingleObject(NetworkMutex(), INFINITE);
    packet->line = -1;
    packet->id = 0;
    packet->size = 0;
    packet->next = nullptr;
    if (packet->data) {
        GlobalFree(packet->data);
        packet->data = nullptr;
    }
    ReleaseMutex(NetworkMutex());
}
void __thiscall RKC_NETWORK_PACKET_destructor(void* self) {
    RKC_NETWORK_PACKET_Release(self);
}
void* __thiscall RKC_NETWORK_PACKET_operatorAssign(void* self, const void* other) {
    std::memcpy(self, other, sizeof(NetworkPacket));
    return self;
}
void __thiscall RKC_NETWORK_PACKET_AllocateData(void* self, long size) {
    auto* packet = static_cast<NetworkPacket*>(self);
    WaitForSingleObject(NetworkMutex(), INFINITE);
    packet->size = size;
    packet->data = GlobalAlloc(0, size);
    ReleaseMutex(NetworkMutex());
}
void __thiscall RKC_NETWORK_PACKET_SetDisc(void* self, long disc) { *(long*)((char*)self + 0x14) = disc; }
void __thiscall RKC_NETWORK_PACKET_SetID(void* self, long id) { *(long*)((char*)self + 0x04) = id; }
void __thiscall RKC_NETWORK_PACKET_SetInfoID(void* self, long infoId) { *(long*)((char*)self + 0x0c) = infoId; }
void __thiscall RKC_NETWORK_PACKET_SetLine(void* self, long line) { *(long*)((char*)self) = line; }
void __thiscall RKC_NETWORK_PACKET_SetSize(void* self, long size) { *(long*)((char*)self + 0x08) = size; }
void __thiscall RKC_NETWORK_PACKET_SetParam(void* self, long line, long id, long size, long infoId, void* data, long disc) {
    auto* packet = static_cast<NetworkPacket*>(self);
    WaitForSingleObject(NetworkMutex(), INFINITE);
    packet->line = line;
    packet->id = id;
    packet->size = size;
    packet->infoId = infoId;
    packet->disc = disc;
    packet->data = GlobalAlloc(0, size);
    if (packet->data && data && size > 0)
        std::memcpy(packet->data, data, size);
    ReleaseMutex(NetworkMutex());
}

// --- RKC_NETWORK_PACKETBLOCK ---

void* __thiscall RKC_NETWORK_PACKETBLOCK_Get(void* self, long index) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    NetworkPacket* packet = *static_cast<NetworkPacket**>(self);
    for (long current = 0; packet && current != index; ++current)
        packet = packet->next;
    ReleaseMutex(NetworkMutex());
    return packet;
}
void __thiscall RKC_NETWORK_PACKETBLOCK_Release(void* self) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    NetworkPacket* packet = *static_cast<NetworkPacket**>(self);
    while (packet) {
        NetworkPacket* next = packet->next;
        RKC_NETWORK_PACKET_destructor(packet);
        delete packet;
        packet = next;
    }
    *static_cast<NetworkPacket**>(self) = nullptr;
    ReleaseMutex(NetworkMutex());
}
void __thiscall RKC_NETWORK_PACKETBLOCK_destructor(void* self) {
    RKC_NETWORK_PACKETBLOCK_Release(self);
}
void* __thiscall RKC_NETWORK_PACKETBLOCK_operatorAssign(void* self, const void* other) {
    std::memcpy(self, other, sizeof(void*));
    return self;
}
int __thiscall RKC_NETWORK_PACKETBLOCK_Delete_index(void* self, long index) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    auto** head = static_cast<NetworkPacket**>(self);
    NetworkPacket* removed = nullptr;
    if (index == 0) {
        removed = *head;
        if (removed)
            *head = removed->next;
    } else {
        auto* previous = static_cast<NetworkPacket*>(
            RKC_NETWORK_PACKETBLOCK_Get(self, index - 1));
        if (previous && previous->next) {
            removed = previous->next;
            previous->next = removed->next;
        }
    }
    if (removed) {
        RKC_NETWORK_PACKET_destructor(removed);
        delete removed;
    }
    ReleaseMutex(NetworkMutex());
    return removed ? 1 : 0;
}
int __thiscall RKC_NETWORK_PACKETBLOCK_Delete_packet(void* self, void* wanted) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    auto** head = static_cast<NetworkPacket**>(self);
    NetworkPacket* previous = nullptr;
    NetworkPacket* packet = *head;
    while (packet && packet != wanted) {
        previous = packet;
        packet = packet->next;
    }
    if (packet) {
        if (previous)
            previous->next = packet->next;
        else
            *head = packet->next;
    }
    ReleaseMutex(NetworkMutex());
    if (packet) {
        RKC_NETWORK_PACKET_destructor(packet);
        delete packet;
        return 1;
    }
    return 0;
}
long __thiscall RKC_NETWORK_PACKETBLOCK_GetCount_line(void* self, long line) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    long count = 0;
    for (NetworkPacket* packet = *static_cast<NetworkPacket**>(self);
         packet; packet = packet->next)
        if (packet->line == line)
            ++count;
    ReleaseMutex(NetworkMutex());
    return count;
}
long __thiscall RKC_NETWORK_PACKETBLOCK_GetCount(void* self) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    long count = 0;
    for (NetworkPacket* packet = *static_cast<NetworkPacket**>(self);
         packet; packet = packet->next)
        ++count;
    ReleaseMutex(NetworkMutex());
    return count;
}
long __thiscall RKC_NETWORK_PACKETBLOCK_GetNo(void* self, void* wanted) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    long index = 0;
    NetworkPacket* packet = *static_cast<NetworkPacket**>(self);
    while (packet && packet != wanted) {
        packet = packet->next;
        ++index;
    }
    ReleaseMutex(NetworkMutex());
    return packet ? index : -1;
}
void* __thiscall RKC_NETWORK_PACKETBLOCK_Insert(void* self, long index) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    auto* packet = new (std::nothrow) NetworkPacket;
    if (!packet) {
        ReleaseMutex(NetworkMutex());
        return nullptr;
    }
    RKC_NETWORK_PACKET_constructor(packet);
    auto** head = static_cast<NetworkPacket**>(self);
    if (index == 0) {
        packet->next = *head;
        *head = packet;
    } else {
        auto* previous = static_cast<NetworkPacket*>(
            RKC_NETWORK_PACKETBLOCK_Get(self, index - 1));
        if (!previous) {
            delete packet;
            ReleaseMutex(NetworkMutex());
            return nullptr;
        }
        packet->next = previous->next;
        previous->next = packet;
    }
    ReleaseMutex(NetworkMutex());
    return packet;
}

// --- RKC_NETWORK_USERINFO ---

void __thiscall RKC_NETWORK_USERINFO_destructor(void* self) {
    auto* info = static_cast<NetworkUserInfo*>(self);
    if (info->password)
        GlobalFree(info->password);
    if (info->userData)
        GlobalFree(info->userData);
    if (info->userName)
        GlobalFree(info->userName);
    info->password = nullptr;
    info->userData = nullptr;
    info->userName = nullptr;
}
void* __thiscall RKC_NETWORK_USERINFO_operatorAssign(void* self, const void* other) {
    std::memcpy(self, other, sizeof(NetworkUserInfo));
    return self;
}
char* __thiscall RKC_NETWORK_USERINFO_GetPassword(void* self) { return *(char**)((char*)self + 0x08); }
void* __thiscall RKC_NETWORK_USERINFO_GetUserData(void* self) { return *(void**)((char*)self + 0x0c); }
void __thiscall RKC_NETWORK_USERINFO_SetID(void* self, long id) { *(long*)((char*)self) = id; }
void __thiscall RKC_NETWORK_USERINFO_SetPassword(void* self, char* password) {
    auto* info = static_cast<NetworkUserInfo*>(self);
    if (info->password)
        GlobalFree(info->password);
    info->password = CopyGlobalString(password);
}
void __thiscall RKC_NETWORK_USERINFO_SetUserData(void* self, void* data) { *(void**)((char*)self + 0x0c) = data; }
void __thiscall RKC_NETWORK_USERINFO_SetUserName(void* self, char* name) {
    auto* info = static_cast<NetworkUserInfo*>(self);
    if (info->userName)
        GlobalFree(info->userName);
    info->userName = CopyGlobalString(name);
}

// --- RKC_NETWORK_USERINFOBLOCK ---

void __thiscall RKC_NETWORK_USERINFOBLOCK_Release(void* self);
void __thiscall RKC_NETWORK_USERINFOBLOCK_destructor(void* self) {
    RKC_NETWORK_USERINFOBLOCK_Release(self);
}
void* __thiscall RKC_NETWORK_USERINFOBLOCK_operatorAssign(void* self, const void* other) { return self; }
void* __thiscall RKC_NETWORK_USERINFOBLOCK_Append(void* self) {
    auto* info = new (std::nothrow) NetworkUserInfo;
    if (!info)
        return nullptr;
    RKC_NETWORK_USERINFO_constructor(info);
    auto** head = static_cast<NetworkUserInfo**>(self);
    if (!*head)
        *head = info;
    else {
        NetworkUserInfo* tail = *head;
        while (tail->next)
            tail = tail->next;
        tail->next = info;
    }
    return info;
}
void* __thiscall RKC_NETWORK_USERINFOBLOCK_Get(void* self, long index) {
    NetworkUserInfo* info = *static_cast<NetworkUserInfo**>(self);
    for (long current = 0; info && current != index; ++current)
        info = info->next;
    return info;
}
long __thiscall RKC_NETWORK_USERINFOBLOCK_GetCount(void* self) {
    long count = 0;
    for (NetworkUserInfo* info = *static_cast<NetworkUserInfo**>(self);
         info; info = info->next)
        ++count;
    return count;
}
int __thiscall RKC_NETWORK_USERINFOBLOCK_Delete(void* self, long index) {
    if (index < 0 || index >= RKC_NETWORK_USERINFOBLOCK_GetCount(self))
        return 0;
    auto** head = static_cast<NetworkUserInfo**>(self);
    NetworkUserInfo* removed = nullptr;
    if (index == 0) {
        removed = *head;
        *head = removed->next;
    } else {
        auto* previous = static_cast<NetworkUserInfo*>(
            RKC_NETWORK_USERINFOBLOCK_Get(self, index - 1));
        removed = previous->next;
        previous->next = removed->next;
    }
    RKC_NETWORK_USERINFO_destructor(removed);
    delete removed;
    return 1;
}
void* __thiscall RKC_NETWORK_USERINFOBLOCK_GetFromID(void* self, long id) {
    for (NetworkUserInfo* info = *static_cast<NetworkUserInfo**>(self);
         info; info = info->next)
        if (info->id == id)
            return info;
    return nullptr;
}
void* __thiscall RKC_NETWORK_USERINFOBLOCK_GetFromName(void* self, char* name) {
    for (NetworkUserInfo* info = *static_cast<NetworkUserInfo**>(self);
         info; info = info->next)
        if (info->userName && name && std::strcmp(info->userName, name) == 0)
            return info;
    return nullptr;
}
void __thiscall RKC_NETWORK_USERINFOBLOCK_Release(void* self) {
    while (RKC_NETWORK_USERINFOBLOCK_Delete(self, 0) == 1) {}
}

// --- RKC_NETWORK_TRANSFER ---

void __thiscall RKC_NETWORK_TRANSFER_destructor(void* self) { }
void* __thiscall RKC_NETWORK_TRANSFER_operatorAssign(void* self, const void* other) { return self; }
int __thiscall RKC_NETWORK_TRANSFER_RecvSub(void*, unsigned int socket, unsigned char* buf, long len) {
    long received = 0;
    while (received != len) {
        const int result = recv(socket, reinterpret_cast<char*>(buf), len - received, 0);
        if (result == 0 || result == SOCKET_ERROR)
            return 0;
        buf += result;
        received += result;
        Sleep(1);
    }
    return 1;
}
int __thiscall RKC_NETWORK_TRANSFER_SendSub(void*, unsigned int socket, unsigned char* buf, long len) {
    long sent = 0;
    for (;;) {
        const int result = send(
            socket, reinterpret_cast<const char*>(buf + sent), len - sent, 0);
        if (result == 0 || result == SOCKET_ERROR)
            return 0;
        sent += result;
        if (sent == result)
            return 1;
        Sleep(1);
    }
}
int __thiscall RKC_NETWORK_TRANSFER_Recv(void* self, unsigned int socket, unsigned char* buf, long len) {
    if (!RKC_NETWORK_TRANSFER_RecvSub(self, socket, buf, len))
        return 0;
    long acknowledgement = -1;
    return RKC_NETWORK_TRANSFER_SendSub(
        self, socket, reinterpret_cast<unsigned char*>(&acknowledgement), 4);
}
int __thiscall RKC_NETWORK_TRANSFER_Send(void* self, unsigned int socket, unsigned char* buf, long len) {
    if (!RKC_NETWORK_TRANSFER_SendSub(self, socket, buf, len))
        return 0;
    long acknowledgement = 0;
    return RKC_NETWORK_TRANSFER_RecvSub(
        self, socket, reinterpret_cast<unsigned char*>(&acknowledgement), 4)
        && acknowledgement == -1;
}

// --- RKC_NETWORK_SERVER_SOCKET stubs ---

void* __thiscall RKC_NETWORK_SERVER_SOCKET_constructor(void* self) { return self; }
void __thiscall RKC_NETWORK_SERVER_SOCKET_destructor(void* self) { }
void* __thiscall RKC_NETWORK_SERVER_SOCKET_operatorAssign(void* self, const void* other) { return self; }
unsigned int __thiscall RKC_NETWORK_SERVER_SOCKET_Get(void* self) { return 0; }
void __thiscall RKC_NETWORK_SERVER_SOCKET_SocketComparisionThread(void* self) { }

// --- RKC_NETWORK_SERVER_SOCKETCOMP stubs ---

void* __thiscall RKC_NETWORK_SERVER_SOCKETCOMP_constructor(void* self) { return self; }
void __thiscall RKC_NETWORK_SERVER_SOCKETCOMP_destructor(void* self) { }
void* __thiscall RKC_NETWORK_SERVER_SOCKETCOMP_operatorAssign(void* self, const void* other) { return self; }
int __thiscall RKC_NETWORK_SERVER_SOCKETCOMP_InsertComparisionSocket(void* self, unsigned int socket) { return 0; }
void __thiscall RKC_NETWORK_SERVER_SOCKETCOMP_Release(void* self) { }

// --- RKC_NETWORK_SERVER_CONNECTION stubs ---

void* __thiscall RKC_NETWORK_SERVER_CONNECTION_constructor(void* self) {
    memset(self, 0, 0x20);  // Clear structure
    return self;
}
void __thiscall RKC_NETWORK_SERVER_CONNECTION_destructor(void* self) { }
void* __thiscall RKC_NETWORK_SERVER_CONNECTION_operatorAssign(void* self, const void* other) { return self; }
void __thiscall RKC_NETWORK_SERVER_CONNECTION_ConnectionThread(void* self, long param) { }
int __thiscall RKC_NETWORK_SERVER_CONNECTION_CreateConnectionThread(void* self, long param) { return 0; }
void* __thiscall RKC_NETWORK_SERVER_CONNECTION_GetIP(void* self) { return (void*)((char*)self + 0x08); }
unsigned int __thiscall RKC_NETWORK_SERVER_CONNECTION_GetSocket(void* self, long index) { return 0; }
void __thiscall RKC_NETWORK_SERVER_CONNECTION_SetID(void* self, long id) { *(long*)((char*)self) = id; }
void __thiscall RKC_NETWORK_SERVER_CONNECTION_SetIP(void* self, void* ip) { }
void __thiscall RKC_NETWORK_SERVER_CONNECTION_SetSocket(void* self, long index, unsigned int socket) { }
void __thiscall RKC_NETWORK_SERVER_CONNECTION_SetStatus(void* self, long status) { *(long*)((char*)self + 0x10) = status; }
void __thiscall RKC_NETWORK_SERVER_CONNECTION_SetUserID(void* self, long userId) { *(long*)((char*)self + 0x04) = userId; }

// --- RKC_NETWORK_SERVER stubs ---

void* __thiscall RKC_NETWORK_SERVER_constructor(void* self) {
    memset(self, 0, 0x20);
    return self;
}
void __thiscall RKC_NETWORK_SERVER_destructor(void* self) { }
void* __thiscall RKC_NETWORK_SERVER_operatorAssign(void* self, const void* other) { return self; }
void __thiscall RKC_NETWORK_SERVER_AcceptThreadFunction(void* self) { }
long __thiscall RKC_NETWORK_SERVER_GetActiveConnectionCount(void* self) { return 0; }
void* __thiscall RKC_NETWORK_SERVER_GetEmptyConnection(void* self) { return nullptr; }
long __thiscall RKC_NETWORK_SERVER_GetSocketCount(void* self) { return 0; }
void* __thiscall RKC_NETWORK_SERVER_GetUseConnection(void* self, long id, void* ip, long port) { return nullptr; }
void* __thiscall RKC_NETWORK_SERVER_GetUserInfoBlock(void* self) { return nullptr; }
int __thiscall RKC_NETWORK_SERVER_Stop(void* self) { return 0; }

// --- RKC_NETWORK_CLIENT stubs ---

void* __thiscall RKC_NETWORK_CLIENT_constructor(void* self) {
    memset(self, 0, 0x20);
    return self;
}
void __thiscall RKC_NETWORK_CLIENT_destructor(void* self) { }
void* __thiscall RKC_NETWORK_CLIENT_operatorAssign(void* self, const void* other) { return self; }
void __thiscall RKC_NETWORK_CLIENT_ConnectionThread(void* self, long param) { }
void __thiscall RKC_NETWORK_CLIENT_ConnectThread(void* self) { }
int __thiscall RKC_NETWORK_CLIENT_CreateConnectionThread(void* self, long param) { return 0; }
long __thiscall RKC_NETWORK_CLIENT_GetConnectionID(void* self) { return 0; }
void* __thiscall RKC_NETWORK_CLIENT_GetIP(void* self) { return nullptr; }
void* __thiscall RKC_NETWORK_CLIENT_GetSendPacketBlock(void* self) { return nullptr; }
void* __thiscall RKC_NETWORK_CLIENT_GetUserInfo(void* self) { return nullptr; }

} // extern "C"
