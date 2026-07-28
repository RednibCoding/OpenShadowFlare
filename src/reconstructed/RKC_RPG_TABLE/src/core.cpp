/**
 * RKC_RPG_TABLE - Game data table management
 * 
 * Manages linked list of table data structures for game parameters,
 * item stats, enemy data, etc.
 * USED BY: ShadowFlare.exe
 */

#include <windows.h>
#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <new>
#include <string>
#include <vector>

class RKC_RPG_TABLEDATA {
public:
    long tableNo;                 // Located at offset 0x00
    int32_t rowCount;             // Located at offset +4
    int32_t columnCount;          // Located at offset +8
    long** table;                 // Located at offset +0x0C
    char*** stringsTable;         // Located at offset +0x10
    RKC_RPG_TABLEDATA* next;      // Located at offset +0x14, Pointer to the next data in the linked list

};


class RKC_RPG_TABLE {
public:
    RKC_RPG_TABLEDATA* headData;
};

struct RkCompressionHeader {
    char magic[8];
    int uncompressedSize;
    int compressedSize;
};

template <typename Function>
static Function GetLzFunction(const char* name)
{
    HMODULE module = GetModuleHandleA("RK_FUNCTION.dll");
    if (!module)
        module = LoadLibraryA("RK_FUNCTION.dll");
    return module
        ? reinterpret_cast<Function>(GetProcAddress(module, name))
        : nullptr;
}

static bool ReadWholeFile(const char* filename, std::vector<unsigned char>& bytes)
{
    HANDLE file = CreateFileA(
        filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    LARGE_INTEGER size{};
    bool success = GetFileSizeEx(file, &size)
        && size.QuadPart >= 0
        && size.QuadPart <= 0x7fffffff;
    if (success) {
        bytes.resize(static_cast<std::size_t>(size.QuadPart));
        DWORD read = 0;
        success = bytes.empty()
            || (ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr)
                && read == bytes.size());
    }
    CloseHandle(file);
    return success;
}

static bool WriteWholeFile(
    const char* filename, const std::vector<unsigned char>& bytes)
{
    HANDLE file = CreateFileA(
        filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    DWORD written = 0;
    const bool success = bytes.empty()
        || (WriteFile(
                file, bytes.data(), static_cast<DWORD>(bytes.size()),
                &written, nullptr)
            && written == bytes.size());
    CloseHandle(file);
    return success;
}

static bool IsShiftJisLead(unsigned char value)
{
    return (value >= 0x81 && value <= 0x9f)
        || (value >= 0xe0 && value <= 0xfc);
}

static bool IsIntegerText(const std::string& text)
{
    if (text.empty())
        return true;
    std::size_t index = text[0] == '-' ? 1 : 0;
    if (index == text.size())
        return false;
    for (; index < text.size(); ++index)
        if (text[index] < '0' || text[index] > '9')
            return false;
    return true;
}

template <typename Value>
static bool ReadValue(
    const unsigned char*& cursor, const unsigned char* end, Value& value)
{
    if (static_cast<std::size_t>(end - cursor) < sizeof(Value))
        return false;
    std::memcpy(&value, cursor, sizeof(Value));
    cursor += sizeof(Value);
    return true;
}

template <typename Value>
static void AppendValue(std::vector<unsigned char>& bytes, const Value& value)
{
    const auto* source = reinterpret_cast<const unsigned char*>(&value);
    bytes.insert(bytes.end(), source, source + sizeof(Value));
}


extern "C" {
    // Forward declarations
    RKC_RPG_TABLEDATA* __thiscall RKC_RPG_TABLEDATA_constructor(RKC_RPG_TABLEDATA* self);
    void __thiscall RKC_RPG_TABLEDATA_deconstructor(RKC_RPG_TABLEDATA* self);
    void __thiscall RKC_RPG_TABLEDATA_Release(RKC_RPG_TABLEDATA* self);
    void __thiscall RKC_RPG_TABLE_Release(RKC_RPG_TABLE* self);
    RKC_RPG_TABLEDATA* __thiscall Get(RKC_RPG_TABLE* self, long index);
    long __thiscall GetCount(RKC_RPG_TABLE* self);

    /*************************************************************/
    /*                        RKC_TABLE                          */
    /*************************************************************/

    /**
     * Constructor - initialize empty table list
     * USED BY: ShadowFlare.exe
     */
    RKC_RPG_TABLE* __thiscall RKC_RPG_TABLE_constructor(RKC_RPG_TABLE* self)
    {
        self->headData = nullptr;
        return self;
    }

    /**
     * Destructor - free all table data
     * USED BY: ShadowFlare.exe
     */
    void __thiscall RKC_TABLE_deconstructor(RKC_RPG_TABLE* self)
    {
        RKC_RPG_TABLE_Release(self);
    }

    /**
     * Assignment operator - shallow copy
     * NOT REFERENCED - not imported by any module
     */
    RKC_RPG_TABLE& __thiscall RKC_TABLE_equalsOperator(RKC_RPG_TABLE* self, RKC_RPG_TABLE rhs)
    {
        // Creates a shallow copy (only pointers are copied but not the values)
        if (self != &rhs) { // Protect against self-assignment
            self->headData = rhs.headData;
        }
        return *self;
    }

    RKC_RPG_TABLEDATA* __thiscall Insert(RKC_RPG_TABLE* self, long tableNo, RKC_RPG_TABLEDATA* data)
    {
        if (tableNo < 0)
            return nullptr;
        RKC_RPG_TABLEDATA* node = data;
        if (!node) {
            node = new(std::nothrow) RKC_RPG_TABLEDATA;
            if (!node)
                return nullptr;
            RKC_RPG_TABLEDATA_constructor(node);
        }
        if (tableNo == 0) {
            node->next = self->headData;
            self->headData = node;
            return node;
        }
        RKC_RPG_TABLEDATA* previous = Get(self, tableNo - 1);
        if (!previous) {
            if (!data)
                delete node;
            return nullptr;
        }
        node->next = previous->next;
        previous->next = node;
        return node;
    }

    int32_t __thiscall ReadBinaryFile(RKC_RPG_TABLE* self, char* fileName)
    {
        RKC_RPG_TABLE_Release(self);
        std::vector<unsigned char> file;
        if (!fileName || !ReadWholeFile(fileName, file)
            || file.size() < 24
            || std::memcmp(file.data(), "TABLE DATA V", 12) != 0)
            return 0;

        std::uint32_t compressed = 0;
        std::memcpy(&compressed, file.data() + 16, sizeof(compressed));
        const unsigned char* payload = nullptr;
        std::size_t payloadSize = 0;
        void* decoded = nullptr;
        RkCompressionHeader compressionHeader{};
        if (compressed == 0) {
            std::uint32_t storedSize = 0;
            std::memcpy(&storedSize, file.data() + 20, sizeof(storedSize));
            if (storedSize > file.size() - 24)
                return 0;
            payload = file.data() + 24;
            payloadSize = storedSize;
        } else {
            using DecodeFunction = int (__cdecl*)(
                const void*, int, void**, void*);
            DecodeFunction decode =
                GetLzFunction<DecodeFunction>("RK_LzDecodeMemoryToMemory");
            if (!decode
                || !decode(
                    file.data() + 20,
                    static_cast<int>(file.size() - 20),
                    &decoded,
                    &compressionHeader)
                || !decoded
                || compressionHeader.uncompressedSize < 0)
                return 0;
            payload = static_cast<const unsigned char*>(decoded);
            payloadSize = static_cast<std::size_t>(
                compressionHeader.uncompressedSize);
        }

        const unsigned char* cursor = payload;
        const unsigned char* end = payload + payloadSize;
        std::int32_t tableCount = 0;
        bool valid = ReadValue(cursor, end, tableCount)
            && tableCount >= 0
            && tableCount <= 100000;
        for (std::int32_t tableIndex = 0; valid && tableIndex < tableCount; ++tableIndex) {
            RKC_RPG_TABLEDATA* tableData = Insert(self, tableIndex, nullptr);
            std::int32_t rows = 0;
            std::int32_t columns = 0;
            valid = tableData
                && ReadValue(cursor, end, tableData->tableNo)
                && ReadValue(cursor, end, rows)
                && ReadValue(cursor, end, columns)
                && rows >= 0 && columns >= 0
                && rows <= 100000 && columns <= 100000
                && (rows == 0 || columns <= 0x7fffffff / rows);
            if (!valid)
                break;
            tableData->rowCount = rows;
            tableData->columnCount = columns;
            tableData->table = static_cast<long**>(
                GlobalAlloc(GPTR, static_cast<SIZE_T>(rows) * sizeof(long*)));
            tableData->stringsTable = static_cast<char***>(
                GlobalAlloc(GPTR, static_cast<SIZE_T>(rows) * sizeof(char**)));
            valid = rows == 0 || (tableData->table && tableData->stringsTable);
            for (std::int32_t row = 0; valid && row < rows; ++row) {
                tableData->table[row] = static_cast<long*>(
                    GlobalAlloc(GPTR, static_cast<SIZE_T>(columns) * sizeof(long)));
                tableData->stringsTable[row] = static_cast<char**>(
                    GlobalAlloc(GPTR, static_cast<SIZE_T>(columns) * sizeof(char*)));
                valid = columns == 0
                    || (tableData->table[row] && tableData->stringsTable[row]);
            }
            for (std::int32_t row = 0; valid && row < rows; ++row)
                for (std::int32_t column = 0; valid && column < columns; ++column)
                    valid = ReadValue(
                        cursor, end, tableData->table[row][column]);
            for (std::int32_t row = 0; valid && row < rows; ++row) {
                for (std::int32_t column = 0; valid && column < columns; ++column) {
                    std::uint32_t length = 0;
                    valid = ReadValue(cursor, end, length)
                        && length <= static_cast<std::size_t>(end - cursor);
                    if (!valid)
                        break;
                    if (length != 0) {
                        char* text = static_cast<char*>(
                            GlobalAlloc(GPTR, static_cast<SIZE_T>(length) + 1));
                        if (!text) {
                            valid = false;
                            break;
                        }
                        for (std::uint32_t index = 0; index < length; ++index)
                            text[index] = static_cast<char>(~cursor[index]);
                        cursor += length;
                        tableData->stringsTable[row][column] = text;
                    }
                }
            }
        }
        if (decoded)
            GlobalFree(decoded);
        if (!valid) {
            RKC_RPG_TABLE_Release(self);
            return 0;
        }
        return 1;
    }

    /**
     * Write table data to binary file
     * NOT REFERENCED - not imported by any module
     * Note: Forwards to original DLL
     */
    int32_t __thiscall WriteBinaryFile(RKC_RPG_TABLE* self, char* fileName)
    {
        if (!fileName)
            return 0;
        std::vector<unsigned char> payload;
        const std::int32_t count = GetCount(self);
        AppendValue(payload, count);
        for (RKC_RPG_TABLEDATA* item = self->headData; item; item = item->next) {
            AppendValue(payload, item->tableNo);
            AppendValue(payload, item->rowCount);
            AppendValue(payload, item->columnCount);
            for (long row = 0; row < item->rowCount; ++row)
                for (long column = 0; column < item->columnCount; ++column)
                    AppendValue(payload, item->table[row][column]);
            for (long row = 0; row < item->rowCount; ++row) {
                for (long column = 0; column < item->columnCount; ++column) {
                    const char* text = item->stringsTable[row][column];
                    const std::uint32_t length = text
                        ? static_cast<std::uint32_t>(std::strlen(text))
                        : 0;
                    AppendValue(payload, length);
                    for (std::uint32_t index = 0; index < length; ++index)
                        payload.push_back(static_cast<unsigned char>(~text[index]));
                }
            }
        }

        std::vector<unsigned char> output;
        const unsigned char header[16] = {
            'T','A','B','L','E',' ','D','A','T','A',' ','V','0','0','0',0x1a
        };
        output.insert(output.end(), header, header + sizeof(header));
        const int encodedCapacity = static_cast<int>(
            32 + payload.size() + (payload.size() + 7) / 8);
        std::vector<unsigned char> encoded(
            static_cast<std::size_t>(encodedCapacity));
        RkCompressionHeader compressionHeader{};
        using EncodeFunction = int (__cdecl*)(
            const void*, int, void*, int, void*);
        EncodeFunction encode =
            GetLzFunction<EncodeFunction>("RK_LzEncodeMemoryToMemory");
        const std::int32_t useCompression =
            encode && encode(
                payload.data(), static_cast<int>(payload.size()),
                encoded.data(), encodedCapacity, &compressionHeader)
                ? 1 : 0;
        AppendValue(output, useCompression);
        if (useCompression) {
            const std::size_t encodedSize =
                16 + static_cast<std::size_t>(
                    compressionHeader.compressedSize);
            output.insert(
                output.end(), encoded.begin(),
                encoded.begin() + encodedSize);
        } else {
            const std::uint32_t size =
                static_cast<std::uint32_t>(payload.size());
            AppendValue(output, size);
            output.insert(output.end(), payload.begin(), payload.end());
        }
        return WriteWholeFile(fileName, output) ? 1 : 0;
    }

    /**
     * Read single text table
     * NOT REFERENCED - not imported by any module
     * Note: Forwards to original DLL
     */
    int32_t ReadTextTable(RKC_RPG_TABLEDATA* self, char* fileName)
    {
        RKC_RPG_TABLEDATA_Release(self);
        std::vector<unsigned char> file;
        if (!fileName || !ReadWholeFile(fileName, file) || file.empty())
            return 0;

        long rows = 0;
        long columns = 0;
        bool firstRow = true;
        for (std::size_t index = 0; index < file.size(); ++index) {
            if (IsShiftJisLead(file[index]) && index + 1 < file.size()) {
                ++index;
                continue;
            }
            if (firstRow && file[index] == '\t')
                ++columns;
            if (file[index] == '\r') {
                if (firstRow) {
                    ++columns;
                    firstRow = false;
                }
                ++rows;
            }
        }
        if (rows <= 0 || columns <= 0)
            return 0;

        self->rowCount = rows;
        self->columnCount = columns;
        self->table = static_cast<long**>(
            GlobalAlloc(GPTR, static_cast<SIZE_T>(rows) * sizeof(long*)));
        self->stringsTable = static_cast<char***>(
            GlobalAlloc(GPTR, static_cast<SIZE_T>(rows) * sizeof(char**)));
        if (!self->table || !self->stringsTable) {
            RKC_RPG_TABLEDATA_Release(self);
            return 0;
        }
        for (long row = 0; row < rows; ++row) {
            self->table[row] = static_cast<long*>(
                GlobalAlloc(GPTR, static_cast<SIZE_T>(columns) * sizeof(long)));
            self->stringsTable[row] = static_cast<char**>(
                GlobalAlloc(GPTR, static_cast<SIZE_T>(columns) * sizeof(char*)));
            if (!self->table[row] || !self->stringsTable[row]) {
                RKC_RPG_TABLEDATA_Release(self);
                return 0;
            }
        }

        long row = 0;
        long column = 0;
        std::size_t tokenStart = 0;
        for (std::size_t index = 0; index <= file.size() && row < rows; ++index) {
            const bool atEnd = index == file.size();
            if (!atEnd && file[index] != '\t' && file[index] != '\r')
                continue;
            std::string token(
                reinterpret_cast<const char*>(file.data() + tokenStart),
                index - tokenStart);
            if (!token.empty() && token.back() == '\n')
                token.pop_back();
            if (IsIntegerText(token)) {
                self->table[row][column] =
                    token.empty() ? 0 : std::strtol(token.c_str(), nullptr, 10);
            } else {
                char* stored = static_cast<char*>(
                    GlobalAlloc(GPTR, token.size() + 1));
                if (!stored) {
                    RKC_RPG_TABLEDATA_Release(self);
                    return 0;
                }
                std::memcpy(stored, token.data(), token.size());
                self->stringsTable[row][column] = stored;
            }
            ++column;
            if (column == columns) {
                column = 0;
                ++row;
            }
            tokenStart = index + 1;
            if (!atEnd && file[index] == '\r'
                && tokenStart < file.size() && file[tokenStart] == '\n')
                ++tokenStart;
        }
        return 1;
    }

    /**
     * Read all text tables from file
     * NOT REFERENCED - not imported by any module
     * Note: Forwards to original DLL
     */
    int32_t ReadAllTextTable(RKC_RPG_TABLE* self, char* fileName)
    {
        if (!fileName)
            return 0;
        std::string root(fileName);
        const std::size_t separator = root.find_last_of("\\/");
        if (separator != std::string::npos)
            root.resize(separator + 1);
        else
            root.clear();
        const std::string pattern = root + "Table_*.txt";
        WIN32_FIND_DATAA findData{};
        HANDLE search = FindFirstFileA(pattern.c_str(), &findData);
        if (search == INVALID_HANDLE_VALUE)
            return 1;
        bool success = true;
        do {
            const std::string name(findData.cFileName);
            const std::size_t underscore = name.find('_');
            const long tableNo = underscore == std::string::npos
                ? 0 : std::strtol(name.c_str() + underscore + 1, nullptr, 10);
            RKC_RPG_TABLEDATA* item = Insert(self, GetCount(self), nullptr);
            std::string path = root + name;
            if (!item
                || !ReadTextTable(
                    item, const_cast<char*>(path.c_str()))) {
                success = false;
                break;
            }
            item->tableNo = tableNo;
        } while (FindNextFileA(search, &findData));
        FindClose(search);
        return success ? 1 : 0;
    }

    /**
     * Delete table data at index
     * NOT REFERENCED - not imported by any module
     */
    bool __thiscall Delete(RKC_RPG_TABLE* self, long index, RKC_RPG_TABLEDATA** dataToDelete)
    {
        if (!self->headData || !dataToDelete) {
            return false;
        }

        if (index == 0) {
            *dataToDelete = self->headData;
            self->headData = self->headData->next;
            return true;
        }

        RKC_RPG_TABLEDATA* prevNode = nullptr;
        RKC_RPG_TABLEDATA* currentNode = self->headData;
        long currentIndex = 0;

        while (currentNode && currentIndex < index) {
            prevNode = currentNode;
            currentNode = currentNode->next;
            currentIndex++;
        }

        if (currentNode) {
            if (prevNode) {
                prevNode->next = currentNode->next;
            }
            *dataToDelete = currentNode;
            return true;
        }

        return false;
    }

    /**
     * Release all table data
     * NOT REFERENCED - not imported by any module (but called internally
     * by the original DLL's ReadBinaryFile before re-populating).
     * 
     * Can't implement locally: nodes are allocated by the original DLL's
     * operator_new (via forwarded Insert/ReadBinaryFile), so freeing them
     * with our CRT's delete causes a heap corruption crash.
     * Must stay forwarded until Insert and ReadBinaryFile are also local.
     */
    void __thiscall RKC_RPG_TABLE_Release(RKC_RPG_TABLE* self)
    {
        RKC_RPG_TABLEDATA* current = self->headData;
        while (current) {
            RKC_RPG_TABLEDATA* next = current->next;
            RKC_RPG_TABLEDATA_deconstructor(current);
            delete current;
            current = next;
        }
        self->headData = nullptr;
    }

    /**
     * Get table data at index
     * NOT REFERENCED - not imported by any module
     */
    RKC_RPG_TABLEDATA* __thiscall Get(RKC_RPG_TABLE* self, long index)
    {
        if (index < 0) {
            return nullptr;
        }

        RKC_RPG_TABLEDATA* currentNode = self->headData;
        for (int i = 0; currentNode != nullptr && i < index; ++i) {
            currentNode = currentNode->next;
        }

        return currentNode;
    }

    /**
     * Get total number of tables
     * NOT REFERENCED - not imported by any module
     */
    long __thiscall GetCount(RKC_RPG_TABLE* self)
    {
        int count = 0;
        RKC_RPG_TABLEDATA* currentData = self->headData;

        while (currentData != nullptr) {
            count++;
            currentData = currentData->next;
        }

        return count;
    }

    /**
     * Get table data by table number
     * USED BY: ShadowFlare.exe
     */
    RKC_RPG_TABLEDATA* __thiscall GetFromTableNo(RKC_RPG_TABLE* self, long tableNo)
    {
        RKC_RPG_TABLEDATA* currentData = self->headData;

        while (currentData != nullptr) {
            if (currentData->tableNo == tableNo) {
                return currentData;
            }
            currentData = currentData->next;
        }

        return nullptr;  // If not found, return nullptr.
    }

    /**
     * Get index of table data in list
     * NOT REFERENCED - not imported by any module
     */
    long __thiscall GetNo(RKC_RPG_TABLE* self, RKC_RPG_TABLEDATA* targetData)
    {
        RKC_RPG_TABLEDATA* currentData = self->headData;
        int index = 0;

        while (currentData != nullptr) {
            if (currentData == targetData) {
                return index;
            }
            currentData = currentData->next; // Traversing to the next node
            index++;
        }

        return -1;  // If the node isn't found in the list
    }


    /*************************************************************/
    /*                      RKC_TABLEDATA                        */
    /*************************************************************/

    /**
     * Constructor - initialize table data
     * NOT REFERENCED - not imported by any module
     */
    RKC_RPG_TABLEDATA* __thiscall RKC_RPG_TABLEDATA_constructor(RKC_RPG_TABLEDATA* self)
    {
        self->tableNo = 0xffffffff;
        self->rowCount = 0;
        self->columnCount = 0;
        self->table = nullptr;
        self->stringsTable = nullptr;
        return self;
    }

    /**
     * Destructor - release table data
     * NOT REFERENCED - not imported by any module
     */
    void __thiscall RKC_RPG_TABLEDATA_deconstructor(RKC_RPG_TABLEDATA* self)
    {
        RKC_RPG_TABLEDATA_Release(self);
    }

    /**
     * Assignment operator - shallow copy
     * NOT REFERENCED - not imported by any module
     */
    RKC_RPG_TABLEDATA& __thiscall RKC_RPG_TABLEDATA_equalsOperator(RKC_RPG_TABLEDATA* self, RKC_RPG_TABLEDATA rhs)
    {
        // Creates a shallow copy (only pointers are copied but not the values)
        if (self != &rhs) { // Protect against self-assignment
            self->tableNo = rhs.tableNo;
            self->rowCount = rhs.rowCount;
            self->columnCount = rhs.columnCount;
            self->table = rhs.table;
            self->stringsTable = rhs.stringsTable;
            self->next = rhs.next;
        }
        return *self;
    }

    /**
     * Release table data memory
     * NOT REFERENCED - not imported by any module
     */
    void __thiscall RKC_RPG_TABLEDATA_Release(RKC_RPG_TABLEDATA* self)
    {
        // Deallocate memory for table (numeric values)
        if (self->table != nullptr) {
            for (int rowIndex = 0; rowIndex < self->rowCount; ++rowIndex) {
                GlobalFree(reinterpret_cast<HGLOBAL>(self->table[rowIndex]));
            }
            GlobalFree(reinterpret_cast<HGLOBAL>(self->table));
            self->table = nullptr;
        }

        // Deallocate memory for stringsTable
        if (self->stringsTable != nullptr) {
            for (int rowIndex = 0; rowIndex < self->rowCount; ++rowIndex) {
                if (self->stringsTable[rowIndex] != nullptr) {
                    for (int columnIndex = 0; columnIndex < self->columnCount; ++columnIndex) {
                        if (self->stringsTable[rowIndex][columnIndex] != nullptr) {
                            GlobalFree(reinterpret_cast<HGLOBAL>(self->stringsTable[rowIndex][columnIndex]));
                        }
                    }
                    GlobalFree(reinterpret_cast<HGLOBAL>(self->stringsTable[rowIndex]));
                }
            }
            GlobalFree(reinterpret_cast<HGLOBAL>(self->stringsTable));
            self->stringsTable = nullptr;
        }

        // Reset other members
        self->tableNo = 0xffffffff;
        self->rowCount = 0;
        self->columnCount = 0;
    }

    /**
     * Get string value at row/column
     * USED BY: ShadowFlare.exe
     */
    char* __thiscall GetStrings(RKC_RPG_TABLEDATA* self, long row, long column)
    {
        if (row >= 0 && row < self->rowCount && column >= 0 && column < self->columnCount) {
            return self->stringsTable[row][column];
        }
        return nullptr;
    }

    /**
     * Get entire strings table
     * NOT REFERENCED - not imported by any module
     */
    char*** __thiscall GetStringsTable(RKC_RPG_TABLEDATA* self)
    {
        return self->stringsTable;
    }

    /**
     * Get row count
     * USED BY: ShadowFlare.exe
     */
    long __thiscall GetRowCount(RKC_RPG_TABLEDATA* self)
    {
        return self->rowCount;
    }

    /**
     * Get column count
     * USED BY: ShadowFlare.exe
     */
    long __thiscall GetColCount(RKC_RPG_TABLEDATA* self)
    {
        return self->columnCount;
    }

    /**
     * Get entire table
     * NOT REFERENCED - not imported by any module
     */
    long** GetTable(RKC_RPG_TABLEDATA* self)
    {
        return self->table;
    }

    /**
     * Get table number
     * NOT REFERENCED - not imported by any module
     */
    long __thiscall GetTableNo(RKC_RPG_TABLEDATA* self)
    {
        return self->tableNo;
    }

    /**
     * Get numeric value at row/column
     * USED BY: ShadowFlare.exe
     */
    long __thiscall GetValue(RKC_RPG_TABLEDATA* self, long row, long column)
    {
        if (row >= 0 && row < self->rowCount && column >= 0 && column < self->columnCount)
        {
            return self->table[row][column];
        }
        return -1;
    }
}

bool __stdcall DllMain(LPVOID, std::uint32_t call_reason, LPVOID)
{
    return true;
}
