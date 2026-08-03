#include "gameplay_status_renderer.hpp"

#include "gapi/gapi.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "states/gameplay_status.hpp"
#include "world/player_data.hpp"
#include "world/player_combat_defense.hpp"
#include "world/player_job.hpp"
#include "world/player_runtime_profile.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace osf {
namespace {

constexpr gapi::Color kNormal{224, 224, 224, 255};
constexpr gapi::Color kLower{224, 64, 64, 255};
constexpr gapi::Color kHigher{224, 192, 128, 255};

gapi::Color valueColor(
    std::int32_t value,
    std::int32_t base) {
    if (value == base) {
        return kNormal;
    }
    return value < base ? kLower : kHigher;
}

void drawText(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    std::string_view text,
    std::int32_t x,
    std::int32_t y,
    gapi::Color color = kNormal) {
    renderer.drawText(
        font,
        text,
        {x + 1, y + 1, {0, 0, 0, 255}, 1000, 1});
    renderer.drawText(
        font,
        text,
        {x, y, color, 1000, 1});
}

void drawNumber(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    std::int32_t value,
    std::int32_t right,
    std::int32_t y,
    gapi::Color color = kNormal) {
    const std::string text =
        std::to_string(std::max<std::int32_t>(value, 0));
    const std::int32_t x =
        right - static_cast<std::int32_t>(text.size()) * 8;
    renderer.drawText(
        font,
        text,
        {x + 1, y + 1, {0, 0, 0, 255}, 1000, 2});
    renderer.drawText(
        font,
        text,
        {x, y, color, 1000, 2});
}

}  // namespace

void renderGameplayStatusPanel(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const GameplayStatus& status,
    const WorldScene& world) {
    if (!status.active() || !world.hasPlayer()) {
        return;
    }

    // CharacterStatusDisplay (0x00405750) is the Status tab of the same
    // live left-hand window as Magic. Status.njp pattern 5 contains all
    // authored labels and framing; the executable overlays only live values.
    renderer.drawPattern(status_patterns, 5);

    const PlayerData& player = world.playerData();
    const PlayerRuntimeProfile profile =
        world.playerRuntimeProfile();
    drawText(
        renderer,
        font,
        retailPlayerJobName(
            player.job(), player.gender()),
        22,
        42);
    drawText(renderer, font, player.name(), 92, 42);
    drawNumber(
        renderer, font, player.level(), 303, 43,
        player.level() == 100 ? kHigher : kNormal);
    if (player.level() == 100) {
        drawText(renderer, font, "Max", 283, 67, kHigher);
    } else {
        drawNumber(
            renderer, font, player.experience(), 303, 67);
    }

    const gapi::Color life_color = valueColor(
        profile.maximum_life, player.baseMaximumLife());
    drawNumber(
        renderer, font, world.playerCurrentLife(), 81, 91,
        life_color);
    drawNumber(
        renderer, font, profile.maximum_life, 124, 91,
        life_color);

    drawNumber(
        renderer, font, profile.weight_capacity, 95, 115,
        valueColor(
            profile.weight_capacity,
            player.baseWeightCapacity()));
    drawNumber(
        renderer, font, profile.physical_attack, 198, 115,
        valueColor(
            profile.physical_attack,
            player.basePhysicalAttack()));
    drawNumber(
        renderer, font, profile.physical_defense, 303, 115,
        valueColor(
            profile.physical_defense,
            player.basePhysicalDefense()));
    drawNumber(
        renderer, font, profile.hit_rate, 150, 139,
        valueColor(profile.hit_rate, player.baseHitRate()));
    drawNumber(
        renderer, font, profile.physical_evasion, 303, 139,
        valueColor(
            profile.physical_evasion,
            player.baseEvasionRate()));
    drawNumber(
        renderer, font, profile.walking_speed_raw, 150, 163,
        valueColor(
            profile.walking_speed_raw,
            player.initialParameter(1)));
    drawNumber(
        renderer, font, profile.attack_speed_raw, 303, 163,
        valueColor(
            profile.attack_speed_raw,
            player.baseAttackSpeed()));

    const gapi::Color mana_color = valueColor(
        profile.maximum_mana, player.baseMaximumMana());
    drawNumber(
        renderer, font, world.playerCurrentMana(), 81, 194,
        mana_color);
    drawNumber(
        renderer, font, profile.maximum_mana, 124, 194,
        mana_color);
    drawNumber(
        renderer, font, profile.magical_attack, 150, 219,
        valueColor(
            profile.magical_attack,
            player.baseMagicalAttack()));
    drawNumber(
        renderer, font, profile.magical_defense, 303, 219,
        valueColor(
            profile.magical_defense,
            player.baseMagicalDefense()));
    drawNumber(
        renderer, font, profile.magical_hit_rate, 150, 243,
        valueColor(
            profile.magical_hit_rate,
            player.baseMagicalHitRate()));
    drawNumber(
        renderer, font, profile.magical_evasion, 303, 243,
        valueColor(
            profile.magical_evasion,
            player.baseMagicalEvasionRate()));

    const auto affinities = buildPlayerElementAffinities(
        {
            0,
            profile.physical_attack,
            profile.physical_defense,
            profile.magical_defense,
            player.elementX(),
            player.elementY(),
        },
        world.playerEquipment(),
        world.playerInventory(),
        world.itemDatabase());
    for (std::size_t element = 0;
         element < affinities.size();
         ++element) {
        renderer.drawPattern(
            status_patterns,
            static_cast<std::size_t>(
                36 + std::clamp<std::int32_t>(
                    affinities[element] + 10,
                    0,
                    20)),
            {0, static_cast<std::int32_t>(element) * 16});
    }

    // The marker is positioned directly from the two saved elemental axes.
    renderer.drawPattern(
        status_patterns,
        57,
        {
            player.elementX() * 48 / 20000 + 80,
            330 - player.elementY() * 48 / 20000,
        });
}

}  // namespace osf
