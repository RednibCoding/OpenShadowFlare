/**
 * RKC_RPG_AICONTROL - ShadowFlare AI action database.
 *
 * The original DLL stores lists and actions in simple singly-linked lists.
 * These layouts are part of the ABI: the game reads several fields directly.
 */

#include <windows.h>

#include <cstdint>
#include <cstring>
#include <new>

namespace {

struct AIData {
    AIData* next;
    LONG eventNo;
    LONG actionNo;
    std::uint8_t condition[0x18];
    std::uint8_t parameter[0x24];
};

struct AIEvent {
    LONG reserved;
    AIData* head;
};

struct AIList {
    AIList* next;
    char* name;
    LONG walkPointSpeed;
    AIEvent* events;
};

struct AIControl {
    AIList* head;
};

static_assert(sizeof(AIData) == 0x48, "RKC_RPG_AIDATA ABI mismatch");
static_assert(sizeof(AIEvent) == 0x08, "RKC_RPG_AIEVENT ABI mismatch");
static_assert(sizeof(AIList) == 0x10, "RKC_RPG_AILIST ABI mismatch");
static_assert(sizeof(AIControl) == 0x04, "RKC_RPG_AICONTROL ABI mismatch");

constexpr LONG kEventCount = 0x12;

template <typename T>
T* nth(T* item, LONG index) {
    if (index < 0) {
        return nullptr;
    }
    for (LONG current = 0; item && current < index; ++current) {
        item = item->next;
    }
    return item;
}

template <typename T>
LONG countList(T* item) {
    LONG count = 0;
    while (item) {
        ++count;
        item = item->next;
    }
    return count;
}

bool readExact(HANDLE file, void* destination, DWORD size) {
    DWORD read = 0;
    return ReadFile(file, destination, size, &read, nullptr) && read == size;
}

bool writeExact(HANDLE file, const void* source, DWORD size) {
    DWORD written = 0;
    return WriteFile(file, source, size, &written, nullptr) && written == size;
}

LONG readVersion(const char header[16]) {
    LONG version = 0;
    for (int i = 12; i < 15 && header[i] >= '0' && header[i] <= '9'; ++i) {
        version = version * 10 + (header[i] - '0');
    }
    return version;
}

} // namespace

extern "C" {

// Forward declarations used by the deep-copy and ownership operations.
void* __thiscall RKC_RPG_AIDATA_constructor(void* self);
void __thiscall RKC_RPG_AIDATA_destructor(void* self);
int __thiscall RKC_RPG_AIDATA_Copy(void* self, void* source);
void __thiscall RKC_RPG_AIEVENT_destructor(void* self);
void __thiscall RKC_RPG_AIEVENT_Release(void* self);
int __thiscall RKC_RPG_AIEVENT_Copy(void* self, void* source);
void* __thiscall RKC_RPG_AIEVENT_Insert(void* self, LONG index, void* item);
void* __thiscall RKC_RPG_AILIST_constructor(void* self);
void __thiscall RKC_RPG_AILIST_destructor(void* self);
int __thiscall RKC_RPG_AILIST_Copy(void* self, void* source);
void __thiscall RKC_RPG_AILIST_SetName(void* self, char* name);
void __thiscall RKC_RPG_AICONTROL_Release(void* self);
void* __thiscall RKC_RPG_AICONTROL_Insert(void* self, LONG index, void* item);

// -------------------------------------------------------------------------
// RKC_RPG_AIDATA
// -------------------------------------------------------------------------

void* __thiscall RKC_RPG_AIDATA_constructor(void* self) {
    auto* data = static_cast<AIData*>(self);
    data->next = nullptr;
    data->eventNo = 0;
    data->actionNo = 0;
    std::memset(data->condition, 0, sizeof(data->condition));
    *reinterpret_cast<LONG*>(data->parameter + 0x00) = 100;
    *reinterpret_cast<LONG*>(data->parameter + 0x04) = -1;
    *reinterpret_cast<LONG*>(data->parameter + 0x08) = 100;
    *reinterpret_cast<LONG*>(data->parameter + 0x0c) = 0;
    *reinterpret_cast<LONG*>(data->parameter + 0x10) = 60;
    *reinterpret_cast<LONG*>(data->parameter + 0x14) = 15;
    // The original constructor deliberately leaves parameter[0x18..0x23].
    return self;
}

void __thiscall RKC_RPG_AIDATA_destructor(void* self) {
    auto* data = static_cast<AIData*>(self);
    data->eventNo = 0;
    data->actionNo = 0;
    std::memset(data->condition, 0, sizeof(data->condition));
    *reinterpret_cast<LONG*>(data->parameter + 0x00) = 100;
    *reinterpret_cast<LONG*>(data->parameter + 0x04) = -1;
    *reinterpret_cast<LONG*>(data->parameter + 0x08) = 100;
    *reinterpret_cast<LONG*>(data->parameter + 0x0c) = 0;
    *reinterpret_cast<LONG*>(data->parameter + 0x10) = 60;
    *reinterpret_cast<LONG*>(data->parameter + 0x14) = 15;
}

void __thiscall RKC_RPG_AIDATA_Release(void* self) {
    RKC_RPG_AIDATA_destructor(self);
}

void* __thiscall RKC_RPG_AIDATA_operatorAssign(void* self, const void* source) {
    std::memcpy(self, source, sizeof(AIData));
    return self;
}

int __thiscall RKC_RPG_AIDATA_Copy(void* self, void* source) {
    if (!self || !source) {
        return 0;
    }
    auto* destination = static_cast<AIData*>(self);
    auto* input = static_cast<AIData*>(source);
    destination->eventNo = input->eventNo;
    destination->actionNo = input->actionNo;
    std::memcpy(destination->condition, input->condition, sizeof(destination->condition));
    std::memcpy(destination->parameter, input->parameter, sizeof(destination->parameter));
    return 1;
}

LONG __thiscall RKC_RPG_AIDATA_GetEventNo(void* self) {
    return static_cast<AIData*>(self)->eventNo;
}

LONG __thiscall RKC_RPG_AIDATA_GetActionNo(void* self) {
    return static_cast<AIData*>(self)->actionNo;
}

void* __thiscall RKC_RPG_AIDATA_GetCondition(void* self) {
    return static_cast<AIData*>(self)->condition;
}

void* __thiscall RKC_RPG_AIDATA_GetParameter(void* self) {
    return static_cast<AIData*>(self)->parameter;
}

void __thiscall RKC_RPG_AIDATA_SetEventNo(void* self, LONG number) {
    static_cast<AIData*>(self)->eventNo = number;
}

void __thiscall RKC_RPG_AIDATA_SetActionNo(void* self, LONG number) {
    static_cast<AIData*>(self)->actionNo = number;
}

void __thiscall RKC_RPG_AIDATA_SetCondition(void* self, void* condition) {
    std::memcpy(static_cast<AIData*>(self)->condition, condition, 0x18);
}

void __thiscall RKC_RPG_AIDATA_SetParameter(void* self, void* parameter) {
    std::memcpy(static_cast<AIData*>(self)->parameter, parameter, 0x24);
}

// -------------------------------------------------------------------------
// RKC_RPG_AIEVENT
// -------------------------------------------------------------------------

void* __thiscall RKC_RPG_AIEVENT_constructor(void* self) {
    auto* event = static_cast<AIEvent*>(self);
    event->reserved = 0;
    event->head = nullptr;
    return self;
}

void __thiscall RKC_RPG_AIEVENT_Release(void* self) {
    auto* event = static_cast<AIEvent*>(self);
    AIData* data = event->head;
    while (data) {
        AIData* next = data->next;
        RKC_RPG_AIDATA_destructor(data);
        delete data;
        data = next;
    }
    event->head = nullptr;
}

void __thiscall RKC_RPG_AIEVENT_destructor(void* self) {
    RKC_RPG_AIEVENT_Release(self);
}

void* __thiscall RKC_RPG_AIEVENT_operatorAssign(void* self, const void* source) {
    std::memcpy(self, source, sizeof(AIEvent));
    return self;
}

void* __thiscall RKC_RPG_AIEVENT_Get(void* self, LONG index) {
    return nth(static_cast<AIEvent*>(self)->head, index);
}

LONG __thiscall RKC_RPG_AIEVENT_GetNo(void* self, void* item) {
    LONG index = 0;
    for (AIData* data = static_cast<AIEvent*>(self)->head; data; data = data->next, ++index) {
        if (data == item) {
            return index;
        }
    }
    return -1;
}

LONG __thiscall RKC_RPG_AIEVENT_GetCount(void* self) {
    return countList(static_cast<AIEvent*>(self)->head);
}

void* __thiscall RKC_RPG_AIEVENT_Insert(void* self, LONG index, void* item) {
    auto* event = static_cast<AIEvent*>(self);
    AIData* data = static_cast<AIData*>(item);
    if (!data) {
        data = new (std::nothrow) AIData;
        if (!data) {
            return nullptr;
        }
        RKC_RPG_AIDATA_constructor(data);
    }

    if (index == 0) {
        data->next = event->head;
        event->head = data;
        return data;
    }

    AIData* previous = nth(event->head, index - 1);
    if (!previous) {
        if (!item) {
            RKC_RPG_AIDATA_destructor(data);
            delete data;
        }
        return nullptr;
    }
    data->next = previous->next;
    previous->next = data;
    return data;
}

int __thiscall RKC_RPG_AIEVENT_Delete(void* self, LONG index, void** output) {
    auto* event = static_cast<AIEvent*>(self);
    AIData* removed = nullptr;
    if (index == 0) {
        removed = event->head;
        if (!removed) {
            return 0;
        }
        event->head = removed->next;
    } else {
        AIData* previous = nth(event->head, index - 1);
        if (!previous || !previous->next) {
            return 0;
        }
        removed = previous->next;
        previous->next = removed->next;
    }

    if (output) {
        *output = removed;
    } else {
        RKC_RPG_AIDATA_destructor(removed);
        delete removed;
    }
    return 1;
}

int __thiscall RKC_RPG_AIEVENT_Copy(void* self, void* source) {
    if (!self || !source) {
        return 0;
    }
    RKC_RPG_AIEVENT_Release(self);
    auto* destination = static_cast<AIEvent*>(self);
    auto* input = static_cast<AIEvent*>(source);
    AIData** tail = &destination->head;
    for (AIData* current = input->head; current; current = current->next) {
        AIData* copy = new (std::nothrow) AIData;
        if (!copy) {
            return 0;
        }
        RKC_RPG_AIDATA_constructor(copy);
        RKC_RPG_AIDATA_Copy(copy, current);
        *tail = copy;
        tail = &copy->next;
    }
    return 1;
}

// -------------------------------------------------------------------------
// RKC_RPG_AILIST
// -------------------------------------------------------------------------

void __thiscall RKC_RPG_AILIST_SetName(void* self, char* name) {
    auto* list = static_cast<AIList*>(self);
    if (list->name) {
        GlobalFree(list->name);
    }
    if (!name) {
        list->name = nullptr;
        return;
    }
    const SIZE_T size = std::strlen(name) + 1;
    list->name = static_cast<char*>(GlobalAlloc(GPTR, size));
    if (list->name) {
        std::memcpy(list->name, name, size);
    }
}

void* __thiscall RKC_RPG_AILIST_constructor(void* self) {
    auto* list = static_cast<AIList*>(self);
    list->next = nullptr;
    list->name = nullptr;
    RKC_RPG_AILIST_SetName(list, const_cast<char*>("No ActionList Name"));
    list->walkPointSpeed = 10;
    list->events = new (std::nothrow) AIEvent[kEventCount];
    if (list->events) {
        for (LONG i = 0; i < kEventCount; ++i) {
            RKC_RPG_AIEVENT_constructor(&list->events[i]);
        }
    }
    return self;
}

void __thiscall RKC_RPG_AILIST_Release(void* self) {
    auto* list = static_cast<AIList*>(self);
    if (list->events) {
        for (LONG i = kEventCount - 1; i >= 0; --i) {
            RKC_RPG_AIEVENT_destructor(&list->events[i]);
        }
        delete[] list->events;
        list->events = nullptr;
    }
    if (list->name) {
        GlobalFree(list->name);
        list->name = nullptr;
    }
    list->walkPointSpeed = 10;
}

void __thiscall RKC_RPG_AILIST_destructor(void* self) {
    RKC_RPG_AILIST_Release(self);
}

void* __thiscall RKC_RPG_AILIST_operatorAssign(void* self, const void* source) {
    std::memcpy(self, source, sizeof(AIList));
    return self;
}

LONG __thiscall RKC_RPG_AILIST_GetCount(void*) {
    return kEventCount;
}

void* __thiscall RKC_RPG_AILIST_Get(void* self, LONG index) {
    auto* list = static_cast<AIList*>(self);
    if (index < 0 || index >= kEventCount || !list->events) {
        return nullptr;
    }
    return &list->events[index];
}

int __thiscall RKC_RPG_AILIST_Copy(void* self, void* source) {
    if (!self || !source) {
        return 0;
    }
    auto* destination = static_cast<AIList*>(self);
    auto* input = static_cast<AIList*>(source);
    if (!destination->events || !input->events) {
        return 0;
    }
    for (LONG i = 0; i < kEventCount; ++i) {
        if (!RKC_RPG_AIEVENT_Copy(&destination->events[i], &input->events[i])) {
            return 0;
        }
    }
    RKC_RPG_AILIST_SetName(destination, input->name);
    destination->walkPointSpeed = input->walkPointSpeed;
    return 1;
}

LONG __thiscall RKC_RPG_AILIST_GetAIDataCount(void* self) {
    auto* list = static_cast<AIList*>(self);
    LONG count = 0;
    if (list->events) {
        for (LONG i = 0; i < kEventCount; ++i) {
            count += countList(list->events[i].head);
        }
    }
    return count;
}

LONG __thiscall RKC_RPG_AILIST_GetEventNoFromAIDataNo(void* self, LONG dataNumber) {
    auto* list = static_cast<AIList*>(self);
    LONG accumulated = 0;
    if (list->events) {
        for (LONG event = 0; event < kEventCount; ++event) {
            accumulated += countList(list->events[event].head);
            if (dataNumber < accumulated) {
                return event;
            }
        }
    }
    return -1;
}

void* __thiscall RKC_RPG_AILIST_GetAIDataFromAIDataNo(void* self, LONG dataNumber) {
    auto* list = static_cast<AIList*>(self);
    LONG accumulated = 0;
    if (list->events) {
        for (LONG event = 0; event < kEventCount; ++event) {
            const LONG count = countList(list->events[event].head);
            accumulated += count;
            if (dataNumber < accumulated) {
                return nth(list->events[event].head, count + dataNumber - accumulated);
            }
        }
    }
    return nullptr;
}

void* __thiscall RKC_RPG_AILIST_GetAIData(void* self, LONG eventNumber, LONG dataNumber) {
    auto* list = static_cast<AIList*>(self);
    // This is the original's odd validation: it checks the data number rather
    // than the event number before indexing the event array.
    if (dataNumber < 0 || dataNumber >= kEventCount || !list->events) {
        return nullptr;
    }
    return nth(list->events[eventNumber].head, dataNumber);
}

char* __thiscall RKC_RPG_AILIST_GetName(void* self) {
    return static_cast<AIList*>(self)->name;
}

LONG __thiscall RKC_RPG_AILIST_GetWalkPointSpeed(void* self) {
    return static_cast<AIList*>(self)->walkPointSpeed;
}

void __thiscall RKC_RPG_AILIST_SetWalkPointSpeed(void* self, LONG speed) {
    static_cast<AIList*>(self)->walkPointSpeed = speed;
}

// -------------------------------------------------------------------------
// RKC_RPG_AICONTROL
// -------------------------------------------------------------------------

void* __thiscall RKC_RPG_AICONTROL_constructor(void* self) {
    static_cast<AIControl*>(self)->head = nullptr;
    return self;
}

void __thiscall RKC_RPG_AICONTROL_Release(void* self) {
    auto* control = static_cast<AIControl*>(self);
    AIList* list = control->head;
    while (list) {
        AIList* next = list->next;
        RKC_RPG_AILIST_destructor(list);
        delete list;
        list = next;
    }
    control->head = nullptr;
}

void __thiscall RKC_RPG_AICONTROL_destructor(void* self) {
    RKC_RPG_AICONTROL_Release(self);
}

void* __thiscall RKC_RPG_AICONTROL_operatorAssign(void* self, const void* source) {
    std::memcpy(self, source, sizeof(AIControl));
    return self;
}

void* __thiscall RKC_RPG_AICONTROL_Get(void* self, LONG index) {
    return nth(static_cast<AIControl*>(self)->head, index);
}

LONG __thiscall RKC_RPG_AICONTROL_GetNo(void* self, void* item) {
    LONG index = 0;
    for (AIList* list = static_cast<AIControl*>(self)->head; list; list = list->next, ++index) {
        if (list == item) {
            return index;
        }
    }
    return -1;
}

LONG __thiscall RKC_RPG_AICONTROL_GetCount(void* self) {
    return countList(static_cast<AIControl*>(self)->head);
}

void* __thiscall RKC_RPG_AICONTROL_GetFromName(void* self, char* name) {
    for (AIList* list = static_cast<AIControl*>(self)->head; list; list = list->next) {
        if (list->name && name && std::strcmp(list->name, name) == 0) {
            return list;
        }
    }
    return nullptr;
}

void* __thiscall RKC_RPG_AICONTROL_Insert(void* self, LONG index, void* item) {
    auto* control = static_cast<AIControl*>(self);
    AIList* list = static_cast<AIList*>(item);
    if (!list) {
        list = new (std::nothrow) AIList;
        if (!list) {
            return nullptr;
        }
        RKC_RPG_AILIST_constructor(list);
    }

    if (index == 0) {
        list->next = control->head;
        control->head = list;
        return list;
    }

    AIList* previous = nth(control->head, index - 1);
    if (!previous) {
        if (!item) {
            RKC_RPG_AILIST_destructor(list);
            delete list;
        }
        return nullptr;
    }
    list->next = previous->next;
    previous->next = list;
    return list;
}

int __thiscall RKC_RPG_AICONTROL_Delete(void* self, LONG index, void** output) {
    auto* control = static_cast<AIControl*>(self);
    AIList* removed = nullptr;
    if (index == 0) {
        removed = control->head;
        if (!removed) {
            return 0;
        }
        control->head = removed->next;
    } else {
        AIList* previous = nth(control->head, index - 1);
        if (!previous || !previous->next) {
            return 0;
        }
        removed = previous->next;
        previous->next = removed->next;
    }

    if (output) {
        *output = removed;
    } else {
        RKC_RPG_AILIST_destructor(removed);
        delete removed;
    }
    return 1;
}

int __thiscall RKC_RPG_AICONTROL_Copy(void* self, void* source) {
    if (!self || !source) {
        return 0;
    }
    RKC_RPG_AICONTROL_Release(self);
    auto* destination = static_cast<AIControl*>(self);
    auto* input = static_cast<AIControl*>(source);
    AIList** tail = &destination->head;
    for (AIList* current = input->head; current; current = current->next) {
        AIList* copy = new (std::nothrow) AIList;
        if (!copy) {
            return 0;
        }
        RKC_RPG_AILIST_constructor(copy);
        if (!RKC_RPG_AILIST_Copy(copy, current)) {
            RKC_RPG_AILIST_destructor(copy);
            delete copy;
            return 0;
        }
        *tail = copy;
        tail = &copy->next;
    }
    return 1;
}

int __thiscall RKC_RPG_AICONTROL_WriteFile(void* self, char* path) {
    auto* control = static_cast<AIControl*>(self);
    HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }

    const char header[16] = {
        'R', 'K', 'C', '_', 'A', 'I', 'D', 'A', 'T', 'A', ' ', 'v', '0', '0', '1', 0x1a
    };
    const LONG listCount = countList(control->head);
    bool valid = writeExact(file, header, sizeof(header)) &&
                 writeExact(file, &listCount, sizeof(listCount)) &&
                 writeExact(file, &kEventCount, sizeof(kEventCount));

    for (AIList* list = control->head; valid && list; list = list->next) {
        const LONG nameLength = list->name ? static_cast<LONG>(std::strlen(list->name)) : 0;
        valid = writeExact(file, &nameLength, sizeof(nameLength)) &&
                writeExact(file, list->name, nameLength) &&
                writeExact(file, &list->walkPointSpeed, sizeof(list->walkPointSpeed));
        for (LONG eventNumber = 0; valid && eventNumber < kEventCount; ++eventNumber) {
            AIEvent* event = list->events ? &list->events[eventNumber] : nullptr;
            const LONG dataCount = event ? countList(event->head) : 0;
            valid = writeExact(file, &dataCount, sizeof(dataCount));
            for (AIData* data = event ? event->head : nullptr; valid && data; data = data->next) {
                valid = writeExact(file, &data->actionNo, sizeof(data->actionNo)) &&
                        writeExact(file, data->parameter, sizeof(data->parameter)) &&
                        writeExact(file, data->condition, sizeof(data->condition));
            }
        }
    }

    CloseHandle(file);
    return valid ? 1 : 0;
}

int __thiscall RKC_RPG_AICONTROL_ReadFile(void* self, char* path) {
    auto* control = static_cast<AIControl*>(self);
    RKC_RPG_AICONTROL_Release(control);
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }

    char header[16];
    LONG listCount = 0;
    LONG eventCount = 0;
    bool valid = readExact(file, header, sizeof(header)) &&
                 std::memcmp(header, "RKC_AIDATA v", 12) == 0 &&
                 readExact(file, &listCount, sizeof(listCount)) &&
                 readExact(file, &eventCount, sizeof(eventCount)) &&
                 listCount >= 0 && eventCount >= 0;
    const LONG version = valid ? readVersion(header) : 0;

    for (LONG listNumber = 0; valid && listNumber < listCount; ++listNumber) {
        AIList* list = static_cast<AIList*>(
            RKC_RPG_AICONTROL_Insert(control, listNumber, nullptr));
        LONG nameLength = 0;
        valid = list && readExact(file, &nameLength, sizeof(nameLength)) &&
                nameLength >= 0 && nameLength <= 0x100000;
        char* name = nullptr;
        if (valid) {
            name = new (std::nothrow) char[static_cast<SIZE_T>(nameLength) + 1];
            valid = name && readExact(file, name, static_cast<DWORD>(nameLength));
        }
        if (valid) {
            name[nameLength] = '\0';
            RKC_RPG_AILIST_SetName(list, name);
            if (version > 0) {
                valid = readExact(file, &list->walkPointSpeed, sizeof(list->walkPointSpeed));
            }
        }
        delete[] name;

        for (LONG eventNumber = 0; valid && eventNumber < eventCount; ++eventNumber) {
            AIEvent* event =
                (eventNumber < kEventCount && list->events) ? &list->events[eventNumber] : nullptr;
            LONG dataCount = 0;
            valid = readExact(file, &dataCount, sizeof(dataCount)) && dataCount >= 0;
            for (LONG dataNumber = 0; valid && dataNumber < dataCount; ++dataNumber) {
                AIData temporary;
                RKC_RPG_AIDATA_constructor(&temporary);
                valid = readExact(file, &temporary.actionNo, sizeof(temporary.actionNo)) &&
                        readExact(file, temporary.parameter, sizeof(temporary.parameter)) &&
                        readExact(file, temporary.condition, sizeof(temporary.condition));
                if (valid && event) {
                    AIData* data = static_cast<AIData*>(
                        RKC_RPG_AIEVENT_Insert(event, dataNumber, nullptr));
                    valid = data != nullptr;
                    if (data) {
                        data->eventNo = eventNumber;
                        data->actionNo = temporary.actionNo;
                        std::memcpy(data->parameter, temporary.parameter, sizeof(data->parameter));
                        std::memcpy(data->condition, temporary.condition, sizeof(data->condition));
                    }
                }
            }
        }
    }

    CloseHandle(file);
    return valid ? 1 : 0;
}

} // extern "C"
