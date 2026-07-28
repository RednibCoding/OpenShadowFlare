/**
 * RKC_RPG_SCRIPT - ShadowFlare scenario script container and binary codec.
 */

#include <windows.h>

#include <cstdint>
#include <cstring>
#include <new>

namespace {

struct Operand {
    LONG type;
    LONG value;
};

struct Command {
    LONG opcode;
    LONG operandCount;
    Operand* operands;
    Command* next;
};

struct Message {
    LONG id;
    char* name;
    char* data;
    Message* next;
};

struct Sentence {
    Command* head;
    Sentence* next;
};

struct Status {
    LONG networkFlag;
    LONG status;
    LONG characterNo;
    LONG sentence;
    Status* next;
};

template <typename T>
struct Block {
    T* head;
};

struct Flag {
    LONG id;
    LONG value;
    LONG current;
};

struct Script {
    LONG tempFlagCount;
    Flag* tempFlags;
    LONG netFlagCount;
    Flag* netFlags;
    Block<Sentence>* sentences;
    Block<Status>* statuses;
    Block<Message>* messages;
};

static_assert(sizeof(Operand) == 8, "operand ABI");
static_assert(sizeof(Command) == 0x10, "command ABI");
static_assert(sizeof(Message) == 0x10, "message ABI");
static_assert(sizeof(Sentence) == 8, "sentence ABI");
static_assert(sizeof(Status) == 0x14, "status ABI");
static_assert(sizeof(Script) == 0x1c, "script ABI");

bool readExact(HANDLE file, void* destination, DWORD size) {
    DWORD amount = 0;
    return ReadFile(file, destination, size, &amount, nullptr) && amount == size;
}

bool writeExact(HANDLE file, const void* source, DWORD size) {
    DWORD amount = 0;
    return WriteFile(file, source, size, &amount, nullptr) && amount == size;
}

template <typename T>
LONG count(T* item, T* T::* nextMember) {
    LONG result = 0;
    while (item) {
        ++result;
        item = item->*nextMember;
    }
    return result;
}

template <typename T>
T* get(T* item, LONG index, T* T::* nextMember) {
    if (index < 0) {
        return nullptr;
    }
    for (LONG current = 0; item && current < index; ++current) {
        item = item->*nextMember;
    }
    return item;
}

template <typename T>
LONG getNo(T* item, const void* wanted, T* T::* nextMember) {
    LONG index = 0;
    while (item) {
        if (item == wanted) {
            return index;
        }
        item = item->*nextMember;
        ++index;
    }
    return -1;
}

template <typename T>
T* insert(Block<T>* block, LONG index, T* item, T* (*create)(), void (*destroy)(T*)) {
    bool allocated = false;
    if (!item) {
        item = create();
        allocated = true;
        if (!item) {
            return nullptr;
        }
    }
    if (index == 0) {
        item->next = block->head;
        block->head = item;
        return item;
    }
    T* previous = get(block->head, index - 1, &T::next);
    if (!previous) {
        if (allocated) {
            destroy(item);
            delete item;
        }
        return nullptr;
    }
    item->next = previous->next;
    previous->next = item;
    return item;
}

template <typename T>
int erase(Block<T>* block, LONG index, void** output, void (*destroy)(T*)) {
    T* removed = nullptr;
    if (index == 0) {
        removed = block->head;
        if (!removed) {
            return 0;
        }
        block->head = removed->next;
    } else {
        T* previous = get(block->head, index - 1, &T::next);
        if (!previous || !previous->next) {
            return 0;
        }
        removed = previous->next;
        previous->next = removed->next;
    }
    if (output) {
        *output = removed;
    } else {
        destroy(removed);
        delete removed;
    }
    return 1;
}

char* allocateString(SIZE_T size) {
    return static_cast<char*>(GlobalAlloc(GPTR, size));
}

} // namespace

extern "C" {

// Forward declarations for ownership helpers.
void __thiscall RKC_RPG_SCRIPT_COMMAND_destructor(void*);
void __thiscall RKC_RPG_SCRIPT_MESSAGE_destructor(void*);
void __thiscall RKC_RPG_SCRIPT_SENTENCE_destructor(void*);
void __thiscall RKC_RPG_SCRIPT_STATUS_destructor(void*);
void __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_Release(void*);
void __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_Release(void*);
void __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_Release(void*);
void __thiscall RKC_RPG_SCRIPT_Release(void*);

static Command* createCommand() {
    auto* item = new (std::nothrow) Command;
    if (item) {
        item->opcode = -1;
        item->operandCount = 0;
        item->operands = nullptr;
        item->next = nullptr;
    }
    return item;
}

static Message* createMessage() {
    auto* item = new (std::nothrow) Message;
    if (item) {
        item->id = -1;
        item->name = nullptr;
        item->data = nullptr;
        item->next = nullptr;
    }
    return item;
}

static Sentence* createSentence() {
    auto* item = new (std::nothrow) Sentence;
    if (item) {
        item->head = nullptr;
        item->next = nullptr;
    }
    return item;
}

static Status* createStatus() {
    auto* item = new (std::nothrow) Status;
    if (item) {
        item->networkFlag = 0;
        item->status = -1;
        item->characterNo = -1;
        item->sentence = -1;
        item->next = nullptr;
    }
    return item;
}

static void destroyCommand(Command* item) {
    RKC_RPG_SCRIPT_COMMAND_destructor(item);
}
static void destroyMessage(Message* item) {
    RKC_RPG_SCRIPT_MESSAGE_destructor(item);
}
static void destroySentence(Sentence* item) {
    RKC_RPG_SCRIPT_SENTENCE_destructor(item);
}
static void destroyStatus(Status* item) {
    RKC_RPG_SCRIPT_STATUS_destructor(item);
}

// -------------------------------------------------------------------------
// Commands and sentences
// -------------------------------------------------------------------------

void* __thiscall RKC_RPG_SCRIPT_COMMAND_constructor(void* self) {
    auto* command = static_cast<Command*>(self);
    command->opcode = -1;
    command->operandCount = 0;
    command->operands = nullptr;
    command->next = nullptr;
    return self;
}

void __thiscall RKC_RPG_SCRIPT_COMMAND_Release(void* self) {
    auto* command = static_cast<Command*>(self);
    if (command->operands) {
        GlobalFree(command->operands);
        command->operands = nullptr;
    }
    command->opcode = -1;
    command->operandCount = 0;
}

void __thiscall RKC_RPG_SCRIPT_COMMAND_destructor(void* self) {
    RKC_RPG_SCRIPT_COMMAND_Release(self);
}

void* __thiscall RKC_RPG_SCRIPT_COMMAND_operatorAssign(void* self, const void* source) {
    std::memcpy(self, source, sizeof(Command));
    return self;
}

LONG __thiscall RKC_RPG_SCRIPT_COMMAND_GetOpecode(void* self) {
    return static_cast<Command*>(self)->opcode;
}

LONG __thiscall RKC_RPG_SCRIPT_COMMAND_GetOperandCount(void* self) {
    return static_cast<Command*>(self)->operandCount;
}

void* __thiscall RKC_RPG_SCRIPT_COMMAND_GetOperand(void* self, LONG index) {
    auto* command = static_cast<Command*>(self);
    if (index < 0 || index >= command->operandCount) {
        return nullptr;
    }
    return &command->operands[index];
}

void* __thiscall RKC_RPG_SCRIPT_SENTENCE_constructor(void* self) {
    auto* sentence = static_cast<Sentence*>(self);
    sentence->head = nullptr;
    sentence->next = nullptr;
    return self;
}

void __thiscall RKC_RPG_SCRIPT_SENTENCE_Release(void* self) {
    auto* sentence = static_cast<Sentence*>(self);
    Command* command = sentence->head;
    while (command) {
        Command* next = command->next;
        destroyCommand(command);
        delete command;
        command = next;
    }
    sentence->head = nullptr;
}

void __thiscall RKC_RPG_SCRIPT_SENTENCE_destructor(void* self) {
    RKC_RPG_SCRIPT_SENTENCE_Release(self);
}

void* __thiscall RKC_RPG_SCRIPT_SENTENCE_operatorAssign(void* self, const void* source) {
    std::memcpy(self, source, sizeof(Sentence));
    return self;
}

void* __thiscall RKC_RPG_SCRIPT_SENTENCE_Get(void* self, LONG index) {
    return get(static_cast<Sentence*>(self)->head, index, &Command::next);
}

LONG __thiscall RKC_RPG_SCRIPT_SENTENCE_GetCount(void* self) {
    return count(static_cast<Sentence*>(self)->head, &Command::next);
}

LONG __thiscall RKC_RPG_SCRIPT_SENTENCE_GetNo(void* self, void* item) {
    return getNo(static_cast<Sentence*>(self)->head, item, &Command::next);
}

void* __thiscall RKC_RPG_SCRIPT_SENTENCE_Insert(void* self, LONG index, void* item) {
    auto* sentence = static_cast<Sentence*>(self);
    Block<Command> view{sentence->head};
    Command* result =
        insert(&view, index, static_cast<Command*>(item), createCommand, destroyCommand);
    sentence->head = view.head;
    return result;
}

int __thiscall RKC_RPG_SCRIPT_SENTENCE_Delete(void* self, LONG index, void** output) {
    auto* sentence = static_cast<Sentence*>(self);
    Block<Command> view{sentence->head};
    const int result = erase(&view, index, output, destroyCommand);
    sentence->head = view.head;
    return result;
}

// -------------------------------------------------------------------------
// Messages
// -------------------------------------------------------------------------

void* __thiscall RKC_RPG_SCRIPT_MESSAGE_constructor(void* self) {
    auto* message = static_cast<Message*>(self);
    message->id = -1;
    message->name = nullptr;
    message->data = nullptr;
    message->next = nullptr;
    return self;
}

void __thiscall RKC_RPG_SCRIPT_MESSAGE_Release(void* self) {
    auto* message = static_cast<Message*>(self);
    if (message->data) {
        GlobalFree(message->data);
        message->data = nullptr;
    }
    if (message->name) {
        GlobalFree(message->name);
        message->name = nullptr;
    }
    message->next = nullptr;
    message->id = -1;
}

void __thiscall RKC_RPG_SCRIPT_MESSAGE_destructor(void* self) {
    RKC_RPG_SCRIPT_MESSAGE_Release(self);
}

void* __thiscall RKC_RPG_SCRIPT_MESSAGE_operatorAssign(void* self, const void* source) {
    std::memcpy(self, source, sizeof(Message));
    return self;
}

LONG __thiscall RKC_RPG_SCRIPT_MESSAGE_GetID(void* self) {
    return static_cast<Message*>(self)->id;
}

char* __thiscall RKC_RPG_SCRIPT_MESSAGE_GetName(void* self) {
    return static_cast<Message*>(self)->name;
}

char* __thiscall RKC_RPG_SCRIPT_MESSAGE_GetData(void* self) {
    return static_cast<Message*>(self)->data;
}

void* __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_constructor(void* self) {
    static_cast<Block<Message>*>(self)->head = nullptr;
    return self;
}

void __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_Release(void* self) {
    auto* block = static_cast<Block<Message>*>(self);
    Message* item = block->head;
    while (item) {
        Message* next = item->next;
        destroyMessage(item);
        delete item;
        item = next;
    }
    block->head = nullptr;
}

void __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_destructor(void* self) {
    RKC_RPG_SCRIPT_MESSAGEBLOCK_Release(self);
}

void* __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_operatorAssign(void* self, const void* source) {
    std::memcpy(self, source, sizeof(Block<Message>));
    return self;
}

void* __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_Get(void* self, LONG index) {
    return get(static_cast<Block<Message>*>(self)->head, index, &Message::next);
}

LONG __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_GetCount(void* self) {
    return count(static_cast<Block<Message>*>(self)->head, &Message::next);
}

LONG __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_GetNo(void* self, void* item) {
    return getNo(static_cast<Block<Message>*>(self)->head, item, &Message::next);
}

void* __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_Insert(void* self, LONG index, void* item) {
    return insert(
        static_cast<Block<Message>*>(self), index, static_cast<Message*>(item),
        createMessage, destroyMessage);
}

int __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_Delete(void* self, LONG index, void** output) {
    return erase(static_cast<Block<Message>*>(self), index, output, destroyMessage);
}

LONG __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_GetMessageNoFromName(void* self, char* name) {
    for (Message* item = static_cast<Block<Message>*>(self)->head; item; item = item->next) {
        if (item->name && name && std::strcmp(item->name, name) == 0) {
            return item->id;
        }
    }
    return -1;
}

void* __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_GetFromID(void* self, LONG id) {
    for (Message* item = static_cast<Block<Message>*>(self)->head; item; item = item->next) {
        if (item->id == id) {
            return item;
        }
    }
    return nullptr;
}

// -------------------------------------------------------------------------
// Sentence and status blocks
// -------------------------------------------------------------------------

void* __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_constructor(void* self) {
    static_cast<Block<Sentence>*>(self)->head = nullptr;
    return self;
}

void __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_Release(void* self) {
    auto* block = static_cast<Block<Sentence>*>(self);
    Sentence* item = block->head;
    while (item) {
        Sentence* next = item->next;
        destroySentence(item);
        delete item;
        item = next;
    }
    block->head = nullptr;
}

void __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_destructor(void* self) {
    RKC_RPG_SCRIPT_SENTENCEBLOCK_Release(self);
}

void* __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_operatorAssign(void* self, const void* source) {
    std::memcpy(self, source, sizeof(Block<Sentence>));
    return self;
}

void* __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_Get(void* self, LONG index) {
    return get(static_cast<Block<Sentence>*>(self)->head, index, &Sentence::next);
}

LONG __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_GetCount(void* self) {
    return count(static_cast<Block<Sentence>*>(self)->head, &Sentence::next);
}

LONG __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_GetNo(void* self, void* item) {
    return getNo(static_cast<Block<Sentence>*>(self)->head, item, &Sentence::next);
}

void* __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_Insert(void* self, LONG index, void* item) {
    return insert(
        static_cast<Block<Sentence>*>(self), index, static_cast<Sentence*>(item),
        createSentence, destroySentence);
}

int __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_Delete(void* self, LONG index, void** output) {
    return erase(static_cast<Block<Sentence>*>(self), index, output, destroySentence);
}

void* __thiscall RKC_RPG_SCRIPT_STATUS_constructor(void* self) {
    auto* status = static_cast<Status*>(self);
    status->networkFlag = 0;
    status->status = -1;
    status->characterNo = -1;
    status->sentence = -1;
    status->next = nullptr;
    return self;
}

void __thiscall RKC_RPG_SCRIPT_STATUS_destructor(void*) {}

void* __thiscall RKC_RPG_SCRIPT_STATUS_operatorAssign(void* self, const void* source) {
    std::memcpy(self, source, sizeof(Status));
    return self;
}

LONG __thiscall RKC_RPG_SCRIPT_STATUS_GetNetworkFlag(void* self) {
    return static_cast<Status*>(self)->networkFlag;
}

LONG __thiscall RKC_RPG_SCRIPT_STATUS_GetStatus(void* self) {
    return static_cast<Status*>(self)->status;
}

LONG __thiscall RKC_RPG_SCRIPT_STATUS_GetCharacterNo(void* self) {
    return static_cast<Status*>(self)->characterNo;
}

LONG __thiscall RKC_RPG_SCRIPT_STATUS_GetSentence(void* self) {
    return static_cast<Status*>(self)->sentence;
}

void* __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_constructor(void* self) {
    static_cast<Block<Status>*>(self)->head = nullptr;
    return self;
}

void __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_Release(void* self) {
    auto* block = static_cast<Block<Status>*>(self);
    Status* item = block->head;
    while (item) {
        Status* next = item->next;
        destroyStatus(item);
        delete item;
        item = next;
    }
    block->head = nullptr;
}

void __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_destructor(void* self) {
    RKC_RPG_SCRIPT_STATUSBLOCK_Release(self);
}

void* __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_operatorAssign(void* self, const void* source) {
    std::memcpy(self, source, sizeof(Block<Status>));
    return self;
}

void* __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_Get(void* self, LONG index) {
    return get(static_cast<Block<Status>*>(self)->head, index, &Status::next);
}

void* __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_GetPair(
    void* self, LONG statusNumber, LONG characterNo) {
    for (Status* item = static_cast<Block<Status>*>(self)->head; item; item = item->next) {
        if (item->status == statusNumber && item->characterNo == characterNo) {
            return item;
        }
    }
    return nullptr;
}

LONG __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_GetCount(void* self) {
    return count(static_cast<Block<Status>*>(self)->head, &Status::next);
}

LONG __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_GetNo(void* self, void* item) {
    return getNo(static_cast<Block<Status>*>(self)->head, item, &Status::next);
}

void* __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_Insert(void* self, LONG index, void* item) {
    return insert(
        static_cast<Block<Status>*>(self), index, static_cast<Status*>(item),
        createStatus, destroyStatus);
}

int __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_Delete(void* self, LONG index, void** output) {
    return erase(static_cast<Block<Status>*>(self), index, output, destroyStatus);
}

// -------------------------------------------------------------------------
// Script object and binary format
// -------------------------------------------------------------------------

void* __thiscall RKC_RPG_SCRIPT_constructor(void* self) {
    auto* script = static_cast<Script*>(self);
    std::memset(script, 0, sizeof(*script));
    script->sentences = new (std::nothrow) Block<Sentence>{nullptr};
    script->statuses = new (std::nothrow) Block<Status>{nullptr};
    script->messages = new (std::nothrow) Block<Message>{nullptr};
    return self;
}

void __thiscall RKC_RPG_SCRIPT_Release(void* self) {
    auto* script = static_cast<Script*>(self);
    script->tempFlagCount = 0;
    if (script->tempFlags) {
        GlobalFree(script->tempFlags);
        script->tempFlags = nullptr;
    }
    script->netFlagCount = 0;
    if (script->netFlags) {
        GlobalFree(script->netFlags);
        script->netFlags = nullptr;
    }
    if (script->sentences)
        RKC_RPG_SCRIPT_SENTENCEBLOCK_Release(script->sentences);
    if (script->statuses)
        RKC_RPG_SCRIPT_STATUSBLOCK_Release(script->statuses);
    if (script->messages)
        RKC_RPG_SCRIPT_MESSAGEBLOCK_Release(script->messages);
}

void __thiscall RKC_RPG_SCRIPT_destructor(void* self) {
    auto* script = static_cast<Script*>(self);
    RKC_RPG_SCRIPT_Release(script);
    delete script->sentences;
    delete script->statuses;
    delete script->messages;
}

void* __thiscall RKC_RPG_SCRIPT_operatorAssign(void* self, const void* source) {
    std::memcpy(self, source, sizeof(Script));
    return self;
}

void* __thiscall RKC_RPG_SCRIPT_GetSentenceBlock(void* self) {
    return static_cast<Script*>(self)->sentences;
}

void* __thiscall RKC_RPG_SCRIPT_GetStatusBlock(void* self) {
    return static_cast<Script*>(self)->statuses;
}

void* __thiscall RKC_RPG_SCRIPT_GetMessageBlock(void* self) {
    return static_cast<Script*>(self)->messages;
}

void* __thiscall RKC_RPG_SCRIPT_GetNetFlag(void* self) {
    return static_cast<Script*>(self)->netFlags;
}

void* __thiscall RKC_RPG_SCRIPT_GetTempFlag(void* self) {
    return static_cast<Script*>(self)->tempFlags;
}

LONG __thiscall RKC_RPG_SCRIPT_GetNetFlagCount(void* self) {
    return static_cast<Script*>(self)->netFlagCount;
}

LONG __thiscall RKC_RPG_SCRIPT_GetTempFlagCount(void* self) {
    return static_cast<Script*>(self)->tempFlagCount;
}

LONG __thiscall RKC_RPG_SCRIPT_GetTempFlag_index(void* self, LONG id) {
    auto* script = static_cast<Script*>(self);
    for (LONG i = 0; i < script->tempFlagCount; ++i) {
        if (script->tempFlags[i].id == id) {
            return script->tempFlags[i].value;
        }
    }
    return -1;
}

LONG __thiscall RKC_RPG_SCRIPT_GetNetFlag_index(void* self, LONG id) {
    auto* script = static_cast<Script*>(self);
    for (LONG i = 0; i < script->netFlagCount; ++i) {
        if (script->netFlags[i].id == id) {
            return script->netFlags[i].value;
        }
    }
    return -1;
}

int __thiscall RKC_RPG_SCRIPT_ReadBinary(void* self, char* path) {
    auto* script = static_cast<Script*>(self);
    RKC_RPG_SCRIPT_Release(script);
    HANDLE file = CreateFileA(
        path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }

    char header[16];
    bool valid =
        readExact(file, header, sizeof(header)) &&
        std::memcmp(header, "ScenaScriptV", 12) == 0;
    auto readFlags = [&](LONG& flagCount, Flag*& flags) {
        valid = valid && readExact(file, &flagCount, sizeof(flagCount)) &&
                flagCount >= 0;
        if (!valid)
            return;
        flags = static_cast<Flag*>(
            GlobalAlloc(GPTR, static_cast<SIZE_T>(flagCount) * sizeof(Flag)));
        valid = flagCount == 0 || flags != nullptr;
        for (LONG i = 0; valid && i < flagCount; ++i) {
            valid = readExact(file, &flags[i].id, 4) &&
                    readExact(file, &flags[i].value, 4);
        }
    };
    readFlags(script->tempFlagCount, script->tempFlags);
    readFlags(script->netFlagCount, script->netFlags);

    LONG itemCount = 0;
    valid = valid && readExact(file, &itemCount, 4) && itemCount >= 0;
    for (LONG i = 0; valid && i < itemCount; ++i) {
        auto* message = static_cast<Message*>(
            RKC_RPG_SCRIPT_MESSAGEBLOCK_Insert(script->messages, i, nullptr));
        LONG size = 0;
        valid = message && readExact(file, &message->id, 4) &&
                readExact(file, &size, 4) && size >= 0;
        if (valid) {
            message->data = allocateString(static_cast<SIZE_T>(size) + 1);
            valid = message->data &&
                    readExact(file, message->data, static_cast<DWORD>(size));
            for (LONG byte = 0; valid && byte < size; ++byte) {
                message->data[byte] =
                    static_cast<char>(~static_cast<unsigned char>(message->data[byte]));
            }
        }
    }

    valid = valid && readExact(file, &itemCount, 4) && itemCount >= 0;
    for (LONG i = 0; valid && i < itemCount; ++i) {
        auto* status = static_cast<Status*>(
            RKC_RPG_SCRIPT_STATUSBLOCK_Insert(script->statuses, i, nullptr));
        valid = status && readExact(file, &status->networkFlag, 4) &&
                readExact(file, &status->status, 4) &&
                readExact(file, &status->characterNo, 4) &&
                readExact(file, &status->sentence, 4);
    }

    valid = valid && readExact(file, &itemCount, 4) && itemCount >= 0;
    for (LONG i = 0; valid && i < itemCount; ++i) {
        auto* sentence = static_cast<Sentence*>(
            RKC_RPG_SCRIPT_SENTENCEBLOCK_Insert(script->sentences, i, nullptr));
        LONG commandCount = 0;
        valid = sentence && readExact(file, &commandCount, 4) && commandCount >= 0;
        for (LONG commandNumber = 0; valid && commandNumber < commandCount;
             ++commandNumber) {
            auto* command = static_cast<Command*>(
                RKC_RPG_SCRIPT_SENTENCE_Insert(sentence, commandNumber, nullptr));
            valid = command && readExact(file, &command->opcode, 4) &&
                    readExact(file, &command->operandCount, 4) &&
                    command->operandCount >= 0;
            if (valid) {
                const SIZE_T bytes =
                    static_cast<SIZE_T>(command->operandCount) * sizeof(Operand);
                command->operands =
                    static_cast<Operand*>(GlobalAlloc(GPTR, bytes));
                valid = bytes == 0 || command->operands != nullptr;
                if (valid && bytes) {
                    valid = readExact(
                        file, command->operands, static_cast<DWORD>(bytes));
                }
            }
        }
    }

    CloseHandle(file);
    return valid ? 1 : 0;
}

int __thiscall RKC_RPG_SCRIPT_WriteBinary(void* self, char* path) {
    auto* script = static_cast<Script*>(self);
    HANDLE file = CreateFileA(
        path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }

    const char header[16] = "ScenaScriptV000";
    bool valid = writeExact(file, header, sizeof(header));
    auto writeFlags = [&](LONG flagCount, Flag* flags) {
        valid = valid && writeExact(file, &flagCount, 4);
        for (LONG i = 0; valid && i < flagCount; ++i) {
            valid = writeExact(file, &flags[i].id, 4) &&
                    writeExact(file, &flags[i].value, 4);
        }
    };
    writeFlags(script->tempFlagCount, script->tempFlags);
    writeFlags(script->netFlagCount, script->netFlags);

    LONG itemCount =
        count(script->messages ? script->messages->head : nullptr, &Message::next);
    valid = valid && writeExact(file, &itemCount, 4);
    for (Message* message = script->messages ? script->messages->head : nullptr;
         valid && message; message = message->next) {
        const LONG size =
            message->data ? static_cast<LONG>(std::strlen(message->data)) : 0;
        valid = writeExact(file, &message->id, 4) && writeExact(file, &size, 4);
        if (valid && size) {
            char* encoded = allocateString(size);
            valid = encoded != nullptr;
            for (LONG i = 0; valid && i < size; ++i) {
                encoded[i] = static_cast<char>(
                    ~static_cast<unsigned char>(message->data[i]));
            }
            valid = valid && writeExact(file, encoded, size);
            if (encoded)
                GlobalFree(encoded);
        }
    }

    itemCount =
        count(script->statuses ? script->statuses->head : nullptr, &Status::next);
    valid = valid && writeExact(file, &itemCount, 4);
    for (Status* status = script->statuses ? script->statuses->head : nullptr;
         valid && status; status = status->next) {
        valid = writeExact(file, &status->networkFlag, 4) &&
                writeExact(file, &status->status, 4) &&
                writeExact(file, &status->characterNo, 4) &&
                writeExact(file, &status->sentence, 4);
    }

    itemCount =
        count(script->sentences ? script->sentences->head : nullptr, &Sentence::next);
    valid = valid && writeExact(file, &itemCount, 4);
    for (Sentence* sentence = script->sentences ? script->sentences->head : nullptr;
         valid && sentence; sentence = sentence->next) {
        const LONG commandCount = count(sentence->head, &Command::next);
        valid = writeExact(file, &commandCount, 4);
        for (Command* command = sentence->head; valid && command;
             command = command->next) {
            valid = writeExact(file, &command->opcode, 4) &&
                    writeExact(file, &command->operandCount, 4) &&
                    writeExact(
                        file, command->operands,
                        static_cast<DWORD>(command->operandCount * sizeof(Operand)));
        }
    }

    CloseHandle(file);
    return valid ? 1 : 0;
}

// The text compiler is not used by the retail game. Keep these exports local
// and explicit until their language grammar has dedicated differential tests.
LONG __thiscall RKC_RPG_SCRIPT_AnalyzeCharacterNo(void*, char*) { return 0; }
int __thiscall RKC_RPG_SCRIPT_AnalyzeCommand(void*, char*, void*) { return 0; }
LONG __thiscall RKC_RPG_SCRIPT_AnalyzeOPERAND_NETFLAG(void*, char*) { return -1; }
int __thiscall RKC_RPG_SCRIPT_AnalyzeOPERAND(void*, char*, void*) { return 0; }
int __thiscall RKC_RPG_SCRIPT_AnalyzeOperator(void*, char*, void*) { return 0; }
int __thiscall RKC_RPG_SCRIPT_ReadSentence(void*, void*, void*) { return 0; }
int __thiscall RKC_RPG_SCRIPT_ReadText(void*, char*) { return 0; }

} // extern "C"
