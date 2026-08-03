#include "gameplay_map_renderer.hpp"

#include "gapi/gapi.hpp"
#include "libs/RKC_DIB/rkc_dib.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "states/gameplay_map.hpp"
#include "world/world_scene.hpp"

#include <cstdint>
#include <string>

namespace osf {
namespace {

constexpr gapi::Viewport kMapViewport{32, 40, 287, 335};

void drawAreaTitle(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const std::string& title) {
    renderer.drawRectangle({
        68,
        46,
        2 + static_cast<std::int32_t>(title.size()) * 6,
        20,
        {0, 0, 0, 255},
        1000,
        500,
    });
    renderer.drawText(
        font,
        title,
        {73, 51, {0, 0, 0, 255}});
    renderer.drawText(
        font,
        title,
        {72, 50, {224, 224, 224, 255}});
}

std::int32_t pulseStrength(std::int32_t counter) {
    return counter < 30
        ? (counter * 5 + 350) * 2
        : (130 - counter) * 10;
}

}  // namespace

void renderGameplayMap(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const gapi::NjpImage& map_icons,
    const GameplayMap& map,
    const WorldScene& world) {
    if (!map.active() || !world.hasPlayer()) {
        return;
    }

    renderer.drawRectangle({
        kMapViewport.x,
        kMapViewport.y,
        kMapViewport.width,
        kMapViewport.height,
        {0, 0, 0, 255},
        1000,
        1000,
    });

    const ScreenPosition player_real =
        calculateRealPosition({
            world.playerWorldX(),
            world.playerWorldY(),
        });
    const std::int32_t player_map_x = player_real.x / 10;
    const std::int32_t player_map_y = player_real.y / 10;
    const std::int32_t origin_x =
        player_map_x - 160 + map.scrollX();
    const std::int32_t origin_y =
        player_map_y - 210 + map.scrollY();

    renderer.drawPattern(
        world.mapOverviewPatterns(),
        0,
        {
            -origin_x,
            -origin_y,
            1000,
            1000,
            1000,
            1000,
            1000,
            1000,
            1000,
            -1,
            kMapViewport,
        });
    renderer.drawBitMask(
        world.mapExploration().mask(),
        {
            -origin_x,
            -origin_y,
            1000,
            1000,
            {0, 0, 0, 255},
            1000,
            kMapViewport,
        });

    if (map.markerVisible()) {
        const std::int32_t marker_x =
            player_map_x - origin_x;
        const std::int32_t marker_y =
            player_map_y - origin_y;
        renderer.drawPattern(
            map_icons,
            0,
            {
                marker_x,
                marker_y,
                1000,
                1000,
                1000,
                1000,
                1000,
                1000,
                1000,
                0,
                kMapViewport,
            });
        renderer.drawPattern(
            map_icons,
            1,
            {
                marker_x,
                marker_y,
                1000,
                1000,
                1000,
                1000,
                1000,
                1000,
                1000,
                0,
                kMapViewport,
            });
    }

    renderer.drawPattern(status_patterns, 71);
    const std::int32_t strength =
        pulseStrength(map.frameCounter());
    renderer.drawPattern(
        status_patterns,
        118,
        {
            0,
            0,
            1000,
            1000,
            1000,
            1000,
            strength,
            strength,
            strength,
        });
    drawAreaTitle(renderer, font, world.scenario().title());
}

}  // namespace osf
