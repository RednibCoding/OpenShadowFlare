#include "world_scene.hpp"

#include <cstdio>

namespace osf {
namespace {

constexpr std::int32_t kTransportCircleResource = 10000020;

std::filesystem::path scenarioPath(
    const std::filesystem::path& data_root,
    std::int32_t scenario_id) {
    char directory[16]{};
    std::snprintf(
        directory, sizeof(directory), "%08d", scenario_id);
    return data_root / "Scenario" / directory / "Scenario.Mct";
}

}  // namespace

void WorldScene::createPlayerTransport(
    WorldPosition aim_position) {
    WorldPosition circle_position;
    if (!chooseRetailPlayerTransportPosition(
            scenario_world_.ground(),
            scenario_world_.objectMap(),
            player_.position(),
            aim_position,
            circle_position)) {
        return;
    }

    const std::int32_t town_scenario =
        scenario_world_.data().footerValues()[0];
    ScenarioData town_data;
    std::string error;
    if (town_scenario == scenario_world_.id() ||
        !town_data.load(
            scenarioPath(data_root_, town_scenario), &error) ||
        !player_transport_spell_.create(
            scenario_world_.localPlayerNumber(),
            scenario_world_.id(),
            circle_position,
            scenario_world_.data(),
            town_data,
            item_random_)) {
        return;
    }
    player_magic_.train(0, false, parameter_tables_);
    preparePlayerTransportEndpoint();
}

void WorldScene::preparePlayerTransportEndpoint() {
    const PlayerTransportEndpoint* endpoint =
        player_transport_spell_.endpoint(
            scenario_world_.id());
    if (!endpoint ||
        !effect_visuals_.load(
            data_root_, kTransportCircleResource, nullptr) ||
        !effect_pattern_resources_.load(
            data_root_, kTransportCircleResource, nullptr)) {
        return;
    }
}

void WorldScene::updatePlayerTransportPresentation() {
    const EffectVisualResource* visual =
        playerTransportVisual();
    std::int32_t frame_count = 0;
    if (visual && !visual->animation().charts().empty()) {
        frame_count = visual->animation()
            .charts().front().directions[8].frame_count;
    }
    const PlayerTransportPresentationUpdate update =
        player_transport_spell_.updatePresentation(
            scenario_world_.id(),
            frame_count,
            item_random_);
    if (update.start_sound_due) {
        pending_audio_samples_.push_back(79);
    }
    if (update.loop_sound_due) {
        pending_audio_samples_.push_back(51);
    }
}

bool WorldScene::updatePlayerTransportContact() {
    if (!player_transport_spell_.updateContact(
            scenario_world_.id(),
            player_.position(),
            player_.judgement())) {
        return false;
    }

    const std::int32_t current_scenario =
        scenario_world_.id();
    const bool from_town =
        player_transport_spell_.atTownEndpoint(
            current_scenario);
    if (from_town) {
        const PlayerTransportEndpoint* field =
            player_transport_spell_.fieldEndpoint();
        if (!field) {
            return false;
        }
        const WorldPosition destination = field->position;
        const std::int32_t destination_scenario =
            field->scenario_id;
        if (transitionScenario({
                destination_scenario,
                0,
                player_transport_spell_.owner(),
            }) == ScenarioTravelResult::failed) {
            return false;
        }
        player_.relocate(destination, player_.direction());
        companion_.relocate(
            player_.position(), player_.direction());
        scenario_world_.mapExploration().reveal(
            player_.position());
        player_transport_spell_.consume();
        return true;
    }

    const std::int32_t town_scenario =
        scenario_world_.data().footerValues()[0];
    if (transitionScenario({
            town_scenario,
            player_transport_spell_.townEntryValue(),
            player_transport_spell_.owner(),
        }) == ScenarioTravelResult::failed) {
        return false;
    }
    preparePlayerTransportEndpoint();
    // Arrival starts inside the town endpoint. Retail requires the player
    // to step out before that endpoint can send them back.
    player_transport_spell_.updateContact(
        scenario_world_.id(),
        player_.position(),
        player_.judgement());
    return true;
}

}  // namespace osf
