#include "gapi/gapi.hpp"
#include "items/item_appearance.hpp"
#include "items/player_inventory.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "render/gameplay_equipment_color_renderer.hpp"
#include "states/gameplay_equipment_color.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

class RecordingBackend final : public osf::gapi::Backend {
public:
    struct PatternCall {
        const osf::gapi::NjpImage* image = nullptr;
        std::size_t pattern = 0;
        osf::gapi::PatternDraw draw;
    };

    void beginFrame(osf::gapi::Color) override {}

    bool drawPattern(
        const osf::gapi::NjpImage& image,
        std::size_t pattern,
        const osf::gapi::PatternDraw& draw) override {
        patterns.push_back({&image, pattern, draw});
        return true;
    }

    bool drawBitmap(
        const osf::gapi::BitmapImage&,
        const osf::gapi::BitmapDraw&) override {
        return true;
    }

    bool drawText(
        const osf::gapi::NjpImage&,
        std::string_view,
        const osf::gapi::TextDraw&) override {
        return true;
    }

    bool drawRectangle(
        const osf::gapi::RectangleDraw& draw) override {
        rectangles.push_back(draw);
        return true;
    }

    void endFrame() override {}

    std::vector<PatternCall> patterns;
    std::vector<osf::gapi::RectangleDraw> rectangles;
};

bool testRetailItemColorState() {
    osf::InventoryItem item;
    item.category = 0;
    item.durability = 80;
    item.identified = 1;
    if (!check(
            osf::setRetailItemColorIndex(item, -1) &&
                item.retail_state.size() == 200 &&
                osf::retailItemColorIndex(item) == -1 &&
                osf::setRetailItemColorIndex(item, 14) &&
                osf::retailItemColorIndex(item) == 14 &&
                !osf::setRetailItemColorIndex(item, 16),
            "The retail item color word or range differs.")) {
        return false;
    }
    const osf::ItemAppearanceStrength first =
        osf::retailItemColorStrength(0);
    const osf::ItemAppearanceStrength last =
        osf::retailItemColorStrength(15);
    return check(
        first.red == 800 && first.green == 250 && first.blue == 250 &&
            last.red == 300 && last.green == 300 && last.blue == 300,
        "The retail equipment-color strength table differs.");
}

bool testRetailEquipmentColorPanel() {
    osf::GameplayEquipmentColor empty_panel;
    empty_panel.open({false, false, false}, {-1, -1, -1});
    osf::GameplayEquipmentColorResult empty_result =
        empty_panel.update({false, true, 333, 188});
    if (!check(
            empty_panel.selectedTarget() ==
                    osf::EquipmentColorTarget::none &&
                empty_panel.selectedColor() == -1 &&
                !empty_result.color_changed,
            "A hero without colorable equipment selected a target.")) {
        return false;
    }

    osf::GameplayEquipmentColor panel;
    panel.open({true, true, true}, {-1, 3, 7});
    osf::GameplayEquipmentColorResult result =
        panel.update({false, true, 180, 225});
    if (!check(
            panel.active() &&
                panel.selectedTarget() ==
                    osf::EquipmentColorTarget::off_hand &&
                result.play_move_sound,
            "The retail off-hand color target could not be selected.")) {
        return false;
    }
    result = panel.update({false, true, 333, 188});
    if (!check(
            result.color_changed && result.color == 0 &&
                panel.selectedColor() == 0,
            "The first retail color swatch did not apply live.")) {
        return false;
    }
    result = panel.update({false, true, 410, 282});
    if (!check(
            result.color_changed && result.color == -1 &&
                panel.selectedColor() == -1 &&
                result.play_move_sound,
            "The retail default-color control did not clear the dye.")) {
        return false;
    }
    panel.update({false, false, 340, 305});
    if (!check(
            panel.cancelHovered(),
            "The authored Cancel hit rectangle differs.")) {
        return false;
    }
    result = panel.update({false, true, 340, 305});
    if (!check(
            result.cancelled && !panel.active() &&
                panel.originalColors() ==
                    std::array<std::int32_t, 3>{{-1, 3, 7}},
            "Cancel did not preserve the opening color snapshot.")) {
        return false;
    }

    panel.open({true, false, false}, {4, -1, -1});
    panel.update({false, false, 205, 305});
    result = panel.update({false, true, 205, 305});
    return check(
        result.accepted && result.play_move_sound && !panel.active(),
        "The authored OK hit rectangle did not accept the colors.");
}

bool testRetailEquipmentColorRendering() {
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    osf::WorldScene world;
    osf::PlayerLoadRequest player;
    player.name = "Mina";
    osf::gapi::NjpImage status;
    std::string error;
    if (!check(
            world.loadInitialScenario(data_root, player, &error) &&
                status.load(
                    data_root / "System" / "Game" / "Pattern" /
                        "Status.njp",
                    &error),
            error.empty()
                ? "The equipment-color render fixture could not load."
                : error.c_str())) {
        return false;
    }

    osf::GameplayEquipmentColor panel;
    panel.open({true, true, true}, {0, -1, -1});
    RecordingBackend renderer;
    osf::renderGameplayEquipmentColor(
        renderer, status, panel, world, 0);
    std::vector<RecordingBackend::PatternCall> status_calls;
    std::copy_if(
        renderer.patterns.begin(),
        renderer.patterns.end(),
        std::back_inserter(status_calls),
        [&status](const RecordingBackend::PatternCall& call) {
            return call.image == &status;
        });
    return check(
        status_calls.size() == 19 &&
            status_calls[0].pattern == 102 &&
            status_calls[1].pattern == 105 &&
            status_calls[2].pattern == 108 &&
            status_calls[3].pattern == 109 &&
            status_calls[3].draw.x == 0 &&
            status_calls[3].draw.y == 0 &&
            status_calls[3].draw.red_strength == 800 &&
            status_calls[3].draw.green_strength == 250 &&
            status_calls[3].draw.blue_strength == 250 &&
            status_calls[18].pattern == 109 &&
            status_calls[18].draw.x == 96 &&
            status_calls[18].draw.y == 72 &&
            status_calls[18].draw.red_strength == 300 &&
            renderer.rectangles.size() == 1 &&
            renderer.rectangles[0].x == 332 &&
            renderer.rectangles[0].y == 187,
        "The authored equipment-color panel, swatches, or selection "
        "frame differs.");
}

}  // namespace

int main() {
    return testRetailItemColorState() &&
                   testRetailEquipmentColorPanel() &&
                   testRetailEquipmentColorRendering()
               ? 0
               : 1;
}
