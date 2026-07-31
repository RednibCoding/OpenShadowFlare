#include "companion_attack_impact.hpp"

#include "combat_hit_chance.hpp"
#include "core/retail_random.hpp"

namespace osf {
namespace {

constexpr std::int32_t kHitEffectBase = 21000;
constexpr std::int32_t kHitAudioSample = 44;

void initializePacketDefaults(CombatPacket& packet) {
    packet.write(0, 1);
    packet.write(1, 0);
    packet.write(3, 0);
    packet.write(35, 8);
    packet.write(37, 0);
    packet.write(38, 1);
    packet.write(39, 0);
    packet.write(40, 0);
    packet.write(41, -1);
    packet.write(42, 0);
    packet.write(43, -1);
    packet.write(44, 0);
    packet.write(72, 1);
    packet.write(73, -1);
    packet.write(74, -1);
    packet.write(75, 8);
    packet.write(76, 0);
}

}  // namespace

CombatPacket buildCompanionAttackPacket(
    const CompanionAttackImpactInput& input,
    std::int32_t hit_effect) {
    CombatPacket packet;
    initializePacketDefaults(packet);
    packet.write(2, input.source_character_number);
    packet.write(4, input.physical_attack);
    packet.write(31, input.source_level);
    packet.write(32, input.native_element);
    packet.write(34, hit_effect);
    return packet;
}

CompanionAttackImpactResult
resolveCompanionAttackImpact(
    const CompanionAttackImpactInput& input,
    RetailRandom& random) {
    CompanionAttackImpactResult result;
    if (input.source_character_number < 0 ||
        input.target_character_number < 0) {
        return result;
    }
    result.valid = true;
    result.target_character_number =
        input.target_character_number;
    result.hit_chance = retailCombatHitChance(
        input.hit_rate,
        input.target_physical_evasion);
    result.hit_roll = random.next() % 100;
    if (result.hit_roll >= result.hit_chance) {
        result.show_miss = true;
        return result;
    }
    result.packet = buildCompanionAttackPacket(
        input,
        random.next() % 4 + kHitEffectBase);
    result.apply_damage = true;
    result.post_hit_audio_sample = kHitAudioSample;
    return result;
}

}  // namespace osf
