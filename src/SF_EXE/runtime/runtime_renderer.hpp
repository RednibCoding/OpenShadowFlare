#ifndef OPENSHADOWFLARE_RUNTIME_RENDERER_HPP
#define OPENSHADOWFLARE_RUNTIME_RENDERER_HPP

#include "libs/RKC_DBFCONTROL/rkc_dbfcontrol.hpp"
#include "states/game_state.hpp"

#include <cstdint>
#if OSF_ENABLE_DEBUG_TOOLS
#include "debug/profiling_metrics.hpp"
#endif

namespace osf {

class CharacterSelectState;
struct CharacterSelectFrameResult;
struct GameConfig;
class GameplayInventory;
class GameplayBlackjack;
#if OSF_ENABLE_DEBUG_TOOLS
class GameplayDebugMenu;
#endif
class GameplayEquipmentColor;
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
class ResourceManager;

namespace runtime {

struct RuntimeRenderContext {
    GameState game_state;
    const TitleFrameResult& title_frame;
    const CharacterSelectFrameResult& character_frame;
    const GameplayFrameResult& gameplay_frame;
    const CharacterSelectState& character_select;
    const WorldScene& world;
    ResourceManager& resources;
    RetailSavePreview& save_preview;
    const GameplayOptionsMenu& gameplay_options;
    const GameplayBlackjack& gameplay_blackjack;
#if OSF_ENABLE_DEBUG_TOOLS
    const GameplayDebugMenu& gameplay_debug;
#endif
    const GameplayEquipmentColor& gameplay_equipment_color;
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
#if OSF_ENABLE_DEBUG_TOOLS
    std::int32_t frames_per_second = 0;
    debug::ProfilingMetrics profiling_metrics;
#endif
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
};

class RuntimeRenderer {
public:
    RuntimeRenderer(
        std::int32_t width,
        std::int32_t height);

    gapi::SurfaceView render(
        const RuntimeRenderContext& context,
        double interpolation);
    std::uint64_t memoryUsageBytes() const;

private:
    gapi::SoftwareBackend renderer_;
};

}  // namespace runtime
}  // namespace osf

#endif
