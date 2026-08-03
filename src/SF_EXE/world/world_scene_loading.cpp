#include "world_scene.hpp"

#include "items/new_player_loadout.hpp"
#include "retail_save_file.hpp"
#include "retail_save_automatic_items.hpp"
#include "retail_save_companion_progress.hpp"
#include "retail_save_giant_warehouse.hpp"
#include "retail_save_items.hpp"
#include "retail_save_magic.hpp"
#include "retail_save_mines.hpp"
#include "retail_save_progress.hpp"
#include "retail_save_world_state.hpp"

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

namespace osf {
namespace {

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

}  // namespace

bool WorldScene::loadInitialScenario(
    const std::filesystem::path& data_root,
    const PlayerLoadRequest& player_request,
    std::string* error) {
    return loadInitialScenario(
        data_root,
        player_request,
        ScenarioStart{},
        error);
}

bool WorldScene::loadInitialScenario(
    const std::filesystem::path& data_root,
    const PlayerLoadRequest& player_request,
    const ScenarioStart& start,
    std::string* error) {
    clear();

    if (!parameter_tables_.load(
            data_root / "System" / "Game" / "Parameter" /
                "Table.Tbd",
            error) ||
        !ai_control_database_.load(
            data_root / "System" / "Game" / "Parameter" /
                "Control.aid",
            error) ||
        !missions_.load(parameter_tables_, error) ||
        !transports_.load(parameter_tables_, error) ||
        !player_data_.load(
            player_request, parameter_tables_, error) ||
        !item_database_.load(
            data_root / "System" / "Game" / "Parameter" /
                "Item.Ibn",
            error)) {
        clear();
        return false;
    }

    quests_.initialize(missions_.missions().size());
    player_item_controller_.initializeNew();
    player_giant_warehouse_.initializeNew();
    bool saved_running = false;
    ScenarioStart scenario_start = start;
    if (player_request.source ==
        PlayerDataSource::new_character) {
        if (!initializeRetailNewPlayerLoadout(
                item_database_,
                player_inventory_,
                player_equipment_,
                player_belt_,
                player_data_.level(),
                error)) {
            clear();
            return false;
        }
        player_magic_.initializeNew();
    } else {
        // Older portable saves ended after the progress extension. Seed the
        // retail new-character defaults before optionally restoring the
        // magic stream so those saves remain valid.
        player_magic_.initializeNew();
        std::vector<std::uint8_t> payload;
        std::size_t owned_items_end = 0;
        if (!readRetailSavePayload(
                player_request.save_path,
                payload,
                error) ||
            !restoreRetailOwnedItems(
                payload,
                item_database_,
                player_data_.level(),
                player_inventory_,
                player_equipment_,
                player_belt_,
                player_special_items_,
                &owned_items_end,
                error)) {
            clear();
            return false;
        }
        RetailSaveProgress progress{
            quests_.states(),
            transports_.enabledFlags(),
            {},
        };
        std::size_t progress_end = owned_items_end;
        std::size_t magic_end = progress_end;
        if (!restoreRetailProgress(
                payload,
                owned_items_end,
                progress,
                &progress_end,
                error) ||
            !restoreRetailMagic(
                payload,
                progress_end,
                player_magic_,
                &magic_end,
                error)) {
            clear();
            return false;
        }
        std::int32_t mine_count =
            player_item_controller_.mineCount();
        std::size_t companion_progress_end = magic_end;
        std::size_t mine_end = companion_progress_end;
        std::size_t world_state_end = mine_end;
        std::size_t giant_warehouse_end = world_state_end;
        RetailSaveWorldState world_state{
            false,
            start.scenario_id,
            start.entry_value,
        };
        if (!restoreRetailCompanionProgress(
                payload,
                magic_end,
                player_data_,
                &companion_progress_end,
                error) ||
            !restoreRetailMineCount(
                payload,
                companion_progress_end,
                mine_count,
                &mine_end,
                error) ||
            !restoreRetailWorldState(
                payload,
                mine_end,
                world_state,
                &world_state_end,
                error) ||
            !restoreRetailGiantWarehouse(
                payload,
                world_state_end,
                item_database_,
                player_giant_warehouse_,
                &giant_warehouse_end,
                error) ||
            !restoreRetailAutomaticItems(
                payload,
                giant_warehouse_end,
                item_database_,
                player_automatic_items_,
                nullptr,
                error)) {
            clear();
            return false;
        }
        player_item_controller_.restoreMineCount(
            std::min(mine_count, playerMaximumMineCount()));
        if (!transports_.restoreEnabledFlags(
                progress.transport_flags)) {
            setError(
                error,
                "The saved transport state does not match the "
                "transport catalog.");
            clear();
            return false;
        }
        quests_.restore(progress.quest_flags);
        script_state_flags_ =
            std::move(progress.script_state_flags);
        saved_running = world_state.running;
        scenario_start.scenario_id = world_state.scenario_id;
        scenario_start.entry_value = world_state.entry_value;
    }

    // Gameplay bootstrap sets runtime field +0x1288 after both the new-player
    // and saved-player paths. It is transient UI/action state, not part of
    // the saved spell arrays: every entry begins with normal attack selected.
    player_magic_.setTargeting(true);

    if (!item_inventory_patterns_.load(data_root, error) ||
        !speech_patterns_.load(
            data_root / "System" / "Game" / "Pattern" /
                "Hukidasi.njp",
            error)) {
        clear();
        return false;
    }

    // Retail never reaches a normal save action while dead: its locked
    // death action completes a revive transition first. Repair saves made
    // by older portable builds that could persist that impossible state.
    if (player_request.source ==
            PlayerDataSource::retail_save &&
        player_data_.currentLife() <= 0) {
        player_data_.restoreForRespawn();
    }

    const char* player_directory =
        player_data_.gender() ==
                playerGenderValue(PlayerGender::male)
            ? "Male"
            : "Female";
    const std::filesystem::path player_root =
        data_root / "Player" / player_directory;
    std::string player_error;
    if (!player_visual_.loadAnimation(
            player_root, "Animation00", &player_error)) {
        setError(
            error,
            "The player animation could not be loaded: " +
                player_error);
        clear();
        return false;
    }
    if (!player_unlock_switch_visual_.load(
            data_root / "Player" / "Common",
            "UnlockSW",
            &player_error)) {
        setError(
            error,
            "The player unlock-switch animation could not be loaded: " +
                player_error);
        clear();
        return false;
    }

    CompanionProfile companion_profile;
    if (!decodeCompanionProfile(
            parameter_tables_,
            player_data_.companionType(),
            player_data_.companionLevel(),
            companion_profile,
            error)) {
        clear();
        return false;
    }
    const CharacterVisualResource* companion_visual =
        companion_visuals_.load(
            data_root,
            companion_profile.resource_id,
            error);
    if (!companion_visual) {
        clear();
        return false;
    }

    RetailRandom prepared_item_random = item_random_;
    ScenarioWorld prepared_scenario;
    if (!prepared_scenario.load(
            data_root,
            scenario_start,
            ai_control_database_,
            prepared_item_random,
            error)) {
        clear();
        return false;
    }

    data_root_ = data_root;
    std::int32_t prepared_next_ground_item_id = 0;
    if (!prepareGroundItems(
            prepared_scenario.groundItems(),
            0,
            prepared_next_ground_item_id,
            error)) {
        clear();
        return false;
    }
    if (!refreshPlayerAppearance(&player_error)) {
        setError(
            error,
            "The selected player graphics could not be loaded: " +
                player_error);
        clear();
        return false;
    }
    scenario_world_ = std::move(prepared_scenario);
    item_random_ = prepared_item_random;
    next_ground_item_id_ =
        prepared_next_ground_item_id;
    scenario_script_.adopt(
        scenario_world_.takeScriptData());
    player_.reset(
        {
            scenario_world_.entry().world_x,
            scenario_world_.entry().world_y,
        },
        scenario_world_.entry().direction,
        player_data_.walkingSpeedTier());
    if (!companion_.initialize(
            companion_profile,
            *companion_visual,
            scenario_world_.localPlayerNumber(),
            player_.position(),
            player_.direction())) {
        setError(
            error,
            "The owned companion could not be initialized.");
        clear();
        return false;
    }
    if (player_data_.companionRespawnCounter() > 0) {
        companion_.beginDefeatedWait();
    }
    player_.setMovementPace(
        saved_running ? MovementPace::run : MovementPace::walk);
    scenario_world_.mapExploration().reveal(
        player_.position());
    has_player_ = true;
    // Retail installs the local player and current entry before status kind
    // seven initializes the scenario. Those scripts can query both values.
    scenario_script_.runStatusKind(7);
    if (error) {
        error->clear();
    }
    return true;
}

void WorldScene::releaseInactiveEffectResources() {
    constexpr std::int32_t transport_resource = 10000020;
    constexpr std::int32_t moon_resource = 11000040;
    constexpr std::int32_t magic_shield_resource = 11000240;
    constexpr std::int32_t counter_burst_resource = 11000250;

    std::vector<std::int32_t> visual_resources;
    if (player_transport_spell_.active() &&
        effect_visuals_.find(transport_resource)) {
        visual_resources.push_back(transport_resource);
    }
    if (player_moon_spell_.active()) {
        visual_resources.push_back(moon_resource);
    }
    if (player_magic_shield_.active()) {
        visual_resources.push_back(magic_shield_resource);
    }
    if (player_counter_burst_.active()) {
        visual_resources.push_back(counter_burst_resource);
    }
    effect_visuals_.retainOnly(visual_resources);

    std::vector<std::int32_t> pattern_resources;
    if (player_transport_spell_.active() &&
        effect_pattern_resources_.find(transport_resource)) {
        pattern_resources.push_back(transport_resource);
    }
    effect_pattern_resources_.retainOnly(pattern_resources);
}

ScenarioTravelResult WorldScene::transitionScenario(
    const ScenarioStart& start,
    std::string* error) {
    if (!has_player_ || data_root_.empty()) {
        setError(error, "There is no live world to move.");
        return ScenarioTravelResult::failed;
    }

    if (start.scenario_id == scenario_world_.id()) {
        if (start.entry_value < 0 ||
            start.local_player_number < 0 ||
            start.local_player_number > 3 ||
            start.entry_value >
                (std::numeric_limits<std::int32_t>::max() -
                 start.local_player_number) /
                    4) {
            setError(error, "The scenario start request is invalid.");
            return ScenarioTravelResult::failed;
        }
        const ScenarioEntry* entry =
            scenario_world_.data().findEntry(
                start.local_player_number +
                start.entry_value * 4);
        if (!entry) {
            setError(
                error,
                "The scenario does not contain the requested entry.");
            return ScenarioTravelResult::failed;
        }
        pending_interaction_ = {};
        player_attack_target_.cancel();
        pending_player_attack_impact_target_id_ = -1;
        pending_combat_effects_.clear();
        combat_effects_.clear();
        runtime_effects_.clear();
        scenario_text_labels_.clear();
        scenario_visual_.clear();
        scenario_visual_patterns_.clear();
        scenario_visual_continue_patterns_.clear();
        scenario_screen_particles_.clear();
        player_land_mines_.clear();
        miss_effects_.clear();
        releaseInactiveEffectResources();
        camera_shake_counter_ = -1;
        camera_shake_duration_ = 0;
        camera_shake_magnitude_ = 0;
        player_identify_mode_active_ = false;
        player_unlock_switch_active_ = false;
        pointer_.clearSelection();
        scenario_world_.setEntry(
            start.entry_value, *entry);
        player_.relocate(
            {entry->world_x, entry->world_y},
            entry->direction);
        companion_.relocate(
            player_.position(), player_.direction());
        scenario_world_.mapExploration().reveal(
            player_.position());
        scenario_script_.runStatusKind(7);
        if (error) {
            error->clear();
        }
        return ScenarioTravelResult::relocated;
    }

    RetailRandom prepared_item_random = item_random_;
    ScenarioWorld prepared_scenario;
    if (!prepared_scenario.load(
            data_root_,
            start,
            ai_control_database_,
            prepared_item_random,
            error)) {
        return ScenarioTravelResult::failed;
    }
    std::int32_t prepared_next_ground_item_id = 0;
    if (!prepareGroundItems(
            prepared_scenario.groundItems(),
            0,
            prepared_next_ground_item_id,
            error)) {
        return ScenarioTravelResult::failed;
    }

    player_.cancelMovement();
    pointer_.reset();
    pending_interaction_ = {};
    player_attack_target_.cancel();
    pending_player_attack_impact_target_id_ = -1;
    pending_audio_samples_.clear();
    scenario_text_labels_.clear();
    scenario_visual_.clear();
    scenario_visual_patterns_.clear();
    scenario_visual_continue_patterns_.clear();
    scenario_screen_particles_.clear();
    pending_combat_effects_.clear();
    combat_effects_.clear();
    runtime_effects_.clear();
    player_land_mines_.clear();
    miss_effects_.clear();
    camera_shake_counter_ = -1;
    camera_shake_duration_ = 0;
    camera_shake_magnitude_ = 0;
    gameplay_service_request_ = {};
    script_transport_service_ = -1;
    player_identify_mode_active_ = false;
    player_unlock_switch_active_ = false;
    scenario_world_ = std::move(prepared_scenario);
    releaseUnusedItemWorldResources();
    releaseInactiveEffectResources();
    item_random_ = prepared_item_random;
    next_ground_item_id_ =
        prepared_next_ground_item_id;
    scenario_script_.adopt(
        scenario_world_.takeScriptData());
    player_.relocate(
        {
            scenario_world_.entry().world_x,
            scenario_world_.entry().world_y,
        },
        scenario_world_.entry().direction);
    companion_.relocate(
        player_.position(), player_.direction());
    scenario_world_.mapExploration().reveal(
        player_.position());
    scenario_script_.runStatusKind(7);
    if (error) {
        error->clear();
    }
    scenario_changed_ = true;
    return ScenarioTravelResult::loaded;
}

bool WorldScene::takeScenarioChanged() {
    const bool changed = scenario_changed_;
    scenario_changed_ = false;
    return changed;
}

}  // namespace osf
