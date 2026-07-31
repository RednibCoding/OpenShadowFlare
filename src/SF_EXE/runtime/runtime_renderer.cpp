#include "runtime_renderer.hpp"

#include "core/game_config.hpp"
#include "render/character_select_renderer.hpp"
#include "render/gameplay_debug_renderer.hpp"
#include "render/gameplay_help_renderer.hpp"
#include "render/gameplay_hud_renderer.hpp"
#include "render/gameplay_inventory_renderer.hpp"
#include "render/gameplay_map_renderer.hpp"
#include "render/gameplay_magic_renderer.hpp"
#include "render/gameplay_mission_list_renderer.hpp"
#include "render/gameplay_options_renderer.hpp"
#include "render/gameplay_overlay_renderer.hpp"
#include "render/gameplay_renderer.hpp"
#include "render/gameplay_transport_renderer.hpp"
#include "render/item_information_renderer.hpp"
#include "render/loading_renderer.hpp"
#include "render/system_cursor_renderer.hpp"
#include "render/title_renderer.hpp"
#include "runtime/frontend_assets.hpp"
#include "states/character_select_state.hpp"
#include "states/gameplay_inventory.hpp"
#include "states/gameplay_debug_menu.hpp"
#include "states/gameplay_map.hpp"
#include "states/gameplay_magic.hpp"
#include "states/gameplay_mission_list.hpp"
#include "states/gameplay_options_menu.hpp"
#include "states/gameplay_state.hpp"
#include "states/gameplay_transport.hpp"
#include "world/retail_save_preview.hpp"
#include "world/world_scene.hpp"

#include <array>
#include <cstddef>
#include <utility>

namespace osf::runtime {

RuntimeRenderer::RuntimeRenderer(
    std::int32_t width,
    std::int32_t height,
    std::function<void(gapi::SurfaceView)> present)
    : renderer_(width, height, std::move(present)) {}

void RuntimeRenderer::render(
    const RuntimeRenderContext& context,
    double interpolation) {
    renderer_.beginFrame({0, 0, 0, 255});
    if (context.game_state == GameState::title) {
        const auto* pattern =
            context.frontend_assets.pattern(4);
        if (pattern) {
            std::array<TitleSmokeAsset, 10> smoke{};
            for (std::size_t index = 0;
                 index < smoke.size();
                 ++index) {
                const auto* smoke_pattern =
                    context.frontend_assets.pattern(
                        5 + static_cast<std::int32_t>(index) * 2);
                if (smoke_pattern) {
                    smoke[index] = {
                        smoke_pattern,
                        context.frontend_assets.titleAnimation(index),
                    };
                }
            }
            renderTitle(
                renderer_,
                *pattern,
                smoke,
                context.title_frame);
        }
    } else if (
        context.game_state == GameState::character_select) {
        const auto* pattern =
            context.frontend_assets.pattern(4);
        if (pattern) {
            renderCharacterSelect(
                renderer_,
                *pattern,
                context.frontend_assets.pattern(0),
                context.character_select.data(),
                context.character_frame,
                context.frontend_assets.savedGames(),
                context.frontend_assets.savedPreviews());
        }
    } else if (context.game_state == GameState::gameplay) {
        if (context.gameplay_frame.phase ==
            GameplayPhase::loading) {
            const auto* waiting =
                context.frontend_assets.pattern(2);
            if (waiting) {
                renderInitialLoadingScreen(
                    renderer_,
                    *waiting,
                    context.gameplay_frame.loading_counter,
                    context.gameplay_frame.ready_to_continue);
            }
        } else {
            const auto* font =
                context.frontend_assets.pattern(1);
            renderWorldGeometry(
                renderer_,
                context.world,
                context.shadow_opacity,
                interpolation,
                context.game_config.semi_transparent_objects);
            context.save_preview.capture(renderer_.surface());
            const auto* bar =
                context.frontend_assets.pattern(5);
            if (bar) {
                renderGameplayHud(
                    renderer_,
                    *bar,
                    gameplayHudValues(
                        context.world.playerData(),
                        context.world.playerRuntimeProfile(),
                        context.world.playerMovementPace(),
                        context.world
                            .playerExperienceThreshold()));
                renderGameplayBeltItems(
                    renderer_,
                    context.world);
            }
            if (!context.gameplay_debug.active() &&
                !context.gameplay_options.active() &&
                !context.gameplay_mission_list.active() &&
                !context.gameplay_transport.active()) {
                renderGameplayOverlay(
                    renderer_,
                    context.world,
                    font,
                    context.frontend_assets.pattern(8),
                    context.world.renderCameraScreenX(
                        interpolation),
                    context.world.renderCameraScreenY(
                        interpolation),
                    interpolation);
            }
            const auto* magic_icons =
                context.frontend_assets.pattern(9);
            const auto* magic_bar_icons =
                context.frontend_assets.pattern(10);
            if (magic_icons && magic_bar_icons) {
                const bool left_panel_active =
                    context.gameplay_magic.active() ||
                    context.gameplay_map.active() ||
                    context.gameplay_mission_list.active() ||
                    context.gameplay_transport.active() ||
                    context.gameplay_inventory
                        .specialItemsActive();
                renderGameplayMagicBar(
                    renderer_,
                    *magic_icons,
                    *magic_bar_icons,
                    left_panel_active,
                    context.gameplay_inventory.active(),
                    context.world);
            }
            const auto* status =
                context.frontend_assets.pattern(6);
            if (status && font) {
                const auto* map_icons =
                    context.frontend_assets.pattern(7);
                if (context.gameplay_debug.active()) {
                    renderGameplayDebugMenu(
                        renderer_,
                        *status,
                        *font,
                        context.gameplay_debug);
                } else if (context.gameplay_inventory
                        .specialItemsActive()) {
                    renderGameplaySpecialItems(
                        renderer_,
                        *status,
                        context.gameplay_inventory,
                        context.world,
                        context.gameplay_counter);
                } else if (
                    context.gameplay_transport.active()) {
                    renderGameplayTransport(
                        renderer_,
                        *status,
                        *font,
                        context.gameplay_transport,
                        context.world.transports());
                } else if (
                    context.gameplay_magic.active() &&
                    magic_icons) {
                    renderGameplayMagicPanel(
                        renderer_,
                        *status,
                        *magic_icons,
                        *font,
                        context.gameplay_magic,
                        context.world);
                } else if (
                    context.gameplay_map.active() &&
                    map_icons) {
                    renderGameplayMap(
                        renderer_,
                        *status,
                        *font,
                        *map_icons,
                        context.gameplay_map,
                        context.world);
                } else if (
                    context.gameplay_mission_list.active()) {
                    renderGameplayMissionList(
                        renderer_,
                        *status,
                        *font,
                        context.gameplay_mission_list,
                        context.world.missions(),
                        context.world.quests());
                } else if (
                    context.gameplay_options.page() ==
                    GameplayOptionsPage::help) {
                    renderGameplayHelp(
                        renderer_,
                        *status,
                        *font,
                        context.world,
                        context.gameplay_options.animationCounter(),
                        context.gameplay_options.helpCloseVisible(),
                        context.gameplay_options
                            .helpCloseAnimationCounter());
                } else {
                    renderGameplayOptions(
                        renderer_,
                        *status,
                        *font,
                        context.gameplay_options,
                        context.game_config);
                }
                if (!context.gameplay_debug.active()) {
                    if (context.gameplay_inventory.active()) {
                        renderGameplayInventory(
                            renderer_,
                            *status,
                            *font,
                            context.gameplay_inventory,
                            context.world,
                            context.gameplay_counter);
                    }
                    renderHeldInventoryItem(
                        renderer_,
                        *status,
                        context.gameplay_inventory,
                        context.world,
                        context.gameplay_counter);
                    renderItemInformation(
                        renderer_,
                        *font,
                        context.gameplay_inventory,
                        context.world);
                    if (magic_icons) {
                        renderHeldMagic(
                            renderer_,
                            *magic_icons,
                            context.gameplay_magic);
                    }
                }
            }
            if (font &&
                context.gameplay_debug.fpsCounterEnabled()) {
                renderGameplayDebugFps(
                    renderer_,
                    *font,
                    context.frames_per_second);
            }
        }
    }
    const auto* system_patterns =
        context.frontend_assets.pattern(3);
    if (system_patterns) {
        renderSystemCursor(
            renderer_,
            *system_patterns,
            context.pointer_x,
            context.pointer_y,
            context.game_state == GameState::gameplay &&
                context.world.playerIdentifyModeActive());
    }
    renderer_.endFrame();
}

}  // namespace osf::runtime
