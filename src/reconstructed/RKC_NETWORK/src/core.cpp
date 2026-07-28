/**
 * RKC_NETWORK - Network handling
 * 
 * Provides networking functionality for multiplayer.
 */

#include <winsock2.h>
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <new>
#include <vector>

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

void* __thiscall RKC_NETWORK_CLIENT_constructor(void* self);
void __thiscall RKC_NETWORK_CLIENT_destructor(void* self);
void __thiscall RKC_NETWORK_CLIENT_Release(void* self);
void* __thiscall RKC_NETWORK_SERVER_constructor(void* self);
void __thiscall RKC_NETWORK_SERVER_destructor(void* self);
void __thiscall RKC_NETWORK_SERVER_Release(void* self);
void* __thiscall RKC_NETWORK_SERVER_CONNECTION_constructor(void* self);
void __thiscall RKC_NETWORK_SERVER_CONNECTION_destructor(void* self);
void __thiscall RKC_NETWORK_SERVER_CONNECTION_Release(void* self);
void __thiscall RKC_NETWORK_SERVER_CONNECTION_ConnectionThread(
    void* self, long param);
void __thiscall RKC_NETWORK_CLIENT_ConnectionThread(void* self, long param);
int __thiscall RKC_NETWORK_CLIENT_Connect(void* self, long timeout);
void __thiscall RKC_NETWORK_SERVER_SOCKET_SocketComparisionThread(void* self);
int __thiscall RKC_NETWORK_SERVER_SOCKETCOMP_InsertComparisionSocket(
    void* self, unsigned int socketValue);
static DWORD WINAPI SocketComparisonThreadEntry(LPVOID context);

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
// PACKET CONTAINERS AND TRANSPORT
// ============================================================================

// --- RKC_NETWORK ---

void* __thiscall RKC_NETWORK_constructor(void* self) {
    if (gNetworkMutex)
        CloseHandle(gNetworkMutex);
    gNetworkMutex = CreateMutexA(nullptr, FALSE, nullptr);
    void* server = ::operator new(0x1dc, std::nothrow);
    void* client = ::operator new(0x1d0, std::nothrow);
    *(void**)self = server ? RKC_NETWORK_SERVER_constructor(server) : nullptr;
    *(void**)((char*)self + 4) =
        client ? RKC_NETWORK_CLIENT_constructor(client) : nullptr;
    return self;
}
void __thiscall RKC_NETWORK_destructor(void* self) {
    void* server = *(void**)self;
    void* client = *(void**)((char*)self + 4);
    if (server) {
        RKC_NETWORK_SERVER_destructor(server);
        ::operator delete(server);
    }
    if (client) {
        RKC_NETWORK_CLIENT_destructor(client);
        ::operator delete(client);
    }
    *(void**)self = nullptr;
    *(void**)((char*)self + 4) = nullptr;
    if (gNetworkMutex) {
        CloseHandle(gNetworkMutex);
        gNetworkMutex = nullptr;
    }
}
void* __thiscall RKC_NETWORK_operatorAssign(void* self, const void* other) {
    std::memcpy(self, other, 8);
    return self;
}
int __thiscall RKC_NETWORK_SetMutex(void*) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    return 1;
}
int __thiscall RKC_NETWORK_ResetMutex(void*) {
    ReleaseMutex(NetworkMutex());
    return 1;
}

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
void* __thiscall RKC_NETWORK_USERINFOBLOCK_operatorAssign(
    void* self, const void* other) {
    std::memcpy(self, other, sizeof(void*));
    return self;
}
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
void* __thiscall RKC_NETWORK_TRANSFER_operatorAssign(void* self, const void* other) {
    *static_cast<unsigned char*>(self) =
        *static_cast<const unsigned char*>(other);
    return self;
}
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

// --- RKC_NETWORK_SERVER_SOCKET ---

void* __thiscall RKC_NETWORK_SERVER_SOCKET_constructor(void* self) {
    *(void**)self = nullptr;
    *(HANDLE*)((char*)self + 4) = nullptr;
    *(void**)((char*)self + 0xc) = nullptr;
    return self;
}
void __thiscall RKC_NETWORK_SERVER_SOCKET_destructor(void* self) { }
void* __thiscall RKC_NETWORK_SERVER_SOCKET_operatorAssign(
    void* self, const void* other) {
    std::memcpy(self, other, 0x10);
    return self;
}
unsigned int __thiscall RKC_NETWORK_SERVER_SOCKET_Get(void* self) {
    return *(unsigned int*)((char*)self + 8);
}

// --- RKC_NETWORK_SERVER_SOCKETCOMP ---

void* __thiscall RKC_NETWORK_SERVER_SOCKETCOMP_constructor(void* self) {
    *(void**)((char*)self + 4) = nullptr;
    return self;
}
void* __thiscall RKC_NETWORK_SERVER_SOCKETCOMP_operatorAssign(
    void* self, const void* other) {
    std::memcpy(self, other, 8);
    return self;
}
void __thiscall RKC_NETWORK_SERVER_SOCKETCOMP_Release(void* self) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    char* current = *(char**)((char*)self + 4);
    while (current) {
        char* next = *(char**)(current + 0xc);
        HANDLE thread = *(HANDLE*)(current + 4);
        if (thread) {
            TerminateThread(thread, 0);
            CloseHandle(thread);
        }
        SOCKET socketValue = *(SOCKET*)(current + 8);
        if (socketValue != INVALID_SOCKET) {
            shutdown(socketValue, SD_BOTH);
            closesocket(socketValue);
        }
        RKC_NETWORK_SERVER_SOCKET_destructor(current);
        ::operator delete(current);
        current = next;
    }
    *(void**)((char*)self + 4) = nullptr;
    ReleaseMutex(NetworkMutex());
}
void __thiscall RKC_NETWORK_SERVER_SOCKETCOMP_destructor(void* self) {
    RKC_NETWORK_SERVER_SOCKETCOMP_Release(self);
}
int __thiscall RKC_NETWORK_SERVER_SOCKETCOMP_InsertComparisionSocket(
    void* self, unsigned int socketValue) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    char* item = static_cast<char*>(::operator new(0x10, std::nothrow));
    if (!item) {
        ReleaseMutex(NetworkMutex());
        shutdown(socketValue, SD_BOTH);
        closesocket(socketValue);
        return 0;
    }
    RKC_NETWORK_SERVER_SOCKET_constructor(item);
    *(void**)item = *(void**)self;
    *(SOCKET*)(item + 8) = socketValue;
    *(void**)(item + 0xc) = *(void**)((char*)self + 4);
    *(void**)((char*)self + 4) = item;
    DWORD threadId = 0;
    HANDLE thread = CreateThread(
        nullptr, 0, SocketComparisonThreadEntry, item, 0, &threadId);
    *(HANDLE*)(item + 4) = thread;
    if (!thread) {
        *(void**)((char*)self + 4) = *(void**)(item + 0xc);
        RKC_NETWORK_SERVER_SOCKET_destructor(item);
        ::operator delete(item);
        shutdown(socketValue, SD_BOTH);
        closesocket(socketValue);
        ReleaseMutex(NetworkMutex());
        return 0;
    }
    ReleaseMutex(NetworkMutex());
    return 1;
}

static void* FindRecvPacket(void* block, long line, long infoId) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    NetworkPacket* packet = block ? *static_cast<NetworkPacket**>(block) : nullptr;
    while (packet &&
           (packet->line != line ||
            (infoId != -1 && packet->infoId != infoId)))
        packet = packet->next;
    ReleaseMutex(NetworkMutex());
    return packet;
}

static long QueueSendPacket(
    void* block, long& sequence, long line, long size, long infoId,
    void* data, long disc, int replace, int atFront) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    long insertion = atFront
        ? 0
        : RKC_NETWORK_PACKETBLOCK_GetCount(block);
    if (replace && block) {
        NetworkPacket* previous = nullptr;
        NetworkPacket* packet = *static_cast<NetworkPacket**>(block);
        bool removed = false;
        while (packet) {
            NetworkPacket* next = packet->next;
            if (packet->disc == disc && packet->line == line) {
                if (previous)
                    previous->next = next;
                else
                    *static_cast<NetworkPacket**>(block) = next;
                if (!removed)
                    insertion = previous
                        ? RKC_NETWORK_PACKETBLOCK_GetNo(block, previous) + 1
                        : 0;
                removed = true;
                RKC_NETWORK_PACKET_destructor(packet);
                delete packet;
            } else {
                previous = packet;
            }
            packet = next;
        }
    }
    NetworkPacket* packet = static_cast<NetworkPacket*>(
        RKC_NETWORK_PACKETBLOCK_Insert(block, insertion));
    const long result = sequence++;
    if (packet)
        RKC_NETWORK_PACKET_SetParam(
            packet, line, result, size, infoId, data, disc);
    ReleaseMutex(NetworkMutex());
    return result;
}

static bool TransferPacketBatch(
    SOCKET socketValue, void* sendBlock, void* recvBlock, long line,
    bool receive) {
    unsigned char transferObject = 0;
    if (receive) {
        long batchSize = 0;
        if (!RKC_NETWORK_TRANSFER_Recv(
                &transferObject, socketValue,
                reinterpret_cast<unsigned char*>(&batchSize), sizeof(batchSize)))
            return false;
        if (batchSize < 0 || batchSize > 64 * 1024 * 1024)
            return false;
        std::vector<unsigned char> batch(
            static_cast<std::size_t>(batchSize));
        if (batchSize &&
            !RKC_NETWORK_TRANSFER_Recv(
                &transferObject, socketValue, batch.data(), batchSize))
            return false;
        std::size_t position = 0;
        while (position < batch.size()) {
            if (batch.size() - position < 12)
                return false;
            long id = 0;
            long size = 0;
            long infoId = 0;
            std::memcpy(&id, batch.data() + position, 4);
            std::memcpy(&size, batch.data() + position + 4, 4);
            std::memcpy(&infoId, batch.data() + position + 8, 4);
            position += 12;
            if (size < 0 ||
                static_cast<std::size_t>(size) > batch.size() - position)
                return false;
            WaitForSingleObject(NetworkMutex(), INFINITE);
            const long index = RKC_NETWORK_PACKETBLOCK_GetCount(recvBlock);
            void* packet = RKC_NETWORK_PACKETBLOCK_Insert(recvBlock, index);
            if (packet)
                RKC_NETWORK_PACKET_SetParam(
                    packet, line, id, size, infoId,
                    size ? batch.data() + position : nullptr, -1);
            ReleaseMutex(NetworkMutex());
            position += static_cast<std::size_t>(size);
        }
        return true;
    }

    std::vector<unsigned char> batch;
    WaitForSingleObject(NetworkMutex(), INFINITE);
    NetworkPacket* packet =
        sendBlock ? *static_cast<NetworkPacket**>(sendBlock) : nullptr;
    while (packet) {
        NetworkPacket* next = packet->next;
        if (packet->line == line) {
            const std::size_t oldSize = batch.size();
            batch.resize(oldSize + 12 + static_cast<std::size_t>(
                packet->size > 0 ? packet->size : 0));
            std::memcpy(batch.data() + oldSize, &packet->id, 4);
            std::memcpy(batch.data() + oldSize + 4, &packet->size, 4);
            std::memcpy(batch.data() + oldSize + 8, &packet->infoId, 4);
            if (packet->size > 0 && packet->data)
                std::memcpy(
                    batch.data() + oldSize + 12, packet->data,
                    static_cast<std::size_t>(packet->size));
            RKC_NETWORK_PACKETBLOCK_Delete_packet(sendBlock, packet);
        }
        packet = next;
    }
    ReleaseMutex(NetworkMutex());
    if (batch.empty()) {
        Sleep(1);
        return true;
    }
    long batchSize = static_cast<long>(batch.size());
    return RKC_NETWORK_TRANSFER_Send(
               &transferObject, socketValue,
               reinterpret_cast<unsigned char*>(&batchSize), sizeof(batchSize))
        && RKC_NETWORK_TRANSFER_Send(
               &transferObject, socketValue, batch.data(), batchSize);
}

static void RunPacketConnection(
    void* owner, char* record, SOCKET socketValue,
    void* sendBlock, void* recvBlock) {
    const bool receive = *(long*)(record + 0xc) == 1;
    const long line = *(long*)(record + 0x10);
    while (TransferPacketBatch(
        socketValue, sendBlock, recvBlock, line, receive)) {
    }
    *(HANDLE*)(record + 4) = nullptr;
}

static DWORD WINAPI ServerConnectionThreadEntry(LPVOID context) {
    char* record = static_cast<char*>(context);
    char* connection = *(char**)record;
    const long index = *(long*)(record + 8);
    RKC_NETWORK_SERVER_CONNECTION_ConnectionThread(connection, index);
    return 0;
}

static DWORD WINAPI ClientConnectionThreadEntry(LPVOID context) {
    char* record = static_cast<char*>(context);
    char* client = *(char**)record;
    const long index = *(long*)(record + 8);
    RKC_NETWORK_CLIENT_ConnectionThread(client, index);
    return 0;
}

static DWORD WINAPI SocketComparisonThreadEntry(LPVOID context) {
    RKC_NETWORK_SERVER_SOCKET_SocketComparisionThread(context);
    return 0;
}

// --- RKC_NETWORK_SERVER_CONNECTION ---

void* __thiscall RKC_NETWORK_SERVER_CONNECTION_constructor(void* self) {
    std::memset(self, 0, 0x28);
    *(long*)self = -1;
    *(long*)((char*)self + 4) = -1;
    void* send = ::operator new(4, std::nothrow);
    void* recv = ::operator new(4, std::nothrow);
    *(void**)((char*)self + 0x20) =
        send ? RKC_NETWORK_PACKETBLOCK_constructor(send) : nullptr;
    *(void**)((char*)self + 0x24) =
        recv ? RKC_NETWORK_PACKETBLOCK_constructor(recv) : nullptr;
    return self;
}
void __thiscall RKC_NETWORK_SERVER_CONNECTION_destructor(void* self) {
    RKC_NETWORK_SERVER_CONNECTION_Release(self);
    for (int offset : {0x20, 0x24}) {
        void* block = *(void**)((char*)self + offset);
        if (block) {
            RKC_NETWORK_PACKETBLOCK_destructor(block);
            ::operator delete(block);
            *(void**)((char*)self + offset) = nullptr;
        }
    }
    if (*(void**)((char*)self + 0x18))
        GlobalFree(*(void**)((char*)self + 0x18));
    if (*(void**)((char*)self + 0x1c))
        GlobalFree(*(void**)((char*)self + 0x1c));
}
void* __thiscall RKC_NETWORK_SERVER_CONNECTION_operatorAssign(void* self, const void* other) {
    std::memcpy(self, other, 0x28);
    return self;
}
void __thiscall RKC_NETWORK_SERVER_CONNECTION_ConnectionThread(
    void* self, long param) {
    char* connection = static_cast<char*>(self);
    if (param < 0 || param >= *(long*)(connection + 0xc))
        return;
    char* record = *(char**)(connection + 0x18) + param * 0x14;
    SOCKET socketValue = (*(SOCKET**)(connection + 0x1c))[param];
    RunPacketConnection(
        self, record, socketValue,
        *(void**)(connection + 0x20), *(void**)(connection + 0x24));
    RKC_NETWORK_SERVER_CONNECTION_Release(self);
}
int __thiscall RKC_NETWORK_SERVER_CONNECTION_CreateConnectionThread(
    void* self, long param) {
    char* connection = static_cast<char*>(self);
    if (param < 0 || param >= *(long*)(connection + 0xc) ||
        !*(char**)(connection + 0x18))
        return 0;
    char* record = *(char**)(connection + 0x18) + param * 0x14;
    DWORD threadId = 0;
    HANDLE thread = CreateThread(
        nullptr, 0, ServerConnectionThreadEntry, record, 0, &threadId);
    *(HANDLE*)(record + 4) = thread;
    if (!thread) {
        RKC_NETWORK_SERVER_CONNECTION_Release(self);
        return 0;
    }
    return 1;
}
int __thiscall RKC_NETWORK_SERVER_CONNECTION_DeleteRecvPacket(void* self, void* packet) {
    return RKC_NETWORK_PACKETBLOCK_Delete_packet(
        *(void**)((char*)self + 0x24), packet);
}
void* __thiscall RKC_NETWORK_SERVER_CONNECTION_GetRecvPacket(
    void* self, long line, long infoId) {
    return FindRecvPacket(*(void**)((char*)self + 0x24), line, infoId);
}
void* __thiscall RKC_NETWORK_SERVER_CONNECTION_GetRecvPacketBlock(void* self) {
    return *(void**)((char*)self + 0x24);
}
void* __thiscall RKC_NETWORK_SERVER_CONNECTION_GetSendPacketBlock(void* self) {
    return *(void**)((char*)self + 0x20);
}
void* __thiscall RKC_NETWORK_SERVER_CONNECTION_GetIP(void* self) {
    return (char*)self + 0x14;
}
unsigned int __thiscall RKC_NETWORK_SERVER_CONNECTION_GetSocket(void* self, long index) {
    return (*(unsigned int**)((char*)self + 0x1c))[index];
}
void __thiscall RKC_NETWORK_SERVER_CONNECTION_Release(void* self) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    *(long*)((char*)self + 0x10) = 0;
    const long count = *(long*)((char*)self + 0xc);
    char* records = *(char**)((char*)self + 0x18);
    unsigned int* sockets = *(unsigned int**)((char*)self + 0x1c);
    for (long index = 0; index < count; ++index) {
        if (records && *(HANDLE*)(records + index * 0x14 + 4)) {
            TerminateThread(*(HANDLE*)(records + index * 0x14 + 4), 0);
            CloseHandle(*(HANDLE*)(records + index * 0x14 + 4));
            *(HANDLE*)(records + index * 0x14 + 4) = nullptr;
        }
        if (sockets && sockets[index] != INVALID_SOCKET) {
            shutdown(sockets[index], SD_BOTH);
            closesocket(sockets[index]);
            sockets[index] = INVALID_SOCKET;
        }
    }
    if (*(void**)((char*)self + 0x20))
        RKC_NETWORK_PACKETBLOCK_Release(*(void**)((char*)self + 0x20));
    if (*(void**)((char*)self + 0x24))
        RKC_NETWORK_PACKETBLOCK_Release(*(void**)((char*)self + 0x24));
    *(long*)self = -1;
    *(long*)((char*)self + 4) = -1;
    ReleaseMutex(NetworkMutex());
}
void __thiscall RKC_NETWORK_SERVER_CONNECTION_SetID(void* self, long id) { *(long*)((char*)self) = id; }
void __thiscall RKC_NETWORK_SERVER_CONNECTION_SetIP(void* self, void* ip) {
    if (ip)
        std::memcpy((char*)self + 0x14, ip, 4);
}
long __thiscall RKC_NETWORK_SERVER_CONNECTION_SetSendPacket(
    void* self, long line, long size, long infoId, void* data, long disc,
    int replace, int atFront) {
    char* server = *(char**)((char*)self + 8);
    long& sequence = *(long*)(server + 0x1c8);
    return QueueSendPacket(
        *(void**)((char*)self + 0x20), sequence, line, size, infoId,
        data, disc, replace, atFront);
}
void __thiscall RKC_NETWORK_SERVER_CONNECTION_SetSocket(
    void* self, long index, unsigned int socket) {
    (*(unsigned int**)((char*)self + 0x1c))[index] = socket;
}
void __thiscall RKC_NETWORK_SERVER_CONNECTION_SetStatus(void* self, long status) { *(long*)((char*)self + 0x10) = status; }
void __thiscall RKC_NETWORK_SERVER_CONNECTION_SetUserID(void* self, long userId) { *(long*)((char*)self + 0x04) = userId; }

// --- RKC_NETWORK_SERVER ---

void* __thiscall RKC_NETWORK_SERVER_constructor(void* self) {
    std::memset(self, 0, 0x1dc);
    *(long*)((char*)self + 8) = 0x3168;
    *(SOCKET*)((char*)self + 0x1c0) = INVALID_SOCKET;
    void* socketComp = ::operator new(8, std::nothrow);
    *(void**)((char*)self + 0x14) = socketComp
        ? RKC_NETWORK_SERVER_SOCKETCOMP_constructor(socketComp)
        : nullptr;
    if (socketComp)
        *(void**)socketComp = self;
    return self;
}
void __thiscall RKC_NETWORK_SERVER_destructor(void* self) {
    RKC_NETWORK_SERVER_Release(self);
    void* socketComp = *(void**)((char*)self + 0x14);
    if (socketComp) {
        RKC_NETWORK_SERVER_SOCKETCOMP_destructor(socketComp);
        ::operator delete(socketComp);
        *(void**)((char*)self + 0x14) = nullptr;
    }
}
void* __thiscall RKC_NETWORK_SERVER_operatorAssign(void* self, const void* other) {
    std::memcpy(self, other, 0x1dc);
    return self;
}
void __thiscall RKC_NETWORK_SERVER_AcceptThreadFunction(void* self) {
    sockaddr_in address{};
    int length = sizeof(address);
    while (*(long*)self) {
        SOCKET accepted = accept(
            *(SOCKET*)((char*)self + 0x1c0),
            reinterpret_cast<sockaddr*>(&address), &length);
        if (accepted == INVALID_SOCKET)
            break;
        RKC_NETWORK_SERVER_SOCKETCOMP_InsertComparisionSocket(
            *(void**)((char*)self + 0x14), accepted);
    }
}
long __thiscall RKC_NETWORK_SERVER_GetActiveConnectionCount(void* self) {
    long count = 0;
    void** entries = *(void***)((char*)self + 0x10);
    for (long index = 0; entries && index < *(long*)((char*)self + 0xc); ++index)
        if (*(long*)((char*)entries[index] + 0x10) != 0)
            ++count;
    return count;
}
void* __thiscall RKC_NETWORK_SERVER_GetConnection_index(void* self, long index) {
    return (*(void***)((char*)self + 0x10))[index];
}
void* __thiscall RKC_NETWORK_SERVER_GetConnection_search(
    void* self, long id, void* ip, long userId) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    void* result = nullptr;
    void** entries = *(void***)((char*)self + 0x10);
    for (long index = 0; entries && index < *(long*)((char*)self + 0xc); ++index) {
        char* connection = static_cast<char*>(entries[index]);
        if (*(long*)(connection + 0x10) != 2)
            continue;
        if ((ip && std::memcmp(connection + 0x14, ip, 4) == 0) ||
            (!ip && ((id >= 0 && *(long*)connection == id) ||
                     (id < 0 && userId >= 0 &&
                      *(long*)(connection + 4) == userId)))) {
            result = connection;
            break;
        }
    }
    ReleaseMutex(NetworkMutex());
    return result;
}
long __thiscall RKC_NETWORK_SERVER_GetConnectionCount(void* self) {
    return *(long*)((char*)self + 0xc);
}
void* __thiscall RKC_NETWORK_SERVER_GetEmptyConnection(void* self) {
    void** entries = *(void***)((char*)self + 0x10);
    for (long index = 0; entries && index < *(long*)((char*)self + 0xc); ++index)
        if (*(long*)((char*)entries[index] + 0x10) == 0)
            return entries[index];
    return nullptr;
}
void __thiscall RKC_NETWORK_SERVER_GetIP(void*, void* output) {
    if (!output)
        return;
    std::memset(output, 0, 4);
    char host[256]{};
    if (gethostname(host, sizeof(host) - 1) == 0) {
        hostent* entry = gethostbyname(host);
        if (entry && entry->h_addr_list && entry->h_addr_list[0])
            std::memcpy(output, entry->h_addr_list[0], 4);
    }
}
long __thiscall RKC_NETWORK_SERVER_GetSocketCount(void* self) {
    return *(long*)((char*)self + 0x1c);
}
void* __thiscall RKC_NETWORK_SERVER_GetUseConnection(
    void* self, long id, void* ip, long userId) {
    void** entries = *(void***)((char*)self + 0x10);
    for (long index = 0; entries && index < *(long*)((char*)self + 0xc); ++index) {
        char* connection = static_cast<char*>(entries[index]);
        if (*(long*)(connection + 0x10) == 0)
            continue;
        if ((ip && std::memcmp(connection + 0x14, ip, 4) == 0) ||
            (!ip && ((id >= 0 && *(long*)connection == id) ||
                     (id < 0 && userId >= 0 &&
                      *(long*)(connection + 4) == userId))))
            return connection;
    }
    return nullptr;
}
void* __thiscall RKC_NETWORK_SERVER_GetUserInfoBlock(void* self) {
    return *(void**)((char*)self + 0x1d4);
}
int __thiscall RKC_NETWORK_SERVER_Initialize(
    void* self, long maxConnections, long socketPairs, void* users,
    long packetSize, long authentication, long userNameSize) {
    RKC_NETWORK_SERVER_Release(self);
    WSADATA* data = reinterpret_cast<WSADATA*>((char*)self + 0x20);
    if (WSAStartup(MAKEWORD(1, 1), data) != 0)
        return 0;
    *(long*)((char*)self + 4) = 1;
    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) {
        RKC_NETWORK_SERVER_Release(self);
        return 0;
    }
    sockaddr_in* address =
        reinterpret_cast<sockaddr_in*>((char*)self + 0x1b0);
    std::memset(address, 0, sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_port = htons(*(u_short*)((char*)self + 8));
    address->sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listener, reinterpret_cast<sockaddr*>(address), sizeof(*address)) ==
        SOCKET_ERROR) {
        closesocket(listener);
        RKC_NETWORK_SERVER_Release(self);
        return 0;
    }
    *(SOCKET*)((char*)self + 0x1c0) = listener;
    *(long*)((char*)self + 0xc) = maxConnections;
    *(long*)((char*)self + 0x1c) = socketPairs * 2;
    void** entries = static_cast<void**>(
        GlobalAlloc(GPTR, static_cast<SIZE_T>(maxConnections) * sizeof(void*)));
    *(void***)((char*)self + 0x10) = entries;
    for (long index = 0; entries && index < maxConnections; ++index) {
        char* connection = static_cast<char*>(
            ::operator new(0x28, std::nothrow));
        entries[index] = connection
            ? RKC_NETWORK_SERVER_CONNECTION_constructor(connection)
            : nullptr;
        if (!connection)
            continue;
        *(void**)(connection + 8) = self;
        *(long*)(connection + 0xc) = socketPairs * 2;
        *(unsigned int**)(connection + 0x1c) =
            static_cast<unsigned int*>(GlobalAlloc(
                GPTR, static_cast<SIZE_T>(socketPairs * 2) * sizeof(unsigned int)));
        *(char**)(connection + 0x18) = static_cast<char*>(GlobalAlloc(
            GPTR, static_cast<SIZE_T>(socketPairs * 2) * 0x14));
        unsigned int* sockets = *(unsigned int**)(connection + 0x1c);
        char* records = *(char**)(connection + 0x18);
        for (long socketIndex = 0;
             sockets && records && socketIndex < socketPairs * 2;
             ++socketIndex) {
            sockets[socketIndex] = INVALID_SOCKET;
            char* record = records + socketIndex * 0x14;
            *(void**)record = connection;
            *(HANDLE*)(record + 4) = nullptr;
            *(long*)(record + 8) = socketIndex;
            *(long*)(record + 0xc) = socketIndex & 1;
            *(long*)(record + 0x10) = socketIndex / 2;
        }
    }
    *(void**)((char*)self + 0x1d4) = users;
    *(long*)((char*)self + 0x1d8) = authentication;
    *(long*)((char*)self + 0x1cc) = packetSize;
    *(long*)((char*)self + 0x1d0) = userNameSize;
    return entries || maxConnections == 0;
}
void __thiscall RKC_NETWORK_SERVER_Release(void* self) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    HANDLE thread = *(HANDLE*)((char*)self + 0x1c4);
    if (thread) {
        TerminateThread(thread, 0);
        CloseHandle(thread);
        *(HANDLE*)((char*)self + 0x1c4) = nullptr;
    }
    SOCKET& listener = *(SOCKET*)((char*)self + 0x1c0);
    if (listener != INVALID_SOCKET) {
        shutdown(listener, SD_BOTH);
        closesocket(listener);
        listener = INVALID_SOCKET;
    }
    void** entries = *(void***)((char*)self + 0x10);
    for (long index = 0; entries && index < *(long*)((char*)self + 0xc); ++index)
        if (entries[index]) {
            RKC_NETWORK_SERVER_CONNECTION_destructor(entries[index]);
            ::operator delete(entries[index]);
        }
    if (entries)
        GlobalFree(entries);
    *(void**)((char*)self + 0x10) = nullptr;
    *(long*)((char*)self + 0xc) = 0;
    *(long*)self = 0;
    *(void**)((char*)self + 0x1d4) = nullptr;
    if (*(long*)((char*)self + 4)) {
        WSACleanup();
        *(long*)((char*)self + 4) = 0;
    }
    ReleaseMutex(NetworkMutex());
}
int __thiscall RKC_NETWORK_SERVER_Start(void* self) {
    if (*(long*)((char*)self + 4) == 0)
        return 0;
    if (listen(*(SOCKET*)((char*)self + 0x1c0), SOMAXCONN) == SOCKET_ERROR) {
        RKC_NETWORK_SERVER_Release(self);
        return 0;
    }
    *(long*)self = 1;
    DWORD threadId = 0;
    HANDLE thread = CreateThread(
        nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(
            +[](LPVOID context) -> DWORD {
                RKC_NETWORK_SERVER_AcceptThreadFunction(context);
                return 0;
            }),
        self, 0, &threadId);
    *(HANDLE*)((char*)self + 0x1c4) = thread;
    if (!thread) {
        RKC_NETWORK_SERVER_Release(self);
        return 0;
    }
    return 1;
}
int __thiscall RKC_NETWORK_SERVER_Stop(void* self) {
    RKC_NETWORK_SERVER_Release(self);
    return 1;
}

void __thiscall RKC_NETWORK_SERVER_SOCKET_SocketComparisionThread(void* self) {
    char* item = static_cast<char*>(self);
    char* server = *(char**)item;
    SOCKET socketValue = *(SOCKET*)(item + 8);
    unsigned char transferObject = 0;
    long socketIndex = -1;
    long connectionId = -1;
    bool success =
        server && socketValue != INVALID_SOCKET
        && RKC_NETWORK_TRANSFER_Recv(
            &transferObject, socketValue,
            reinterpret_cast<unsigned char*>(&socketIndex), 4)
        && socketIndex >= 0
        && socketIndex < *(long*)(server + 0x1c)
        && RKC_NETWORK_TRANSFER_Recv(
            &transferObject, socketValue,
            reinterpret_cast<unsigned char*>(&connectionId), 4);

    WaitForSingleObject(NetworkMutex(), INFINITE);
    char* connection = nullptr;
    if (success && connectionId >= 0)
        connection = static_cast<char*>(RKC_NETWORK_SERVER_GetUseConnection(
            server, connectionId, nullptr, -1));
    if (success && !connection) {
        connection = static_cast<char*>(
            RKC_NETWORK_SERVER_GetEmptyConnection(server));
        if (connection) {
            *(long*)(connection + 0x10) = 1;
            connectionId = *(long*)(server + 0x18);
            *(long*)(server + 0x18) = connectionId + 1;
            *(long*)connection = connectionId;
            sockaddr_in remote{};
            int remoteSize = sizeof(remote);
            if (getpeername(
                    socketValue, reinterpret_cast<sockaddr*>(&remote),
                    &remoteSize) == 0)
                std::memcpy(connection + 0x14, &remote.sin_addr, 4);
        }
    }
    if (success && connection &&
        (*(SOCKET**)(connection + 0x1c))[socketIndex] == INVALID_SOCKET) {
        if (!RKC_NETWORK_TRANSFER_Send(
                &transferObject, socketValue,
                reinterpret_cast<unsigned char*>(&connectionId), 4))
            success = false;
        else {
            (*(SOCKET**)(connection + 0x1c))[socketIndex] = socketValue;
            *(SOCKET*)(item + 8) = INVALID_SOCKET;
            long populated = 0;
            for (; populated < *(long*)(connection + 0xc); ++populated)
                if ((*(SOCKET**)(connection + 0x1c))[populated] ==
                    INVALID_SOCKET)
                    break;
            if (populated == *(long*)(connection + 0xc)) {
                for (long index = 0; index < populated; ++index)
                    if (!RKC_NETWORK_SERVER_CONNECTION_CreateConnectionThread(
                            connection, index)) {
                        success = false;
                        break;
                    }
                if (success)
                    *(long*)(connection + 0x10) = 2;
            }
        }
    } else {
        success = false;
    }
    ReleaseMutex(NetworkMutex());

    if (!success && socketValue != INVALID_SOCKET) {
        shutdown(socketValue, SD_BOTH);
        closesocket(socketValue);
        *(SOCKET*)(item + 8) = INVALID_SOCKET;
    }
    *(HANDLE*)(item + 4) = nullptr;
}

// --- RKC_NETWORK_CLIENT ---

void* __thiscall RKC_NETWORK_CLIENT_constructor(void* self) {
    std::memset(self, 0, 0x1d0);
    *(short*)((char*)self + 0x1a8) = 0x3168;
    void* send = ::operator new(4, std::nothrow);
    void* recv = ::operator new(4, std::nothrow);
    *(void**)((char*)self + 0x1b0) =
        send ? RKC_NETWORK_PACKETBLOCK_constructor(send) : nullptr;
    *(void**)((char*)self + 0x1b4) =
        recv ? RKC_NETWORK_PACKETBLOCK_constructor(recv) : nullptr;
    *(long*)((char*)self + 0x1bc) = -1;
    return self;
}
void __thiscall RKC_NETWORK_CLIENT_destructor(void* self) {
    RKC_NETWORK_CLIENT_Release(self);
    for (int offset : {0x1b0, 0x1b4}) {
        void* block = *(void**)((char*)self + offset);
        if (block) {
            RKC_NETWORK_PACKETBLOCK_destructor(block);
            ::operator delete(block);
            *(void**)((char*)self + offset) = nullptr;
        }
    }
}
void* __thiscall RKC_NETWORK_CLIENT_operatorAssign(void* self, const void* other) {
    std::memcpy(self, other, 0x1d0);
    return self;
}
void __thiscall RKC_NETWORK_CLIENT_ConnectionThread(void* self, long param) {
    char* client = static_cast<char*>(self);
    if (param < 0 || param >= *(long*)(client + 0x19c))
        return;
    char* record = *(char**)(client + 0x1a0) + param * 0x14;
    SOCKET socketValue = (*(SOCKET**)(client + 0x1a4))[param];
    RunPacketConnection(
        self, record, socketValue,
        *(void**)(client + 0x1b0), *(void**)(client + 0x1b4));
    RKC_NETWORK_CLIENT_Release(self);
}
void __thiscall RKC_NETWORK_CLIENT_ConnectThread(void* self) {
    RKC_NETWORK_CLIENT_Connect(self, 0x7fffffff);
}
int __thiscall RKC_NETWORK_CLIENT_CreateConnectionThread(
    void* self, long param) {
    char* client = static_cast<char*>(self);
    if (param < 0 || param >= *(long*)(client + 0x19c) ||
        !*(char**)(client + 0x1a0))
        return 0;
    char* record = *(char**)(client + 0x1a0) + param * 0x14;
    DWORD threadId = 0;
    HANDLE thread = CreateThread(
        nullptr, 0, ClientConnectionThreadEntry, record, 0, &threadId);
    *(HANDLE*)(record + 4) = thread;
    return thread != nullptr;
}
int __thiscall RKC_NETWORK_CLIENT_Connect(void* self, long timeout) {
    char* client = static_cast<char*>(self);
    if (!*(unsigned int**)(client + 0x1a4))
        return 0;
    const DWORD started = GetTickCount();
    sockaddr_in target{};
    target.sin_family = AF_INET;
    target.sin_port = htons(*(u_short*)(client + 0x1a8));
    std::memcpy(&target.sin_addr, client + 0x198, 4);
    unsigned char transferObject = 0;
    *(long*)(client + 0x1bc) = -1;
    for (long index = 0; index < *(long*)(client + 0x19c); ++index) {
        SOCKET socketValue = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socketValue == INVALID_SOCKET ||
            connect(socketValue, reinterpret_cast<sockaddr*>(&target),
                    sizeof(target)) == SOCKET_ERROR ||
            !RKC_NETWORK_TRANSFER_Send(
                &transferObject, socketValue,
                reinterpret_cast<unsigned char*>(&index), 4) ||
            !RKC_NETWORK_TRANSFER_Send(
                &transferObject, socketValue,
                reinterpret_cast<unsigned char*>(client + 0x1bc), 4)) {
            if (socketValue != INVALID_SOCKET)
                closesocket(socketValue);
            RKC_NETWORK_CLIENT_Release(self);
            return 0;
        }
        long assignedId = -1;
        if (!RKC_NETWORK_TRANSFER_Recv(
                &transferObject, socketValue,
                reinterpret_cast<unsigned char*>(&assignedId), 4)) {
            closesocket(socketValue);
            RKC_NETWORK_CLIENT_Release(self);
            return 0;
        }
        if (*(long*)(client + 0x1bc) == -1)
            *(long*)(client + 0x1bc) = assignedId;
        else if (*(long*)(client + 0x1bc) != assignedId) {
            closesocket(socketValue);
            RKC_NETWORK_CLIENT_Release(self);
            return 0;
        }
        (*(unsigned int**)(client + 0x1a4))[index] = socketValue;
        if (timeout >= 0 && GetTickCount() - started >= static_cast<DWORD>(timeout)) {
            RKC_NETWORK_CLIENT_Release(self);
            return 0;
        }
    }
    for (long index = 0; index < *(long*)(client + 0x19c); ++index)
        if (!RKC_NETWORK_CLIENT_CreateConnectionThread(self, index)) {
            RKC_NETWORK_CLIENT_Release(self);
            return 0;
        }
    if (*(long*)(client + 0x19c) > 0) {
        sockaddr_in local{};
        int size = sizeof(local);
        if (getsockname(
                (*(unsigned int**)(client + 0x1a4))[0],
                reinterpret_cast<sockaddr*>(&local), &size) == 0)
            std::memcpy(client + 0x198, &local.sin_addr, 4);
    }
    *(long*)(client + 4) = 1;
    *(long*)(client + 0x1ac) = 1;
    return 1;
}
int __thiscall RKC_NETWORK_CLIENT_DeleteRecvPacket(void* self, void* packet) {
    return RKC_NETWORK_PACKETBLOCK_Delete_packet(
        *(void**)((char*)self + 0x1b4), packet);
}
long __thiscall RKC_NETWORK_CLIENT_GetConnectionID(void* self) {
    return *(long*)((char*)self + 0x1bc);
}
void* __thiscall RKC_NETWORK_CLIENT_GetIP(void* self) {
    return (char*)self + 0x198;
}
void __thiscall RKC_NETWORK_CLIENT_GetMyIP(void*, void* output) {
    RKC_NETWORK_SERVER_GetIP(nullptr, output);
}
void* __thiscall RKC_NETWORK_CLIENT_GetRecvPacket(
    void* self, long line, long infoId) {
    return FindRecvPacket(*(void**)((char*)self + 0x1b4), line, infoId);
}
void* __thiscall RKC_NETWORK_CLIENT_GetRecvPacketBlock(void* self) {
    return *(void**)((char*)self + 0x1b4);
}
void* __thiscall RKC_NETWORK_CLIENT_GetSendPacketBlock(void* self) {
    return *(void**)((char*)self + 0x1b0);
}
void* __thiscall RKC_NETWORK_CLIENT_GetUserInfo(void* self) {
    return *(void**)((char*)self + 0x1c8);
}
int __thiscall RKC_NETWORK_CLIENT_Initialize(
    void* self, void* ip, long socketPairs, void* user,
    long packetSize, long extra, long userNameSize) {
    RKC_NETWORK_CLIENT_Release(self);
    WSADATA* data = reinterpret_cast<WSADATA*>((char*)self + 8);
    if (WSAStartup(MAKEWORD(1, 1), data) != 0)
        return 0;
    char* client = static_cast<char*>(self);
    *(long*)client = 1;
    *(long*)(client + 0x19c) = socketPairs * 2;
    *(unsigned int**)(client + 0x1a4) =
        static_cast<unsigned int*>(GlobalAlloc(
            GPTR, static_cast<SIZE_T>(socketPairs * 2) * sizeof(unsigned int)));
    *(char**)(client + 0x1a0) = static_cast<char*>(GlobalAlloc(
        GPTR, static_cast<SIZE_T>(socketPairs * 2) * 0x14));
    unsigned int* sockets = *(unsigned int**)(client + 0x1a4);
    char* records = *(char**)(client + 0x1a0);
    for (long index = 0;
         sockets && records && index < socketPairs * 2; ++index) {
        sockets[index] = INVALID_SOCKET;
        char* record = records + index * 0x14;
        *(void**)record = client;
        *(HANDLE*)(record + 4) = nullptr;
        *(long*)(record + 8) = index;
        *(long*)(record + 0xc) = (index & 1) ? 0 : 1;
        *(long*)(record + 0x10) = index / 2;
    }
    if (ip) {
        std::memcpy(client + 0x198, ip, 4);
    }
    if (user) {
        *(void**)(client + 0x1c8) = GlobalAlloc(GPTR, 0x14);
        if (*(void**)(client + 0x1c8))
            std::memcpy(*(void**)(client + 0x1c8), user, 0x14);
    }
    *(long*)(client + 0x1c0) = userNameSize;
    *(long*)(client + 0x1c4) = packetSize;
    *(long*)(client + 0x1cc) = extra;
    return (*(unsigned int**)(client + 0x1a4) &&
            *(char**)(client + 0x1a0)) || socketPairs == 0;
}
void __thiscall RKC_NETWORK_CLIENT_Release(void* self) {
    WaitForSingleObject(NetworkMutex(), INFINITE);
    char* client = static_cast<char*>(self);
    const long count = *(long*)(client + 0x19c);
    char* records = *(char**)(client + 0x1a0);
    unsigned int* sockets = *(unsigned int**)(client + 0x1a4);
    for (long index = 0; index < count; ++index) {
        if (records && *(HANDLE*)(records + index * 0x14 + 4)) {
            TerminateThread(*(HANDLE*)(records + index * 0x14 + 4), 0);
            CloseHandle(*(HANDLE*)(records + index * 0x14 + 4));
        }
        if (sockets && sockets[index] != INVALID_SOCKET) {
            shutdown(sockets[index], SD_BOTH);
            closesocket(sockets[index]);
        }
    }
    if (records) GlobalFree(records);
    if (sockets) GlobalFree(sockets);
    *(void**)(client + 0x1a0) = nullptr;
    *(void**)(client + 0x1a4) = nullptr;
    *(long*)(client + 0x19c) = 0;
    if (*(void**)(client + 0x1b0))
        RKC_NETWORK_PACKETBLOCK_Release(*(void**)(client + 0x1b0));
    if (*(void**)(client + 0x1b4))
        RKC_NETWORK_PACKETBLOCK_Release(*(void**)(client + 0x1b4));
    if (*(void**)(client + 0x1c8)) {
        GlobalFree(*(void**)(client + 0x1c8));
        *(void**)(client + 0x1c8) = nullptr;
    }
    *(long*)(client + 0x1c4) = 0;
    *(long*)(client + 4) = 0;
    if (*(long*)client) {
        WSACleanup();
        *(long*)client = 0;
    }
    ReleaseMutex(NetworkMutex());
}
long __thiscall RKC_NETWORK_CLIENT_SetSendPacket(
    void* self, long line, long size, long infoId, void* data, long disc,
    int replace, int atFront) {
    long& sequence = *(long*)((char*)self + 0x1b8);
    return QueueSendPacket(
        *(void**)((char*)self + 0x1b0), sequence, line, size, infoId,
        data, disc, replace, atFront);
}

} // extern "C"
