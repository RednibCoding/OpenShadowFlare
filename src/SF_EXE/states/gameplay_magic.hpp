#ifndef OPENSHADOWFLARE_GAMEPLAY_MAGIC_HPP
#define OPENSHADOWFLARE_GAMEPLAY_MAGIC_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace osf {

struct GameplayMagicModel {
    static constexpr std::size_t spell_count = 22;
    static constexpr std::size_t bar_slot_count = 8;

    std::array<std::int32_t, spell_count> availability{};
    std::array<std::int32_t, bar_slot_count> bar_slots{};
    std::int32_t selected_spell = -1;
    bool targeting = false;

    bool learned(std::int32_t spell) const;
    std::int32_t barSlot(std::int32_t slot) const;
};

struct GameplayMagicInput {
    bool toggle_pressed = false;
    bool close_pressed = false;
    bool pointer_primary_pressed = false;
    bool pointer_primary_down = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
    bool left_panel_active = false;
    bool right_panel_active = false;
};

struct GameplayMagicResult {
    bool pointer_consumed = false;
    bool play_pick_sound = false;
    bool play_move_sound = false;
    std::int32_t assign_bar_slot = -1;
    std::int32_t assign_spell = -1;
    std::int32_t select_spell = -2;
    bool toggle_targeting = false;
    bool switch_to_status = false;
};

struct MagicBarSlotRegion {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
};

class GameplayMagic {
public:
    static constexpr std::int32_t spells_per_page = 6;
    static constexpr std::int32_t page_count = 4;

    void open();
    void close();
    GameplayMagicResult update(
        const GameplayMagicInput& input,
        const GameplayMagicModel& model);

    bool active() const;
    std::int32_t page() const;
    std::int32_t heldSpell() const;
    std::int32_t pointerX() const;
    std::int32_t pointerY() const;
    std::int32_t hoveredSpell() const;

    static std::array<MagicBarSlotRegion, 8>
        persistentBarSlots(
            const GameplayMagicModel& model,
            bool left_panel_active,
            bool right_panel_active);
    static MagicBarSlotRegion persistentTargetRegion(
        const GameplayMagicModel& model,
        bool left_panel_active,
        bool right_panel_active);

private:
    static std::int32_t panelSpellAt(
        std::int32_t page,
        std::int32_t x,
        std::int32_t y);
    static std::int32_t panelBarSlotAt(
        std::int32_t x,
        std::int32_t y);
    static bool contains(
        const MagicBarSlotRegion& region,
        std::int32_t x,
        std::int32_t y);

    bool active_ = false;
    std::int32_t page_ = 0;
    std::int32_t held_spell_ = -1;
    std::int32_t pointer_x_ = 0;
    std::int32_t pointer_y_ = 0;
};

}  // namespace osf

#endif
