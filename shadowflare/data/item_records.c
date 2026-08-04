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

#include "data/item_records.h"

#include "data/item_decode.h"

#include <stddef.h>

typedef enum SfItemRecordField {
  SF_ITEM_RECORD_CATEGORY_COUNT,
  SF_ITEM_RECORD_NAME_LENGTH,
  SF_ITEM_RECORD_NAME,
  SF_ITEM_RECORD_DESCRIPTION_LENGTH,
  SF_ITEM_RECORD_DESCRIPTION,
  SF_ITEM_RECORD_WORDS
} SfItemRecordField;

typedef struct SfItemRecordScanner {
  SfItemRecordWord word;
  void *user;
  int32_t category_records;
  int32_t record_index;
  int32_t value;
  uint32_t remaining;
  uint16_t record_offset;
  uint8_t category;
  uint8_t value_bytes;
  SfItemRecordField field;
  bool valid;
} SfItemRecordScanner;

static const uint16_t sf_item_record_sizes[5] = {
  804u, 764u, 672u, 140u, 100u};

static void sf_item_record_begin_i32(
    SfItemRecordScanner *scanner, SfItemRecordField field) {
  scanner->field = field;
  scanner->value = 0;
  scanner->value_bytes = 0u;
}

static void sf_item_record_next(SfItemRecordScanner *scanner) {
  ++scanner->record_index;
  if (scanner->record_index < scanner->category_records) {
    sf_item_record_begin_i32(scanner, SF_ITEM_RECORD_NAME_LENGTH);
  } else {
    ++scanner->category;
    scanner->record_index = 0;
    if (scanner->category < 5u)
      sf_item_record_begin_i32(scanner, SF_ITEM_RECORD_CATEGORY_COUNT);
  }
}

static void sf_item_record_begin_words(SfItemRecordScanner *scanner) {
  scanner->field = SF_ITEM_RECORD_WORDS;
  scanner->record_offset = 0u;
  scanner->value = 0;
  scanner->value_bytes = 0u;
}

static bool sf_item_record_text_byte(SfItemRecordScanner *scanner) {
  if (scanner->remaining > 0u) --scanner->remaining;
  if (scanner->remaining != 0u) return true;
  if (scanner->field == SF_ITEM_RECORD_NAME)
    sf_item_record_begin_i32(scanner, SF_ITEM_RECORD_DESCRIPTION_LENGTH);
  else
    sf_item_record_begin_words(scanner);
  return true;
}

static bool sf_item_record_word_byte(
    SfItemRecordScanner *scanner, uint8_t byte) {
  const uint16_t byte_in_word = (uint16_t) (scanner->record_offset & 3u);
  scanner->value |= (int32_t) ((uint32_t) byte <<
    ((unsigned) byte_in_word * 8u));
  ++scanner->record_offset;
  if (byte_in_word == 3u) {
    const uint16_t offset = (uint16_t) (scanner->record_offset - 4u);
    if (!scanner->word(
          scanner->user, scanner->category, offset, scanner->value))
      return false;
    scanner->value = 0;
  }
  if (scanner->record_offset == sf_item_record_sizes[scanner->category])
    sf_item_record_next(scanner);
  return true;
}

static bool sf_item_record_length(SfItemRecordScanner *scanner) {
  if (scanner->value < 0) return false;
  if (scanner->field == SF_ITEM_RECORD_CATEGORY_COUNT) {
    scanner->category_records = scanner->value;
    scanner->record_index = 0;
    if (scanner->category_records == 0) {
      ++scanner->category;
      if (scanner->category < 5u)
        sf_item_record_begin_i32(scanner, SF_ITEM_RECORD_CATEGORY_COUNT);
    } else {
      sf_item_record_begin_i32(scanner, SF_ITEM_RECORD_NAME_LENGTH);
    }
    return true;
  }
  scanner->remaining = (uint32_t) scanner->value;
  if (scanner->remaining == 0u) {
    if (scanner->field == SF_ITEM_RECORD_NAME_LENGTH)
      sf_item_record_begin_i32(scanner, SF_ITEM_RECORD_DESCRIPTION_LENGTH);
    else
      sf_item_record_begin_words(scanner);
  } else {
    scanner->field = scanner->field == SF_ITEM_RECORD_NAME_LENGTH
      ? SF_ITEM_RECORD_NAME : SF_ITEM_RECORD_DESCRIPTION;
  }
  return true;
}

static bool sf_item_record_byte(void *user, size_t offset, uint8_t value) {
  SfItemRecordScanner *scanner = (SfItemRecordScanner *) user;
  (void) offset;
  if (!scanner->valid || scanner->category >= 5u) return scanner->valid;
  if (scanner->field == SF_ITEM_RECORD_NAME ||
      scanner->field == SF_ITEM_RECORD_DESCRIPTION)
    return sf_item_record_text_byte(scanner);
  if (scanner->field == SF_ITEM_RECORD_WORDS)
    return sf_item_record_word_byte(scanner, value);
  scanner->value |= (int32_t) ((uint32_t) value <<
    ((unsigned) scanner->value_bytes * 8u));
  if (++scanner->value_bytes < 4u) return true;
  scanner->valid = sf_item_record_length(scanner);
  return scanner->valid;
}

bool sf_item_scan_records(
    const char *path, SfItemRecordWord word, void *user) {
  SfItemRecordScanner scanner;
  if (!path || !word) return false;
  scanner = (SfItemRecordScanner) {
    word, user, 0, 0, 0, 0u, 0u, 0u, 0u,
    SF_ITEM_RECORD_CATEGORY_COUNT, true};
  return sf_item_decode_stream(path, sf_item_record_byte, &scanner) &&
    scanner.valid && scanner.category == 5u;
}
