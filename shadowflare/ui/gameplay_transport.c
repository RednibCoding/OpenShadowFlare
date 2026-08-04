/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of OpenShadowFlare.
 *
 * OpenShadowFlare is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenShadowFlare is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with OpenShadowFlare. If not, see <https://www.gnu.org/licenses/>.
 */

#include "ui/gameplay_transport.h"

#include "ui/gameplay_status_pattern.h"

#include <stdio.h>
#include <string.h>

static uint8_t sf_transport_enabled_rows(
    const SfTransportCatalog *catalog,
    const SfScenarioProgressState *progress,
    uint8_t *rows) {
  uint8_t count = 0u;
  uint8_t row;
  if (!catalog || !progress || !rows) return 0u;
  for (row = 0u; row < catalog->count; ++row)
    if (row < progress->transport_count &&
        progress->transport_values[row] != 0)
      rows[count++] = row;
  return count;
}

static uint8_t sf_transport_page_count(uint8_t enabled_count) {
  const uint8_t pages = (uint8_t) (
    (enabled_count + SF_GAMEPLAY_TRANSPORT_ROWS_PER_PAGE - 1u) /
      SF_GAMEPLAY_TRANSPORT_ROWS_PER_PAGE);
  return pages == 0u ? 1u : pages;
}

static bool sf_transport_inside(
    const SfGameInput *input, int left, int top, int right, int bottom) {
  return input->pointer_active && input->pointer_x >= left &&
    input->pointer_x < right && input->pointer_y >= top &&
    input->pointer_y < bottom;
}

static int16_t sf_transport_destination_at(
    const SfGameplayTransportUi *transport,
    const uint8_t *enabled, uint8_t enabled_count,
    int pointer_x, int pointer_y) {
  uint8_t visible;
  uint8_t index;
  if (pointer_x < 32 || pointer_x >= 289) return -1;
  visible = (uint8_t) (transport->page *
    SF_GAMEPLAY_TRANSPORT_ROWS_PER_PAGE);
  for (index = 0u; index < SF_GAMEPLAY_TRANSPORT_ROWS_PER_PAGE &&
       visible + index < enabled_count; ++index) {
    const int top = 63 + index * 30;
    if (pointer_y >= top && pointer_y < top + 23)
      return enabled[visible + index];
  }
  return -1;
}

void sf_gameplay_transport_init(SfGameplayTransportUi *transport) {
  if (!transport) return;
  memset(transport, 0, sizeof(*transport));
  transport->service_argument = -1;
  transport->hovered_destination = -1;
}

void sf_gameplay_transport_open(
    SfGameplayTransportUi *transport, int32_t service_argument) {
  if (!transport) return;
  transport->service_argument = service_argument;
  transport->hovered_destination = -1;
  transport->page = 0u;
  transport->active = true;
}

void sf_gameplay_transport_close(SfGameplayTransportUi *transport) {
  sf_gameplay_transport_init(transport);
}

bool sf_gameplay_transport_input_resolve(
    SfGameplayTransportUi *transport,
    const SfTransportCatalog *catalog,
    const SfScenarioProgressState *progress,
    SfGameInput *input) {
  uint8_t enabled[SF_TRANSPORT_DESTINATION_COUNT];
  uint8_t enabled_count;
  uint8_t pages;
  int16_t hovered;
  bool changed = false;
  if (!transport || !catalog || !progress || !input || !transport->active)
    return false;
  enabled_count = sf_transport_enabled_rows(catalog, progress, enabled);
  pages = sf_transport_page_count(enabled_count);
  if (transport->page >= pages) {
    transport->page = (uint8_t) (pages - 1u);
    changed = true;
  }
  hovered = input->pointer_active
    ? sf_transport_destination_at(
        transport, enabled, enabled_count,
        input->pointer_x, input->pointer_y) : -1;
  if (transport->hovered_destination != hovered) {
    transport->hovered_destination = hovered;
    changed = true;
  }
  if (input->cancel_pressed) {
    sf_gameplay_transport_close(transport);
    input->cancel_pressed = false;
    return true;
  }
  if (input->pointer_primary_pressed &&
      sf_transport_inside(input, 0, 0, 320, 412))
    input->pointer_over_gameplay_ui = true;
  if (!input->pointer_primary_pressed) return changed;
  if (transport->page > 0u &&
      sf_transport_inside(input, 28, 368, 93, 387)) {
    --transport->page;
    transport->hovered_destination = -1;
    input->interface_sound = 58u;
    return true;
  }
  if (transport->page + 1u < pages &&
      sf_transport_inside(input, 224, 368, 289, 387)) {
    ++transport->page;
    transport->hovered_destination = -1;
    input->interface_sound = 58u;
    return true;
  }
  if (hovered >= 0) {
    input->transport_destination = (int8_t) hovered;
    input->transport_selected = true;
    input->interface_sound = 58u;
    sf_gameplay_transport_close(transport);
    return true;
  }
  return changed;
}

static void sf_transport_draw_text(
    SfRenderer *renderer, const SfIndexedImage *font,
    const char *text, int x, int y, uint16_t strength) {
  const uint8_t channel = (uint8_t) (strength > 31u ? 31u : strength);
  const uint16_t color = sf_rgb555(channel, channel, channel);
  sf_renderer_draw_text(renderer, font, text, x + 1, y + 1, 0u, 1000u);
  sf_renderer_draw_text(renderer, font, text, x, y, color, 1000u);
}

void sf_gameplay_transport_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfScenarioProgressState *progress,
    const SfGameplayTransportUi *transport, const SfRect *clip) {
  uint8_t enabled[SF_TRANSPORT_DESTINATION_COUNT];
  uint8_t enabled_count;
  uint8_t pages;
  uint8_t first;
  uint8_t index;
  const SfIndexedImage *font;
  if (!renderer || !assets || !progress || !transport ||
      !transport->active || assets->font.image_count == 0u) return;
  if (clip && (clip->x >= 320 || clip->x + clip->width <= 0 ||
               clip->y >= 412 || clip->y + clip->height <= 0)) return;
  font = &assets->font.images[0].image;
  enabled_count = sf_transport_enabled_rows(
    &assets->transports, progress, enabled);
  pages = sf_transport_page_count(enabled_count);
  first = (uint8_t) (
    transport->page * SF_GAMEPLAY_TRANSPORT_ROWS_PER_PAGE);
  sf_renderer_fill_rect(renderer, (SfRect) {0, 0, 320, 412}, 0u);
  sf_gameplay_status_pattern_draw(
    renderer, &assets->inventory_panel, 13u, 0, 0, NULL);
  for (index = 0u; index < SF_GAMEPLAY_TRANSPORT_ROWS_PER_PAGE &&
       first + index < enabled_count; ++index) {
    const uint8_t row = enabled[first + index];
    const int y = 59 + index * 30;
    const bool hovered = transport->hovered_destination == row;
    sf_gameplay_status_pattern_draw(
      renderer, &assets->inventory_panel, hovered ? 24u : 22u, 52, y, NULL);
    sf_transport_draw_text(
      renderer, font, assets->transports.destinations[row].name,
      79, y + 6, hovered ? 28u : 16u);
  }
  if (transport->page > 0u)
    sf_gameplay_status_pattern_draw(
      renderer, &assets->inventory_panel, 11u, 0, 0, NULL);
  if (transport->page + 1u < pages)
    sf_gameplay_status_pattern_draw(
      renderer, &assets->inventory_panel, 12u, 0, 0, NULL);
  if (enabled_count > 0u) {
    char text[16];
    const int length = snprintf(
      text, sizeof(text), "%u / %u",
      (unsigned) transport->page + 1u, (unsigned) pages);
    if (length > 0 && (size_t) length < sizeof(text))
      sf_transport_draw_text(
        renderer, font, text, 84 + (153 - length * 6) / 2, 370, 28u);
  }
}
