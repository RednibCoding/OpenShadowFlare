#include "runtime_renderer.hpp"

#include "core/game_config.hpp"
#include "render/character_select_renderer.hpp"
#include "render/gameplay_blackjack_renderer.hpp"
#if OSF_ENABLE_DEBUG_TOOLS
#include "render/gameplay_debug_renderer.hpp"
#endif
#include "render/gameplay_equipment_color_renderer.hpp"
#include "render/gameplay_help_renderer.hpp"
#include "render/gameplay_hud_renderer.hpp"
#include "render/gameplay_inventory_renderer.hpp"
#include "render/gameplay_map_renderer.hpp"
#include "render/gameplay_magic_renderer.hpp"
#include "render/gameplay_mission_list_renderer.hpp"
#include "render/gameplay_options_renderer.hpp"
#include "render/gameplay_overlay_renderer.hpp"
#include "render/gameplay_renderer.hpp"
#include "render/gameplay_status_renderer.hpp"
#include "render/gameplay_transport_renderer.hpp"
#include "render/gameplay_vendor_renderer.hpp"
#include "render/item_information_renderer.hpp"
#include "render/loading_renderer.hpp"
#include "render/quest_notice_renderer.hpp"
#include "render/scenario_presentation_renderer.hpp"
#include "render/system_cursor_renderer.hpp"
#include "render/title_renderer.hpp"
#include "resources/resource_manager.hpp"
#include "states/character_select_state.hpp"
#include "states/gameplay_blackjack.hpp"
#include "states/gameplay_inventory.hpp"
#if OSF_ENABLE_DEBUG_TOOLS
#include "states/gameplay_debug_menu.hpp"
#endif
#include "states/gameplay_equipment_color.hpp"
#include "states/gameplay_map.hpp"
#include "states/gameplay_magic.hpp"
#include "states/gameplay_mission_list.hpp"
#include "states/gameplay_options_menu.hpp"
#include "states/gameplay_state.hpp"
#include "states/gameplay_status.hpp"
#include "states/gameplay_transport.hpp"
#include "states/gameplay_vendor.hpp"
#include "world/retail_save_preview.hpp"
#include "world/world_scene.hpp"

#include <array>
#include <cstddef>

namespace osf::runtime {

RuntimeRenderer::RuntimeRenderer(
    std::int32_t width,
    std::int32_t height)
    : renderer_(width, height) {}

gapi::SurfaceView RuntimeRenderer::render(
    const RuntimeRenderContext& context,
    double interpolation) {
    renderer_.beginFrame({0, 0, 0, 255});
#if OSF_ENABLE_DEBUG_TOOLS
    const bool debug_active = context.gameplay_debug.active();
#else
    constexpr bool debug_active = false;
#endif
    if (context.game_state == GameState::title) {
        const auto* pattern =
            context.resources.pattern(4);
        if (pattern) {
            std::array<TitleSmokeAsset, 10> smoke{};
            for (std::size_t index = 0;
                 index < smoke.size();
                 ++index) {
                const auto* smoke_pattern =
                    context.resources.pattern(
                        5 + static_cast<std::int32_t>(index) * 2);
                if (smoke_pattern) {
                    smoke[index] = {
                        smoke_pattern,
                        context.resources.titleAnimation(index),
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
            context.resources.pattern(4);
        if (pattern) {
            renderCharacterSelect(
                renderer_,
                *pattern,
                context.resources.pattern(0),
                context.character_select.data(),
                context.character_frame,
                context.resources.savedGames(),
                context.resources.savedPreviews());
        }
    } else if (context.game_state == GameState::gameplay) {
        if (context.gameplay_frame.phase ==
            GameplayPhase::loading) {
            const auto* waiting =
                context.resources.pattern(2);
            if (waiting) {
                renderInitialLoadingScreen(
                    renderer_,
                    *waiting,
                    context.gameplay_frame.loading_counter,
                    context.gameplay_frame.ready_to_continue);
            }
        } else if (context.world.scenarioVisualActive()) {
            renderScenarioVisual(renderer_, context.world);
        } else {
            const auto* font =
                context.resources.pattern(1);
            renderWorldGeometry(
                renderer_,
                context.world,
                context.shadow_opacity,
                interpolation,
                context.game_config.semi_transparent_objects);
            renderScenarioScreenParticles(
                renderer_, context.world);
            context.save_preview.captureIfRequested(
                renderer_.surface());
            if (!debug_active &&
                !context.gameplay_blackjack.active() &&
                !context.gameplay_equipment_color.active() &&
                !context.gameplay_options.active() &&
                !context.gameplay_mission_list.active() &&
                !context.gameplay_transport.active() &&
                !context.gameplay_vendor.active()) {
                renderGameplayOverlay(
                    renderer_,
                    context.world,
                    font,
                    context.resources.pattern(8),
                    context.world.renderCameraScreenX(
                        interpolation),
                    context.world.renderCameraScreenY(
                        interpolation),
                    interpolation);
            }
            const bool quest_notice_hidden =
                context.world.conversationActive() ||
                context.gameplay_blackjack.active() ||
                debug_active ||
                context.gameplay_equipment_color.active() ||
                context.gameplay_options.active() ||
                context.gameplay_inventory.anyItemPanelActive() ||
                context.gameplay_map.active() ||
                context.gameplay_magic.active() ||
                context.gameplay_status.active() ||
                context.gameplay_mission_list.active() ||
                context.gameplay_transport.active() ||
                context.gameplay_vendor.active();
            if (font && !quest_notice_hidden) {
                renderQuestNotice(
                    renderer_,
                    *font,
                    context.resources.pattern(8),
                    context.world.quests(),
                    context.world.missions());
            }
            const auto* magic_icons =
                context.resources.pattern(9);
            const auto* magic_bar_icons =
                context.resources.pattern(10);
            const auto* status =
                context.resources.pattern(6);
            const auto* cards =
                context.resources.pattern(11);
            if (status && cards &&
                context.gameplay_blackjack.active()) {
                renderGameplayBlackjack(
                    renderer_,
                    *cards,
                    *status,
                    context.gameplay_blackjack,
                    context.world,
                    context.gameplay_counter);
            } else if (status && font) {
                const auto* map_icons =
                    context.resources.pattern(7);
                if (context.gameplay_equipment_color.active()) {
                    renderGameplayEquipmentColor(
                        renderer_,
                        *status,
                        context.gameplay_equipment_color,
                        context.world,
                        context.gameplay_counter);
                }
#if OSF_ENABLE_DEBUG_TOOLS
                else if (debug_active) {
                    renderGameplayDebugMenu(
                        renderer_,
                        *status,
                        *font,
                        context.gameplay_debug);
                }
#endif
                else if (context.gameplay_inventory
                        .leftStorageActive()) {
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
                } else if (context.gameplay_vendor.active()) {
                    renderGameplayVendor(
                        renderer_,
                        *status,
                        context.gameplay_vendor,
                        context.world,
                        context.gameplay_counter);
                } else if (
                    context.gameplay_status.active()) {
                    renderGameplayStatusPanel(
                        renderer_,
                        *status,
                        *font,
                        context.gameplay_status,
                        context.world);
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
                if (!debug_active) {
                    if (context.gameplay_inventory.active()) {
                        renderGameplayInventory(
                            renderer_,
                            *status,
                            *font,
                            context.gameplay_inventory,
                            context.world,
                            context.gameplay_counter);
                    }
                }
            }
            const auto* bar =
                context.resources.pattern(5);
            if (bar) {
                renderGameplayHud(
                    renderer_,
                    *bar,
                    gameplayHudValues(
                        context.world.playerData(),
                        context.world.playerRuntimeProfile(),
                        context.world.playerMovementPace(),
                        context.world
                            .playerExperienceThreshold(),
                        context.world.playerCurrentLife(),
                        context.world.playerCurrentMana(),
                        context.world.playerIncreasedPowerReady(),
                        context.world
                            .playerIncreasedPowerActivationFeedback(),
                        context.gameplay_counter,
                        context.world.hasCompanion()
                            ? &context.world.companion()
                            : nullptr,
                        context.world.ownedCompanionInactive()));
                renderGameplayBeltItems(
                    renderer_,
                    context.world);
            }
            if (magic_icons && magic_bar_icons) {
                const bool left_panel_active =
                    context.gameplay_magic.active() ||
                    context.gameplay_status.active() ||
                    context.gameplay_map.active() ||
                    context.gameplay_mission_list.active() ||
                    context.gameplay_transport.active() ||
                    context.gameplay_vendor.active() ||
                    context.gameplay_inventory
                        .leftStorageActive();
                renderGameplayMagicBar(
                    renderer_,
                    *magic_icons,
                    *magic_bar_icons,
                    left_panel_active,
                    context.gameplay_inventory.active(),
                    context.world);
            }
            if (status && font &&
                !debug_active &&
                !context.gameplay_blackjack.active()) {
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
                renderVendorItemInformation(
                    renderer_,
                    *font,
                    context.gameplay_vendor,
                    context.world);
                if (magic_icons) {
                    renderHeldMagic(
                        renderer_,
                        *magic_icons,
                        context.gameplay_magic);
                }
            }
#if OSF_ENABLE_DEBUG_TOOLS
            if (font &&
                context.gameplay_debug.fpsCounterEnabled()) {
                renderGameplayDebugFps(
                    renderer_,
                    *font,
                    context.frames_per_second);
            }
            if (font &&
                context.gameplay_debug.profilingEnabled()) {
                renderGameplayProfiling(
                    renderer_,
                    *font,
                    context.profiling_metrics,
                    context.gameplay_debug.fpsCounterEnabled());
            }
#endif
        }
    }
    const auto* system_patterns =
        context.resources.pattern(3);
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
    return renderer_.surface();
}

}  // namespace osf::runtime
