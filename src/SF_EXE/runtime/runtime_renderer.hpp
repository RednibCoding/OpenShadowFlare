#ifndef OPENSHADOWFLARE_RUNTIME_RENDERER_HPP
#define OPENSHADOWFLARE_RUNTIME_RENDERER_HPP

#include "libs/RKC_DBFCONTROL/rkc_dbfcontrol.hpp"
#include "states/game_state.hpp"

#include <cstdint>
#include <functional>

namespace osf {

class CharacterSelectState;
struct CharacterSelectFrameResult;
struct GameConfig;
class GameplayInventory;
class GameplayDebugMenu;
class GameplayMap;
class GameplayMagic;
class GameplayMissionList;
class GameplayOptionsMenu;
class GameplayStatus;
class GameplayTransport;
class GameplayVendor;
struct GameplayFrameResult;
class RetailSavePreview;
struct TitleFrameResult;
class WorldScene;

namespace runtime {

class FrontendAssets;

struct RuntimeRenderContext {
    GameState game_state;
    const TitleFrameResult& title_frame;
    const CharacterSelectFrameResult& character_frame;
    const GameplayFrameResult& gameplay_frame;
    const CharacterSelectState& character_select;
    const WorldScene& world;
    FrontendAssets& frontend_assets;
    RetailSavePreview& save_preview;
    const GameplayOptionsMenu& gameplay_options;
    const GameplayDebugMenu& gameplay_debug;
    const GameplayInventory& gameplay_inventory;
    const GameplayMap& gameplay_map;
    const GameplayMagic& gameplay_magic;
    const GameplayStatus& gameplay_status;
    const GameplayMissionList& gameplay_mission_list;
    const GameplayTransport& gameplay_transport;
    const GameplayVendor& gameplay_vendor;
    const GameConfig& game_config;
    std::int32_t shadow_opacity = 500;
    std::uint32_t gameplay_counter = 0;
    std::int32_t frames_per_second = 0;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
};

class RuntimeRenderer {
public:
    RuntimeRenderer(
        std::int32_t width,
        std::int32_t height,
        std::function<void(gapi::SurfaceView)> present);

    void render(
        const RuntimeRenderContext& context,
        double interpolation);

private:
    gapi::SoftwareBackend renderer_;
};

}  // namespace runtime
}  // namespace osf

#endif
