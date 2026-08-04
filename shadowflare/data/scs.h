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

#ifndef SHADOWFLARE_DATA_SCS_H
#define SHADOWFLARE_DATA_SCS_H

#include <stdbool.h>
#include <stdint.h>

#define SF_SCS_FLAG_LIMIT 256u
#define SF_SCS_STATUS_LIMIT 256u
#define SF_SCS_MESSAGE_LIMIT 320u
#define SF_SCS_MESSAGE_BYTES_LIMIT (32u * 1024u)
#define SF_SCS_SENTENCE_LIMIT 512u
#define SF_SCS_COMMAND_LIMIT 2048u
#define SF_SCS_OPERAND_LIMIT 8192u

typedef struct SfScsFlag {
  int32_t id;
  int32_t initial_value;
} SfScsFlag;

typedef struct SfScsStatus {
  int32_t kind;
  int32_t character_number;
  int32_t sentence;
  bool networked;
} SfScsStatus;

typedef struct SfScsMessage {
  int32_t id;
  uint16_t offset;
  uint16_t length;
} SfScsMessage;

typedef struct SfScsSentence {
  uint16_t first_command;
  uint16_t command_count;
} SfScsSentence;

typedef struct SfScsCommand {
  int32_t opcode;
  uint16_t first_operand;
  uint16_t operand_count;
} SfScsCommand;

typedef struct SfScsOperand {
  int32_t type;
  int32_t value;
} SfScsOperand;

typedef struct SfScsScript {
  SfScsFlag temporary_flags[SF_SCS_FLAG_LIMIT];
  SfScsMessage messages[SF_SCS_MESSAGE_LIMIT];
  char message_bytes[SF_SCS_MESSAGE_BYTES_LIMIT];
  SfScsStatus statuses[SF_SCS_STATUS_LIMIT];
  SfScsSentence sentences[SF_SCS_SENTENCE_LIMIT];
  SfScsCommand commands[SF_SCS_COMMAND_LIMIT];
  SfScsOperand operands[SF_SCS_OPERAND_LIMIT];
  uint16_t temporary_flag_count;
  uint16_t message_count;
  uint16_t message_bytes_count;
  uint16_t status_count;
  uint16_t sentence_count;
  uint16_t command_count;
  uint16_t operand_count;
} SfScsScript;

bool sf_scs_load(const char *path, SfScsScript *script);
const SfScsSentence *sf_scs_sentence(
  const SfScsScript *script, int32_t index);
const SfScsMessage *sf_scs_message(
  const SfScsScript *script, int32_t id);
const char *sf_scs_message_text(
  const SfScsScript *script, const SfScsMessage *message);

#endif
