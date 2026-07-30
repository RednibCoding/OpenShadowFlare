#include "world_scene.hpp"

#include "items/new_player_loadout.hpp"
#include "retail_save_file.hpp"
#include "retail_save_items.hpp"
#include "retail_save_progress.hpp"

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
        player_item_controller_.initializeNew();
    } else {
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
        std::vector<std::int32_t> transport_flags =
            transports_.enabledFlags();
        if (!restoreRetailTransportFlags(
                payload,
                owned_items_end,
                transport_flags,
                error)) {
            clear();
            return false;
        }
        if (!transports_.restoreEnabledFlags(
                transport_flags)) {
            setError(
                error,
                "The saved transport state does not match the "
                "transport catalog.");
            clear();
            return false;
        }
    }

    if (!item_inventory_patterns_.load(data_root, error) ||
        !speech_patterns_.load(
            data_root / "System" / "Game" / "Pattern" /
                "Hukidasi.njp",
            error)) {
        clear();
        return false;
    }

    const char* player_directory =
        player_data_.gender() == 1 ? "Female" : "Male";
    const std::filesystem::path player_root =
        data_root / "Player" / player_directory;
    std::string player_error;
    if (!player_visual_.load(
            player_root, "Animation00", &player_error)) {
        setError(
            error,
            "The player animation could not be loaded: " +
                player_error);
        clear();
        return false;
    }

    ScenarioWorld prepared_scenario;
    if (!prepared_scenario.load(data_root, start, error)) {
        clear();
        return false;
    }

    data_root_ = data_root;
    refreshPlayerAppearance();
    scenario_world_ = std::move(prepared_scenario);
    scenario_script_.adopt(
        scenario_world_.takeScriptData());
    player_.reset(
        {
            scenario_world_.entry().world_x,
            scenario_world_.entry().world_y,
        },
        scenario_world_.entry().direction,
        player_data_.walkingSpeedTier());
    scenario_world_.mapExploration().reveal(
        player_.position());
    has_player_ = true;
    if (error) {
        error->clear();
    }
    return true;
}

}  // namespace osf
