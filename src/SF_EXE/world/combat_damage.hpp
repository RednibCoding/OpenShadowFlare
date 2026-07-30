#ifndef OPENSHADOWFLARE_COMBAT_DAMAGE_HPP
#define OPENSHADOWFLARE_COMBAT_DAMAGE_HPP

#include "combat_packet.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace osf {

class RetailRandom;
class TableDatabase;

constexpr std::size_t kCombatDefenseWordCount = 14;

struct CombatDefense {
    const std::int32_t& operator[](
        std::size_t index) const {
        return words[index];
    }

    std::int32_t& operator[](
        std::size_t index) {
        return words[index];
    }

    std::array<
        std::int32_t,
        kCombatDefenseWordCount>
        words{};
};

struct CombatDamageResult {
    bool valid = false;
    std::int32_t damage = 0;
    bool requests_source_lookup = false;
    std::int32_t source_character_number = -1;
};

CombatDamageResult resolveCombatDamage(
    const CombatPacket& packet,
    const CombatDefense& defense,
    const TableDatabase& tables,
    RetailRandom& random);

}  // namespace osf

#endif
