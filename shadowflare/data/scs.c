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

#include "data/scs.h"

#include <stdio.h>
#include <string.h>

#define SF_SCS_MESSAGE_SIZE_LIMIT (16u * 1024u * 1024u)

static bool sf_scs_read(FILE *file, void *output, size_t size) {
  return size == 0u || fread(output, 1u, size, file) == size;
}

static bool sf_scs_skip(FILE *file, uint32_t size) {
  return size <= UINT32_C(0x7fffffff) &&
    fseek(file, (long) size, SEEK_CUR) == 0;
}

static bool sf_scs_i32(FILE *file, int32_t *value) {
  uint8_t bytes[4];
  uint32_t raw;
  if (!sf_scs_read(file, bytes, sizeof(bytes))) return false;
  raw = (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
  *value = (int32_t) raw;
  return true;
}

static bool sf_scs_count(
    FILE *file, uint32_t limit, uint32_t *count) {
  int32_t value;
  if (!sf_scs_i32(file, &value) || value < 0 ||
      (uint32_t) value > limit) return false;
  *count = (uint32_t) value;
  return true;
}

static bool sf_scs_flags(
    FILE *file, SfScsFlag *flags, uint32_t capacity,
    uint16_t *stored_count) {
  uint32_t count;
  uint32_t index;
  if (!sf_scs_count(file, capacity, &count)) return false;
  for (index = 0u; index < count; ++index) {
    int32_t id;
    int32_t value;
    if (!sf_scs_i32(file, &id) || !sf_scs_i32(file, &value)) return false;
    if (flags) {
      flags[index].id = id;
      flags[index].initial_value = value;
    }
  }
  if (stored_count) *stored_count = (uint16_t) count;
  return true;
}

bool sf_scs_load(const char *path, SfScsScript *script) {
  static const char expected_header[12] = "ScenaScriptV";
  FILE *file;
  char header[16];
  uint32_t count;
  uint32_t index;
  bool success = false;
  if (!path || !script) return false;
  memset(script, 0, sizeof(*script));
  file = fopen(path, "rb");
  if (!file) return false;
  if (!sf_scs_read(file, header, sizeof(header)) ||
      memcmp(header, expected_header, sizeof(expected_header)) != 0 ||
      !sf_scs_flags(
        file, script->temporary_flags, SF_SCS_FLAG_LIMIT,
        &script->temporary_flag_count) ||
      !sf_scs_flags(file, NULL, SF_SCS_FLAG_LIMIT, NULL) ||
      !sf_scs_count(file, UINT16_MAX, &count)) goto done;
  for (index = 0u; index < count; ++index) {
    int32_t ignored_id;
    int32_t size;
    if (!sf_scs_i32(file, &ignored_id) || !sf_scs_i32(file, &size) ||
        size < 0 || (uint32_t) size > SF_SCS_MESSAGE_SIZE_LIMIT ||
        !sf_scs_skip(file, (uint32_t) size)) goto done;
  }
  if (!sf_scs_count(file, SF_SCS_STATUS_LIMIT, &count)) goto done;
  script->status_count = (uint16_t) count;
  for (index = 0u; index < count; ++index) {
    SfScsStatus *status = &script->statuses[index];
    int32_t networked;
    if (!sf_scs_i32(file, &networked) ||
        !sf_scs_i32(file, &status->kind) ||
        !sf_scs_i32(file, &status->character_number) ||
        !sf_scs_i32(file, &status->sentence)) goto done;
    status->networked = networked != 0;
  }
  if (!sf_scs_count(file, SF_SCS_SENTENCE_LIMIT, &count)) goto done;
  script->sentence_count = (uint16_t) count;
  for (index = 0u; index < count; ++index) {
    SfScsSentence *sentence = &script->sentences[index];
    uint32_t command_count;
    uint32_t command_index;
    if (!sf_scs_count(file, UINT16_MAX, &command_count) ||
        script->command_count > SF_SCS_COMMAND_LIMIT - command_count)
      goto done;
    sentence->first_command = script->command_count;
    sentence->command_count = (uint16_t) command_count;
    for (command_index = 0u; command_index < command_count;
         ++command_index) {
      SfScsCommand *command = &script->commands[script->command_count++];
      uint32_t operand_count;
      uint32_t operand_index;
      if (!sf_scs_i32(file, &command->opcode) ||
          !sf_scs_count(file, UINT16_MAX, &operand_count) ||
          script->operand_count > SF_SCS_OPERAND_LIMIT - operand_count)
        goto done;
      command->first_operand = script->operand_count;
      command->operand_count = (uint16_t) operand_count;
      for (operand_index = 0u; operand_index < operand_count;
           ++operand_index) {
        SfScsOperand *operand = &script->operands[script->operand_count++];
        if (!sf_scs_i32(file, &operand->type) ||
            !sf_scs_i32(file, &operand->value)) goto done;
      }
    }
  }
  success = fgetc(file) == EOF && !ferror(file);
done:
  fclose(file);
  if (!success) memset(script, 0, sizeof(*script));
  return success;
}

const SfScsSentence *sf_scs_sentence(
    const SfScsScript *script, int32_t index) {
  return script && index >= 0 && index < script->sentence_count
    ? &script->sentences[index] : NULL;
}
