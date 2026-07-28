#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

template <typename Function>
Function LoadFunction(HMODULE module, const char* name)
{
    FARPROC address = GetProcAddress(module, name);
    if (!address) {
        std::fprintf(stderr, "missing export: %s (error %lu)\n", name, GetLastError());
        ExitProcess(2);
    }
    return reinterpret_cast<Function>(address);
}

static std::uint32_t Fnv1a(
    const void* memory, std::size_t size, std::uint32_t value = 2166136261u)
{
    const auto* bytes = static_cast<const std::uint8_t*>(memory);
    for (std::size_t index = 0; index < size; ++index) {
        value ^= bytes[index];
        value *= 16777619u;
    }
    return value;
}

static int ProbeFile(HMODULE module, const char* scratchPath)
{
    using Constructor = void (__thiscall*)(void*);
    using Destructor = void (__thiscall*)(void*);
    using Create = int (__thiscall*)(void*, char*, long);
    using Close = int (__thiscall*)(void*);
    using Write = int (__thiscall*)(void*, void*, long);
    using Read = int (__thiscall*)(void*, void*, long);
    using Seek = int (__thiscall*)(void*, long, long);
    using GetSize = long (__thiscall*)(void*);
    using GetHandle = HANDLE (__thiscall*)(void*);

    const auto construct = LoadFunction<Constructor>(module, "??0RKC_FILE@@QAE@XZ");
    const auto destruct = LoadFunction<Destructor>(module, "??1RKC_FILE@@QAE@XZ");
    const auto create = LoadFunction<Create>(module, "?Create@RKC_FILE@@QAEHPADJ@Z");
    const auto close = LoadFunction<Close>(module, "?Close@RKC_FILE@@QAEHXZ");
    const auto write = LoadFunction<Write>(module, "?Write@RKC_FILE@@QAEHPAXJ@Z");
    const auto read = LoadFunction<Read>(module, "?Read@RKC_FILE@@QAEHPAXJ@Z");
    const auto seek = LoadFunction<Seek>(module, "?Seek@RKC_FILE@@QAEHJJ@Z");
    const auto getSize = LoadFunction<GetSize>(module, "?GetSize@RKC_FILE@@QAEJXZ");
    const auto getHandle = LoadFunction<GetHandle>(module, "?GetHandle@RKC_FILE@@QAEPAXXZ");

    std::uint8_t object[4]{};
    char path[MAX_PATH]{};
    std::strncpy(path, scratchPath, sizeof(path) - 1);
    DeleteFileA(path);

    construct(object);
    char payload[] = {'S', 'F', '0', '1'};
    const int createWrite = create(object, path, 1);
    const int writeResult = write(object, payload, sizeof(payload));
    const int seekZero = seek(object, 0, FILE_BEGIN);
    const int closeResult = close(object);
    const int createRead = create(object, path, 0);
    char output[4]{};
    const int readResult = read(object, output, sizeof(output));
    const long size = getSize(object);
    HANDLE handleBeforeDestruct = getHandle(object);
    destruct(object);
    DWORD flags = 0;
    const int handleClosed = GetHandleInformation(handleBeforeDestruct, &flags) == FALSE;
    const int objectCleared = getHandle(object) == nullptr;
    DeleteFileA(path);

    std::printf(
        "file create_write=%d write=%d seek_zero=%d close=%d create_read=%d "
        "read=%d size=%ld payload=%08lx destructor_closed=%d object_cleared=%d\n",
        createWrite,
        writeResult,
        seekZero,
        closeResult,
        createRead,
        readResult,
        size,
        static_cast<unsigned long>(Fnv1a(output, sizeof(output))),
        handleClosed,
        objectCleared);
    return 0;
}

static int ProbeMemory(HMODULE module)
{
    using Constructor = void (__thiscall*)(void*);
    using Destructor = void (__thiscall*)(void*);
    using Allocation = char* (__thiscall*)(void*, long, int);
    using Copy = int (__thiscall*)(void*, char*, long, long);
    using Clear = int (__thiscall*)(void*, char, long, long);
    using Get = char* (__thiscall*)(void*);
    using GetSize = long (__thiscall*)(void*);
    using Release = void (__thiscall*)(void*);

    const auto construct = LoadFunction<Constructor>(module, "??0RKC_MEMORY@@QAE@XZ");
    const auto destruct = LoadFunction<Destructor>(module, "??1RKC_MEMORY@@QAE@XZ");
    const auto allocation = LoadFunction<Allocation>(module, "?Allocation@RKC_MEMORY@@QAEPADJH@Z");
    const auto copy = LoadFunction<Copy>(module, "?Copy@RKC_MEMORY@@QAEHPADJJ@Z");
    const auto clear = LoadFunction<Clear>(module, "?Clear@RKC_MEMORY@@QAEHDJJ@Z");
    const auto get = LoadFunction<Get>(module, "?Get@RKC_MEMORY@@QAEPADXZ");
    const auto getSize = LoadFunction<GetSize>(module, "?GetSize@RKC_MEMORY@@QAEJXZ");
    const auto release = LoadFunction<Release>(module, "?Release@RKC_MEMORY@@QAEXXZ");

    std::uint8_t object[8]{};
    construct(object);
    char* memory = allocation(object, 16, 1);
    const std::uint32_t zeroHash = Fnv1a(memory, 16);
    char payload[] = {1, 2, 3, 4, 5};
    const int copyResult = copy(object, payload, sizeof(payload), 3);
    const int clearResult = clear(object, static_cast<char>(0x5a), 4, 5);
    const std::uint32_t mixedHash = Fnv1a(get(object), 16);
    const int clearAllResult = clear(object, static_cast<char>(0x7f), -1, 0);
    const std::uint32_t fullHash = Fnv1a(get(object), 16);
    const long size = getSize(object);
    release(object);
    const int released = get(object) == nullptr && getSize(object) == 0;
    const int zeroAllocation = allocation(object, 0, 1) == nullptr;
    destruct(object);

    std::printf(
        "memory allocated=%d zero_hash=%08lx copy=%d clear=%d mixed_hash=%08lx "
        "clear_all=%d full_hash=%08lx size=%ld released=%d zero_allocation=%d\n",
        memory != nullptr,
        static_cast<unsigned long>(zeroHash),
        copyResult,
        clearResult,
        static_cast<unsigned long>(mixedHash),
        clearAllResult,
        static_cast<unsigned long>(fullHash),
        size,
        released,
        zeroAllocation);
    return 0;
}

static int ProbeWindow(HMODULE module)
{
    using Constructor = void* (__thiscall*)(void*);
    using Assignment = void* (__thiscall*)(void*, const void*);

    const auto construct = LoadFunction<Constructor>(module, "??0RKC_WINDOW@@QAE@XZ");
    const auto assign = LoadFunction<Assignment>(
        module, "??4RKC_WINDOW@@QAEAAV0@ABV0@@Z");

    std::uint8_t object[0x550];
    std::memset(object, 0xa5, sizeof(object));
    void* result = construct(object);
    const std::uint32_t constructorHash = Fnv1a(object, sizeof(object));

    std::uint8_t source[0x550];
    for (std::size_t index = 0; index < sizeof(source); ++index)
        source[index] = static_cast<std::uint8_t>((index * 37u + 11u) & 0xffu);
    void* assignmentResult = assign(object, source);
    const std::uint32_t assignmentHash = Fnv1a(object, sizeof(object));
    const int fullCopy = std::memcmp(object, source, sizeof(object)) == 0;

    std::printf(
        "window constructor_return=%d constructor_hash=%08lx assignment_return=%d "
        "assignment_hash=%08lx full_copy=%d\n",
        result == object,
        static_cast<unsigned long>(constructorHash),
        assignmentResult == object,
        static_cast<unsigned long>(assignmentHash),
        fullCopy);
    return 0;
}

struct DibObject {
    BITMAPINFOHEADER* info;
    RGBQUAD* palette;
    unsigned char* bitmap;
};

static int ProbeDib(HMODULE module)
{
    using Constructor = void* (__thiscall*)(void*);
    using Destructor = void (__thiscall*)(void*);
    using Create = int (__thiscall*)(void*, long, long, long, int);
    using FillByte = int (__thiscall*)(void*, unsigned char);
    using AddOffset = int (__thiscall*)(void*, RGBQUAD, int);
    using ClearUnusedArea = int (__thiscall*)(void*);
    using Convert = int (__thiscall*)(void*, void*, long);
    using Copy = int (__thiscall*)(void*, void*);

    const auto construct = LoadFunction<Constructor>(module, "??0RKC_DIB@@QAE@XZ");
    const auto destruct = LoadFunction<Destructor>(module, "??1RKC_DIB@@QAE@XZ");
    const auto create = LoadFunction<Create>(module, "?Create@RKC_DIB@@QAEHJJJH@Z");
    const auto fillByte = LoadFunction<FillByte>(module, "?FillByte@RKC_DIB@@QAEHE@Z");
    const auto addOffset = LoadFunction<AddOffset>(
        module, "?AddOffset@RKC_DIB@@QAEHUtagRGBQUAD@@H@Z");
    const auto clearUnused = LoadFunction<ClearUnusedArea>(
        module, "?ClearUnusedArea@RKC_DIB@@QAEHXZ");
    const auto convert = LoadFunction<Convert>(
        module, "?Convert@RKC_DIB@@QAEHPAV1@J@Z");
    const auto copy = LoadFunction<Copy>(module, "?Copy@RKC_DIB@@QAEHPAV1@@Z");

    DibObject source{};
    DibObject converted{};
    DibObject copied{};
    construct(&source);
    construct(&converted);
    construct(&copied);

    const int created = create(&source, 5, 3, 8, 1);
    for (int index = 0; index < 256; ++index) {
        source.palette[index].rgbBlue = static_cast<BYTE>(index);
        source.palette[index].rgbGreen = static_cast<BYTE>(255 - index);
        source.palette[index].rgbRed = static_cast<BYTE>((index * 3) & 0xff);
        source.palette[index].rgbReserved = 0;
    }
    fillByte(&source, 0xcc);
    const unsigned char pixels[15] = {
        0, 1, 2, 3, 4,
        5, 0, 7, 8, 9,
        10, 11, 12, 0, 14
    };
    const long stride = ((source.info->biWidth * source.info->biBitCount + 31) / 32) * 4;
    for (long row = 0; row < source.info->biHeight; ++row)
        std::memcpy(source.bitmap + row * stride, pixels + row * 5, 5);

    RGBQUAD offset{};
    offset.rgbReserved = 7;
    const int offsetResult = addOffset(&source, offset, 1);
    const int clearResult = clearUnused(&source);
    const std::uint32_t sourceHash =
        Fnv1a(source.bitmap, stride * source.info->biHeight);

    const int convertResult = convert(&converted, &source, 4);
    const long convertedStride =
        ((converted.info->biWidth * converted.info->biBitCount + 31) / 32) * 4;
    clearUnused(&converted);
    const std::uint32_t convertedHash =
        Fnv1a(converted.bitmap, convertedStride * converted.info->biHeight);

    const int copyResult = copy(&copied, &source);
    const std::uint32_t copyBitmapHash =
        Fnv1a(copied.bitmap, stride * copied.info->biHeight);
    const std::uint32_t copyPaletteHash =
        Fnv1a(copied.palette, 256 * sizeof(RGBQUAD));

    std::printf(
        "dib created=%d offset=%d clear=%d source=%08lx convert4=%d "
        "converted=%08lx converted_bytes=",
        created,
        offsetResult,
        clearResult,
        static_cast<unsigned long>(sourceHash),
        convertResult,
        static_cast<unsigned long>(convertedHash));
    for (long index = 0; index < convertedStride * converted.info->biHeight; ++index)
        std::printf("%02x", converted.bitmap[index]);
    std::printf(
        " copy=%d copy_bitmap=%08lx copy_palette=%08lx\n",
        copyResult,
        static_cast<unsigned long>(copyBitmapHash),
        static_cast<unsigned long>(copyPaletteHash));

    destruct(&copied);
    destruct(&converted);
    destruct(&source);
    return 0;
}

static int ProbeDibHighSpeed(HMODULE module)
{
    using Constructor = void* (__thiscall*)(void*);
    using Assignment = void* (__thiscall*)(void*, const void*);

    const auto construct = LoadFunction<Constructor>(
        module, "??0RKC_DIBHISPEEDMODE@@QAE@XZ");
    const auto assign = LoadFunction<Assignment>(
        module, "??4RKC_DIBHISPEEDMODE@@QAEAAV0@ABV0@@Z");

    constexpr std::size_t size = 0x468c0u * sizeof(std::uint32_t);
    auto* source = static_cast<std::uint8_t*>(
        VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    auto* destination = static_cast<std::uint8_t*>(
        VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!source || !destination)
        return 2;

    std::memset(source, 0xa5, size);
    const void* constructorResult = construct(source);
    const std::uint32_t constructorHash = Fnv1a(source, size);
    std::memset(destination, 0x5a, size);
    const void* assignmentResult = assign(destination, source);
    const std::uint32_t assignmentHash = Fnv1a(destination, size);
    const int fullCopy = std::memcmp(destination, source, size) == 0;

    std::printf(
        "dib_hispeed constructor_return=%d constructor_hash=%08lx "
        "assignment_return=%d assignment_hash=%08lx full_copy=%d\n",
        constructorResult == source,
        static_cast<unsigned long>(constructorHash),
        assignmentResult == destination,
        static_cast<unsigned long>(assignmentHash),
        fullCopy);

    VirtualFree(destination, 0, MEM_RELEASE);
    VirtualFree(source, 0, MEM_RELEASE);
    return 0;
}

static int ProbeTable(HMODULE module, const char* tablePath)
{
    using Constructor = void* (__thiscall*)(void*);
    using Destructor = void (__thiscall*)(void*);
    using ReadBinary = int (__thiscall*)(void*, char*);
    using GetCount = long (__thiscall*)(void*);
    using Get = void* (__thiscall*)(void*, long);
    using GetLong = long (__thiscall*)(void*);
    using GetValue = long (__thiscall*)(void*, long, long);
    using GetString = char* (__thiscall*)(void*, long, long);

    const auto construct = LoadFunction<Constructor>(
        module, "??0RKC_RPG_TABLE@@QAE@XZ");
    const auto destruct = LoadFunction<Destructor>(
        module, "??1RKC_RPG_TABLE@@QAE@XZ");
    const auto readBinary = LoadFunction<ReadBinary>(
        module, "?ReadBinaryFile@RKC_RPG_TABLE@@QAEHPAD@Z");
    const auto getCount = LoadFunction<GetCount>(
        module, "?GetCount@RKC_RPG_TABLE@@QAEJXZ");
    const auto get = LoadFunction<Get>(
        module, "?Get@RKC_RPG_TABLE@@QAEPAVRKC_RPG_TABLEDATA@@J@Z");
    const auto getTableNo = LoadFunction<GetLong>(
        module, "?GetTableNo@RKC_RPG_TABLEDATA@@QAEJXZ");
    const auto getRows = LoadFunction<GetLong>(
        module, "?GetRowCount@RKC_RPG_TABLEDATA@@QAEJXZ");
    const auto getColumns = LoadFunction<GetLong>(
        module, "?GetColCount@RKC_RPG_TABLEDATA@@QAEJXZ");
    const auto getValue = LoadFunction<GetValue>(
        module, "?GetValue@RKC_RPG_TABLEDATA@@QAEJJJ@Z");
    const auto getString = LoadFunction<GetString>(
        module, "?GetStrings@RKC_RPG_TABLEDATA@@QAEPADJJ@Z");

    std::uint8_t object[4]{};
    construct(object);
    char path[MAX_PATH * 4]{};
    std::strncpy(path, tablePath, sizeof(path) - 1);
    const int readResult = readBinary(object, path);
    const long count = getCount(object);
    std::vector<std::uint8_t> canonical;
    long totalCells = 0;
    long stringCells = 0;
    auto append = [&](const void* value, std::size_t size) {
        const auto* bytes = static_cast<const std::uint8_t*>(value);
        canonical.insert(canonical.end(), bytes, bytes + size);
    };
    for (long tableIndex = 0; tableIndex < count; ++tableIndex) {
        void* item = get(object, tableIndex);
        const long tableNo = getTableNo(item);
        const long rows = getRows(item);
        const long columns = getColumns(item);
        append(&tableNo, sizeof(tableNo));
        append(&rows, sizeof(rows));
        append(&columns, sizeof(columns));
        for (long row = 0; row < rows; ++row) {
            for (long column = 0; column < columns; ++column) {
                const long value = getValue(item, row, column);
                append(&value, sizeof(value));
                const char* text = getString(item, row, column);
                const std::uint32_t length =
                    text ? static_cast<std::uint32_t>(std::strlen(text)) : 0;
                append(&length, sizeof(length));
                if (text) {
                    append(text, length);
                    ++stringCells;
                }
                ++totalCells;
            }
        }
    }
    long invalidRow = -2;
    if (count > 0) {
        void* first = get(object, 0);
        invalidRow = getValue(first, getRows(first), 0);
    }
    std::printf(
        "table read=%d count=%ld cells=%ld strings=%ld hash=%08lx "
        "invalid_row=%ld\n",
        readResult,
        count,
        totalCells,
        stringCells,
        static_cast<unsigned long>(
            Fnv1a(canonical.data(), canonical.size())),
        invalidRow);
    destruct(object);
    return 0;
}

static int ProbeUpdIb(HMODULE module)
{
    using Constructor = void* (__thiscall*)(void*);
    using Destructor = void (__thiscall*)(void*);
    using Initialize = int (__thiscall*)(void*, long, long, long, int);
    using GetCount = long (__thiscall*)(void*);
    using GetIndex = void* (__thiscall*)(void*, long);
    using SetPacket = int (__thiscall*)(
        void*, long, long, long, long, long, long,
        long, long, long, long, long, long, long,
        short, short, short, RECT*, void*);
    using PacketConstructor = void* (__thiscall*)(void*);

    const auto construct = LoadFunction<Constructor>(
        module, "??0RKC_UPDIB@@QAE@XZ");
    const auto destruct = LoadFunction<Destructor>(
        module, "??1RKC_UPDIB@@QAE@XZ");
    const auto initialize = LoadFunction<Initialize>(
        module, "?Initialize@RKC_UPDIB@@QAEHJJJH@Z");
    const auto getUpdCount = LoadFunction<GetCount>(
        module, "?GetUpdCount@RKC_UPDIB@@QAEJXZ");
    const auto getBlockCount = LoadFunction<GetCount>(
        module, "?GetVSBlockCount@RKC_UPDIB@@QAEJXZ");
    const auto getBlock = LoadFunction<GetIndex>(
        module, "?GetVSBlock@RKC_UPDIB@@QAEPAVRKC_UPDIB_VSBLOCK@@J@Z");
    const auto getVsCount = LoadFunction<GetCount>(
        module, "?GetVSCount@RKC_UPDIB_VSBLOCK@@QAEJXZ");
    const auto getScreen = LoadFunction<GetIndex>(
        module, "?GetVScreen@RKC_UPDIB_VSBLOCK@@QAEPAVRKC_UPDIB_VS@@J@Z");
    const auto getPacketCount = LoadFunction<GetCount>(
        module, "?GetVSPacketCount@RKC_UPDIB_VS@@QAEJXZ");
    const auto getPacket = LoadFunction<GetIndex>(
        module, "?GetVSPacket@RKC_UPDIB_VS@@QAEPAVRKC_UPDIB_VSPACKET@@J@Z");
    const auto setPacket = LoadFunction<SetPacket>(
        module,
        "?SetPacket@RKC_UPDIB@@QAEHJJJJJJJJJJJJJFFFPAUtagRECT@@PAVRKC_DIB@@@Z");
    const auto constructPacket = LoadFunction<PacketConstructor>(
        module, "??0RKC_UPDIB_VSPACKET@@QAE@XZ");

    std::uint8_t packetConstructorBytes[0x54];
    std::memset(packetConstructorBytes, 0xa5, sizeof(packetConstructorBytes));
    void* packetConstructorResult = constructPacket(packetConstructorBytes);
    const std::uint32_t packetConstructorHash =
        Fnv1a(packetConstructorBytes, sizeof(packetConstructorBytes));

    std::uint8_t object[0x30];
    std::memset(object, 0xa5, sizeof(object));
    construct(object);
    const int initializeResult = initialize(object, 2, 3, 4, 0);
    const long updCount = getUpdCount(object);
    const long blockCount = getBlockCount(object);
    void* block = getBlock(object, 1);
    const long vsCount = block ? getVsCount(block) : -1;
    void* screen = block ? getScreen(block, 2) : nullptr;
    const long packetCountBefore = screen ? getPacketCount(screen) : -1;
    RECT clip{1, 2, 30, 40};
    const int setResult = setPacket(
        object,
        1, 2, 3, 4, 5, 0x400,
        6, 7, 8, 9, 10, 11, 12,
        13, 14, 15,
        &clip, nullptr);
    const long packetCountAfter = screen ? getPacketCount(screen) : -1;
    void* packet = screen ? getPacket(screen, 0) : nullptr;
    std::uint8_t canonicalPacket[0x54]{};
    if (packet) {
        std::memcpy(canonicalPacket, packet, sizeof(canonicalPacket));
        std::memset(canonicalPacket + 0x00, 0, 4);
        std::memset(canonicalPacket + 0x38, 0, 8);
        std::memset(canonicalPacket + 0x50, 0, 4);
    }
    const std::uint32_t packetHash =
        Fnv1a(canonicalPacket, sizeof(canonicalPacket));

    std::printf(
        "updib packet_ctor_return=%d packet_ctor_hash=%08lx initialize=%d "
        "upds=%ld blocks=%ld vs=%ld packet_before=%ld set=%d "
        "packet_after=%ld packet_hash=%08lx packet_bytes=",
        packetConstructorResult == packetConstructorBytes,
        static_cast<unsigned long>(packetConstructorHash),
        initializeResult,
        updCount,
        blockCount,
        vsCount,
        packetCountBefore,
        setResult,
        packetCountAfter,
        static_cast<unsigned long>(packetHash));
    for (std::size_t index = 0; index < sizeof(canonicalPacket); ++index)
        std::printf("%02x", canonicalPacket[index]);
    std::printf("\n");
    destruct(object);
    return 0;
}

static int ProbeDbf(HMODULE module)
{
    using Constructor = void* (__thiscall*)(void*);
    using Destructor = void (__thiscall*)(void*);
    using Assignment = void* (__thiscall*)(void*, const void*);
    using SetRectFunction = void (__thiscall*)(void*, RECT*);
    using GetRectFunction = void (__thiscall*)(void*, RECT*, long);
    using GetLongArgument = long (__thiscall*)(void*, long);
    using GetPosition = void (__thiscall*)(void*, POINT*, long);

    const auto constructDbf = LoadFunction<Constructor>(
        module, "??0RKC_DBF@@QAE@XZ");
    const auto destructDbf = LoadFunction<Destructor>(
        module, "??1RKC_DBF@@QAE@XZ");
    const auto assignDbf = LoadFunction<Assignment>(
        module, "??4RKC_DBF@@QAEAAV0@ABV0@@Z");
    const auto construct = LoadFunction<Constructor>(
        module, "??0RKC_DBFCONTROL@@QAE@XZ");
    const auto destruct = LoadFunction<Destructor>(
        module, "??1RKC_DBFCONTROL@@QAE@XZ");
    const auto assign = LoadFunction<Assignment>(
        module, "??4RKC_DBFCONTROL@@QAEAAV0@ABV0@@Z");
    const auto setClip = LoadFunction<SetRectFunction>(
        module, "?SetClipRect@RKC_DBFCONTROL@@QAEXPAUtagRECT@@@Z");
    const auto getClip = LoadFunction<GetRectFunction>(
        module, "?GetClipRect@RKC_DBFCONTROL@@QAEXPAUtagRECT@@J@Z");
    const auto getStyle = LoadFunction<GetLongArgument>(
        module, "?GetStyle@RKC_DBFCONTROL@@QAEJJ@Z");
    const auto getExStyle = LoadFunction<GetLongArgument>(
        module, "?GetExStyle@RKC_DBFCONTROL@@QAEJJ@Z");
    const auto getPosition = LoadFunction<GetPosition>(
        module, "?GetPosition@RKC_DBFCONTROL@@QAE?AUtagPOINT@@J@Z");

    std::uint8_t dbf[0x24];
    std::memset(dbf, 0xa5, sizeof(dbf));
    void* dbfResult = constructDbf(dbf);
    const std::uint32_t dbfConstructorHash = Fnv1a(dbf, sizeof(dbf));
    std::uint8_t dbfSource[0x24];
    for (std::size_t index = 0; index < sizeof(dbfSource); ++index)
        dbfSource[index] = static_cast<std::uint8_t>(index * 13 + 5);
    void* dbfAssignmentResult = assignDbf(dbf, dbfSource);
    const int dbfFullCopy =
        std::memcmp(dbf, dbfSource, sizeof(dbf)) == 0;
    // Avoid freeing the synthetic pointers copied by the assignment.
    constructDbf(dbf);
    destructDbf(dbf);

    std::uint8_t object[0x144];
    std::memset(object, 0xa5, sizeof(object));
    void* constructorResult = construct(object);
    std::uint8_t normalized[sizeof(object)];
    std::memcpy(normalized, object, sizeof(object));
    std::memset(normalized + 0x13c, 0, 4);
    const std::uint32_t constructorHash =
        Fnv1a(normalized, sizeof(normalized));

    RECT input{11, 22, 333, 444};
    RECT first{};
    RECT second{};
    setClip(object, &input);
    getClip(object, &first, 0);
    getClip(object, &second, 1);
    POINT position0{};
    POINT position1{};
    getPosition(object, &position0, 0);
    getPosition(object, &position1, 1);

    std::uint8_t source[0x144];
    for (std::size_t index = 0; index < sizeof(source); ++index)
        source[index] = static_cast<std::uint8_t>(index * 29 + 7);
    void* assignmentResult = assign(object, source);
    const int fullCopy = std::memcmp(object, source, sizeof(object)) == 0;
    // Restore a valid locally owned object before destruction.
    construct(object);

    std::printf(
        "dbf dbf_ctor_return=%d dbf_ctor_hash=%08lx dbf_assign_return=%d "
        "dbf_full_copy=%d ctor_return=%d ctor_hash=%08lx clips=%d "
        "style=%08lx,%08lx,%08lx exstyle=%08lx,%08lx,%08lx "
        "positions=%08lx:%08lx,%08lx:%08lx assign_return=%d full_copy=%d\n",
        dbfResult == dbf,
        static_cast<unsigned long>(dbfConstructorHash),
        dbfAssignmentResult == dbf,
        dbfFullCopy,
        constructorResult == object,
        static_cast<unsigned long>(constructorHash),
        std::memcmp(&first, &input, sizeof(input)) == 0
            && std::memcmp(&second, &input, sizeof(input)) == 0,
        static_cast<unsigned long>(getStyle(object, 0)),
        static_cast<unsigned long>(getStyle(object, 1)),
        static_cast<unsigned long>(getStyle(object, 2)),
        static_cast<unsigned long>(getExStyle(object, 0)),
        static_cast<unsigned long>(getExStyle(object, 1)),
        static_cast<unsigned long>(getExStyle(object, 2)),
        static_cast<unsigned long>(position0.x),
        static_cast<unsigned long>(position0.y),
        static_cast<unsigned long>(position1.x),
        static_cast<unsigned long>(position1.y),
        assignmentResult == object,
        fullCopy);
    destruct(object);
    return 0;
}

static int ProbeRkFunction(
    HMODULE module, const char* tablePath, const char* scratchPath)
{
    struct CompressionHeader {
        char magic[8];
        int uncompressedSize;
        int compressedSize;
    };
    using DecodeMemory = int (__cdecl*)(
        const void*, int, void**, void*);
    using EncodeMemory = int (__cdecl*)(
        const void*, int, void*, int, void*);
    using FileToMemory = int (__cdecl*)(const char*, void**, void*);
    using MemoryToFile = int (__cdecl*)(
        const void*, int, const char*, void*);

    const auto decodeMemory = LoadFunction<DecodeMemory>(
        module, "RK_LzDecodeMemoryToMemory");
    const auto encodeMemory = LoadFunction<EncodeMemory>(
        module, "RK_LzEncodeMemoryToMemory");
    const auto decodeFile = LoadFunction<FileToMemory>(
        module, "RK_LzDecodeFileToMemory");
    const auto decodeToFile = LoadFunction<MemoryToFile>(
        module, "RK_LzDecodeMemoryToFile");

    HANDLE input = CreateFileA(
        tablePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (input == INVALID_HANDLE_VALUE)
        return 2;
    const DWORD inputSize = GetFileSize(input, nullptr);
    std::vector<std::uint8_t> file(inputSize);
    DWORD read = 0;
    const bool fileRead = ReadFile(
        input, file.data(), inputSize, &read, nullptr) && read == inputSize;
    CloseHandle(input);
    if (!fileRead || file.size() <= 20)
        return 2;

    const void* encodedSource = file.data() + 20;
    const int encodedSourceSize = static_cast<int>(file.size() - 20);
    void* decoded = nullptr;
    CompressionHeader decodedHeader{};
    const int decodeResult = decodeMemory(
        encodedSource, encodedSourceSize, &decoded, &decodedHeader);
    const int decodedSize = decodedHeader.uncompressedSize;
    const std::uint32_t decodedHash =
        decoded ? Fnv1a(decoded, decodedSize) : 0;

    const int reencodedCapacity =
        32 + decodedSize + (decodedSize + 7) / 8;
    std::vector<std::uint8_t> reencoded(
        static_cast<std::size_t>(reencodedCapacity));
    CompressionHeader reencodedHeader{};
    const int encodeResult = decoded
        ? encodeMemory(
            decoded, decodedSize, reencoded.data(), reencodedCapacity,
            &reencodedHeader)
        : 0;
    const int reencodedSize = encodeResult == 1
        ? 16 + reencodedHeader.compressedSize
        : 0;
    const std::uint32_t reencodedHash =
        encodeResult == 1 ? Fnv1a(reencoded.data(), reencodedSize) : 0;

    HANDLE compressedFile = CreateFileA(
        scratchPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD written = 0;
    const int scratchWritten = compressedFile != INVALID_HANDLE_VALUE
        && WriteFile(
            compressedFile, encodedSource, encodedSourceSize,
            &written, nullptr)
        && written == static_cast<DWORD>(encodedSourceSize);
    if (compressedFile != INVALID_HANDLE_VALUE)
        CloseHandle(compressedFile);
    void* fileDecoded = nullptr;
    CompressionHeader fileDecodedHeader{};
    const int fileDecodeResult = scratchWritten
        ? decodeFile(scratchPath, &fileDecoded, &fileDecodedHeader) : 0;
    const int fileDecodedSize = fileDecodedHeader.uncompressedSize;
    const std::uint32_t fileDecodedHash =
        fileDecoded ? Fnv1a(fileDecoded, fileDecodedSize) : 0;

    char outputPath[MAX_PATH * 4]{};
    std::snprintf(outputPath, sizeof(outputPath), "%s.out", scratchPath);
    CompressionHeader fileOutputHeader{};
    const int memoryToFileResult = decodeToFile(
        encodedSource, encodedSourceSize, outputPath, &fileOutputHeader);
    HANDLE output = CreateFileA(
        outputPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD outputSize =
        output == INVALID_HANDLE_VALUE ? 0 : GetFileSize(output, nullptr);
    std::vector<std::uint8_t> outputBytes(outputSize);
    read = 0;
    const bool outputRead = output != INVALID_HANDLE_VALUE
        && ReadFile(
            output, outputBytes.data(), outputSize, &read, nullptr)
        && read == outputSize;
    if (output != INVALID_HANDLE_VALUE)
        CloseHandle(output);
    DeleteFileA(outputPath);
    DeleteFileA(scratchPath);

    std::printf(
        "rk_lz decode=%d decoded_size=%d decoded_hash=%08lx encode=%d "
        "encoded_size=%d encoded_hash=%08lx file_decode=%d "
        "file_size=%d file_hash=%08lx memory_file=%d output_size=%lu "
        "output_hash=%08lx\n",
        decodeResult,
        decodedSize,
        static_cast<unsigned long>(decodedHash),
        encodeResult,
        reencodedSize,
        static_cast<unsigned long>(reencodedHash),
        fileDecodeResult,
        fileDecodedSize,
        static_cast<unsigned long>(fileDecodedHash),
        memoryToFileResult,
        static_cast<unsigned long>(outputSize),
        static_cast<unsigned long>(
            outputRead ? Fnv1a(outputBytes.data(), outputBytes.size()) : 0));

    if (fileDecoded)
        GlobalFree(fileDecoded);
    if (decoded)
        GlobalFree(decoded);
    return 0;
}

static int ProbeRkUtilities(HMODULE module)
{
    using IntInt = int (__cdecl*)(int);
    using IntString = int (__cdecl*)(const char*);
    using StringIndex = int (__cdecl*)(const char*, int);
    using Compare = int (__cdecl*)(const char*, const char*, int);
    using Mutate = void (__cdecl*)(char*);
    using MutateMode = void (__cdecl*)(char*, int);
    using CopyNumber = void (__cdecl*)(const char*, char*, int);
    using Analyze = void (__cdecl*)(const char*, char*, char*);
    using Wildcard = int (__cdecl*)(const char*, const char*);
    using TimeCompare = int (__cdecl*)(const SYSTEMTIME*, const SYSTEMTIME*);

    const auto checkSjis = LoadFunction<IntInt>(module, "RK_CheckSJIS");
    const auto checkStringSjis =
        LoadFunction<StringIndex>(module, "RK_CheckStringSJIS");
    const auto checkLastRoot =
        LoadFunction<IntString>(module, "RK_CheckLastRoot");
    const auto stringsCompare =
        LoadFunction<Compare>(module, "RK_StringsCompare");
    const auto deleteWhitespace =
        LoadFunction<MutateMode>(module, "RK_DeleteTabSpaceString");
    const auto cutLastRoot = LoadFunction<Mutate>(module, "RK_CutLastRoot");
    const auto setLastRoot = LoadFunction<Mutate>(module, "RK_SetLastRoot");
    const auto cutFilename =
        LoadFunction<Mutate>(module, "RK_CutFilenameFromFullPath");
    const auto cutDirectory =
        LoadFunction<Mutate>(module, "RK_CutDirectoryFromFullPath");
    const auto copyNumber =
        LoadFunction<CopyNumber>(module, "RK_StringCopyNumber");
    const auto analyze = LoadFunction<Analyze>(module, "RK_AnalyzeFilename");
    const auto wildcard =
        LoadFunction<Wildcard>(module, "RK_FilenameCompareWildCard");
    const auto mesCheck = LoadFunction<IntString>(module, "RK_MesDefineCheck");
    const auto mesCut = LoadFunction<Mutate>(module, "RK_MesDefineCut");
    const auto mesSet = LoadFunction<Mutate>(module, "RK_MesDefineSet");
    const auto timeCompare =
        LoadFunction<TimeCompare>(module, "RK_SystemTimeCompare");

    char sjisResults[32]{};
    int sjisResultCount = 0;
    const int sjisInputs[] = {
        0x00, 0x7f, 0x80, 0x9f, 0xa0, 0xdf, 0xe0, 0xfc, 0xfd, 0xff
    };
    for (int input : sjisInputs) {
        const int result = checkSjis(input);
        sjisResults[sjisResultCount++] =
            static_cast<char>('0' + (result != 0));
    }
    const char multibyte[] = {
        'a', static_cast<char>(0x82), 'x', 'b', 0
    };
    for (int index = -1; index <= 4; ++index)
        sjisResults[sjisResultCount++] = static_cast<char>(
            '0' + (checkStringSjis(multibyte, index) != 0));

    char compareResults[128]{};
    int compareLength = 0;
    const char* comparisons[][2] = {
        {"", ""}, {"a", ""}, {"", "a"}, {"abc", "abc"},
        {"abc", "ABC"}, {"abc", "abd"}, {"abd", "abc"},
        {"abc", "abcd"}, {"abcd", "abc"}
    };
    for (const auto& comparison : comparisons) {
        for (int insensitive = 0; insensitive <= 1; ++insensitive) {
            const int result = stringsCompare(
                comparison[0], comparison[1], insensitive);
            compareLength += std::snprintf(
                compareResults + compareLength,
                sizeof(compareResults) - compareLength,
                "%d,", result);
        }
    }

    std::uint32_t rootCheckHash = 2166136261u;
    std::uint32_t cutRootHash = 2166136261u;
    std::uint32_t setRootHash = 2166136261u;
    const char* rootInputs[] = {"", "C:", "C:\\", "one\\two", "one\\two\\"};
    for (const char* input : rootInputs) {
        char buffer[128]{};
        std::strcpy(buffer, input);
        const int before = checkLastRoot(buffer);
        rootCheckHash = Fnv1a(&before, sizeof(before), rootCheckHash);
        cutLastRoot(buffer);
        cutRootHash = Fnv1a(
            buffer, std::strlen(buffer) + 1, cutRootHash);
        setLastRoot(buffer);
        setRootHash = Fnv1a(
            buffer, std::strlen(buffer) + 1, setRootHash);
    }
    char fullPath[128] = "C:\\one\\two.txt";
    cutFilename(fullPath);
    char directoryResult[128]{};
    std::strcpy(directoryResult, fullPath);
    std::strcpy(fullPath, "C:\\one\\two.txt");
    cutDirectory(fullPath);
    char filenameResult[128]{};
    std::strcpy(filenameResult, fullPath);

    std::uint32_t whitespaceHash = 2166136261u;
    const char* whitespaceInputs[] = {
        "  alpha  beta  ", "\t alpha\t", "plain"
    };
    for (const char* input : whitespaceInputs) {
        for (int mode = 0; mode <= 2; ++mode) {
            char buffer[128]{};
            std::strcpy(buffer, input);
            deleteWhitespace(buffer, mode);
            whitespaceHash = Fnv1a(
                buffer, std::strlen(buffer) + 1, whitespaceHash);
        }
    }
    std::uint32_t copyHash = 2166136261u;
    for (int length : {0, 1, 3, 8}) {
        char buffer[32];
        std::memset(buffer, 0xcc, sizeof(buffer));
        copyNumber("abcdef", buffer, length);
        copyHash = Fnv1a(
            buffer, static_cast<std::size_t>(length) + 1, copyHash);
    }
    std::uint32_t analyzeHash = 2166136261u;
    for (const char* input : {"file", "file.txt", ".profile", ".", "..", "a.b.c"}) {
        char name[64]{};
        char extension[64]{};
        analyze(input, name, extension);
        analyzeHash = Fnv1a(
            name, std::strlen(name) + 1, analyzeHash);
        analyzeHash = Fnv1a(
            extension, std::strlen(extension) + 1, analyzeHash);
    }
    char wildcardResults[32]{};
    int wildcardResultCount = 0;
    const char* wildcardInputs[][2] = {
        {"*.txt", "FILE.TXT"}, {"a?c", "abc"}, {"a*c", "abbbc"},
        {"*", "name.bin"}, {"*.*", "name.bin"}, {"*.txt", "name.bin"},
        {"abc", "abcd"}
    };
    for (const auto& input : wildcardInputs) {
        const int result = wildcard(input[0], input[1]);
        wildcardResults[wildcardResultCount++] =
            static_cast<char>('0' + (result != 0));
    }
    std::uint32_t mesHash = 2166136261u;
    for (const char* input : {"", "plain", "\"quoted\"", "\"open"}) {
        char buffer[64]{};
        std::strcpy(buffer, input);
        const int quoted = mesCheck(buffer);
        mesHash = Fnv1a(&quoted, sizeof(quoted), mesHash);
        mesCut(buffer);
        mesHash = Fnv1a(buffer, std::strlen(buffer) + 1, mesHash);
        std::strcpy(buffer, input);
        mesSet(buffer);
        mesHash = Fnv1a(buffer, std::strlen(buffer) + 1, mesHash);
    }

    SYSTEMTIME first{2020, 1, 3, 2, 4, 5, 6, 7};
    SYSTEMTIME second = first;
    int timeResults[3] = {
        timeCompare(&first, &second), 0, 0
    };
    second.wSecond = 8;
    timeResults[1] = timeCompare(&first, &second);
    timeResults[2] = timeCompare(&second, &first);
    const std::uint32_t timeHash = Fnv1a(timeResults, sizeof(timeResults));

    std::printf(
        "rk_utils sjis=%s compare=%s roots=%08lx,%08lx,%08lx "
        "path=%s|%s whitespace=%08lx copy=%08lx analyze=%08lx "
        "wildcard=%s mes=%08lx time=%08lx\n",
        sjisResults,
        compareResults,
        static_cast<unsigned long>(rootCheckHash),
        static_cast<unsigned long>(cutRootHash),
        static_cast<unsigned long>(setRootHash),
        directoryResult,
        filenameResult,
        static_cast<unsigned long>(whitespaceHash),
        static_cast<unsigned long>(copyHash),
        static_cast<unsigned long>(analyzeHash),
        wildcardResults,
        static_cast<unsigned long>(mesHash),
        static_cast<unsigned long>(timeHash));
    return 0;
}

static int ProbeFontMaker(HMODULE module, const char* scratchPath)
{
    using Constructor = void (__thiscall*)(void*);
    using Destructor = void (__thiscall*)(void*);
    using Assignment = void* (__thiscall*)(void*, const void*);
    using Create = int (__thiscall*)(void*, HDC);
    using DrawNormal = int (__thiscall*)(void*, HDC, unsigned char);
    using DrawDouble = int (__thiscall*)(void*, HDC, unsigned char*);
    using Save = int (__thiscall*)(void*, HDC, char*);

    const auto construct = LoadFunction<Constructor>(
        module, "??0RKC_FONTMAKER@@QAE@XZ");
    const auto destruct = LoadFunction<Destructor>(
        module, "??1RKC_FONTMAKER@@QAE@XZ");
    const auto assign = LoadFunction<Assignment>(
        module, "??4RKC_FONTMAKER@@QAEAAV0@ABV0@@Z");
    const auto create = LoadFunction<Create>(
        module, "?CreateDIB@RKC_FONTMAKER@@QAEHPAUHDC__@@@Z");
    const auto drawNormal = LoadFunction<DrawNormal>(
        module, "?DrawNormalFont@RKC_FONTMAKER@@QAEHPAUHDC__@@E@Z");
    const auto drawDouble = LoadFunction<DrawDouble>(
        module, "?DrawDoubleFont@RKC_FONTMAKER@@QAEHPAUHDC__@@PAE@Z");
    const auto save = LoadFunction<Save>(
        module, "?SaveNJPFile@RKC_FONTMAKER@@QAEHPAUHDC__@@PAD@Z");

    struct FontObject {
        int width;
        int height;
        HFONT font;
        BITMAPINFO* normalInfo;
        unsigned char* normalBits;
        HBITMAP normalBitmap;
        int normalStride;
        BITMAPINFO* doubleInfo;
        unsigned char* doubleBits;
        HBITMAP doubleBitmap;
        int doubleStride;
    };
    static_assert(sizeof(FontObject) == 0x2c, "font object ABI");

    alignas(4) unsigned char constructorBytes[sizeof(FontObject)];
    std::memset(constructorBytes, 0xa5, sizeof(constructorBytes));
    construct(constructorBytes);
    const std::uint32_t constructorHash =
        Fnv1a(constructorBytes, sizeof(constructorBytes));

    alignas(4) unsigned char assignmentSource[sizeof(FontObject)];
    for (std::size_t index = 0; index < sizeof(assignmentSource); ++index)
        assignmentSource[index] = static_cast<unsigned char>(index * 17 + 3);
    const void* assignmentResult = assign(
        constructorBytes, assignmentSource);
    const int assignmentCopy = std::memcmp(
        constructorBytes, assignmentSource, sizeof(constructorBytes)) == 0;

    // Restore a valid object before testing owned resources.
    construct(constructorBytes);
    auto* object = reinterpret_cast<FontObject*>(constructorBytes);
    object->width = 8;
    object->height = 16;
    HDC screen = GetDC(nullptr);
    object->font = CreateFontA(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FIXED_PITCH, "Fixedsys");
    const int created = create(object, screen);
    RGBQUAD palette[3]{};
    UINT paletteEntries = 0;
    if (created) {
        HDC memory = CreateCompatibleDC(screen);
        SelectObject(memory, object->normalBitmap);
        paletteEntries = GetDIBColorTable(memory, 0, 3, palette);
        DeleteDC(memory);
    }
    const int normalResult = created
        ? drawNormal(object, screen, static_cast<unsigned char>('A'))
        : 0;
    const std::uint32_t normalHash =
        normalResult
        ? Fnv1a(
            object->normalBits,
            static_cast<std::size_t>(object->normalStride) * object->height)
        : 0;
    unsigned char validDouble[2] = {0x82, 0xa0};
    unsigned char invalidDouble[2] = {'A', 'B'};
    const int validDoubleResult =
        created ? drawDouble(object, screen, validDouble) : 0;
    const std::uint32_t doubleHash =
        validDoubleResult
        ? Fnv1a(
            object->doubleBits,
            static_cast<std::size_t>(object->doubleStride) * object->height)
        : 0;
    const int invalidDoubleResult =
        created ? drawDouble(object, screen, invalidDouble) : 0;

    char output[MAX_PATH]{};
    std::strncpy(output, scratchPath, sizeof(output) - 1);
    DeleteFileA(output);
    const int saveResult = created ? save(object, screen, output) : 0;
    DWORD outputSize = 0;
    std::uint32_t outputHash = 2166136261u;
    HANDLE outputFile = CreateFileA(
        output, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (outputFile != INVALID_HANDLE_VALUE) {
        outputSize = GetFileSize(outputFile, nullptr);
        std::vector<std::uint8_t> bytes(outputSize);
        DWORD amount = 0;
        if (ReadFile(outputFile, bytes.data(), outputSize, &amount, nullptr) &&
            amount == outputSize)
            outputHash = Fnv1a(bytes.data(), bytes.size());
        CloseHandle(outputFile);
    }
    DeleteFileA(output);

    std::printf(
        "font ctor=%08lx assign_return=%d assign_copy=%d create=%d "
        "strides=%d,%d palette=%u:%02x%02x%02x,%02x%02x%02x,%02x%02x%02x "
        "normal=%d:%08lx double=%d:%08lx invalid=%d "
        "save=%d:%lu:%08lx\n",
        static_cast<unsigned long>(constructorHash),
        assignmentResult == constructorBytes,
        assignmentCopy,
        created,
        object->normalStride,
        object->doubleStride,
        static_cast<unsigned>(paletteEntries),
        palette[0].rgbRed, palette[0].rgbGreen, palette[0].rgbBlue,
        palette[1].rgbRed, palette[1].rgbGreen, palette[1].rgbBlue,
        palette[2].rgbRed, palette[2].rgbGreen, palette[2].rgbBlue,
        normalResult,
        static_cast<unsigned long>(normalHash),
        validDoubleResult,
        static_cast<unsigned long>(doubleHash),
        invalidDoubleResult,
        saveResult,
        static_cast<unsigned long>(outputSize),
        static_cast<unsigned long>(outputHash));
    destruct(object);
    if (screen)
        ReleaseDC(nullptr, screen);
    return 0;
}

static int ProbeAiControl(
    HMODULE module, const char* databasePath, const char* scratchPath)
{
    using Constructor = void* (__thiscall*)(void*);
    using Destructor = void (__thiscall*)(void*);
    using Read = int (__thiscall*)(void*, char*);
    using Write = int (__thiscall*)(void*, char*);
    using Get = void* (__thiscall*)(void*, long);
    using GetCount = long (__thiscall*)(void*);
    using GetNo = long (__thiscall*)(void*, void*);
    using GetFromName = void* (__thiscall*)(void*, char*);
    using GetName = char* (__thiscall*)(void*);
    using GetSpeed = long (__thiscall*)(void*);
    using GetDataCount = long (__thiscall*)(void*);
    using GetEventNumber = long (__thiscall*)(void*, long);
    using GetFlatData = void* (__thiscall*)(void*, long);
    using GetEvent = void* (__thiscall*)(void*, long);
    using GetEventData = void* (__thiscall*)(void*, long);
    using GetEventCount = long (__thiscall*)(void*);
    using GetLong = long (__thiscall*)(void*);
    using GetBytes = void* (__thiscall*)(void*);

    const auto construct = LoadFunction<Constructor>(
        module, "??0RKC_RPG_AICONTROL@@QAE@XZ");
    const auto destruct = LoadFunction<Destructor>(
        module, "??1RKC_RPG_AICONTROL@@QAE@XZ");
    const auto read = LoadFunction<Read>(
        module, "?ReadFile@RKC_RPG_AICONTROL@@QAEHPAD@Z");
    const auto write = LoadFunction<Write>(
        module, "?WriteFile@RKC_RPG_AICONTROL@@QAEHPAD@Z");
    const auto get = LoadFunction<Get>(
        module, "?Get@RKC_RPG_AICONTROL@@QAEPAVRKC_RPG_AILIST@@J@Z");
    const auto getCount = LoadFunction<GetCount>(
        module, "?GetCount@RKC_RPG_AICONTROL@@QAEJXZ");
    const auto getNo = LoadFunction<GetNo>(
        module, "?GetNo@RKC_RPG_AICONTROL@@QAEJPAVRKC_RPG_AILIST@@@Z");
    const auto getFromName = LoadFunction<GetFromName>(
        module, "?GetFromName@RKC_RPG_AICONTROL@@QAEPAVRKC_RPG_AILIST@@PAD@Z");
    const auto getName = LoadFunction<GetName>(
        module, "?GetName@RKC_RPG_AILIST@@QAEPADXZ");
    const auto getSpeed = LoadFunction<GetSpeed>(
        module, "?GetWalkPointSpeed@RKC_RPG_AILIST@@QAEJXZ");
    const auto getDataCount = LoadFunction<GetDataCount>(
        module, "?GetAIDataCount@RKC_RPG_AILIST@@QAEJXZ");
    const auto getEventNumber = LoadFunction<GetEventNumber>(
        module, "?GetEventNoFromAIDataNo@RKC_RPG_AILIST@@QAEJJ@Z");
    const auto getFlatData = LoadFunction<GetFlatData>(
        module, "?GetAIDataFromAIDataNo@RKC_RPG_AILIST@@QAEPAVRKC_RPG_AIDATA@@J@Z");
    const auto getEvent = LoadFunction<GetEvent>(
        module, "?Get@RKC_RPG_AILIST@@QAEPAVRKC_RPG_AIEVENT@@J@Z");
    const auto getEventData = LoadFunction<GetEventData>(
        module, "?Get@RKC_RPG_AIEVENT@@QAEPAVRKC_RPG_AIDATA@@J@Z");
    const auto getEventCount = LoadFunction<GetEventCount>(
        module, "?GetCount@RKC_RPG_AIEVENT@@QAEJXZ");
    const auto getAction = LoadFunction<GetLong>(
        module, "?GetActionNo@RKC_RPG_AIDATA@@QAEJXZ");
    const auto getDataEvent = LoadFunction<GetLong>(
        module, "?GetEventNo@RKC_RPG_AIDATA@@QAEJXZ");
    const auto getParameter = LoadFunction<GetBytes>(
        module, "?GetParameter@RKC_RPG_AIDATA@@QAEPAURKC_RPG_AIPARAM@@XZ");
    const auto getCondition = LoadFunction<GetBytes>(
        module, "?GetCondition@RKC_RPG_AIDATA@@QAEPAURKC_RPG_AICONDITION@@XZ");

    std::uint8_t object[4];
    std::memset(object, 0xa5, sizeof(object));
    const void* constructorResult = construct(object);
    const int constructorCleared =
        *reinterpret_cast<void**>(object) == nullptr;

    char input[MAX_PATH]{};
    char output[MAX_PATH]{};
    std::strncpy(input, databasePath, sizeof(input) - 1);
    std::strncpy(output, scratchPath, sizeof(output) - 1);
    DeleteFileA(output);
    const int readResult = read(object, input);
    const long listCount = getCount(object);

    std::uint32_t semanticHash = 2166136261u;
    long totalData = 0;
    int lookupResult = 0;
    int flattenResult = 1;
    for (long listNumber = 0; listNumber < listCount; ++listNumber) {
        void* list = get(object, listNumber);
        char* name = list ? getName(list) : nullptr;
        const long speed = list ? getSpeed(list) : -1;
        const long dataCount = list ? getDataCount(list) : -1;
        semanticHash = Fnv1a(&listNumber, sizeof(listNumber), semanticHash);
        if (name)
            semanticHash =
                Fnv1a(name, std::strlen(name) + 1, semanticHash);
        semanticHash = Fnv1a(&speed, sizeof(speed), semanticHash);
        semanticHash = Fnv1a(&dataCount, sizeof(dataCount), semanticHash);
        totalData += dataCount;

        if (listNumber == 0 && name) {
            lookupResult =
                getFromName(object, name) == list && getNo(object, list) == 0;
        }

        long flatNumber = 0;
        for (long eventNumber = 0; eventNumber < 18; ++eventNumber) {
            void* event = getEvent(list, eventNumber);
            const long eventDataCount = event ? getEventCount(event) : -1;
            semanticHash =
                Fnv1a(&eventDataCount, sizeof(eventDataCount), semanticHash);
            for (long dataNumber = 0; dataNumber < eventDataCount;
                 ++dataNumber, ++flatNumber) {
                void* data = getEventData(event, dataNumber);
                const long action = getAction(data);
                const long storedEvent = getDataEvent(data);
                semanticHash = Fnv1a(&action, sizeof(action), semanticHash);
                semanticHash =
                    Fnv1a(&storedEvent, sizeof(storedEvent), semanticHash);
                semanticHash =
                    Fnv1a(getParameter(data), 0x24, semanticHash);
                semanticHash =
                    Fnv1a(getCondition(data), 0x18, semanticHash);
                flattenResult =
                    flattenResult &&
                    getEventNumber(list, flatNumber) == eventNumber &&
                    getFlatData(list, flatNumber) == data;
            }
        }
        flattenResult = flattenResult && flatNumber == dataCount;
    }

    const int writeResult = write(object, output);
    HANDLE outputFile = CreateFileA(
        output, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD outputSize = 0;
    std::uint32_t outputHash = 2166136261u;
    if (outputFile != INVALID_HANDLE_VALUE) {
        outputSize = GetFileSize(outputFile, nullptr);
        std::vector<std::uint8_t> bytes(outputSize);
        DWORD bytesRead = 0;
        if (ReadFile(
                outputFile, bytes.data(), outputSize, &bytesRead, nullptr) &&
            bytesRead == outputSize)
            outputHash = Fnv1a(bytes.data(), bytes.size());
        CloseHandle(outputFile);
    }
    DeleteFileA(output);
    destruct(object);

    std::printf(
        "ai ctor_return=%d ctor_clear=%d read=%d lists=%ld data=%ld "
        "lookup=%d flatten=%d semantic=%08lx write=%d size=%lu file=%08lx\n",
        constructorResult == object,
        constructorCleared,
        readResult,
        listCount,
        totalData,
        lookupResult,
        flattenResult,
        static_cast<unsigned long>(semanticHash),
        writeResult,
        static_cast<unsigned long>(outputSize),
        static_cast<unsigned long>(outputHash));
    return 0;
}

static int ProbeScript(
    HMODULE module, const char* scriptPath, const char* scratchPath)
{
    using Constructor = void* (__thiscall*)(void*);
    using Destructor = void (__thiscall*)(void*);
    using ReadWrite = int (__thiscall*)(void*, char*);
    using GetPointer = void* (__thiscall*)(void*);
    using GetIndex = void* (__thiscall*)(void*, long);
    using GetPair = void* (__thiscall*)(void*, long, long);
    using GetCount = long (__thiscall*)(void*);
    using GetLong = long (__thiscall*)(void*);
    using GetOperand = void* (__thiscall*)(void*, long);
    using GetFlag = long (__thiscall*)(void*, long);

    const auto construct = LoadFunction<Constructor>(
        module, "??0RKC_RPG_SCRIPT@@QAE@XZ");
    const auto destruct = LoadFunction<Destructor>(
        module, "??1RKC_RPG_SCRIPT@@QAE@XZ");
    const auto read = LoadFunction<ReadWrite>(
        module, "?ReadBinary@RKC_RPG_SCRIPT@@QAEHPAD@Z");
    const auto write = LoadFunction<ReadWrite>(
        module, "?WriteBinary@RKC_RPG_SCRIPT@@QAEHPAD@Z");
    const auto getTempFlags = LoadFunction<GetPointer>(
        module, "?GetTempFlag@RKC_RPG_SCRIPT@@QAEPAURKC_RPG_SCRIPT_FLAG@@XZ");
    const auto getNetFlags = LoadFunction<GetPointer>(
        module, "?GetNetFlag@RKC_RPG_SCRIPT@@QAEPAURKC_RPG_SCRIPT_FLAG@@XZ");
    const auto getTempFlagCount = LoadFunction<GetCount>(
        module, "?GetTempFlagCount@RKC_RPG_SCRIPT@@QAEJXZ");
    const auto getNetFlagCount = LoadFunction<GetCount>(
        module, "?GetNetFlagCount@RKC_RPG_SCRIPT@@QAEJXZ");
    const auto getTempFlag = LoadFunction<GetFlag>(
        module, "?GetTempFlag@RKC_RPG_SCRIPT@@QAEJJ@Z");
    const auto getNetFlag = LoadFunction<GetFlag>(
        module, "?GetNetFlag@RKC_RPG_SCRIPT@@QAEJJ@Z");
    const auto getMessageBlock = LoadFunction<GetPointer>(
        module, "?GetMessageBlock@RKC_RPG_SCRIPT@@QAEPAVRKC_RPG_SCRIPT_MESSAGEBLOCK@@XZ");
    const auto getStatusBlock = LoadFunction<GetPointer>(
        module, "?GetStatusBlock@RKC_RPG_SCRIPT@@QAEPAVRKC_RPG_SCRIPT_STATUSBLOCK@@XZ");
    const auto getSentenceBlock = LoadFunction<GetPointer>(
        module, "?GetSentenceBlock@RKC_RPG_SCRIPT@@QAEPAVRKC_RPG_SCRIPT_SENTENCEBLOCK@@XZ");
    const auto blockCount = LoadFunction<GetCount>(
        module, "?GetCount@RKC_RPG_SCRIPT_MESSAGEBLOCK@@QAEJXZ");
    const auto messageGet = LoadFunction<GetIndex>(
        module, "?Get@RKC_RPG_SCRIPT_MESSAGEBLOCK@@QAEPAVRKC_RPG_SCRIPT_MESSAGE@@J@Z");
    const auto messageId = LoadFunction<GetLong>(
        module, "?GetID@RKC_RPG_SCRIPT_MESSAGE@@QAEJXZ");
    const auto messageData = LoadFunction<GetPointer>(
        module, "?GetData@RKC_RPG_SCRIPT_MESSAGE@@QAEPADXZ");
    const auto messageFromId = LoadFunction<GetIndex>(
        module, "?GetFromID@RKC_RPG_SCRIPT_MESSAGEBLOCK@@QAEPAVRKC_RPG_SCRIPT_MESSAGE@@J@Z");
    const auto statusCount = LoadFunction<GetCount>(
        module, "?GetCount@RKC_RPG_SCRIPT_STATUSBLOCK@@QAEJXZ");
    const auto statusGet = LoadFunction<GetIndex>(
        module, "?Get@RKC_RPG_SCRIPT_STATUSBLOCK@@QAEPAVRKC_RPG_SCRIPT_STATUS@@J@Z");
    const auto statusPair = LoadFunction<GetPair>(
        module, "?Get@RKC_RPG_SCRIPT_STATUSBLOCK@@QAEPAVRKC_RPG_SCRIPT_STATUS@@JJ@Z");
    const auto statusNetwork = LoadFunction<GetLong>(
        module, "?GetNetworkFlag@RKC_RPG_SCRIPT_STATUS@@QAEHXZ");
    const auto statusValue = LoadFunction<GetLong>(
        module, "?GetStatus@RKC_RPG_SCRIPT_STATUS@@QAEJXZ");
    const auto statusCharacter = LoadFunction<GetLong>(
        module, "?GetCharacterNo@RKC_RPG_SCRIPT_STATUS@@QAEJXZ");
    const auto statusSentence = LoadFunction<GetLong>(
        module, "?GetSentence@RKC_RPG_SCRIPT_STATUS@@QAEJXZ");
    const auto sentenceBlockCount = LoadFunction<GetCount>(
        module, "?GetCount@RKC_RPG_SCRIPT_SENTENCEBLOCK@@QAEJXZ");
    const auto sentenceBlockGet = LoadFunction<GetIndex>(
        module, "?Get@RKC_RPG_SCRIPT_SENTENCEBLOCK@@QAEPAVRKC_RPG_SCRIPT_SENTENCE@@J@Z");
    const auto sentenceCount = LoadFunction<GetCount>(
        module, "?GetCount@RKC_RPG_SCRIPT_SENTENCE@@QAEJXZ");
    const auto sentenceGet = LoadFunction<GetIndex>(
        module, "?Get@RKC_RPG_SCRIPT_SENTENCE@@QAEPAVRKC_RPG_SCRIPT_COMMAND@@J@Z");
    const auto commandOpcode = LoadFunction<GetLong>(
        module, "?GetOpecode@RKC_RPG_SCRIPT_COMMAND@@QAEJXZ");
    const auto commandOperandCount = LoadFunction<GetCount>(
        module, "?GetOperandCount@RKC_RPG_SCRIPT_COMMAND@@QAEJXZ");
    const auto commandOperand = LoadFunction<GetOperand>(
        module, "?GetOperand@RKC_RPG_SCRIPT_COMMAND@@QAEPAURKC_RPG_SCRIPT_COMMAND_OPERAND@@J@Z");

    std::uint8_t object[0x1c];
    std::memset(object, 0xa5, sizeof(object));
    const void* constructorResult = construct(object);
    char input[MAX_PATH]{};
    char output[MAX_PATH]{};
    std::strncpy(input, scriptPath, sizeof(input) - 1);
    std::strncpy(output, scratchPath, sizeof(output) - 1);
    DeleteFileA(output);
    const int readResult = read(object, input);

    struct ProbeFlag { long id, value, current; };
    const long tempCount = getTempFlagCount(object);
    const long netCount = getNetFlagCount(object);
    auto* tempFlags = static_cast<ProbeFlag*>(getTempFlags(object));
    auto* netFlags = static_cast<ProbeFlag*>(getNetFlags(object));
    std::uint32_t semanticHash = 2166136261u;
    int flagLookup = 1;
    for (long i = 0; i < tempCount; ++i) {
        semanticHash = Fnv1a(&tempFlags[i], sizeof(ProbeFlag), semanticHash);
        flagLookup =
            flagLookup && getTempFlag(object, tempFlags[i].id) == tempFlags[i].value;
    }
    for (long i = 0; i < netCount; ++i) {
        semanticHash = Fnv1a(&netFlags[i], sizeof(ProbeFlag), semanticHash);
        flagLookup =
            flagLookup && getNetFlag(object, netFlags[i].id) == netFlags[i].value;
    }

    void* messages = getMessageBlock(object);
    const long messageCount = blockCount(messages);
    int messageLookup = 1;
    for (long i = 0; i < messageCount; ++i) {
        void* message = messageGet(messages, i);
        const long id = messageId(message);
        const char* data = static_cast<const char*>(messageData(message));
        semanticHash = Fnv1a(&id, sizeof(id), semanticHash);
        if (data)
            semanticHash = Fnv1a(data, std::strlen(data) + 1, semanticHash);
        messageLookup = messageLookup && messageFromId(messages, id) == message;
    }

    void* statuses = getStatusBlock(object);
    const long numberOfStatuses = statusCount(statuses);
    int statusLookup = 1;
    for (long i = 0; i < numberOfStatuses; ++i) {
        void* status = statusGet(statuses, i);
        long values[4] = {
            statusNetwork(status), statusValue(status),
            statusCharacter(status), statusSentence(status)
        };
        semanticHash = Fnv1a(values, sizeof(values), semanticHash);
        statusLookup =
            statusLookup && statusPair(statuses, values[1], values[2]) != nullptr;
    }

    void* sentences = getSentenceBlock(object);
    const long numberOfSentences = sentenceBlockCount(sentences);
    long commandTotal = 0;
    long operandTotal = 0;
    for (long i = 0; i < numberOfSentences; ++i) {
        void* sentence = sentenceBlockGet(sentences, i);
        const long commandCount = sentenceCount(sentence);
        semanticHash = Fnv1a(&commandCount, sizeof(commandCount), semanticHash);
        commandTotal += commandCount;
        for (long commandNumber = 0; commandNumber < commandCount; ++commandNumber) {
            void* command = sentenceGet(sentence, commandNumber);
            const long opcode = commandOpcode(command);
            const long operandCount = commandOperandCount(command);
            semanticHash = Fnv1a(&opcode, sizeof(opcode), semanticHash);
            semanticHash = Fnv1a(&operandCount, sizeof(operandCount), semanticHash);
            operandTotal += operandCount;
            for (long operand = 0; operand < operandCount; ++operand) {
                semanticHash =
                    Fnv1a(commandOperand(command, operand), 8, semanticHash);
            }
        }
    }

    const int writeResult = write(object, output);
    HANDLE outputFile = CreateFileA(
        output, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD outputSize = 0;
    std::uint32_t outputHash = 2166136261u;
    if (outputFile != INVALID_HANDLE_VALUE) {
        outputSize = GetFileSize(outputFile, nullptr);
        std::vector<std::uint8_t> bytes(outputSize);
        DWORD amount = 0;
        if (ReadFile(outputFile, bytes.data(), outputSize, &amount, nullptr) &&
            amount == outputSize)
            outputHash = Fnv1a(bytes.data(), bytes.size());
        CloseHandle(outputFile);
    }
    DeleteFileA(output);
    destruct(object);

    std::printf(
        "script ctor_return=%d read=%d flags=%ld,%ld flag_lookup=%d "
        "messages=%ld message_lookup=%d statuses=%ld status_lookup=%d "
        "sentences=%ld commands=%ld operands=%ld semantic=%08lx "
        "write=%d size=%lu file=%08lx\n",
        constructorResult == object, readResult, tempCount, netCount, flagLookup,
        messageCount, messageLookup, numberOfStatuses, statusLookup,
        numberOfSentences, commandTotal, operandTotal,
        static_cast<unsigned long>(semanticHash), writeResult,
        static_cast<unsigned long>(outputSize),
        static_cast<unsigned long>(outputHash));
    return 0;
}

static int ProbeNetwork(HMODULE module)
{
    using Constructor = void* (__thiscall*)(void*);
    using Destructor = void (__thiscall*)(void*);
    using GetLong = long (__thiscall*)(void*);
    using GetPointer = void* (__thiscall*)(void*);
    using SetLong = void (__thiscall*)(void*, long);
    using SetString = void (__thiscall*)(void*, char*);
    using SetParam = void (__thiscall*)(
        void*, long, long, long, long, void*, long);
    using BlockGet = void* (__thiscall*)(void*, long);
    using BlockInsert = void* (__thiscall*)(void*, long);
    using BlockDeleteIndex = int (__thiscall*)(void*, long);
    using BlockDeletePointer = int (__thiscall*)(void*, void*);
    using BlockGetNo = long (__thiscall*)(void*, void*);
    using BlockGetCountLine = long (__thiscall*)(void*, long);
    using BlockRelease = void (__thiscall*)(void*);
    using UserFindName = void* (__thiscall*)(void*, char*);

    const auto packetConstruct = LoadFunction<Constructor>(
        module, "??0RKC_NETWORK_PACKET@@QAE@XZ");
    const auto packetDestruct = LoadFunction<Destructor>(
        module, "??1RKC_NETWORK_PACKET@@QAE@XZ");
    const auto packetSetParam = LoadFunction<SetParam>(
        module, "?SetParam@RKC_NETWORK_PACKET@@QAEXJJJJPAXJ@Z");
    const auto packetGetLine = LoadFunction<GetLong>(
        module, "?GetLine@RKC_NETWORK_PACKET@@QAEJXZ");
    const auto packetGetId = LoadFunction<GetLong>(
        module, "?GetID@RKC_NETWORK_PACKET@@QAEJXZ");
    const auto packetGetSize = LoadFunction<GetLong>(
        module, "?GetSize@RKC_NETWORK_PACKET@@QAEJXZ");
    const auto packetGetInfoId = LoadFunction<GetLong>(
        module, "?GetInfoID@RKC_NETWORK_PACKET@@QAEJXZ");
    const auto packetGetDisc = LoadFunction<GetLong>(
        module, "?GetDisc@RKC_NETWORK_PACKET@@QAEJXZ");
    const auto packetGetData = LoadFunction<GetPointer>(
        module, "?GetData@RKC_NETWORK_PACKET@@QAEPAXXZ");

    const auto blockConstruct = LoadFunction<Constructor>(
        module, "??0RKC_NETWORK_PACKETBLOCK@@QAE@XZ");
    const auto blockDestruct = LoadFunction<Destructor>(
        module, "??1RKC_NETWORK_PACKETBLOCK@@QAE@XZ");
    const auto blockGet = LoadFunction<BlockGet>(
        module, "?Get@RKC_NETWORK_PACKETBLOCK@@QAEPAVRKC_NETWORK_PACKET@@J@Z");
    const auto blockInsert = LoadFunction<BlockInsert>(
        module, "?Insert@RKC_NETWORK_PACKETBLOCK@@QAEPAVRKC_NETWORK_PACKET@@J@Z");
    const auto blockDeleteIndex = LoadFunction<BlockDeleteIndex>(
        module, "?Delete@RKC_NETWORK_PACKETBLOCK@@QAEHJ@Z");
    const auto blockDeletePointer = LoadFunction<BlockDeletePointer>(
        module, "?Delete@RKC_NETWORK_PACKETBLOCK@@QAEHPAVRKC_NETWORK_PACKET@@@Z");
    const auto blockGetCount = LoadFunction<GetLong>(
        module, "?GetCount@RKC_NETWORK_PACKETBLOCK@@QAEJXZ");
    const auto blockGetCountLine = LoadFunction<BlockGetCountLine>(
        module, "?GetCount@RKC_NETWORK_PACKETBLOCK@@QAEJJ@Z");
    const auto blockGetNo = LoadFunction<BlockGetNo>(
        module, "?GetNo@RKC_NETWORK_PACKETBLOCK@@QAEJPAVRKC_NETWORK_PACKET@@@Z");
    const auto blockRelease = LoadFunction<BlockRelease>(
        module, "?Release@RKC_NETWORK_PACKETBLOCK@@QAEXXZ");

    const auto userBlockConstruct = LoadFunction<Constructor>(
        module, "??0RKC_NETWORK_USERINFOBLOCK@@QAE@XZ");
    const auto userBlockDestruct = LoadFunction<Destructor>(
        module, "??1RKC_NETWORK_USERINFOBLOCK@@QAE@XZ");
    const auto userAppend = LoadFunction<GetPointer>(
        module, "?Append@RKC_NETWORK_USERINFOBLOCK@@QAEPAVRKC_NETWORK_USERINFO@@XZ");
    const auto userBlockGet = LoadFunction<BlockGet>(
        module, "?Get@RKC_NETWORK_USERINFOBLOCK@@QAEPAVRKC_NETWORK_USERINFO@@J@Z");
    const auto userBlockCount = LoadFunction<GetLong>(
        module, "?GetCount@RKC_NETWORK_USERINFOBLOCK@@QAEJXZ");
    const auto userFromId = LoadFunction<BlockGet>(
        module, "?GetFromID@RKC_NETWORK_USERINFOBLOCK@@QAEPAVRKC_NETWORK_USERINFO@@J@Z");
    const auto userFromName = LoadFunction<UserFindName>(
        module, "?GetFromName@RKC_NETWORK_USERINFOBLOCK@@QAEPAVRKC_NETWORK_USERINFO@@PAD@Z");
    const auto userDelete = LoadFunction<BlockDeleteIndex>(
        module, "?Delete@RKC_NETWORK_USERINFOBLOCK@@QAEHJ@Z");
    const auto userSetId = LoadFunction<SetLong>(
        module, "?SetID@RKC_NETWORK_USERINFO@@QAEXJ@Z");
    const auto userSetName = LoadFunction<SetString>(
        module, "?SetUserName@RKC_NETWORK_USERINFO@@QAEXPAD@Z");
    const auto userSetPassword = LoadFunction<SetString>(
        module, "?SetPassword@RKC_NETWORK_USERINFO@@QAEXPAD@Z");
    const auto userGetId = LoadFunction<GetLong>(
        module, "?GetID@RKC_NETWORK_USERINFO@@QAEJXZ");
    const auto userGetName = LoadFunction<GetPointer>(
        module, "?GetUserNameA@RKC_NETWORK_USERINFO@@QAEPADXZ");
    const auto userGetPassword = LoadFunction<GetPointer>(
        module, "?GetPassword@RKC_NETWORK_USERINFO@@QAEPADXZ");

    std::uint8_t packet[0x1c];
    std::memset(packet, 0xa5, sizeof(packet));
    void* packetConstructorResult = packetConstruct(packet);
    const std::uint32_t packetConstructorHash = Fnv1a(packet, sizeof(packet));
    char payload[] = {1, 3, 5, 7, 9};
    packetSetParam(packet, 12, 34, sizeof(payload), 56, payload, 78);
    const int packetFields =
        packetGetLine(packet) == 12
        && packetGetId(packet) == 34
        && packetGetSize(packet) == sizeof(payload)
        && packetGetInfoId(packet) == 56
        && packetGetDisc(packet) == 78;
    const int packetCopy =
        packetGetData(packet) != payload
        && std::memcmp(packetGetData(packet), payload, sizeof(payload)) == 0;
    packetDestruct(packet);

    void* packetBlock = nullptr;
    blockConstruct(&packetBlock);
    void* first = blockInsert(&packetBlock, 0);
    void* second = blockInsert(&packetBlock, 1);
    void* middle = blockInsert(&packetBlock, 1);
    packetSetParam(first, 4, 1, 0, 0, nullptr, 0);
    packetSetParam(middle, 8, 2, 0, 0, nullptr, 0);
    packetSetParam(second, 4, 3, 0, 0, nullptr, 0);
    const long packetCount = blockGetCount(&packetBlock);
    const long lineCount = blockGetCountLine(&packetBlock, 4);
    const long middleNo = blockGetNo(&packetBlock, middle);
    const int order =
        blockGet(&packetBlock, 0) == first
        && blockGet(&packetBlock, 1) == middle
        && blockGet(&packetBlock, 2) == second;
    const int deletePointer = blockDeletePointer(&packetBlock, middle);
    const int deleteIndex = blockDeleteIndex(&packetBlock, 1);
    const long packetCountAfter = blockGetCount(&packetBlock);
    blockRelease(&packetBlock);
    const int packetBlockReleased = packetBlock == nullptr;
    blockDestruct(&packetBlock);

    void* userBlock = nullptr;
    userBlockConstruct(&userBlock);
    void* alice = userAppend(&userBlock);
    void* bob = userAppend(&userBlock);
    char aliceName[] = "Alice";
    char bobName[] = "Bob";
    char secret[] = "swordfish";
    userSetId(alice, 101);
    userSetName(alice, aliceName);
    userSetPassword(alice, secret);
    userSetId(bob, 202);
    userSetName(bob, bobName);
    const int userValues =
        userGetId(alice) == 101
        && std::strcmp(static_cast<char*>(userGetName(alice)), aliceName) == 0
        && std::strcmp(static_cast<char*>(userGetPassword(alice)), secret) == 0;
    const int userLookup =
        userFromId(&userBlock, 202) == bob
        && userFromName(&userBlock, aliceName) == alice
        && userBlockGet(&userBlock, 1) == bob;
    const long userCount = userBlockCount(&userBlock);
    const int userDeleteResult = userDelete(&userBlock, 0);
    const long userCountAfter = userBlockCount(&userBlock);
    userBlockDestruct(&userBlock);
    const int userBlockReleased = userBlock == nullptr;

    std::printf(
        "network packet_ctor_return=%d packet_ctor_hash=%08lx "
        "packet_fields=%d packet_copy=%d packets=%ld line4=%ld middle_no=%ld "
        "order=%d delete=%d,%d packets_after=%ld released=%d "
        "users=%ld user_values=%d user_lookup=%d user_delete=%d "
        "users_after=%ld user_released=%d\n",
        packetConstructorResult == packet,
        static_cast<unsigned long>(packetConstructorHash),
        packetFields, packetCopy, packetCount, lineCount, middleNo, order,
        deletePointer, deleteIndex, packetCountAfter, packetBlockReleased,
        userCount, userValues, userLookup, userDeleteResult, userCountAfter,
        userBlockReleased);
    return 0;
}

static int ProbeRpgScreen(HMODULE module)
{
    using Constructor = void* (__thiscall*)(void*);
    using GetLong = long (__thiscall*)(void*);
    using GetShort = short (__thiscall*)(void*);
    using GetPointer = void* (__thiscall*)(void*);
    using SetLong = void (__thiscall*)(void*, long);
    using SetShort = void (__thiscall*)(void*, short);
    using SetPointer = void (__thiscall*)(void*, void*);
    using SetBase = void (__thiscall*)(void*, long, long, long);
    using GetBase = void (__thiscall*)(void*, long*, long*, long*);
    using SetPair = void (__thiscall*)(void*, long, long);
    using GetPair = void (__thiscall*)(void*, long*, long*);
    using ConvertPosition = void (__thiscall*)(
        void*, long, long, long*, long*);

    const auto objectConstruct = LoadFunction<Constructor>(
        module, "??0RKC_RPGSCRN_OBJECT@@QAE@XZ");
    const auto getX = LoadFunction<GetLong>(
        module, "?GetX@RKC_RPGSCRN_OBJECT@@QAEJXZ");
    const auto getY = LoadFunction<GetLong>(
        module, "?GetY@RKC_RPGSCRN_OBJECT@@QAEJXZ");
    const auto getCharacter = LoadFunction<GetLong>(
        module, "?GetCharacterNo@RKC_RPGSCRN_OBJECT@@QAEJXZ");
    const auto getPattern = LoadFunction<GetLong>(
        module, "?GetPatternNo@RKC_RPGSCRN_OBJECT@@QAEJXZ");
    const auto getUpd = LoadFunction<GetShort>(
        module, "?GetUpdNo@RKC_RPGSCRN_OBJECT@@QAEFXZ");
    const auto getPalette = LoadFunction<GetShort>(
        module, "?GetPaletteNo@RKC_RPGSCRN_OBJECT@@QAEFXZ");
    const auto getTrans = LoadFunction<GetShort>(
        module, "?GetTrans@RKC_RPGSCRN_OBJECT@@QAEFXZ");
    const auto getStatus = LoadFunction<GetShort>(
        module, "?GetStatus@RKC_RPGSCRN_OBJECT@@QAEFXZ");
    const auto getHeight = LoadFunction<GetShort>(
        module, "?GetHeight@RKC_RPGSCRN_OBJECT@@QAEFXZ");
    const auto getRed = LoadFunction<GetShort>(
        module, "?GetRStrong@RKC_RPGSCRN_OBJECT@@QAEFXZ");
    const auto getGreen = LoadFunction<GetShort>(
        module, "?GetGStrong@RKC_RPGSCRN_OBJECT@@QAEFXZ");
    const auto getBlue = LoadFunction<GetShort>(
        module, "?GetBStrong@RKC_RPGSCRN_OBJECT@@QAEFXZ");
    const auto setCharacter = LoadFunction<SetLong>(
        module, "?SetCharacterNo@RKC_RPGSCRN_OBJECT@@QAEXJ@Z");
    const auto setPattern = LoadFunction<SetLong>(
        module, "?SetPatternNo@RKC_RPGSCRN_OBJECT@@QAEXJ@Z");
    const auto setUpd = LoadFunction<SetShort>(
        module, "?SetUpdNo@RKC_RPGSCRN_OBJECT@@QAEXF@Z");
    const auto setPalette = LoadFunction<SetShort>(
        module, "?SetPaletteNo@RKC_RPGSCRN_OBJECT@@QAEXF@Z");
    const auto setTrans = LoadFunction<SetShort>(
        module, "?SetTrans@RKC_RPGSCRN_OBJECT@@QAEXF@Z");
    const auto setStatus = LoadFunction<SetShort>(
        module, "?SetStatus@RKC_RPGSCRN_OBJECT@@QAEXF@Z");
    const auto setHeight = LoadFunction<SetShort>(
        module, "?SetHeight@RKC_RPGSCRN_OBJECT@@QAEXF@Z");
    const auto setRed = LoadFunction<SetShort>(
        module, "?SetRStrong@RKC_RPGSCRN_OBJECT@@QAEXF@Z");
    const auto setGreen = LoadFunction<SetShort>(
        module, "?SetGStrong@RKC_RPGSCRN_OBJECT@@QAEXF@Z");
    const auto setBlue = LoadFunction<SetShort>(
        module, "?SetBStrong@RKC_RPGSCRN_OBJECT@@QAEXF@Z");

    std::uint8_t object[0x40];
    std::memset(object, 0xa5, sizeof(object));
    const void* objectResult = objectConstruct(object);
    const std::uint32_t objectHash = Fnv1a(object, sizeof(object));
    *reinterpret_cast<long*>(object + 4) = 111;
    *reinterpret_cast<long*>(object + 8) = -222;
    setCharacter(object, 333);
    setPattern(object, 444);
    setUpd(object, 5);
    setPalette(object, 6);
    setTrans(object, 700);
    setStatus(object, 8);
    setHeight(object, 9);
    setRed(object, 101);
    setGreen(object, 202);
    setBlue(object, 303);
    const int objectValues =
        getX(object) == 111 && getY(object) == -222
        && getCharacter(object) == 333 && getPattern(object) == 444
        && getUpd(object) == 5 && getPalette(object) == 6
        && getTrans(object) == 700 && getStatus(object) == 8
        && getHeight(object) == 9 && getRed(object) == 101
        && getGreen(object) == 202 && getBlue(object) == 303;

    const auto displayConstruct = LoadFunction<Constructor>(
        module, "??0RKC_RPGSCRN_OBJECTDISPCELL@@QAE@XZ");
    const auto displaySet = LoadFunction<SetPointer>(
        module, "?Set@RKC_RPGSCRN_OBJECTDISPCELL@@QAEXPAVRKC_RPGSCRN_OBJECT@@@Z");
    const auto displayGet = LoadFunction<GetPointer>(
        module, "?Get@RKC_RPGSCRN_OBJECTDISPCELL@@QAEPAVRKC_RPGSCRN_OBJECT@@XZ");
    const auto displaySetStatus = LoadFunction<SetShort>(
        module, "?SetStatus@RKC_RPGSCRN_OBJECTDISPCELL@@QAEXF@Z");
    const auto displaySetTrans = LoadFunction<SetShort>(
        module, "?SetTrans@RKC_RPGSCRN_OBJECTDISPCELL@@QAEXF@Z");
    const auto displayGetStatus = LoadFunction<GetShort>(
        module, "?GetStatus@RKC_RPGSCRN_OBJECTDISPCELL@@QAEFXZ");
    const auto displayGetTrans = LoadFunction<GetShort>(
        module, "?GetTrans@RKC_RPGSCRN_OBJECTDISPCELL@@QAEFXZ");
    std::uint8_t display[0x14];
    std::memset(display, 0xa5, sizeof(display));
    displayConstruct(display);
    const std::uint32_t displayHash = Fnv1a(display, sizeof(display));
    displaySet(display, object);
    displaySetStatus(display, 12);
    displaySetTrans(display, 345);
    const int displayValues =
        displayGet(display) == object && displayGetStatus(display) == 12
        && displayGetTrans(display) == 345;

    const auto cellConstruct = LoadFunction<Constructor>(
        module, "??0RKC_RPGSCRN_CHARANIMCELL@@QAE@XZ");
    const auto cellSetPattern = LoadFunction<SetLong>(
        module, "?SetPatternNo@RKC_RPGSCRN_CHARANIMCELL@@QAEXJ@Z");
    const auto cellSetStatus = LoadFunction<SetShort>(
        module, "?SetStatus@RKC_RPGSCRN_CHARANIMCELL@@QAEXF@Z");
    const auto cellSetPriority = LoadFunction<SetShort>(
        module, "?SetPriority@RKC_RPGSCRN_CHARANIMCELL@@QAEXF@Z");
    const auto cellSetTrans = LoadFunction<SetShort>(
        module, "?SetTrans@RKC_RPGSCRN_CHARANIMCELL@@QAEXF@Z");
    std::uint8_t cell[0xc];
    std::memset(cell, 0xa5, sizeof(cell));
    cellConstruct(cell);
    const std::uint32_t cellHash = Fnv1a(cell, sizeof(cell));
    cellSetStatus(cell, 2);
    cellSetPriority(cell, 3);
    cellSetTrans(cell, 4);
    cellSetPattern(cell, 500);
    const int cellValues =
        *reinterpret_cast<short*>(cell) == 2
        && *reinterpret_cast<short*>(cell + 2) == 3
        && *reinterpret_cast<short*>(cell + 4) == 4
        && *reinterpret_cast<long*>(cell + 8) == 500;

    const auto groundConstruct = LoadFunction<Constructor>(
        module, "??0RKC_RPGSCRN_GROUNDBLOCK@@QAE@XZ");
    const auto setBaseMag = LoadFunction<SetPair>(
        module, "?SetBaseMag@RKC_RPGSCRN_GROUNDBLOCK@@QAEXJJ@Z");
    const auto getBaseMag = LoadFunction<GetPair>(
        module, "?GetBaseMag@RKC_RPGSCRN_GROUNDBLOCK@@QAEXPAJ0@Z");
    const auto setJudgeOffset = LoadFunction<SetPair>(
        module, "?SetJudgeOffset@RKC_RPGSCRN_GROUNDBLOCK@@QAEXJJ@Z");
    const auto getJudgeOffset = LoadFunction<GetPair>(
        module, "?GetJudgeOffset@RKC_RPGSCRN_GROUNDBLOCK@@QAEXPAJ0@Z");
    std::uint8_t ground[0x3c];
    std::memset(ground, 0xa5, sizeof(ground));
    groundConstruct(ground);
    const std::uint32_t groundHash = Fnv1a(ground, sizeof(ground));
    long first = 0;
    long second = 0;
    setBaseMag(ground, 17, 19);
    getBaseMag(ground, &first, &second);
    const int baseMag = first == 17 && second == 19;
    setJudgeOffset(ground, -7, 23);
    getJudgeOffset(ground, &first, &second);
    const int judgeOffset = first == -7 && second == 23;

    const auto setBase = LoadFunction<SetBase>(
        module, "?SetBaseParam@RKC_RPGSCRN@@QAEXJJJ@Z");
    const auto getBase = LoadFunction<GetBase>(
        module, "?GetBaseParam@RKC_RPGSCRN@@QAEXPAJ00@Z");
    const auto calcReal = LoadFunction<ConvertPosition>(
        module, "?CalcRealPos@RKC_RPGSCRN@@QAEXJJPAJ0@Z");
    const auto calcWorld = LoadFunction<ConvertPosition>(
        module, "?CalcWorldPos@RKC_RPGSCRN@@QAEXJJPAJ0@Z");
    std::uint8_t screen[0x2c]{};
    setBase(screen, 15, 10, 20);
    long baseA = 0;
    long baseB = 0;
    long baseC = 0;
    getBase(screen, &baseA, &baseB, &baseC);
    long realX = 0;
    long realY = 0;
    long worldX = 0;
    long worldY = 0;
    calcReal(screen, -123, 45, &realX, &realY);
    calcWorld(screen, realX, realY, &worldX, &worldY);

    std::printf(
        "rpgscrn object_return=%d object_hash=%08lx object_values=%d "
        "display_hash=%08lx display_values=%d cell_hash=%08lx cell_values=%d "
        "ground_hash=%08lx base_mag=%d judge_offset=%d "
        "base=%ld,%ld,%ld positions=%ld,%ld:%ld,%ld\n",
        objectResult == object, static_cast<unsigned long>(objectHash),
        objectValues, static_cast<unsigned long>(displayHash), displayValues,
        static_cast<unsigned long>(cellHash), cellValues,
        static_cast<unsigned long>(groundHash), baseMag, judgeOffset,
        baseA, baseB, baseC, realX, realY, worldX, worldY);
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::fprintf(
            stderr,
            "usage: foundation_probe.exe <dll> <file|memory|window|dib|dib_hispeed|table|updib|dbf|rk_lz> [input]\n");
        return 2;
    }

    HMODULE module = LoadLibraryA(argv[1]);
    if (!module) {
        std::fprintf(stderr, "unable to load %s (error %lu)\n", argv[1], GetLastError());
        return 2;
    }

    int result = 2;
    if (std::strcmp(argv[2], "file") == 0 && argc == 4)
        result = ProbeFile(module, argv[3]);
    else if (std::strcmp(argv[2], "memory") == 0)
        result = ProbeMemory(module);
    else if (std::strcmp(argv[2], "window") == 0)
        result = ProbeWindow(module);
    else if (std::strcmp(argv[2], "dib") == 0)
        result = ProbeDib(module);
    else if (std::strcmp(argv[2], "dib_hispeed") == 0)
        result = ProbeDibHighSpeed(module);
    else if (std::strcmp(argv[2], "table") == 0 && argc == 4)
        result = ProbeTable(module, argv[3]);
    else if (std::strcmp(argv[2], "updib") == 0)
        result = ProbeUpdIb(module);
    else if (std::strcmp(argv[2], "dbf") == 0)
        result = ProbeDbf(module);
    else if (std::strcmp(argv[2], "rk_lz") == 0 && argc == 5)
        result = ProbeRkFunction(module, argv[3], argv[4]);
    else if (std::strcmp(argv[2], "rk_utils") == 0)
        result = ProbeRkUtilities(module);
    else if (std::strcmp(argv[2], "font") == 0 && argc == 4)
        result = ProbeFontMaker(module, argv[3]);
    else if (std::strcmp(argv[2], "ai") == 0 && argc == 5)
        result = ProbeAiControl(module, argv[3], argv[4]);
    else if (std::strcmp(argv[2], "script") == 0 && argc == 5)
        result = ProbeScript(module, argv[3], argv[4]);
    else if (std::strcmp(argv[2], "network") == 0)
        result = ProbeNetwork(module);
    else if (std::strcmp(argv[2], "rpgscrn") == 0)
        result = ProbeRpgScreen(module);
    else
        std::fprintf(stderr, "invalid probe arguments\n");

    FreeLibrary(module);
    return result;
}
