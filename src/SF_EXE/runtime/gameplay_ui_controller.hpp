#ifndef OPENSHADOWFLARE_GAMEPLAY_UI_CONTROLLER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_UI_CONTROLLER_HPP

#include "states/gameplay_inventory.hpp"
#include "states/gameplay_map.hpp"
#include "states/gameplay_mission_list.hpp"
#include "states/gameplay_options_menu.hpp"
#include "states/gameplay_transport.hpp"

#include <cstdint>

namespace osf {

struct GameConfig;
class GameStateDispatcher;
struct GameplayFrameResult;
struct PlayerLoadRequest;
class RetailRandom;
class RetailSavePreview;
class WorldScene;

namespace runtime {

class AudioSystem;
class InputAdapter;

class GameplayUiController {
public:
    void reset();

    bool update(
        const GameplayFrameResult& gameplay_frame,
        InputAdapter& input,
        WorldScene& world,
        AudioSystem& audio,
        GameConfig& game_config,
        bool& config_dirty,
        RetailRandom& random,
        PlayerLoadRequest& player,
        RetailSavePreview& save_preview,
        GameStateDispatcher& game_state,
        bool& running,
        std::int32_t& shadow_opacity);
    bool takeScenarioChanged();

    const GameplayOptionsMenu& options() const;
    const GameplayInventory& inventory() const;
    const GameplayMap& map() const;
    const GameplayMissionList& missionList() const;
    const GameplayTransport& transport() const;

private:
    void applyConfig(
        const GameConfig& config,
        WorldScene& world,
        AudioSystem& audio,
        std::int32_t& shadow_opacity);

    GameplayOptionsMenu options_;
    GameplayInventory inventory_;
    GameplayMap map_;
    GameplayMissionList mission_list_;
    GameplayTransport transport_;
    GameplayOptionsAction pending_action_ =
        GameplayOptionsAction::none;
    bool scenario_changed_ = false;
};

}  // namespace runtime
}  // namespace osf

#endif
