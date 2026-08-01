#include "core/retail_random.hpp"
#include "world/player_transport_spell.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool testTransportState(
    const std::filesystem::path& game_root,
    const osf::WorldScene& world,
    osf::WorldPosition field_position) {
    osf::ScenarioData town;
    std::string error;
    if (!check(
            town.load(
                game_root / "Scenario" / "00000000" /
                    "Scenario.Mct",
                &error),
            "The Remote Town transport fixture could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerTransportSpell transport;
    osf::RetailRandom random;
    if (!check(
            transport.create(
                0,
                world.scenarioId(),
                field_position,
                world.scenario(),
                town,
                random) &&
                transport.active() &&
                transport.owner() == 0 &&
                transport.townEntryValue() == 100,
            "The paired Transport endpoints were not created.")) {
        return false;
    }
    const osf::PlayerTransportEndpoint* field =
        transport.fieldEndpoint();
    const osf::PlayerTransportEndpoint* remote_town =
        transport.endpoint(0);
    if (!check(
            field && remote_town &&
                field->scenario_id == world.scenarioId() &&
                field->position.x == field_position.x &&
                field->position.y == field_position.y &&
                remote_town->scenario_id == 0,
            "Transport did not retain its field and town endpoints.")) {
        return false;
    }

    const osf::PlayerTransportPresentationUpdate first_frame =
        transport.updatePresentation(
            world.scenarioId(), 4, random);
    if (!check(
            first_frame.start_sound_due &&
                !first_frame.loop_sound_due &&
                transport.beams()[0].height == 250 &&
                transport.beams()[0].strength == 200 &&
                transport.beams()[1].height == 300 &&
                transport.beams()[1].strength == 0,
            "Transport did not start its first staggered falling layer.")) {
        return false;
    }
    for (std::int32_t update = 1; update < 27; ++update) {
        const osf::PlayerTransportPresentationUpdate presentation =
            transport.updatePresentation(
                world.scenarioId(), 4, random);
        if (presentation.start_sound_due) {
            return check(
                false,
                "Transport repeated its one-time creation sample.");
        }
    }
    bool loop_sound = false;
    for (std::int32_t update = 0;
         update < 100 && !loop_sound;
         ++update) {
        loop_sound = transport.updatePresentation(
            world.scenarioId(), 4, random).loop_sound_due;
    }
    if (!check(
            transport.centerVisible() && loop_sound,
            "Transport did not reveal and start its paused center loop.")) {
        return false;
    }

    const osf::ObjectBounds player_bounds{-20, -20, 20, 20};
    const osf::WorldPosition away{
        field_position.x + 300,
        field_position.y + 300,
    };
    return check(
        !transport.updateContact(
            world.scenarioId(), away, player_bounds) &&
            transport.updateContact(
                world.scenarioId(), field_position, player_bounds) &&
            !transport.updateContact(
                world.scenarioId(), field_position, player_bounds) &&
            !transport.updateContact(
                world.scenarioId(), away, player_bounds) &&
            transport.updateContact(
                world.scenarioId(), field_position, player_bounds),
        "A Transport endpoint did not require an exit before re-entry.");
}

bool testLiveTransportCast(
    const std::filesystem::path& game_root) {
    osf::PlayerLoadRequest player;
    player.name = "TransportLive";
    player.gender = osf::playerGenderValue(osf::PlayerGender::male);
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                game_root,
                player,
                {3000507, 3, 0},
                &error),
            "The shipped Transport scenario could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldPosition placement;
    const osf::WorldPosition player_position{
        world.playerWorldX(), world.playerWorldY()};
    if (!check(
            osf::chooseRetailPlayerTransportPosition(
                world.ground(),
                world.objectMap(),
                player_position,
                {player_position.x + 1000, player_position.y},
                placement) &&
                ((placement.x == player_position.x &&
                  std::abs(placement.y - player_position.y) == 500) ||
                 (placement.y == player_position.y &&
                  std::abs(placement.x - player_position.x) == 500)),
            "Transport did not select one of its four retail offsets.")) {
        return false;
    }
    if (!testTransportState(game_root, world, placement)) {
        return false;
    }

    osf::PlayerMagicState magic;
    magic.availability.fill(0);
    magic.levels.fill(1);
    magic.experience.fill(0);
    magic.bar_slots.fill(-1);
    magic.availability[0] = 3;
    world.playerMagic().restore(magic);
    world.configurePlayerDebugResources(false, true);
    if (!check(
            world.playerMagic().selectSpell(0) &&
                world.commandPlayerMagic(600, 240) &&
                world.playerSpellActive() &&
                world.playerAnimationChart() == 11,
            "Right-click did not enter Transport's retail cast action.")) {
        return false;
    }

    world.update();
    const std::vector<std::int32_t> initial_audio =
        world.takeAudioSamples();
    if (!check(
            std::find(
                initial_audio.begin(), initial_audio.end(), 79) !=
                initial_audio.end(),
            "Transport did not play its endpoint creation sample.")) {
        return false;
    }

    const osf::PlayerTransportEndpoint* endpoint =
        world.playerTransportSpell().endpoint(world.scenarioId());
    if (!check(
            endpoint && world.playerTransportPatterns() &&
                world.playerTransportVisual(),
            "Transport did not prepare its endpoint presentation.")) {
        return false;
    }
    const std::int32_t dx =
        endpoint->position.x - player_position.x;
    const std::int32_t dy =
        endpoint->position.y - player_position.y;
    return check(
        (dx == 0 && std::abs(dy) == 500) ||
            (dy == 0 && std::abs(dx) == 500),
        "The live Transport endpoint was not 500 world units away.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path game_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_regular_file(
            game_root / "System" / "Game" / "Parameter" /
                "Table.Tbd")) {
        return 0;
    }
    return testLiveTransportCast(game_root) ? 0 : 1;
#else
    return 0;
#endif
}
