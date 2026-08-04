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

#include "data/save_payload.h"

#include <limits.h>
#include <string.h>

#define SF_SAVE_PLAIN_SIZE (16u + SF_SAVE_PLAYER_RECORD_SIZE)
#define SF_SAVE_ENVELOPE_SIZE 9u
#define SF_SAVE_MAXIMUM_PAYLOAD (64u * 1024u * 1024u)

static const char sf_save_substitution_hex[] =
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

static uint32_t sf_save_u32(const uint8_t *bytes) {
  return (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
}

static uint8_t sf_save_hex_nibble(char value) {
  return (uint8_t) (value >= 'a' ? value - 'a' + 10 : value - '0');
}

static void sf_save_build_substitution(uint8_t table[256]) {
  uint16_t index;
  for (index = 0u; index < 256u; ++index) {
    const uint16_t offset = (uint16_t) (index * 2u);
    table[index] = (uint8_t) (
      (sf_save_hex_nibble(sf_save_substitution_hex[offset]) << 4u) |
      sf_save_hex_nibble(sf_save_substitution_hex[offset + 1u]));
  }
}

static bool sf_save_decode_at(
    SfSavePayloadReader *reader, uint32_t offset,
    uint8_t *bytes, size_t size) {
  const long payload_start = (long) (SF_SAVE_PLAIN_SIZE + SF_SAVE_ENVELOPE_SIZE);
  const long current = ftell(reader->file);
  size_t index;
  if (current < 0 || size > reader->remaining ||
      offset > reader->remaining - (uint32_t) size ||
      fseek(reader->file, payload_start + (long) offset, SEEK_SET) != 0)
    return false;
  for (index = 0u; index < size; ++index) {
    const int encoded = fgetc(reader->file);
    if (encoded == EOF) return false;
    bytes[index] = (uint8_t) (
      reader->substitution[(uint8_t) encoded] ^ reader->xor_key);
  }
  return fseek(reader->file, current, SEEK_SET) == 0;
}

static bool sf_save_extension_header(
    SfSavePayloadReader *reader, uint32_t size,
    uint32_t expected_version) {
  static const uint8_t signature[8] = {
    'O', 'S', 'F', 'S', 'T', '0', '1', 0
  };
  uint8_t header[24];
  const size_t header_size = size < sizeof(header) ? size : sizeof(header);
  const uint32_t start = reader->remaining - size;
  memset(header, 0, sizeof(header));
  if (!sf_save_decode_at(reader, start, header, header_size) ||
      memcmp(header, signature, sizeof(signature)) != 0 ||
      sf_save_u32(header + 8u) != size ||
      sf_save_u32(header + 12u) != expected_version) return false;
  reader->extension_size = size;
  reader->extension_version = expected_version;
  reader->extension_running = sf_save_u32(header + 16u) != 0u;
  reader->extension_has_mine_count = size >= 24u;
  if (reader->extension_has_mine_count)
    reader->extension_mine_count = (int32_t) sf_save_u32(header + 20u);
  reader->extension_present = true;
  return true;
}

static bool sf_save_inspect_extension(SfSavePayloadReader *reader) {
  uint8_t tail[4];
  uint32_t size;
  if (reader->remaining >= 28u && sf_save_decode_at(
        reader, reader->remaining - 4u, tail, sizeof(tail))) {
    size = sf_save_u32(tail);
    if (size >= 28u && size <= reader->remaining &&
        sf_save_extension_header(reader, size, 4u)) return true;
  }
  if (reader->remaining >= 24u &&
      sf_save_extension_header(reader, 24u, 3u)) return true;
  if (reader->remaining >= 20u) {
    uint8_t header[16];
    const uint32_t start = reader->remaining - 20u;
    if (sf_save_decode_at(reader, start, header, sizeof(header)) &&
        memcmp(header, "OSFST01", 7u) == 0 && header[7] == 0u &&
        sf_save_u32(header + 8u) == 20u &&
        (sf_save_u32(header + 12u) == 1u ||
         sf_save_u32(header + 12u) == 2u))
      return sf_save_extension_header(
        reader, 20u, sf_save_u32(header + 12u));
  }
  return true;
}

bool sf_save_payload_open(
    SfSavePayloadReader *reader, const char *path,
    uint8_t player_record[SF_SAVE_PLAYER_RECORD_SIZE], bool *has_envelope) {
  static const uint8_t signature[16] = {
    'S', 'h', 'a', 'd', 'o', 'w', 'F', 'l',
    'a', 'r', 'e', '0', '0', '0', '5', 0
  };
  uint8_t header[16];
  uint8_t envelope[SF_SAVE_ENVELOPE_SIZE];
  long file_size;
  if (!reader || !path || !player_record || !has_envelope) return false;
  memset(reader, 0, sizeof(*reader));
  *has_envelope = false;
  reader->file = fopen(path, "rb");
  if (!reader->file || fseek(reader->file, 0, SEEK_END) != 0) goto failed;
  file_size = ftell(reader->file);
  if (file_size < 0 || fseek(reader->file, 0, SEEK_SET) != 0 ||
      fread(header, 1u, sizeof(header), reader->file) != sizeof(header) ||
      memcmp(header, signature, sizeof(signature)) != 0 ||
      fread(player_record, 1u, SF_SAVE_PLAYER_RECORD_SIZE, reader->file) !=
        SF_SAVE_PLAYER_RECORD_SIZE) goto failed;
  if ((unsigned long) file_size == SF_SAVE_PLAIN_SIZE) return true;
  if ((unsigned long) file_size < SF_SAVE_PLAIN_SIZE + SF_SAVE_ENVELOPE_SIZE ||
      fread(envelope, 1u, sizeof(envelope), reader->file) != sizeof(envelope))
    goto failed;
  reader->remaining = sf_save_u32(envelope);
  reader->xor_key = envelope[4];
  reader->expected_checksum = sf_save_u32(envelope + 5u);
  if (reader->remaining > SF_SAVE_MAXIMUM_PAYLOAD ||
      (unsigned long) file_size != SF_SAVE_PLAIN_SIZE +
        SF_SAVE_ENVELOPE_SIZE + reader->remaining) goto failed;
  sf_save_build_substitution(reader->substitution);
  if (!sf_save_inspect_extension(reader)) goto failed;
  *has_envelope = true;
  return true;
failed:
  sf_save_payload_close(reader);
  return false;
}

bool sf_save_payload_read(
    SfSavePayloadReader *reader, void *bytes, size_t size) {
  uint8_t *output = (uint8_t *) bytes;
  size_t index;
  if (!reader || !reader->file || (!bytes && size > 0u) ||
      size > reader->remaining) return false;
  for (index = 0u; index < size; ++index) {
    const int encoded = fgetc(reader->file);
    uint8_t decoded;
    if (encoded == EOF) return false;
    decoded = (uint8_t) (
      reader->substitution[(uint8_t) encoded] ^ reader->xor_key);
    output[index] = decoded;
    reader->checksum += decoded < 128u
      ? decoded : (uint32_t) ((int32_t) decoded - 256);
  }
  reader->remaining -= (uint32_t) size;
  return true;
}

bool sf_save_payload_skip(SfSavePayloadReader *reader, size_t size) {
  uint8_t bytes[64];
  while (size > 0u) {
    const size_t chunk = size < sizeof(bytes) ? size : sizeof(bytes);
    if (!sf_save_payload_read(reader, bytes, chunk)) return false;
    size -= chunk;
  }
  return true;
}

uint32_t sf_save_payload_content_remaining(
    const SfSavePayloadReader *reader) {
  if (!reader || reader->extension_size > reader->remaining) return 0u;
  return reader->remaining - reader->extension_size;
}

bool sf_save_payload_finish(SfSavePayloadReader *reader) {
  if (!reader || !reader->file ||
      !sf_save_payload_skip(reader, reader->remaining)) return false;
  return reader->checksum == reader->expected_checksum &&
    fgetc(reader->file) == EOF;
}

void sf_save_payload_close(SfSavePayloadReader *reader) {
  if (!reader) return;
  if (reader->file) fclose(reader->file);
  memset(reader, 0, sizeof(*reader));
}
