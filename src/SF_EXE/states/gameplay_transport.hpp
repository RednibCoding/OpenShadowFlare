#ifndef OPENSHADOWFLARE_GAMEPLAY_TRANSPORT_HPP
#define OPENSHADOWFLARE_GAMEPLAY_TRANSPORT_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {

struct GameplayTransportInput {
    bool close_pressed = false;
    bool pointer_primary_pressed = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
};

struct GameplayTransportResult {
    std::int32_t selected_destination = -1;
    bool play_move_sound = false;
    bool pointer_consumed = false;
};

class GameplayTransport {
public:
    static constexpr std::int32_t entries_per_page = 10;

    void open();
    void close();
    GameplayTransportResult update(
        const GameplayTransportInput& input,
        const std::vector<std::int32_t>&
            enabled_destinations);

    bool active() const;
    std::int32_t page() const;
    std::int32_t pageCount(
        std::size_t enabled_destination_count) const;
    std::int32_t hoveredDestination() const;
    std::vector<std::int32_t> visibleDestinations(
        const std::vector<std::int32_t>&
            enabled_destinations) const;

private:
    std::int32_t destinationAt(
        std::int32_t pointer_x,
        std::int32_t pointer_y,
        const std::vector<std::int32_t>&
            enabled_destinations) const;
    void updateHover(
        std::int32_t pointer_x,
        std::int32_t pointer_y,
        const std::vector<std::int32_t>&
            enabled_destinations);

    bool active_ = false;
    std::int32_t page_ = 0;
    std::int32_t hovered_destination_ = -1;
};

}  // namespace osf

#endif
