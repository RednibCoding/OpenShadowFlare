#include "gapi/gapi.hpp"
#include "libs/RKC_DBFCONTROL/rkc_dbfcontrol.hpp"
#include "libs/RKC_DIB/rkc_dib.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "render/gameplay_map_renderer.hpp"
#include "states/gameplay_map.hpp"
#include "states/gameplay_options_menu.hpp"
#include "world/world_scene.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct PatternCall {
    const osf::gapi::NjpImage* image = nullptr;
    std::size_t index = 0;
    osf::gapi::PatternDraw draw;
};

struct BitmapCall {
    const osf::gapi::BitmapImage* image = nullptr;
    osf::gapi::BitmapDraw draw;
};

struct TextCall {
    std::string text;
    osf::gapi::TextDraw draw;
};

class RecordingBackend final : public osf::gapi::Backend {
public:
    void beginFrame(osf::gapi::Color) override {}

    bool drawPattern(
        const osf::gapi::NjpImage& image,
        std::size_t pattern_index,
        const osf::gapi::PatternDraw& draw) override {
        patterns.push_back({&image, pattern_index, draw});
        return true;
    }

    bool drawBitmap(
        const osf::gapi::BitmapImage& image,
        const osf::gapi::BitmapDraw& draw) override {
        bitmaps.push_back({&image, draw});
        return true;
    }

    bool drawText(
        const osf::gapi::NjpImage&,
        std::string_view text,
        const osf::gapi::TextDraw& draw) override {
        texts.push_back({std::string(text), draw});
        return true;
    }

    bool drawRectangle(
        const osf::gapi::RectangleDraw& draw) override {
        rectangles.push_back(draw);
        return true;
    }

    void endFrame() override {}

    std::vector<PatternCall> patterns;
    std::vector<BitmapCall> bitmaps;
    std::vector<TextCall> texts;
    std::vector<osf::gapi::RectangleDraw> rectangles;
};

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool testMapState() {
    osf::GameplayMap map;
    map.open();
    map.update({
        false, false,
        true, true, false, false, false,
    });
    if (!check(
            map.active() &&
                map.scrollX() == -16 &&
                map.scrollY() == -10 &&
                map.frameCounter() == 1 &&
                map.markerVisible(),
            "The Map did not apply the retail arrow-key scroll step.")) {
        return false;
    }

    for (std::int32_t index = 0; index < 14; ++index) {
        map.update({});
    }
    if (!check(
            !map.markerVisible(),
            "The Map player marker did not use its 15-of-20 blink.")) {
        return false;
    }
    map.update({
        false, false,
        false, false, false, false, true,
    });
    if (!check(
            map.scrollX() == 0 && map.scrollY() == 0,
            "Enter did not recenter the Map.")) {
        return false;
    }
    map.update({});
    if (!check(
            map.active(),
            "The live Map did not remain open during world play.")) {
        return false;
    }
    map.update({false, true});
    if (!check(
            !map.active(),
            "Escape did not close the Map.")) {
        return false;
    }
    map.update({true});
    map.update({true});
    return check(
        !map.active(),
        "The N shortcut did not toggle the Map.");
}

bool testMapResourcesAndRendering() {
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    osf::WorldScene world;
    osf::PlayerLoadRequest player;
    player.name = "Mina";
    std::string error;
    if (!check(
            world.loadInitialScenario(
                data_root, player, &error),
            error.empty()
                ? "Remote Town could not prepare its Map."
                : error.c_str())) {
        return false;
    }

    const osf::MapExploration& exploration =
        world.mapExploration();
    const std::int32_t normal_camera_x =
        world.cameraScreenX();
    world.setCameraAnchor(480, 240);
    if (!check(
            world.mapOverviewPatterns().patterns().size() == 1 &&
                world.cameraScreenX() ==
                    normal_camera_x - 160 &&
                exploration.mask().width() == 1920 &&
                exploration.mask().height() == 1440 &&
                exploration.explored(1272, 904) &&
                exploration.explored(1339, 949) &&
                !exploration.explored(1271, 904) &&
                !exploration.explored(1340, 949),
            "Remote Town's live Map camera or exploration data differs.")) {
        return false;
    }

    osf::gapi::NjpImage status;
    osf::gapi::NjpImage font;
    osf::gapi::NjpImage icons;
    if (!check(
            status.load(
                data_root / "System" / "Game" / "Pattern" /
                    "Status.njp",
                &error) &&
                font.load(
                    data_root / "System" / "Common" / "Pattern" /
                        "Font01.njp",
                    &error) &&
                icons.load(
                    data_root / "System" / "Game" / "Pattern" /
                        "MapIcon.njp",
                    &error),
            error.empty()
                ? "The retail Map artwork could not be loaded."
                : error.c_str())) {
        return false;
    }

    osf::GameplayMap map;
    map.open();
    RecordingBackend renderer;
    osf::renderGameplayMap(
        renderer, status, font, icons, map, world);
    if (!check(
        renderer.patterns.size() == 5 &&
            renderer.patterns[0].image ==
                &world.mapOverviewPatterns() &&
            renderer.patterns[0].index == 0 &&
            renderer.patterns[0].draw.x == -1146 &&
            renderer.patterns[0].draw.y == -717 &&
            renderer.patterns[0].draw.clip.x == 32 &&
            renderer.patterns[0].draw.clip.y == 40 &&
            renderer.patterns[0].draw.clip.width == 287 &&
            renderer.patterns[0].draw.clip.height == 335 &&
            renderer.patterns[1].image == &icons &&
            renderer.patterns[1].index == 0 &&
            renderer.patterns[1].draw.x == 160 &&
            renderer.patterns[1].draw.y == 210 &&
            renderer.patterns[2].index == 1 &&
            renderer.patterns[3].index == 71 &&
            renderer.patterns[4].index == 118 &&
            renderer.patterns[4].draw.red_strength == 700 &&
            renderer.bitmaps.size() == 1 &&
            renderer.bitmaps[0].image ==
                &exploration.mask() &&
            renderer.bitmaps[0].draw.x == -1146 &&
            renderer.bitmaps[0].draw.y == -717 &&
            renderer.rectangles.size() == 2 &&
            renderer.rectangles[0].x == 32 &&
            renderer.rectangles[0].y == 40 &&
            renderer.rectangles[1].x == 68 &&
            renderer.rectangles[1].y == 46 &&
            renderer.texts.size() == 2 &&
            renderer.texts[0].text == "Remote Town" &&
            renderer.texts[0].draw.x == 73 &&
            renderer.texts[1].draw.x == 72,
        "The authored Map frame, origin, mask, marker, or title differs.")) {
        return false;
    }

    osf::gapi::SoftwareBackend software(640, 480);
    for (std::int32_t frame = 0; frame < 6; ++frame) {
        software.beginFrame({0, 0, 0, 255});
        osf::renderGameplayMap(
            software, status, font, icons, map, world);
        software.endFrame();
    }
    const osf::gapi::SurfaceView surface = software.surface();
    bool authored_pixel_found = false;
    for (std::int32_t y = 0;
         y < surface.height && !authored_pixel_found;
         ++y) {
        for (std::int32_t x = 0; x < 320; ++x) {
            const osf::gapi::Color color =
                surface.pixels[
                    static_cast<std::size_t>(y) *
                        static_cast<std::size_t>(surface.width) +
                    static_cast<std::size_t>(x)];
            if (color.red != 0 ||
                color.green != 0 ||
                color.blue != 0) {
                authored_pixel_found = true;
                break;
            }
        }
    }
    return check(
        authored_pixel_found,
        "The software renderer did not draw the authored Map panel.");
}

bool testOptionsEntry() {
    osf::GameplayOptionsMenu menu;
    osf::GameConfig config;
    menu.update({true, false, false, 0, 0}, config);
    const osf::GameplayOptionsResult result =
        menu.update({false, true, true, 300, 270}, config);
    return check(
        result.action == osf::GameplayOptionsAction::open_map &&
            result.play_confirm_sound,
        "The Settings Map row is not wired.");
}

}  // namespace

int main() {
    return testMapState() &&
                   testMapResourcesAndRendering() &&
                   testOptionsEntry()
        ? 0
        : 1;
}
