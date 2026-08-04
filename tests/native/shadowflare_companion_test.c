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

#include "assets/gameplay_assets.h"
#include "core/arena.h"
#include "core/memory_budget.h"
#include "game/companion.h"
#include "game/player_companion.h"
#include "render/renderer.h"
#include "screens/gameplay_companion.h"
#include "ui/gameplay_companion_hud.h"
#include "ui/gameplay_companion_hud_input.h"

#include <stdio.h>
#include <string.h>

typedef union TestArena {
  long double alignment;
  void *pointer;
  uint8_t bytes[SF_MAIN_ARENA_BYTES];
} TestArena;

static TestArena test_arena;
static uint16_t test_pixels[640u * 480u];

static int check(bool condition, const char *message) {
  if (condition) return 0;
  fprintf(stderr, "%s\n", message);
  return 1;
}

static int test_profile(const char *root) {
  char path[1024];
  SfCompanionProfile profile;
  (void) snprintf(
    path, sizeof(path), "%s/System/Game/Parameter/Table.Tbd", root);
  if (check(sf_companion_profile_load(path, 0, 1, &profile),
            "Table 60 companion zero could not be decoded") ||
      check(strcmp(profile.name, "Kerberos") == 0,
            "the first retail companion name changed") ||
      check(profile.resource_id == 0 && profile.red_strength == 1000 &&
            profile.green_strength == 1000 && profile.blue_strength == 1000,
            "the first retail companion catalog values changed") ||
      check(profile.values[0] == 128 && profile.values[1] == 125 &&
            profile.values[2] == 225 && profile.values[3] == 400,
            "the first retail companion level values changed") ||
      check(sf_companion_profile_load(path, 4, 2, &profile) &&
            strcmp(profile.name, "Harley") == 0 &&
            profile.resource_id == 1 && profile.values[3] == 600,
            "companion level growth was not accumulated")) return 1;
  return 0;
}

static int test_progress(void) {
  SfPlayerCompanionProgress progress;
  int32_t levels[SF_COMPANION_COUNT] = {3, 2, 1, 1, 1, 1};
  int32_t experience[SF_COMPANION_COUNT] = {8, 7, 0, 0, 0, 0};
  sf_player_companion_progress_init(&progress);
  if (check(progress.type == 0 && sf_player_companion_level(&progress) == 1,
            "new companion progression does not begin at retail level one") ||
      check(sf_player_companion_progress_restore(
              &progress, 1, 27, levels, experience, SF_COMPANION_COUNT),
            "saved companion progression was rejected") ||
      check(progress.type == 1 && sf_player_companion_level(&progress) == 2 &&
            progress.experience[1] == 7 && progress.defeated_updates == 27,
            "saved companion progression was not retained")) return 1;
  return 0;
}

static int test_follow(const SfCompanionProfile *profile) {
  SfCollisionWorld collision_world = {NULL, NULL};
  SfCollisionQuery collision = {&collision_world, NULL,
                                SF_COMPANION_CHARACTER_NUMBER, 0u};
  SfPlayerState owner;
  SfCompanionState companion;
  uint8_t update;
  sf_player_init(&owner, 1u);
  sf_player_enter(&owner, (SfWorldPoint) {1000, 0}, 0u);
  if (!sf_companion_init(
        &companion, profile, (SfWorldPoint) {0, 0}, 0u, false)) return 1;
  sf_companion_update_follow(&companion, &owner, &collision);
  if (check(companion.motion == SF_COMPANION_RUNNING &&
            companion.position.x == profile->values[2] / 5,
            "a distant companion did not use its retail run speed")) return 1;
  owner.position = (SfWorldPoint) {300, 0};
  companion.position = (SfWorldPoint) {0, 0};
  companion.previous_position = companion.position;
  sf_companion_update_follow(&companion, &owner, &collision);
  if (check(companion.motion == SF_COMPANION_IDLE &&
            companion.close_linger_updates == 5u,
            "the retail close-distance idle was not selected")) return 1;
  owner.position = (SfWorldPoint) {500, 0};
  for (update = 0u; update < 5u; ++update)
    sf_companion_update_follow(&companion, &owner, &collision);
  if (check(companion.position.x == 0,
            "the close-distance linger ended too early")) return 1;
  sf_companion_update_follow(&companion, &owner, &collision);
  if (check(companion.motion == SF_COMPANION_WALKING &&
            companion.position.x == profile->values[1] / 5,
            "the companion did not resume at its retail walk speed")) return 1;
  owner.position = (SfWorldPoint) {5000, 7000};
  sf_companion_update_follow(&companion, &owner, &collision);
  if (check(companion.position.x == 5200 && companion.position.y == 7200 &&
            companion.previous_position.x == 5200,
            "the far-distance companion relocation differs from retail"))
    return 1;
  return 0;
}

static int test_obstacle_route(const SfCompanionProfile *profile) {
  const SfCollisionWorld collision_world = {NULL, NULL};
  const SfMovementBlocker blocker = {
    {400, 0}, {-100, -100, 100, 100}, 77};
  const SfCollisionQuery collision = {
    &collision_world, &blocker, SF_COMPANION_CHARACTER_NUMBER, 1u};
  SfPlayerState owner;
  SfCompanionState companion;
  bool detoured = false;
  uint8_t update;
  sf_player_init(&owner, 1u);
  sf_player_enter(&owner, (SfWorldPoint) {1000, 0}, 0u);
  if (!sf_companion_init(
        &companion, profile, (SfWorldPoint) {0, 0}, 0u, false)) return 1;
  for (update = 0u; update < 80u; ++update) {
    sf_companion_update_follow(&companion, &owner, &collision);
    if (companion.position.y != 0) detoured = true;
  }
  if (check(detoured,
            "the owned companion did not route around an actor blocker") ||
      check(companion.position.x > 580,
            "the owned companion remained caught on an actor blocker"))
    return 1;
  return 0;
}

static int test_assets_and_ui(const char *root) {
  SfArena arena;
  SfGameplayAssets assets;
  SfPlayerState player;
  SfCompanionState companion;
  SfItemReference retained[SF_GROUND_ITEM_DEFINITION_LIMIT];
  uint8_t retained_count;
  SfWorldRenderView view;
  SfRenderer renderer;
  SfGameInput input;
  size_t index;
  size_t changed = 0u;
  sf_arena_init(&arena, test_arena.bytes, sizeof(test_arena.bytes));
  sf_player_init(&player, 1u);
  if (!sf_player_required_item_definitions(
        &player, retained, SF_GROUND_ITEM_DEFINITION_LIMIT, &retained_count) ||
      !sf_gameplay_assets_load(
        &assets, root, 0, 0, player.gender, player.level,
        player.companions.type, sf_player_companion_level(&player.companions),
        player.appearance_parts, player.appearance_part_count,
        player.visible_items, player.visible_item_count,
        retained, retained_count, &arena) ||
      !sf_companion_init(
        &companion, &assets.companion_profile,
        (SfWorldPoint) {0, 0}, 0u, false) ||
      !sf_renderer_init(
        &renderer, test_pixels, sizeof(test_pixels), 640u, 480u)) return 1;
  if (check(assets.companion.resource_id == 0 &&
            assets.companion.artwork.pattern_count > 0u &&
            assets.companion.shadows.pattern_count > 0u,
            "the active PARTNER charts were not loaded")) return 1;
  memset(&view, 0, sizeof(view));
  view.camera_x = -320;
  view.camera_y = -240;
  sf_renderer_clear(&renderer, 0u);
  sf_gameplay_companion_draw(
    &renderer, &assets.companion, &companion, &view, 1000u, false, NULL);
  for (index = 0u; index < 640u * 480u; ++index)
    if (test_pixels[index] != 0u) ++changed;
  if (check(changed > 20u, "the owned companion did not render")) return 1;
  sf_renderer_clear(&renderer, 0u);
  sf_gameplay_companion_hud_draw(
    &renderer, &assets, &companion, 0u, NULL);
  changed = 0u;
  for (index = 640u * 393u; index < 640u * 409u; ++index)
    if (test_pixels[index] != 0u) ++changed;
  if (check(changed > 50u, "the companion HUD strip did not render")) return 1;
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_x = 50;
  input.pointer_y = 400;
  sf_gameplay_companion_hud_input_resolve(&input);
  if (check(input.pointer_over_gameplay_ui && input.companion_toggle_pressed,
            "the retail companion HUD hit rectangle is inactive")) return 1;
  sf_companion_toggle_activity(&companion);
  if (check(!companion.inactive,
            "the companion active/inactive state did not toggle")) return 1;
  return 0;
}

int main(void) {
  char root[768];
  char probe[1024];
  FILE *file;
  SfCompanionProfile profile;
  (void) snprintf(
    root, sizeof(root), "%s/tmp/ShadowFlare", OPENSHADOWFLARE_SOURCE_DIR);
  (void) snprintf(
    probe, sizeof(probe), "%s/System/Game/Parameter/Table.Tbd", root);
  file = fopen(probe, "rb");
  if (!file) return 0;
  fclose(file);
  if (test_profile(root) || test_progress()) return 1;
  if (!sf_companion_profile_load(probe, 0, 1, &profile) ||
      test_follow(&profile) || test_obstacle_route(&profile) ||
      test_assets_and_ui(root)) return 1;
  return 0;
}
