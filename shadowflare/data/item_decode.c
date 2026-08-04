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

#include "data/item_decode.h"

#include "data/rclib.h"

#include <stdio.h>
#include <string.h>

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

typedef struct SfItemDecodeContext {
  SfItemDecodedByte output;
  void *user;
  int32_t checksum;
} SfItemDecodeContext;

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

static bool sf_item_output(void *user, size_t offset, uint8_t value) {
  SfItemDecodeContext *context = (SfItemDecodeContext *) user;
  context->checksum += (int8_t) value;
  return context->output(context->user, offset, value);
}

bool sf_item_decode_stream(
    const char *path, SfItemDecodedByte output, void *user) {
  uint8_t header[24];
  uint8_t compression_header[16];
  SfItemDecodeContext context;
  FILE *file;
  uint32_t decoded_size;
  bool decoded;
  if (!path || !output) return false;
  file = fopen(path, "rb");
  if (!file) return false;
  if (fread(header, 1u, sizeof(header), file) != sizeof(header) ||
      memcmp(header, "SFItemDataV0000\x1a", 16u) != 0 ||
      sf_item_u32(header + 20u) != 1u ||
      fread(compression_header, 1u, sizeof(compression_header), file) !=
        sizeof(compression_header) ||
      memcmp(compression_header, "RCLIB-L", 7u) != 0 ||
      fseek(file, -16L, SEEK_CUR) != 0) {
    fclose(file);
    return false;
  }
  context.output = output;
  context.user = user;
  context.checksum = 0;
  decoded_size = sf_item_u32(compression_header + 8u);
  decoded = sf_rclib_decode_stream_to_transformed(
    file, decoded_size, sf_item_substitute, NULL,
    sf_item_output, &context);
  fclose(file);
  return decoded && context.checksum == (int32_t) sf_item_u32(header + 16u);
}
