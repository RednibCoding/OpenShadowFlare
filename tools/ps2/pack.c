/*
 * Host-side packer for the PlayStation 2 game-data archive.
 *
 * Produces SFGAME.BIN, a single index-first archive that the PS2 port reads
 * through tools/ps2's fileio backend (see ps2_data_backend.cpp).  The game's
 * disc-access filesystem hook (the BIOS fileio module) can only reach flat,
 * uppercase, 8.3 root files, so the whole data tree is packed into one root
 * file whose first block is a self-describing index.
 *
 * Layout (all little-endian u32, offsets from start of file):
 *   [0]  header: magic 'SFB1' | version | entry_count | index_size
 *   [16] entries[entry_count]: name_offset | name_size | data_offset | data_size
 *   ...  name bytes (name_offset is relative to the start of the index block)
 *   ...  zero padding to the next 2048-byte boundary (index_size ends here)
 *   ...  data blobs, each padded to a 2048-byte boundary
 */

#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define SFB_MAGIC 0x53464231u
#define SFB_VERSION 1u
#define SECTOR 2048u
#define HEADER_SIZE 16u
#define ENTRY_SIZE 16u

struct entry {
    char *name;
    size_t name_len;
    size_t file_size;
    unsigned long data_offset;
};

static struct entry *entries;
static size_t entry_count;
static size_t entry_capacity;

static int entry_compare(const void *a, const void *b) {
    const struct entry *ea = (const struct entry *)a;
    const struct entry *eb = (const struct entry *)b;
    return strcmp(ea->name, eb->name);
}

static void add_entry(const char *name, size_t file_size) {
    if (entry_count == entry_capacity) {
        entry_capacity = entry_capacity ? entry_capacity * 2 : 4096;
        entries = (struct entry *)realloc(
            entries, entry_capacity * sizeof(struct entry));
        if (!entries) {
            fprintf(stderr, "pack: out of memory\n");
            exit(1);
        }
    }
    struct entry *entry = &entries[entry_count++];
    entry->name = strdup(name);
    if (!entry->name) {
        fprintf(stderr, "pack: out of memory\n");
        exit(1);
    }
    entry->name_len = strlen(name);
    entry->file_size = file_size;
    entry->data_offset = 0;
}

static size_t align_up(size_t value, size_t boundary) {
    return (value + boundary - 1) & ~(boundary - 1);
}

static int walk_directory(const char *directory, const char *prefix) {
    DIR *dir = opendir(directory);
    if (!dir) {
        fprintf(stderr, "pack: cannot open directory %s: %s\n",
                directory, strerror(errno));
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char child_path[4096];
        snprintf(child_path, sizeof(child_path), "%s/%s", directory,
                 entry->d_name);

        struct stat st;
        if (stat(child_path, &st) != 0) {
            fprintf(stderr, "pack: cannot stat %s: %s\n", child_path,
                    strerror(errno));
            continue;
        }

        char child_name[4096];
        if (prefix[0]) {
            snprintf(child_name, sizeof(child_name), "%s/%s", prefix,
                     entry->d_name);
        } else {
            snprintf(child_name, sizeof(child_name), "%s", entry->d_name);
        }

        if (S_ISDIR(st.st_mode)) {
            if (walk_directory(child_path, child_name) != 0) {
                closedir(dir);
                return -1;
            }
        } else if (S_ISREG(st.st_mode)) {
            add_entry(child_name, (size_t)st.st_size);
        }
    }

    closedir(dir);
    return 0;
}

static int pack_file(FILE *out, const char *data_dir, const char *name) {
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", data_dir, name);

    FILE *input = fopen(path, "rb");
    if (!input) {
        fprintf(stderr, "pack: cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }

    char buffer[SECTOR];
    size_t read;
    while ((read = fread(buffer, 1, sizeof(buffer), input)) > 0) {
        if (fwrite(buffer, 1, read, out) != read) {
            fprintf(stderr, "pack: write error\n");
            fclose(input);
            return -1;
        }
    }
    fclose(input);

    static char pad[SECTOR] = {0};
    if (ftell(out) % SECTOR != 0) {
        const size_t remaining = SECTOR - (ftell(out) % SECTOR);
        fwrite(pad, 1, remaining, out);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <data_dir> <out_file>\n", argv[0]);
        return 1;
    }
    const char *data_dir = argv[1];
    const char *out_path = argv[2];

    if (walk_directory(data_dir, "") != 0) {
        return 1;
    }
    if (entry_count == 0) {
        fprintf(stderr, "pack: no files found under %s\n", data_dir);
        return 1;
    }
    qsort(entries, entry_count, sizeof(struct entry), entry_compare);

    size_t index_size = HEADER_SIZE + entry_count * ENTRY_SIZE;
    size_t name_offsets[entry_count];
    for (size_t index = 0; index < entry_count; ++index) {
        name_offsets[index] = index_size - HEADER_SIZE;
        index_size += entries[index].name_len;
    }
    index_size = align_up(index_size, SECTOR);

    unsigned long data_offset = (unsigned long)index_size;
    for (size_t index = 0; index < entry_count; ++index) {
        entries[index].data_offset = data_offset;
        data_offset += (unsigned long)align_up(entries[index].file_size, SECTOR);
    }

    FILE *out = fopen(out_path, "wb");
    if (!out) {
        fprintf(stderr, "pack: cannot create %s: %s\n", out_path,
                strerror(errno));
        return 1;
    }

    unsigned char header[HEADER_SIZE];
    header[0] = (unsigned char)(SFB_MAGIC);
    header[1] = (unsigned char)(SFB_MAGIC >> 8);
    header[2] = (unsigned char)(SFB_MAGIC >> 16);
    header[3] = (unsigned char)(SFB_MAGIC >> 24);
    header[4] = (unsigned char)(SFB_VERSION);
    header[5] = (unsigned char)(SFB_VERSION >> 8);
    header[6] = (unsigned char)(SFB_VERSION >> 16);
    header[7] = (unsigned char)(SFB_VERSION >> 24);
    header[8] = (unsigned char)(entry_count);
    header[9] = (unsigned char)(entry_count >> 8);
    header[10] = (unsigned char)(entry_count >> 16);
    header[11] = (unsigned char)(entry_count >> 24);
    header[12] = (unsigned char)(index_size);
    header[13] = (unsigned char)(index_size >> 8);
    header[14] = (unsigned char)(index_size >> 16);
    header[15] = (unsigned char)(index_size >> 24);
    fwrite(header, 1, sizeof(header), out);

    for (size_t index = 0; index < entry_count; ++index) {
        unsigned char entry[ENTRY_SIZE];
        const unsigned long name_offset = (unsigned long)name_offsets[index];
        const unsigned long name_len = (unsigned long)entries[index].name_len;
        const unsigned long data_off = entries[index].data_offset;
        const unsigned long data_len = (unsigned long)entries[index].file_size;
        entry[0] = (unsigned char)(name_offset);
        entry[1] = (unsigned char)(name_offset >> 8);
        entry[2] = (unsigned char)(name_offset >> 16);
        entry[3] = (unsigned char)(name_offset >> 24);
        entry[4] = (unsigned char)(name_len);
        entry[5] = (unsigned char)(name_len >> 8);
        entry[6] = (unsigned char)(name_len >> 16);
        entry[7] = (unsigned char)(name_len >> 24);
        entry[8] = (unsigned char)(data_off);
        entry[9] = (unsigned char)(data_off >> 8);
        entry[10] = (unsigned char)(data_off >> 16);
        entry[11] = (unsigned char)(data_off >> 24);
        entry[12] = (unsigned char)(data_len);
        entry[13] = (unsigned char)(data_len >> 8);
        entry[14] = (unsigned char)(data_len >> 16);
        entry[15] = (unsigned char)(data_len >> 24);
        fwrite(entry, 1, sizeof(entry), out);
    }

    for (size_t index = 0; index < entry_count; ++index) {
        fwrite(entries[index].name, 1, entries[index].name_len, out);
    }

    static unsigned char pad[SECTOR] = {0};
    const size_t remaining = index_size - ftell(out);
    fwrite(pad, 1, remaining, out);

    for (size_t index = 0; index < entry_count; ++index) {
        if (pack_file(out, data_dir, entries[index].name) != 0) {
            fclose(out);
            return 1;
        }
    }

    fclose(out);
    fprintf(stderr, "pack: %zu files, index %zu bytes, total %lu bytes\n",
            entry_count, index_size, data_offset);
    return 0;
}
