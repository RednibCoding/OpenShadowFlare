/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of OpenShadowFlare.
 *
 * OpenShadowFlare is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * OpenShadowFlare is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with OpenShadowFlare. If not, see <https://www.gnu.org/licenses/>.
 */

#include "data/item.h"

#include "data/rclib.h"

#include <stdio.h>
#include <string.h>

static const uint16_t sf_item_record_sizes[5] = {804u, 764u, 672u, 140u, 100u};

static const char sf_item_substitution_hex[] =
  "be66b32f016e6dc81f98a546765c3d0e"
  "aa5e9dffeaa00d4b75f661855dbbdcfb"
  "8bc34f450490811e6bc9d373c6e724ba"
  "32f3c0ec57ccc4b6c1aeaf88f284ce4a"
  "fc3c9f1a56c5e2f547d9d78ccd97f07b"
  "3106e514e6da4826ac879ad8a6eb92cf0"
  "f9441b4742ad1701cd4b0c20908169bfd"
  "771d219e3635533ed0d562585f637cb58"
  "d2bd289b799a1306554409671febff4a9"
  "5bf722605a6ffa1b79e917b1009c7e522"
  "9122c78059155e3a2b9f8509513807f1"
  "127cb374e5115efa7724d8349a469de20"
  "a367df1042396c2dc723e4ddedd6f959"
  "b2ad6a7dbceee03a3fca4c2568931833"
  "280b07038202438a86db383419642e7a"
  "abf1e8440cb88fa80a8ebde13b";

typedef enum SfItemScanField {
  SF_ITEM_SCAN_CATEGORY_COUNT,
  SF_ITEM_SCAN_NAME_LENGTH,
  SF_ITEM_SCAN_NAME,
  SF_ITEM_SCAN_DESCRIPTION_LENGTH,
  SF_ITEM_SCAN_DESCRIPTION,
  SF_ITEM_SCAN_RECORD
} SfItemScanField;

typedef struct SfItemScanner {
  SfItemAppearance result;
  int32_t target_id;
  int32_t category_records;
  int32_t record_index;
  int32_t field_value;
  int32_t record_id;
  int32_t checksum;
  uint32_t remaining;
  uint16_t record_offset;
  uint16_t appearance_offset;
  uint8_t category;
  uint8_t target_category;
  uint8_t field_bytes;
  SfItemScanField field;
  bool found;
  bool valid;
} SfItemScanner;

static uint32_t sf_item_u32(const uint8_t *bytes) {
  return (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
}

static uint8_t sf_item_hex(char value) {
  return (uint8_t) (value >= 'a' ? value - 'a' + 10 : value - '0');
}

static uint8_t sf_item_substitute(void *user, uint8_t value) {
  const size_t offset = (size_t) value * 2u;
  (void) user;
  return (uint8_t) ((sf_item_hex(sf_item_substitution_hex[offset]) << 4u) |
    sf_item_hex(sf_item_substitution_hex[offset + 1u]));
}

static void sf_item_begin_i32(SfItemScanner *scanner, SfItemScanField field) {
  scanner->field = field;
  scanner->field_value = 0;
  scanner->field_bytes = 0u;
}

static void sf_item_next_record(SfItemScanner *scanner) {
  ++scanner->record_index;
  if (scanner->record_index < scanner->category_records) {
    sf_item_begin_i32(scanner, SF_ITEM_SCAN_NAME_LENGTH);
  } else {
    ++scanner->category;
    scanner->record_index = 0;
    if (scanner->category < 5u)
      sf_item_begin_i32(scanner, SF_ITEM_SCAN_CATEGORY_COUNT);
  }
}

static bool sf_item_scan_byte(void *user, size_t offset, uint8_t value) {
  SfItemScanner *scanner = (SfItemScanner *) user;
  (void) offset;
  scanner->checksum += (int8_t) value;
  if (!scanner->valid || scanner->category >= 5u) return scanner->valid;
  if (scanner->field == SF_ITEM_SCAN_NAME ||
      scanner->field == SF_ITEM_SCAN_DESCRIPTION) {
    if (scanner->remaining > 0u) --scanner->remaining;
    if (scanner->remaining == 0u) {
      if (scanner->field == SF_ITEM_SCAN_NAME)
        sf_item_begin_i32(scanner, SF_ITEM_SCAN_DESCRIPTION_LENGTH);
      else {
        scanner->field = SF_ITEM_SCAN_RECORD;
        scanner->record_offset = 0u;
        scanner->field_value = 0;
        scanner->field_bytes = 0u;
      }
    }
    return true;
  }
  if (scanner->field == SF_ITEM_SCAN_RECORD) {
    const uint16_t offset_in_word = (uint16_t) (scanner->record_offset & 3u);
    scanner->field_value |= (int32_t) ((uint32_t) value <<
      (unsigned) offset_in_word * 8u);
    ++scanner->record_offset;
    if (offset_in_word == 3u) {
      const uint16_t word_offset = (uint16_t) (scanner->record_offset - 4u);
      if (word_offset == 4u) scanner->record_id = scanner->field_value;
      if (scanner->category == scanner->target_category &&
          scanner->record_id == scanner->target_id) {
        if (word_offset == scanner->appearance_offset)
          scanner->result.part = scanner->field_value;
        if (word_offset == scanner->appearance_offset + 4u)
          scanner->result.red = scanner->field_value;
        if (word_offset == scanner->appearance_offset + 8u)
          scanner->result.green = scanner->field_value;
        if (word_offset == scanner->appearance_offset + 12u) {
          scanner->result.blue = scanner->field_value;
          scanner->found = true;
        }
      }
      scanner->field_value = 0;
    }
    if (scanner->record_offset == sf_item_record_sizes[scanner->category])
      sf_item_next_record(scanner);
    return true;
  }
  scanner->field_value |= (int32_t) ((uint32_t) value <<
    (unsigned) scanner->field_bytes * 8u);
  if (++scanner->field_bytes < 4u) return true;
  if (scanner->field_value < 0) {
    scanner->valid = false;
    return false;
  }
  if (scanner->field == SF_ITEM_SCAN_CATEGORY_COUNT) {
    scanner->category_records = scanner->field_value;
    scanner->record_index = 0;
    if (scanner->category_records == 0) {
      ++scanner->category;
      if (scanner->category < 5u)
        sf_item_begin_i32(scanner, SF_ITEM_SCAN_CATEGORY_COUNT);
    } else {
      sf_item_begin_i32(scanner, SF_ITEM_SCAN_NAME_LENGTH);
    }
  } else {
    scanner->remaining = (uint32_t) scanner->field_value;
    if (scanner->remaining == 0u) {
      if (scanner->field == SF_ITEM_SCAN_NAME_LENGTH)
        sf_item_begin_i32(scanner, SF_ITEM_SCAN_DESCRIPTION_LENGTH);
      else {
        scanner->field = SF_ITEM_SCAN_RECORD;
        scanner->record_offset = 0u;
        scanner->field_value = 0;
      }
    } else {
      scanner->field = scanner->field == SF_ITEM_SCAN_NAME_LENGTH
        ? SF_ITEM_SCAN_NAME : SF_ITEM_SCAN_DESCRIPTION;
    }
  }
  return true;
}

bool sf_item_read_appearance(
    const char *path, uint8_t category, int32_t definition_id,
    SfItemAppearance *appearance) {
  uint8_t header[24];
  SfItemScanner scanner;
  FILE *file;
  uint32_t decoded_size;
  bool decoded;
  if (!path || category > 1u || !appearance) return false;
  file = fopen(path, "rb");
  if (!file) return false;
  memset(&scanner, 0, sizeof(scanner));
  scanner.target_category = category;
  scanner.target_id = definition_id;
  scanner.appearance_offset = category == 0u ? 168u : 152u;
  scanner.valid = true;
  scanner.result.part = -1;
  scanner.result.red = 1000;
  scanner.result.green = 1000;
  scanner.result.blue = 1000;
  scanner.field = SF_ITEM_SCAN_CATEGORY_COUNT;
  if (fread(header, 1u, sizeof(header), file) != sizeof(header) ||
      memcmp(header, "SFItemDataV0000\x1a", 16u) != 0 ||
      sf_item_u32(header + 20u) != 1u) {
    fclose(file);
    return false;
  }
  {
    uint8_t compression_header[16];
    if (fread(compression_header, 1u, sizeof(compression_header), file) !=
        sizeof(compression_header) ||
        memcmp(compression_header, "RCLIB-L", 7u) != 0) {
      fclose(file);
      return false;
    }
    decoded_size = sf_item_u32(compression_header + 8u);
    if (fseek(file, -16L, SEEK_CUR) != 0) {
      fclose(file);
      return false;
    }
  }
  decoded = sf_rclib_decode_stream_to_transformed(
    file, decoded_size, sf_item_substitute, NULL,
    sf_item_scan_byte, &scanner);
  fclose(file);
  if (!decoded || !scanner.valid || !scanner.found ||
      scanner.checksum != (int32_t) sf_item_u32(header + 16u)) return false;
  *appearance = scanner.result;
  return true;
}
