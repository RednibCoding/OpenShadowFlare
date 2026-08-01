#ifndef OPENSHADOWFLARE_GAMEPLAY_EQUIPMENT_COLOR_HPP
#define OPENSHADOWFLARE_GAMEPLAY_EQUIPMENT_COLOR_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace osf {

enum class EquipmentColorTarget : std::size_t {
    main_hand,
    off_hand,
    body,
    count,
    none = count,
};

struct GameplayEquipmentColorInput {
    bool cancel_pressed = false;
    bool pointer_primary_pressed = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
};

struct GameplayEquipmentColorResult {
    bool accepted = false;
    bool cancelled = false;
    bool color_changed = false;
    bool play_move_sound = false;
    bool pointer_consumed = false;
    EquipmentColorTarget target = EquipmentColorTarget::main_hand;
    std::int32_t color = -1;
};

class GameplayEquipmentColor {
public:
    static constexpr std::size_t target_count =
        static_cast<std::size_t>(EquipmentColorTarget::count);

    void open(
        const std::array<bool, target_count>& available,
        const std::array<std::int32_t, target_count>& colors);
    void close();
    GameplayEquipmentColorResult update(
        const GameplayEquipmentColorInput& input);

    bool active() const;
    EquipmentColorTarget selectedTarget() const;
    std::int32_t selectedColor() const;
    std::int32_t previewChart() const;
    bool acceptHovered() const;
    bool cancelHovered() const;
    const std::array<std::int32_t, target_count>& originalColors() const;

private:
    static bool inside(
        std::int32_t x,
        std::int32_t y,
        std::int32_t left,
        std::int32_t top,
        std::int32_t right,
        std::int32_t bottom);
    std::int32_t colorAt(
        std::int32_t pointer_x,
        std::int32_t pointer_y) const;

    bool active_ = false;
    std::array<bool, target_count> available_{};
    std::array<std::int32_t, target_count> colors_{{-1, -1, -1}};
    std::array<std::int32_t, target_count> original_colors_{{-1, -1, -1}};
    EquipmentColorTarget selected_target_ = EquipmentColorTarget::none;
    std::int32_t preview_chart_ = 7;
    bool accept_hovered_ = false;
    bool cancel_hovered_ = false;
};

}  // namespace osf

#endif
