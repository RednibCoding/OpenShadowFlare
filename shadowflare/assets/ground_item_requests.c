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

#include "assets/ground_item_requests.h"

static int sf_ground_item_definition(
    const SfItemGroundDefinition *definitions, uint8_t count,
    uint8_t category, int32_t definition_id) {
  uint8_t index;
  for (index = 0u; index < count; ++index)
    if (definitions[index].category == category &&
        definitions[index].definition_id == definition_id) return index;
  return -1;
}

static bool sf_ground_item_request_value(
    const SfScsScript *script, const SfScsOperand *operand,
    int32_t *value) {
  uint16_t index;
  if (operand->type >= 0 && operand->type <= 2) {
    *value = operand->value;
    return true;
  }
  if (operand->type != 4) return false;
  for (index = 0u; index < script->temporary_flag_count; ++index) {
    if (script->temporary_flags[index].id == operand->value) {
      *value = script->temporary_flags[index].initial_value;
      return true;
    }
  }
  return false;
}

bool sf_ground_item_collect_definitions(
    const SfScsScript *script, SfItemGroundDefinition *definitions,
    uint8_t *definition_count) {
  uint16_t command_index;
  if (!script || !definitions || !definition_count) return false;
  *definition_count = 0u;
  for (command_index = 0u; command_index < script->command_count;
       ++command_index) {
    const SfScsCommand *command = &script->commands[command_index];
    const SfScsOperand *operands;
    int32_t category;
    int32_t definition_id;
    if (command->opcode != 10 || command->operand_count < 2u ||
        command->first_operand + command->operand_count >
          script->operand_count) continue;
    operands = &script->operands[command->first_operand];
    if (!sf_ground_item_request_value(
          script, &operands[0], &category) ||
        !sf_ground_item_request_value(
          script, &operands[1], &definition_id)) continue;
    if (category < 0 || definition_id < 0) continue;
    if (category > 4) return false;
    if (sf_ground_item_definition(
          definitions, *definition_count, (uint8_t) category,
          definition_id) >= 0) continue;
    if (*definition_count >= SF_GROUND_ITEM_DEFINITION_LIMIT) return false;
    definitions[*definition_count].category = (uint8_t) category;
    definitions[*definition_count].definition_id = definition_id;
    ++*definition_count;
  }
  return *definition_count > 0u;
}

static int sf_ground_item_resource_request(
    const SfGroundItemResourceRequest *requests, uint8_t count,
    int32_t resource_id) {
  uint8_t index;
  for (index = 0u; index < count; ++index)
    if (requests[index].resource_id == resource_id) return index;
  return -1;
}

bool sf_ground_item_collect_resources(
    const SfItemGroundDefinition *definitions, uint8_t definition_count,
    SfGroundItemResourceRequest *requests, uint8_t *request_count) {
  uint8_t definition_index;
  if (!definitions || !requests || !request_count) return false;
  *request_count = 0u;
  for (definition_index = 0u; definition_index < definition_count;
       ++definition_index) {
    const SfItemGroundDefinition *definition =
      &definitions[definition_index];
    int request = sf_ground_item_resource_request(
      requests, *request_count, definition->resource_id);
    uint8_t chart_index;
    if (definition->resource_id < 0 || definition->animation_chart < 0 ||
        definition->animation_chart > UINT16_MAX) return false;
    if (request < 0) {
      if (*request_count >= SF_GROUND_ITEM_RESOURCE_LIMIT) return false;
      request = (*request_count)++;
      requests[request].resource_id = definition->resource_id;
    }
    for (chart_index = 0u;
         chart_index < requests[request].chart_count; ++chart_index)
      if (requests[request].charts[chart_index] ==
          (uint16_t) definition->animation_chart) break;
    if (chart_index == requests[request].chart_count) {
      if (chart_index >= SF_GROUND_ITEM_DEFINITION_LIMIT) return false;
      requests[request].charts[chart_index] =
        (uint16_t) definition->animation_chart;
      ++requests[request].chart_count;
    }
  }
  return *request_count > 0u;
}
