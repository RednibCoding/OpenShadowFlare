#ifndef OPENSHADOWFLARE_COMBAT_PACKET_HPP
#define OPENSHADOWFLARE_COMBAT_PACKET_HPP

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>

namespace osf {

constexpr std::size_t kCombatPacketWordCount = 77;

struct CombatPacket {
    void write(
        std::size_t index,
        std::int32_t value) {
        words[index] = value;
        written_words.set(index);
    }

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
        kCombatPacketWordCount>
        words{};
    std::bitset<kCombatPacketWordCount>
        written_words;
};

}  // namespace osf

#endif
