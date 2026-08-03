#ifndef OPENSHADOWFLARE_PLAYER_APPEARANCE_HPP
#define OPENSHADOWFLARE_PLAYER_APPEARANCE_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {

class ItemDatabase;
class PlayerEquipment;

class PlayerAppearance {
public:
    void clear();
    void refresh(
        std::size_t part_count,
        const PlayerEquipment& equipment,
        const ItemDatabase& item_database);

    bool partEnabled(std::size_t part) const;
    const std::vector<std::uint8_t>& enabledParts() const;
    std::int32_t redStrength(std::size_t part) const;
    std::int32_t greenStrength(std::size_t part) const;
    std::int32_t blueStrength(std::size_t part) const;

private:
    std::vector<std::uint8_t> enabled_;
    std::vector<std::int32_t> red_strengths_;
    std::vector<std::int32_t> green_strengths_;
    std::vector<std::int32_t> blue_strengths_;
};

}  // namespace osf

#endif
