#include "states/gameplay_magic.hpp"
#include "states/gameplay_status.hpp"
#include "world/player_magic.hpp"
#include "world/world_scene.hpp"

#include <iostream>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

osf::GameplayMagicInput pointerPress(
    std::int32_t x,
    std::int32_t y) {
    osf::GameplayMagicInput input;
    input.pointer_primary_pressed = true;
    input.pointer_primary_down = true;
    input.pointer_x = x;
    input.pointer_y = y;
    return input;
}

osf::GameplayMagicModel model(
    const osf::PlayerMagic& magic) {
    osf::GameplayMagicModel result;
    result.availability =
        magic.state().availability;
    result.bar_slots =
        magic.state().bar_slots;
    result.selected_spell =
        magic.selectedSpell();
    result.targeting = magic.targeting();
    return result;
}

osf::GameplayMagicResult update(
    osf::GameplayMagic& panel,
    const osf::GameplayMagicInput& input,
    osf::PlayerMagic& magic) {
    const osf::GameplayMagicResult result =
        panel.update(input, model(magic));
    if (result.assign_bar_slot >= 0) {
        magic.assignBarSlot(
            result.assign_bar_slot,
            result.assign_spell);
    }
    if (result.select_spell != -2) {
        magic.selectSpell(result.select_spell);
    }
    if (result.toggle_targeting) {
        magic.setTargeting(!magic.targeting());
    }
    return result;
}

}  // namespace

int main() {
    osf::WorldScene debug_world;
    debug_world.configurePlayerDebugResources(true, true);
    if (!check(
            debug_world.playerInfiniteLife() &&
                debug_world.playerInfiniteMana() &&
                debug_world.playerCurrentLife() == 1 &&
                debug_world.playerCurrentMana() == 1 &&
                debug_world.playerData().currentLife() == 0 &&
                debug_world.playerData().currentMana() == 0,
            "The infinite resource overrides changed persistent player "
            "data.")) {
        return 1;
    }
    debug_world.configurePlayerDebugResources(false, false);

    osf::PlayerMagicState state;
    state.levels.fill(1);
    state.bar_slots.fill(-1);
    state.availability[0] = 3;
    state.availability[1] = 3;
    state.availability[6] = 3;

    osf::PlayerMagic magic;
    magic.restore(state);
    osf::GameplayMagic panel;

    osf::GameplayMagicInput toggle;
    toggle.toggle_pressed = true;
    if (!check(
            update(panel, toggle, magic).play_move_sound &&
                panel.active() &&
                panel.page() == 0,
            "The M-key transition did not open the retail Magic page.")) {
        return 1;
    }

    const osf::GameplayMagicResult picked =
        update(panel, pointerPress(32, 60), magic);
    if (!check(
            picked.pointer_consumed &&
                picked.play_pick_sound &&
                panel.heldSpell() == 0,
            "A learned page icon did not enter retail drag state.")) {
        return 1;
    }

    osf::GameplayMagicInput release;
    release.pointer_x = 70;
    release.pointer_y = 370;
    const osf::GameplayMagicResult assigned =
        update(panel, release, magic);
    if (!check(
            assigned.play_move_sound &&
                panel.heldSpell() == -1 &&
                magic.barSlot(1) == 0,
            "Dropping a spell did not assign the panel bar slot.")) {
        return 1;
    }

    update(panel, pointerPress(32, 60), magic);
    release.pointer_x = 102;
    const osf::GameplayMagicResult moved =
        update(panel, release, magic);
    if (!check(
            moved.play_move_sound &&
                magic.barSlot(1) == -1 &&
                magic.barSlot(2) == 0,
            "Moving a spell left a duplicate in the magic bar.")) {
        return 1;
    }

    const osf::GameplayMagicResult ignored =
        update(panel, pointerPress(32, 156), magic);
    if (!check(
            ignored.pointer_consumed &&
                !ignored.play_pick_sound &&
                panel.heldSpell() == -1,
            "An unavailable spell could be dragged.")) {
        return 1;
    }

    update(panel, pointerPress(280, 340), magic);
    if (!check(
            panel.page() == 1 &&
                panel.hoveredSpell() == -1,
            "The authored next-page control did not advance one page.")) {
        return 1;
    }
    update(panel, pointerPress(24, 340), magic);
    if (!check(
            panel.page() == 0,
            "The authored previous-page control did not return.")) {
        return 1;
    }

    const osf::GameplayMagicResult status_tab =
        update(panel, pointerPress(80, 18), magic);
    if (!check(
            status_tab.pointer_consumed &&
                status_tab.switch_to_status &&
                !panel.active(),
            "The Status tab did not switch away from the shared Magic "
            "window.")) {
        return 1;
    }
    osf::GameplayStatus status;
    status.open();
    const osf::GameplayStatusResult magic_tab =
        status.update({false, false, true, 240, 18});
    if (!check(
            magic_tab.pointer_consumed &&
                magic_tab.switch_to_magic &&
                !status.active(),
            "The Magic tab did not switch away from the shared Status "
            "window.")) {
        return 1;
    }

    panel.open();

    osf::GameplayMagicInput close;
    close.close_pressed = true;
    update(panel, close, magic);
    if (!check(
            !panel.active(),
            "Escape did not close the Magic panel.")) {
        return 1;
    }

    const auto slots =
        osf::GameplayMagic::persistentBarSlots(
            model(magic), false, false);
    const osf::GameplayMagicResult selected =
        update(
            panel,
            pointerPress(
                slots[2].x,
                slots[2].y),
            magic);
    if (!check(
            selected.pointer_consumed &&
                selected.play_move_sound &&
                magic.selectedSpell() == 0 &&
                !magic.targeting(),
            "The gameplay magic bar did not select its learned spell.")) {
        return 1;
    }

    const auto shifted =
        osf::GameplayMagic::persistentBarSlots(
            model(magic), true, false);
    const auto right_shifted =
        osf::GameplayMagic::persistentBarSlots(
            model(magic), false, true);
    if (!check(
            shifted[0].x == 348 &&
                right_shifted[0].x == 128 &&
                slots[0].x == 228,
            "The persistent bar does not follow retail panel offsets.")) {
        return 1;
    }

    const osf::MagicBarSlotRegion target =
        osf::GameplayMagic::persistentTargetRegion(
            model(magic), false, false);
    const osf::GameplayMagicResult targeting =
        update(
            panel,
            pointerPress(target.x, target.y),
            magic);
    return check(
               targeting.pointer_consumed &&
                   targeting.play_move_sound &&
                   magic.targeting() &&
                   magic.selectedSpell() == -1,
               "The magic/attack target toggle did not clear spell selection.")
        ? 0
        : 1;
}
