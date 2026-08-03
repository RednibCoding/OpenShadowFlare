#define _GNU_SOURCE
#define NEWLIB_PORT_AWARE

#include "ps2_data_backend.hpp"

#include <kernel.h>
#include <loadfile.h>
#include <ps2sdkapi.h>
#include <sifrpc.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fileio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace osf {
namespace runtime {
namespace platform {
namespace ps2 {
namespace {

constexpr char kArchiveName[] = "cdrom0:\\SFGAME.BIN";
constexpr char kArchiveDataRoot[] = "cdrom0:\\ShadowFlare";
constexpr char kHostDataRoot[] = "host0:ShadowFlare";
constexpr char kHostConfigPath[] = "host0:ShadowFlare/SFlare.Cfg";
constexpr char kLegacyHostDataRoot[] = "host:ShadowFlare";
constexpr char kLegacyHostConfigPath[] = "host:ShadowFlare/SFlare.Cfg";
constexpr char kMassDataRoot[] = "mass:/ShadowFlare";
constexpr char kMassConfigPath[] = "mass:/ShadowFlare/SFlare.Cfg";
constexpr char kUsbDriverPath[] = "cdrom0:\\USBD.IRX;1";
constexpr char kUsbMassDriverPath[] = "cdrom0:\\USBHDFSD.IRX;1";
constexpr char kDataRootComponent[] = "ShadowFlare";
constexpr std::uint32_t kMagic = 0x53464231u;
constexpr std::uint32_t kVersion = 1u;
constexpr std::uint32_t kHeaderSize = 16u;
constexpr std::uint32_t kEntrySize = 16u;

constexpr int kReadOnlyMode = S_IRUSR | S_IRGRP | S_IROTH;

struct DataEntry {
    const char* name;
    std::uint32_t name_size;
    std::uint32_t data_offset;
    std::uint32_t data_size;
};

struct FileHandle {
    int archive_fd;
    std::uint32_t base;
    std::uint32_t size;
    std::uint32_t position;
    char* filename;
};

struct DirHandle {
    char* children;
    std::uint32_t* child_offsets;
    std::uint32_t count;
    std::uint32_t position;
    char* relative;
    char* filename;
};

bool s_initialized = false;
const char* s_data_root = kArchiveDataRoot;
DataEntry* s_entries = nullptr;
std::uint32_t s_entry_count = 0;
_libcglue_fdman_path_ops_t* s_default_path_ops = nullptr;
_libcglue_fdman_fd_ops_t s_file_ops{};
_libcglue_fdman_fd_ops_t s_dir_ops{};
_libcglue_fdman_path_ops_t s_path_ops{};

char asciiLower(char value) {
    return value >= 'A' && value <= 'Z'
               ? static_cast<char>(value - 'A' + 'a')
               : value;
}

bool nameEquals(
    const char* first,
    std::size_t first_size,
    const char* second,
    std::size_t second_size) {
    if (first_size != second_size) {
        return false;
    }
    for (std::size_t index = 0; index < first_size; ++index) {
        if (asciiLower(first[index]) != asciiLower(second[index])) {
            return false;
        }
    }
    return true;
}

bool nameStartsWith(const char* name, const char* prefix, std::size_t prefix_size) {
    return std::strncmp(name, prefix, prefix_size) == 0;
}

int normalizeRelative(const char* absolute, char* out, std::size_t out_size) {
    const char* cursor = absolute;

    const char* colon = std::strchr(cursor, ':');
    if (colon) {
        if (std::strncmp(cursor, "cdrom", 5) != 0) {
            return -1;
        }
        cursor = colon + 1;
    }

    while (*cursor == '/' || *cursor == '\\') {
        ++cursor;
    }

    std::size_t component = 0;
    while (cursor[component] && cursor[component] != '/' &&
           cursor[component] != '\\') {
        ++component;
    }
    if (component != 0) {
        if (component != std::strlen(kDataRootComponent) ||
            !nameEquals(
                cursor, component, kDataRootComponent,
                std::strlen(kDataRootComponent))) {
            return -1;
        }
        cursor += component;
        if (*cursor == '/' || *cursor == '\\') {
            ++cursor;
        }
    }

    std::size_t out_length = 0;
    bool pending_separator = false;
    for (; *cursor && out_length + 1 < out_size; ++cursor) {
        if (*cursor == '/' || *cursor == '\\') {
            if (out_length != 0) {
                pending_separator = true;
            }
            continue;
        }
        if (pending_separator) {
            out[out_length++] = '/';
            pending_separator = false;
        }
        out[out_length++] = *cursor;
    }
    out[out_length] = '\0';
    return static_cast<int>(out_length);
}

const DataEntry* findEntry(const char* relative) {
    const std::size_t length = std::strlen(relative);
    for (std::uint32_t index = 0; index < s_entry_count; ++index) {
        const DataEntry& entry = s_entries[index];
        if (nameEquals(entry.name, entry.name_size, relative, length)) {
            return &entry;
        }
    }
    return nullptr;
}

bool isDirectoryPath(const char* relative, std::size_t length) {
    if (length == 0) {
        return true;
    }
    char prefix[512];
    if (length >= sizeof(prefix)) {
        return false;
    }
    std::memcpy(prefix, relative, length);
    prefix[length] = '/';
    prefix[length + 1] = '\0';
    for (std::uint32_t index = 0; index < s_entry_count; ++index) {
        const DataEntry& entry = s_entries[index];
        if (entry.name_size > length &&
            nameStartsWith(entry.name, prefix, length + 1)) {
            return true;
        }
    }
    return false;
}

bool isChildDirectory(const char* directory, const char* child) {
    char prefix[512];
    const std::size_t directory_size = std::strlen(directory);
    const std::size_t child_size = std::strlen(child);
    if (directory_size + child_size + 2 >= sizeof(prefix)) {
        return false;
    }
    std::size_t length = 0;
    if (directory_size != 0) {
        std::memcpy(prefix, directory, directory_size);
        length = directory_size;
    }
    prefix[length++] = '/';
    std::memcpy(prefix + length, child, child_size);
    length += child_size;
    prefix[length] = '\0';

    for (std::uint32_t index = 0; index < s_entry_count; ++index) {
        const DataEntry& entry = s_entries[index];
        if (entry.name_size > length &&
            nameStartsWith(entry.name, prefix, length)) {
            return true;
        }
    }
    return false;
}

char* duplicateString(const char* text) {
    const std::size_t length = std::strlen(text);
    char* copy = static_cast<char*>(std::malloc(length + 1));
    if (copy) {
        std::memcpy(copy, text, length + 1);
    }
    return copy;
}

int openDirectory(_libcglue_fdman_fd_info_t* info, const char* filename, const char* relative) {
    DirHandle* handle = static_cast<DirHandle*>(std::calloc(1, sizeof(DirHandle)));
    if (!handle) {
        return -ENOMEM;
    }

    const std::size_t base_size = std::strlen(relative);
    char directory[512];
    std::memcpy(directory, relative, base_size);
    if (base_size != 0 && base_size < sizeof(directory)) {
        directory[base_size] = '/';
        directory[base_size + 1] = '\0';
    } else if (base_size >= sizeof(directory)) {
        std::free(handle);
        return -ENAMETOOLONG;
    } else {
        directory[0] = '\0';
    }
    const std::size_t directory_prefix_size = base_size == 0 ? 0 : base_size + 1;

    handle->filename = duplicateString(filename);
    handle->relative = duplicateString(relative);
    if (!handle->filename || !handle->relative) {
        std::free(handle->filename);
        std::free(handle->relative);
        std::free(handle);
        return -ENOMEM;
    }
    handle->children = static_cast<char*>(std::malloc(32768));
    handle->child_offsets = static_cast<std::uint32_t*>(std::malloc(4096 * sizeof(std::uint32_t)));
    if (!handle->children || !handle->child_offsets) {
        std::free(handle->children);
        std::free(handle->child_offsets);
        std::free(handle->filename);
        std::free(handle->relative);
        std::free(handle);
        return -ENOMEM;
    }

    std::size_t name_cursor = 0;
    for (std::uint32_t index = 0; index < s_entry_count; ++index) {
        const DataEntry& entry = s_entries[index];
        if (entry.name_size <= directory_prefix_size ||
            !nameStartsWith(entry.name, directory, directory_prefix_size)) {
            continue;
        }
        const char* child = entry.name + directory_prefix_size;
        const std::size_t child_name_size = entry.name_size - directory_prefix_size;
        const char* child_separator = static_cast<const char*>(
            std::memchr(child, '/', child_name_size));
        const std::size_t child_size = child_separator
                                           ? static_cast<std::size_t>(child_separator - child)
                                           : child_name_size;
        if (child_size == 0) {
            continue;
        }

        bool duplicate = false;
        for (std::uint32_t seen = 0; seen < handle->count; ++seen) {
            const char* existing = handle->children + handle->child_offsets[seen];
            if (std::memcmp(existing, child, child_size) == 0 &&
                existing[child_size] == '\0') {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            continue;
        }

        if (handle->count >= 4096 || name_cursor + child_size + 1 > 32768) {
            continue;
        }
        handle->child_offsets[handle->count++] = static_cast<std::uint32_t>(name_cursor);
        std::memcpy(handle->children + name_cursor, child, child_size);
        handle->children[name_cursor + child_size] = '\0';
        name_cursor += child_size + 1;
    }

    info->userdata = handle;
    info->ops = &s_dir_ops;
    return 0;
}

int openFile(
    _libcglue_fdman_fd_info_t* info,
    const char* filename,
    const char* relative,
    const DataEntry* entry) {
    (void) relative;
    FileHandle* handle = static_cast<FileHandle*>(std::calloc(1, sizeof(FileHandle)));
    if (!handle) {
        return -ENOMEM;
    }
    handle->filename = duplicateString(filename);
    if (!handle->filename) {
        std::free(handle);
        return -ENOMEM;
    }
    handle->archive_fd = fioOpen(kArchiveName, FIO_O_RDONLY);
    if (handle->archive_fd < 0) {
        const int error = handle->archive_fd;
        std::free(handle->filename);
        std::free(handle);
        return error < 0 ? error : -EIO;
    }
    handle->base = entry->data_offset;
    handle->size = entry->data_size;
    handle->position = 0;

    info->userdata = handle;
    info->ops = &s_file_ops;
    return 0;
}

int pathOpen(_libcglue_fdman_fd_info_t* info, const char* buf, int flags, mode_t mode) {
    char relative[512];
    const int relative_length = normalizeRelative(buf, relative, sizeof(relative));
    if (relative_length < 0) {
        return s_default_path_ops->open(info, buf, flags, mode);
    }

    if ((flags & (O_WRONLY | O_RDWR)) != 0) {
        return -EROFS;
    }

    if ((flags & O_DIRECTORY) != 0 || relative_length == 0) {
        return openDirectory(info, buf, relative);
    }

    const DataEntry* entry = findEntry(relative);
    if (!entry) {
        if (isDirectoryPath(relative, static_cast<std::size_t>(relative_length))) {
            return openDirectory(info, buf, relative);
        }
        return -ENOENT;
    }
    return openFile(info, buf, relative, entry);
}

int pathStat(const char* path, struct stat* buf) {
    char relative[512];
    const int relative_length = normalizeRelative(path, relative, sizeof(relative));
    if (relative_length < 0) {
        return s_default_path_ops->stat(path, buf);
    }

    std::memset(buf, 0, sizeof(*buf));
    const std::size_t length = static_cast<std::size_t>(relative_length);
    const DataEntry* entry = findEntry(relative);
    if (entry) {
        buf->st_mode = S_IFREG | kReadOnlyMode;
        buf->st_size = entry->data_size;
        return 0;
    }
    if (isDirectoryPath(relative, length)) {
        buf->st_mode = S_IFDIR | S_IRWXU;
        return 0;
    }
    return -ENOENT;
}

int pathRemove(const char* path) {
    char relative[512];
    if (normalizeRelative(path, relative, sizeof(relative)) < 0) {
        return s_default_path_ops->remove(path);
    }
    return -EROFS;
}

int pathRename(const char* old_path, const char* new_path) {
    char relative[512];
    if (normalizeRelative(old_path, relative, sizeof(relative)) < 0) {
        return s_default_path_ops->rename(old_path, new_path);
    }
    return -EROFS;
}

int pathMkdir(const char* path, int mode) {
    char relative[512];
    if (normalizeRelative(path, relative, sizeof(relative)) < 0) {
        return s_default_path_ops->mkdir(path, mode);
    }
    return -EROFS;
}

int pathRmdir(const char* path) {
    char relative[512];
    if (normalizeRelative(path, relative, sizeof(relative)) < 0) {
        return s_default_path_ops->rmdir(path);
    }
    return -EROFS;
}

int pathReadlink(const char* path, char* buf, std::size_t bufsiz) {
    char relative[512];
    if (normalizeRelative(path, relative, sizeof(relative)) < 0) {
        return s_default_path_ops->readlink(path, buf, bufsiz);
    }
    return -EROFS;
}

int pathSymlink(const char* target, const char* linkpath) {
    char relative[512];
    if (normalizeRelative(target, relative, sizeof(relative)) < 0) {
        return s_default_path_ops->symlink(target, linkpath);
    }
    return -EROFS;
}

int fileClose(void* userdata) {
    FileHandle* handle = static_cast<FileHandle*>(userdata);
    if (!handle) {
        return -EBADF;
    }
    int result = 0;
    if (handle->archive_fd >= 0) {
        result = fioClose(handle->archive_fd);
    }
    std::free(handle->filename);
    std::free(handle);
    return result;
}

int fileRead(void* userdata, void* buf, int nbytes) {
    FileHandle* handle = static_cast<FileHandle*>(userdata);
    if (!handle) {
        return -EBADF;
    }
    if (handle->position >= handle->size) {
        return 0;
    }
    int want = nbytes;
    const std::uint32_t remaining = handle->size - handle->position;
    if (want < 0 || static_cast<std::uint32_t>(want) > remaining) {
        want = static_cast<int>(remaining);
    }
    if (fioLseek(handle->archive_fd, static_cast<int>(handle->base + handle->position), 0) < 0) {
        return -EIO;
    }
    const int read = fioRead(handle->archive_fd, buf, want);
    if (read > 0) {
        handle->position += static_cast<std::uint32_t>(read);
    }
    return read;
}

int fileWrite(void* userdata, const void* buf, int nbytes) {
    (void)userdata;
    (void)buf;
    (void)nbytes;
    return -EROFS;
}

int fileLseek(void* userdata, int offset, int whence) {
    FileHandle* handle = static_cast<FileHandle*>(userdata);
    if (!handle) {
        return -EBADF;
    }
    long long position = 0;
    switch (whence) {
        case SEEK_SET:
            position = offset;
            break;
        case SEEK_CUR:
            position = static_cast<long long>(handle->position) + offset;
            break;
        case SEEK_END:
            position = static_cast<long long>(handle->size) + offset;
            break;
        default:
            return -EINVAL;
    }
    if (position < 0 || position > static_cast<long long>(handle->size)) {
        return -EINVAL;
    }
    handle->position = static_cast<std::uint32_t>(position);
    return static_cast<int>(handle->position);
}

int64_t fileLseek64(void* userdata, int64_t offset, int whence) {
    return fileLseek(userdata, static_cast<int>(offset), whence);
}

int fileGetfd(void* userdata) {
    FileHandle* handle = static_cast<FileHandle*>(userdata);
    return handle ? handle->archive_fd : -EBADF;
}

char* fileGetfilename(void* userdata) {
    FileHandle* handle = static_cast<FileHandle*>(userdata);
    return handle ? handle->filename : nullptr;
}

int fileIoctl(void* userdata, int request, void* data) {
    (void)userdata;
    (void)request;
    (void)data;
    return -EINVAL;
}

int fileIoctl2(void* userdata, int request, void* arg, unsigned int arglen, void* buf, unsigned int buflen) {
    (void)userdata;
    (void)request;
    (void)arg;
    (void)arglen;
    (void)buf;
    (void)buflen;
    return -EINVAL;
}

int dirClose(void* userdata) {
    DirHandle* handle = static_cast<DirHandle*>(userdata);
    if (!handle) {
        return -EBADF;
    }
    std::free(handle->children);
    std::free(handle->child_offsets);
    std::free(handle->relative);
    std::free(handle->filename);
    std::free(handle);
    return 0;
}

int dirRead(void* userdata, struct dirent* dirp) {
    DirHandle* handle = static_cast<DirHandle*>(userdata);
    if (!handle || !dirp) {
        return -EBADF;
    }
    if (handle->position >= handle->count) {
        return 0;
    }
    const char* child = handle->children + handle->child_offsets[handle->position];
    dirp->d_type = isChildDirectory(handle->relative, child) ? DT_DIR : DT_REG;
    std::strncpy(dirp->d_name, child, MAXNAMLEN);
    dirp->d_name[MAXNAMLEN] = '\0';
    ++handle->position;
    return 1;
}

int dirGetfd(void* userdata) {
    (void)userdata;
    return 0;
}

char* dirGetfilename(void* userdata) {
    DirHandle* handle = static_cast<DirHandle*>(userdata);
    return handle ? handle->filename : nullptr;
}

int dirIoctl(void* userdata, int request, void* data) {
    (void)userdata;
    (void)request;
    (void)data;
    return -EINVAL;
}

int dirIoctl2(void* userdata, int request, void* arg, unsigned int arglen, void* buf, unsigned int buflen) {
    (void)userdata;
    (void)request;
    (void)arg;
    (void)arglen;
    (void)buf;
    (void)buflen;
    return -EINVAL;
}

bool readFull(int fd, void* buffer, int size) {
    char* cursor = static_cast<char*>(buffer);
    int remaining = size;
    while (remaining > 0) {
        const int read = fioRead(fd, cursor, remaining);
        if (read <= 0) {
            return false;
        }
        cursor += read;
        remaining -= read;
    }
    return true;
}

bool fileExists(const char* path) {
    const int fd = fioOpen(path, FIO_O_RDONLY);
    if (fd < 0) {
        return false;
    }
    fioClose(fd);
    return true;
}

void loadUsbMassStorageDrivers() {
    // USBHDFSD registers the mass: filesystem after USBD is available. Ignore
    // load failures so an archive-only disc still boots on every PS2 setup.
    SifLoadFileInit();
    SifLoadModule(kUsbDriverPath, 0, nullptr);
    SifLoadModule(kUsbMassDriverPath, 0, nullptr);
}

}  // namespace

int initDataBackend() {
    if (s_initialized) {
        return 0;
    }

    fioInit();
    if (fileExists(kHostConfigPath)) {
        s_data_root = kHostDataRoot;
        s_initialized = true;
        std::fprintf(stderr, "ps2 data: using %s\n", s_data_root);
        return 0;
    }
    if (fileExists(kLegacyHostConfigPath)) {
        s_data_root = kLegacyHostDataRoot;
        s_initialized = true;
        std::fprintf(stderr, "ps2 data: using %s\n", s_data_root);
        return 0;
    }

    loadUsbMassStorageDrivers();
    if (fileExists(kMassConfigPath)) {
        s_data_root = kMassDataRoot;
        s_initialized = true;
        std::fprintf(stderr, "ps2 data: using %s\n", s_data_root);
        return 0;
    }

    s_default_path_ops = _libcglue_fdman_path_ops;

    const int fd = fioOpen(kArchiveName, FIO_O_RDONLY);
    if (fd < 0) {
        std::fprintf(stderr, "ps2 data: cannot open %s\n", kArchiveName);
        return -1;
    }

    std::uint8_t header[kHeaderSize];
    if (!readFull(fd, header, sizeof(header))) {
        std::fprintf(stderr, "ps2 data: cannot read archive header\n");
        fioClose(fd);
        return -1;
    }
    const auto readU32 = [&header](std::size_t offset) {
        return static_cast<std::uint32_t>(header[offset]) |
               (static_cast<std::uint32_t>(header[offset + 1]) << 8u) |
               (static_cast<std::uint32_t>(header[offset + 2]) << 16u) |
               (static_cast<std::uint32_t>(header[offset + 3]) << 24u);
    };
    if (readU32(0) != kMagic || readU32(4) != kVersion) {
        std::fprintf(stderr, "ps2 data: bad archive header\n");
        fioClose(fd);
        return -1;
    }
    s_entry_count = readU32(8);
    const std::uint32_t index_size = readU32(12);
    if (s_entry_count == 0 || s_entry_count > 65536 ||
        index_size < kHeaderSize + s_entry_count * kEntrySize) {
        std::fprintf(stderr, "ps2 data: bad archive index\n");
        fioClose(fd);
        return -1;
    }

    const std::uint32_t index_data_size = index_size - kHeaderSize;
    std::uint8_t* index = static_cast<std::uint8_t*>(std::malloc(index_data_size));
    if (!index || !readFull(fd, index, static_cast<int>(index_data_size))) {
        std::fprintf(stderr, "ps2 data: cannot read archive index\n");
        std::free(index);
        fioClose(fd);
        return -1;
    }
    fioClose(fd);

    s_entries = static_cast<DataEntry*>(std::calloc(s_entry_count, sizeof(DataEntry)));
    if (!s_entries) {
        std::fprintf(stderr, "ps2 data: out of memory\n");
        std::free(index);
        return -1;
    }
    const char* index_names = reinterpret_cast<const char*>(index);
    for (std::uint32_t entry_index = 0; entry_index < s_entry_count; ++entry_index) {
        const std::uint8_t* raw = index + entry_index * kEntrySize;
        const auto rawU32 = [raw](std::size_t offset) {
            return static_cast<std::uint32_t>(raw[offset]) |
                   (static_cast<std::uint32_t>(raw[offset + 1]) << 8u) |
                   (static_cast<std::uint32_t>(raw[offset + 2]) << 16u) |
                   (static_cast<std::uint32_t>(raw[offset + 3]) << 24u);
        };
        const std::uint32_t name_offset = rawU32(0);
        const std::uint32_t name_size = rawU32(4);
        DataEntry& entry = s_entries[entry_index];
        entry.name = index_names + name_offset;
        entry.name_size = name_size;
        entry.data_offset = rawU32(8);
        entry.data_size = rawU32(12);
    }

    s_file_ops.getfd = fileGetfd;
    s_file_ops.getfilename = fileGetfilename;
    s_file_ops.close = fileClose;
    s_file_ops.read = fileRead;
    s_file_ops.lseek = fileLseek;
    s_file_ops.lseek64 = fileLseek64;
    s_file_ops.write = fileWrite;
    s_file_ops.ioctl = fileIoctl;
    s_file_ops.ioctl2 = fileIoctl2;

    s_dir_ops.getfd = dirGetfd;
    s_dir_ops.getfilename = dirGetfilename;
    s_dir_ops.close = dirClose;
    s_dir_ops.dread = dirRead;
    s_dir_ops.ioctl = dirIoctl;
    s_dir_ops.ioctl2 = dirIoctl2;

    s_path_ops.open = pathOpen;
    s_path_ops.remove = pathRemove;
    s_path_ops.rename = pathRename;
    s_path_ops.mkdir = pathMkdir;
    s_path_ops.rmdir = pathRmdir;
    s_path_ops.stat = pathStat;
    s_path_ops.readlink = pathReadlink;
    s_path_ops.symlink = pathSymlink;

    _libcglue_fdman_path_ops = &s_path_ops;
    s_initialized = true;
    s_data_root = kArchiveDataRoot;
    std::fprintf(
        stderr,
        "ps2 data: %lu files from %s\n",
        static_cast<unsigned long>(s_entry_count),
        kArchiveName);
    return 0;
}

const char* dataRoot() {
    return s_data_root;
}

}  // namespace ps2
}  // namespace platform
}  // namespace runtime
}  // namespace osf
