#ifndef OPENSHADOWFLARE_GAMEPLAY_UI_CONTROLLER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_UI_CONTROLLER_HPP

#include "states/gameplay_blackjack.hpp"
#if OSF_ENABLE_DEBUG_TOOLS
#include "states/gameplay_debug_menu.hpp"
#endif
#include "states/gameplay_equipment_color.hpp"
#include "states/gameplay_inventory.hpp"
#include "states/gameplay_magic.hpp"
#include "states/gameplay_map.hpp"
#include "states/gameplay_mission_list.hpp"
#include "states/gameplay_options_menu.hpp"
#include "states/gameplay_status.hpp"
#include "states/gameplay_transport.hpp"
#include "states/gameplay_vendor.hpp"
#include "ui/pointer_input_guard.hpp"

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

    const GameplayOptionsMenu& options() const;
    const GameplayBlackjack& blackjack() const;
#if OSF_ENABLE_DEBUG_TOOLS
    const GameplayDebugMenu& debug() const;
#endif
    const GameplayEquipmentColor& equipmentColor() const;
    const GameplayInventory& inventory() const;
    const GameplayMap& map() const;
    const GameplayMagic& magic() const;
    const GameplayStatus& status() const;
    const GameplayMissionList& missionList() const;
    const GameplayTransport& transport() const;
    const GameplayVendor& vendor() const;

private:
    bool gameplayPanelsActive() const;
    void closeGameplayPanels(WorldScene& world);
    void closeVendor(WorldScene& world);

    bool updateOptions(
        InputAdapter& input,
        bool hud_toggle,
        WorldScene& world,
        AudioSystem& audio,
        GameConfig& game_config,
        bool& config_dirty,
        RetailRandom& random,
        PlayerLoadRequest& player,
        RetailSavePreview& save_preview,
        std::int32_t& shadow_opacity);

    void applyConfig(
        const GameConfig& config,
        WorldScene& world,
        AudioSystem& audio,
        std::int32_t& shadow_opacity);

    GameplayOptionsMenu options_;
    GameplayBlackjack blackjack_;
#if OSF_ENABLE_DEBUG_TOOLS
    GameplayDebugMenu debug_;
#endif
    GameplayEquipmentColor equipment_color_;
    GameplayInventory inventory_;
    GameplayMap map_;
    GameplayMagic magic_;
    GameplayStatus status_;
    GameplayMissionList mission_list_;
    GameplayTransport transport_;
    GameplayVendor vendor_;
    PointerInputGuard world_drop_pointer_guard_;
    GameplayOptionsAction pending_action_ =
        GameplayOptionsAction::none;
};

}  // namespace runtime
}  // namespace osf

#endif
