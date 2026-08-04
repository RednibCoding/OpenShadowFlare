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

#include "data/table.h"

#include "data/rclib.h"

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef enum SfTableField {
  SF_TABLE_COUNT,
  SF_TABLE_NUMBER,
  SF_TABLE_ROWS,
  SF_TABLE_COLUMNS,
  SF_TABLE_VALUE,
  SF_TABLE_STRING_LENGTH,
  SF_TABLE_STRING,
  SF_TABLE_DONE
} SfTableField;

typedef struct SfTableScanner {
  SfTableNumericValue output;
  void *user;
  int32_t table_count;
  int32_t table_number;
  int32_t rows;
  int32_t columns;
  uint32_t cell_count;
  uint32_t cell_index;
  uint32_t string_index;
  uint32_t string_bytes;
  uint32_t value;
  uint8_t value_bytes;
  SfTableField field;
  bool valid;
} SfTableScanner;

static uint32_t sf_table_u32(const uint8_t *bytes) {
  return (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
}

static void sf_table_begin_i32(
    SfTableScanner *scanner, SfTableField field) {
  scanner->field = field;
  scanner->value = 0u;
  scanner->value_bytes = 0u;
}

static bool sf_table_finish_table(SfTableScanner *scanner) {
  if (--scanner->table_count < 0) return false;
  if (scanner->table_count == 0) scanner->field = SF_TABLE_DONE;
  else sf_table_begin_i32(scanner, SF_TABLE_NUMBER);
  return true;
}

static bool sf_table_finish_i32(SfTableScanner *scanner) {
  const int32_t value = (int32_t) scanner->value;
  if (scanner->field == SF_TABLE_COUNT) {
    if (value < 0) return false;
    scanner->table_count = value;
    if (value == 0) scanner->field = SF_TABLE_DONE;
    else sf_table_begin_i32(scanner, SF_TABLE_NUMBER);
  } else if (scanner->field == SF_TABLE_NUMBER) {
    scanner->table_number = value;
    sf_table_begin_i32(scanner, SF_TABLE_ROWS);
  } else if (scanner->field == SF_TABLE_ROWS) {
    if (value < 0) return false;
    scanner->rows = value;
    sf_table_begin_i32(scanner, SF_TABLE_COLUMNS);
  } else if (scanner->field == SF_TABLE_COLUMNS) {
    uint64_t count;
    if (value < 0) return false;
    scanner->columns = value;
    count = (uint64_t) (uint32_t) scanner->rows * (uint32_t) value;
    if (count > UINT32_MAX) return false;
    scanner->cell_count = (uint32_t) count;
    scanner->cell_index = 0u;
    scanner->string_index = 0u;
    if (scanner->cell_count == 0u) return sf_table_finish_table(scanner);
    sf_table_begin_i32(scanner, SF_TABLE_VALUE);
  } else if (scanner->field == SF_TABLE_VALUE) {
    const uint32_t cell = scanner->cell_index++;
    if (!scanner->output(
          scanner->user, scanner->table_number,
          (int32_t) (cell / (uint32_t) scanner->columns),
          (int32_t) (cell % (uint32_t) scanner->columns), value))
      return false;
    if (scanner->cell_index < scanner->cell_count)
      sf_table_begin_i32(scanner, SF_TABLE_VALUE);
    else
      sf_table_begin_i32(scanner, SF_TABLE_STRING_LENGTH);
  } else if (scanner->field == SF_TABLE_STRING_LENGTH) {
    if (value < 0) return false;
    scanner->string_bytes = (uint32_t) value;
    if (scanner->string_bytes == 0u) {
      ++scanner->string_index;
      if (scanner->string_index == scanner->cell_count)
        return sf_table_finish_table(scanner);
      sf_table_begin_i32(scanner, SF_TABLE_STRING_LENGTH);
    } else {
      scanner->field = SF_TABLE_STRING;
    }
  }
  return true;
}

static bool sf_table_byte(void *user, size_t offset, uint8_t value) {
  SfTableScanner *scanner = (SfTableScanner *) user;
  (void) offset;
  if (!scanner->valid || scanner->field == SF_TABLE_DONE) return false;
  if (scanner->field == SF_TABLE_STRING) {
    if (scanner->string_bytes == 0u) return false;
    --scanner->string_bytes;
    if (scanner->string_bytes == 0u) {
      ++scanner->string_index;
      if (scanner->string_index == scanner->cell_count)
        scanner->valid = sf_table_finish_table(scanner);
      else
        sf_table_begin_i32(scanner, SF_TABLE_STRING_LENGTH);
    }
    return scanner->valid;
  }
  scanner->value |= (uint32_t) value << (scanner->value_bytes * 8u);
  if (++scanner->value_bytes == 4u)
    scanner->valid = sf_table_finish_i32(scanner);
  return scanner->valid;
}

bool sf_table_scan_numeric(
    const char *path, SfTableNumericValue output, void *user) {
  uint8_t header[20];
  uint8_t compression_header[16];
  SfTableScanner scanner;
  FILE *file;
  bool decoded = false;
  if (!path || !output) return false;
  file = fopen(path, "rb");
  if (!file) return false;
  if (fread(header, 1u, sizeof(header), file) != sizeof(header) ||
      memcmp(header, "TABLE DATA V000\x1a", 16u) != 0) goto done;
  memset(&scanner, 0, sizeof(scanner));
  scanner.output = output;
  scanner.user = user;
  scanner.field = SF_TABLE_COUNT;
  scanner.valid = true;
  if (sf_table_u32(header + 16u) == 1u) {
    uint32_t decoded_size;
    if (fread(
          compression_header, 1u, sizeof(compression_header), file) !=
        sizeof(compression_header) ||
        memcmp(compression_header, "RCLIB-L", 7u) != 0 ||
        fseek(file, -16L, SEEK_CUR) != 0) goto done;
    decoded_size = sf_table_u32(compression_header + 8u);
    decoded = sf_rclib_decode_stream_to(
      file, decoded_size, sf_table_byte, &scanner);
  } else if (sf_table_u32(header + 16u) == 0u) {
    uint8_t size_bytes[4];
    uint32_t decoded_size;
    uint32_t offset;
    if (fread(size_bytes, 1u, sizeof(size_bytes), file) !=
        sizeof(size_bytes)) goto done;
    decoded_size = sf_table_u32(size_bytes);
    decoded = true;
    for (offset = 0u; offset < decoded_size; ++offset) {
      const int value = fgetc(file);
      if (value < 0 || !sf_table_byte(&scanner, offset, (uint8_t) value)) {
        decoded = false;
        break;
      }
    }
  }
done:
  fclose(file);
  return decoded && scanner.valid && scanner.field == SF_TABLE_DONE &&
    scanner.table_count == 0;
}
