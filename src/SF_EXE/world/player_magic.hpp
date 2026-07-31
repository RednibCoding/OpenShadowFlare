#ifndef OPENSHADOWFLARE_PLAYER_MAGIC_HPP
#define OPENSHADOWFLARE_PLAYER_MAGIC_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace osf {

struct PlayerMagicState {
    static constexpr std::size_t spell_count = 22;
    static constexpr std::size_t bar_slot_count = 8;

    std::array<std::int32_t, spell_count> availability{};
    std::array<std::int32_t, spell_count> levels{};
    std::array<std::int32_t, spell_count> experience{};
    std::array<std::int32_t, bar_slot_count> bar_slots{};
};

class PlayerMagic {
public:
    static constexpr std::size_t spell_count =
        PlayerMagicState::spell_count;
    static constexpr std::size_t bar_slot_count =
        PlayerMagicState::bar_slot_count;

    void initializeNew();
    void restore(const PlayerMagicState& state);
    void clear();

    const PlayerMagicState& state() const;
    bool learned(std::int32_t spell) const;
    std::int32_t availability(std::int32_t spell) const;
    std::int32_t level(std::int32_t spell) const;
    std::int32_t experience(std::int32_t spell) const;
    std::int32_t barSlot(std::int32_t slot) const;

private:
    static bool validSpell(std::int32_t spell);
    static bool validBarSlot(std::int32_t slot);

    PlayerMagicState state_;
};

}  // namespace osf

#endif
