#include "gameplay_magic.hpp"

#include <cstddef>

namespace osf {
namespace {

constexpr MagicBarSlotRegion kPreviousPage{
    16, 335, 33, 16,
};
constexpr MagicBarSlotRegion kNextPage{
    270, 335, 34, 16,
};
constexpr MagicBarSlotRegion kStatusTab{
    0, 0, 160, 37,
};

}  // namespace

bool GameplayMagicModel::learned(
    std::int32_t spell) const {
    return spell >= 0 &&
           static_cast<std::size_t>(spell) <
               availability.size() &&
           availability[
               static_cast<std::size_t>(spell)] == 3;
}

std::int32_t GameplayMagicModel::barSlot(
    std::int32_t slot) const {
    return slot >= 0 &&
           static_cast<std::size_t>(slot) <
               bar_slots.size()
        ? bar_slots[static_cast<std::size_t>(slot)]
        : -1;
}

void GameplayMagic::open() {
    active_ = true;
    page_ = 0;
    held_spell_ = -1;
}

void GameplayMagic::close() {
    active_ = false;
    page_ = 0;
    held_spell_ = -1;
}

GameplayMagicResult GameplayMagic::update(
    const GameplayMagicInput& input,
    const GameplayMagicModel& model) {
    GameplayMagicResult result;
    pointer_x_ = input.pointer_x;
    pointer_y_ = input.pointer_y;

    if (input.toggle_pressed) {
        if (active_) {
            close();
        } else {
            open();
        }
        result.pointer_consumed = true;
        result.play_move_sound = true;
        return result;
    }
    if (active_ && input.close_pressed) {
        close();
        result.pointer_consumed = true;
        result.play_move_sound = true;
        return result;
    }

    if (held_spell_ >= 0 &&
        !input.pointer_primary_down) {
        const std::int32_t slot =
            panelBarSlotAt(
                input.pointer_x,
                input.pointer_y);
        if (slot >= 0 &&
            model.learned(held_spell_)) {
            result.assign_bar_slot = slot;
            result.assign_spell = held_spell_;
            result.play_move_sound = true;
        }
        held_spell_ = -1;
        result.pointer_consumed = active_ &&
            input.pointer_x < 320 &&
            input.pointer_y < 412;
    }

    if (active_ &&
        input.pointer_primary_pressed) {
        result.pointer_consumed =
            input.pointer_x < 320 &&
            input.pointer_y < 412;
        if (contains(
                kStatusTab,
                input.pointer_x,
                input.pointer_y)) {
            close();
            result.switch_to_status = true;
            result.play_move_sound = true;
            return result;
        }
        if (page_ > 0 &&
            contains(
                kPreviousPage,
                input.pointer_x,
                input.pointer_y)) {
            --page_;
            result.play_move_sound = true;
            return result;
        }
        if (page_ + 1 < page_count &&
            contains(
                kNextPage,
                input.pointer_x,
                input.pointer_y)) {
            ++page_;
            result.play_move_sound = true;
            return result;
        }

        const std::int32_t spell =
            panelSpellAt(
                page_,
                input.pointer_x,
                input.pointer_y);
        if (spell >= 0 && model.learned(spell)) {
            held_spell_ = spell;
            result.play_pick_sound = true;
            return result;
        }

        const std::int32_t slot =
            panelBarSlotAt(
                input.pointer_x,
                input.pointer_y);
        if (slot >= 0) {
            const std::int32_t assigned =
                model.barSlot(slot);
            if (assigned >= 0 &&
                model.learned(assigned)) {
                held_spell_ = assigned;
                result.play_pick_sound = true;
            }
            return result;
        }
    }

    if (!input.pointer_primary_pressed ||
        (input.left_panel_active &&
         input.right_panel_active)) {
        return result;
    }

    const auto slots = persistentBarSlots(
        model,
        input.left_panel_active,
        input.right_panel_active);
    for (std::size_t slot = 0;
         slot < slots.size();
         ++slot) {
        if (!contains(
                slots[slot],
                input.pointer_x,
                input.pointer_y)) {
            continue;
        }
        result.pointer_consumed = true;
        const std::int32_t spell =
            model.barSlot(
                static_cast<std::int32_t>(slot));
        if (spell >= 0 &&
            model.learned(spell) &&
            model.selected_spell != spell) {
            result.select_spell = spell;
            result.play_move_sound = true;
        }
        return result;
    }

    const MagicBarSlotRegion target =
        persistentTargetRegion(
            model,
            input.left_panel_active,
            input.right_panel_active);
    if (contains(
            target,
            input.pointer_x,
            input.pointer_y)) {
        result.pointer_consumed = true;
        result.toggle_targeting = true;
        result.play_move_sound = true;
    }
    return result;
}

bool GameplayMagic::active() const {
    return active_;
}

std::int32_t GameplayMagic::page() const {
    return page_;
}

std::int32_t GameplayMagic::heldSpell() const {
    return held_spell_;
}

std::int32_t GameplayMagic::pointerX() const {
    return pointer_x_;
}

std::int32_t GameplayMagic::pointerY() const {
    return pointer_y_;
}

std::int32_t GameplayMagic::hoveredSpell() const {
    if (!active_ ||
        pointer_x_ <= 59 ||
        pointer_x_ >= 228) {
        return -1;
    }
    for (std::int32_t row = 0;
         row < spells_per_page;
         ++row) {
        const std::int32_t top = 66 + row * 48;
        if (pointer_y_ >= top &&
            pointer_y_ < top + 12) {
            const std::int32_t spell =
                page_ * spells_per_page + row;
            return spell <
                    static_cast<std::int32_t>(
                        GameplayMagicModel::spell_count)
                ? spell
                : -1;
        }
    }
    return -1;
}

std::array<MagicBarSlotRegion, 8>
GameplayMagic::persistentBarSlots(
    const GameplayMagicModel& model,
    bool left_panel_active,
    bool right_panel_active) {
    std::array<MagicBarSlotRegion, 8> result{};
    if (left_panel_active && right_panel_active) {
        return result;
    }
    std::int32_t x = left_panel_active
        ? 344
        : (right_panel_active ? 124 : 224);
    for (std::int32_t slot = 0;
         slot < 8;
         ++slot) {
        if (slot % 4 == 0) {
            x += 4;
        }
        const bool selected =
            model.barSlot(slot) >= 0 &&
            model.barSlot(slot) ==
                model.selected_spell;
        result[static_cast<std::size_t>(slot)] = {
            x,
            selected ? 382 : 392,
            selected ? 26 : 16,
            selected ? 26 : 16,
        };
        x += selected ? 26 : 16;
    }
    return result;
}

MagicBarSlotRegion
GameplayMagic::persistentTargetRegion(
    const GameplayMagicModel& model,
    bool left_panel_active,
    bool right_panel_active) {
    const auto slots = persistentBarSlots(
        model,
        left_panel_active,
        right_panel_active);
    if (left_panel_active && right_panel_active) {
        return {};
    }
    const MagicBarSlotRegion& last = slots.back();
    return {
        last.x + last.width + 4,
        model.targeting ? 392 : 382,
        model.targeting ? 16 : 26,
        model.targeting ? 16 : 26,
    };
}

std::int32_t GameplayMagic::panelSpellAt(
    std::int32_t page,
    std::int32_t x,
    std::int32_t y) {
    if (x < 24 || x >= 56) {
        return -1;
    }
    for (std::int32_t row = 0;
         row < spells_per_page;
         ++row) {
        const std::int32_t top = 56 + row * 48;
        if (y >= top && y < top + 32) {
            const std::int32_t spell =
                page * spells_per_page + row;
            return spell <
                    static_cast<std::int32_t>(
                        GameplayMagicModel::spell_count)
                ? spell
                : -1;
        }
    }
    return -1;
}

std::int32_t GameplayMagic::panelBarSlotAt(
    std::int32_t x,
    std::int32_t y) {
    if (y < 356 || y >= 388) {
        return -1;
    }
    for (std::int32_t slot = 0;
         slot < 8;
         ++slot) {
        const std::int32_t left =
            29 + slot * 32;
        if (x >= left && x < left + 32) {
            return slot;
        }
    }
    return -1;
}

bool GameplayMagic::contains(
    const MagicBarSlotRegion& region,
    std::int32_t x,
    std::int32_t y) {
    return region.width > 0 &&
           region.height > 0 &&
           x >= region.x &&
           x < region.x + region.width &&
           y >= region.y &&
           y < region.y + region.height;
}

}  // namespace osf
