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
class GameplayMap;
class GameplayMissionList;
class GameplayOptionsMenu;
class GameplayTransport;
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
    const GameplayInventory& gameplay_inventory;
    const GameplayMap& gameplay_map;
    const GameplayMissionList& gameplay_mission_list;
    const GameplayTransport& gameplay_transport;
    const GameConfig& game_config;
    std::int32_t shadow_opacity = 500;
    std::uint32_t gameplay_counter = 0;
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
