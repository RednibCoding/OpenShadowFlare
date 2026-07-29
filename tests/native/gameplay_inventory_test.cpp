#include "gapi/gapi.hpp"
#include "items/item_database.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "render/gameplay_inventory_renderer.hpp"
#include "states/gameplay_inventory.hpp"
#include "world/world_scene.hpp"

#include <cstddef>
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
        const osf::gapi::BitmapImage&,
        const osf::gapi::BitmapDraw&) override {
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
    std::vector<TextCall> texts;
    std::vector<osf::gapi::RectangleDraw> rectangles;
};

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool testInventoryState() {
    osf::PlayerInventory owned;
    osf::PlayerEquipment equipment;
    osf::ItemDatabase database;
    osf::ItemDefinition sword;
    sword.category = 0;
    sword.id = 0;
    sword.inventory_width = 1;
    sword.inventory_height = 4;
    if (!owned.add(sword)) {
        return false;
    }

    osf::GameplayInventory inventory;
    inventory.update(
        {true, false, false, 0, 0},
        owned, equipment, database, 1);
    inventory.update(
        {false, false, false, 350, 350},
        owned, equipment, database, 1);
    if (!check(
            inventory.active() &&
                inventory.hoveredItemIndex() == 0,
            "The inventory did not use the item's full grid footprint.")) {
        return false;
    }
    const osf::GameplayInventoryResult pickup =
        inventory.update(
            {false, false, true, 350, 350},
            owned, equipment, database, 1);
    if (!check(
            pickup.pointer_consumed &&
                inventory.holdingItem() &&
                inventory.heldItem() &&
                owned.items().empty(),
            "Clicking an owned item did not put it on the pointer.")) {
        return false;
    }
    inventory.update(
        {false, false, true, 620, 380},
        owned, equipment, database, 1);
    if (!check(
            inventory.holdingItem() &&
                inventory.heldItem()->grid_x == 0 &&
                inventory.heldItem()->grid_y == 0 &&
                owned.items().empty(),
            "An invalid pointer placement moved the item.")) {
        return false;
    }
    inventory.update(
        {false, false, true, 480, 328},
        owned, equipment, database, 1);
    if (!check(
            !inventory.holdingItem() &&
                owned.items().size() == 1 &&
                owned.items()[0].grid_x == 4 &&
                owned.items()[0].grid_y == 0,
            "The centered pointer footprint was not placed.")) {
        return false;
    }
    inventory.update(
        {false, false, true, 480, 300},
        owned, equipment, database, 1);
    inventory.update(
        {true, false, false, 480, 300},
        owned, equipment, database, 1);
    const osf::GameplayInventoryResult drop =
        inventory.update(
            {false, false, true, 100, 200},
            owned, equipment, database, 1);
    if (!check(
            !inventory.active() &&
                inventory.holdingItem() &&
                drop.pointer_consumed &&
                drop.world_drop_requested &&
                drop.world_drop_screen_x == 100 &&
                drop.world_drop_screen_y == 200,
            "A held item did not survive closing the panel or request "
            "a world drop.")) {
        return false;
    }
    inventory.completeWorldDrop(false);
    inventory.update(
        {true, false, false, 480, 328},
        owned, equipment, database, 1);
    inventory.update(
        {false, false, true, 480, 328},
        owned, equipment, database, 1);
    if (!check(
            inventory.active() &&
                !inventory.holdingItem() &&
                owned.items().size() == 1,
            "A rejected world drop did not return to inventory "
            "placement cleanly.")) {
        return false;
    }
    inventory.update(
        {false, false, false, 400, 398},
        owned, equipment, database, 1);
    if (!check(
            inventory.closeHovered(),
            "The authored inventory Close tab did not hover.")) {
        return false;
    }
    const osf::GameplayInventoryResult result =
        inventory.update(
            {false, false, true, 400, 398},
            owned, equipment, database, 1);
    return check(
        !inventory.active() && result.pointer_consumed,
        "Clicking the inventory Close tab did not consume the click.");
}

bool testInventoryResourcesAndRendering() {
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    osf::WorldScene world;
    osf::PlayerLoadRequest player;
    player.name = "Mina";
    std::string error;
    if (!check(
            world.loadInitialScenario(
                data_root,
                player,
                &error),
            error.empty()
                ? "Remote Town could not prepare its inventory."
                : error.c_str())) {
        return false;
    }

    const std::int32_t normal_camera_x =
        world.cameraScreenX();
    world.setCameraAnchor(160, 240);
    if (!check(
            world.cameraScreenX() ==
                normal_camera_x + 160 &&
                world.itemInventoryPatterns()
                    .group(0) &&
                !world.itemInventoryPatterns()
                     .group(0)
                     ->patterns()
                     .empty() &&
                world.itemInventoryPatterns()
                    .group(13),
            "The inventory camera anchor or Item.njp groups differ.")) {
        return false;
    }

    osf::gapi::NjpImage status;
    osf::gapi::NjpImage font;
    if (!check(
            status.load(
                data_root / "System" / "Game" / "Pattern" /
                    "Status.njp",
                &error) &&
                font.load(
                    data_root / "System" / "Common" / "Pattern" /
                        "Font01.njp",
                    &error),
            error.empty()
                ? "The retail inventory artwork could not be loaded."
                : error.c_str())) {
        return false;
    }

    osf::GameplayInventory inventory;
    inventory.open();
    RecordingBackend renderer;
    osf::renderGameplayInventory(
        renderer,
        status,
        font,
        inventory,
        world);
    if (!check(
        renderer.patterns.size() == 4 &&
            renderer.rectangles.size() == 1 &&
            renderer.rectangles[0].x == 320 &&
            renderer.rectangles[0].width == 320 &&
            renderer.rectangles[0].height == 412 &&
            renderer.patterns[0].index == 2 &&
            renderer.patterns[1].index == 3 &&
            renderer.patterns[2].index == 0 &&
            renderer.patterns[3].index == 74 &&
            renderer.texts.size() == 6 &&
            renderer.texts[0].text == "Total Gold" &&
            renderer.texts[1].text == "Total Gold" &&
            renderer.texts[2].text == "0" &&
            renderer.texts[2].draw.x == 464 &&
            renderer.texts[4].text == "0",
        "The authored inventory frame or live values differ.")) {
        return false;
    }

    const osf::ItemDefinition* short_sword =
        world.itemDatabase().find(0, 0);
    if (!check(
            short_sword &&
                world.playerInventory().add(
                    *short_sword),
            "The held-item rendering fixture could not be prepared.")) {
        return false;
    }
    inventory.update(
        {false, false, true, 350, 300},
        world.playerInventory(),
        world.playerEquipment(),
        world.itemDatabase(),
        world.playerData().level());
    const osf::GameplayInventoryResult equipped =
        inventory.update(
            {false, false, true, 510, 80},
            world.playerInventory(),
            world.playerEquipment(),
            world.itemDatabase(),
            world.playerData().level());
    world.refreshPlayerAppearance();
    RecordingBackend equipped_renderer;
    osf::renderGameplayInventory(
        equipped_renderer,
        status,
        font,
        inventory,
        world);
    if (!check(
            equipped.equipment_changed &&
                equipped.pointer_consumed &&
                !inventory.holdingItem() &&
                world.playerEquipment().mainHand() &&
                world.playerEquipment().totalWeight(
                    world.itemDatabase()) == 30 &&
                world.playerEquipment().derivedParameterBonus(
                    0, world.itemDatabase()) == 20 &&
                world.playerEquipment().derivedParameterBonus(
                    1, world.itemDatabase()) == 100 &&
                world.playerPartEnabled(12) &&
                equipped_renderer.patterns.size() == 5 &&
                equipped_renderer.patterns[3].index == 0 &&
                equipped_renderer.patterns[3].draw.x == 496 &&
                equipped_renderer.patterns[3].draw.y == 16 &&
                equipped_renderer.texts[4].text == "30",
            "Equipping the Short Sword did not update ownership, "
            "derived values, artwork, and the CAF mask together.")) {
        return false;
    }

    const osf::GameplayInventoryResult unequipped =
        inventory.update(
            {false, false, true, 510, 80},
            world.playerInventory(),
            world.playerEquipment(),
            world.itemDatabase(),
            world.playerData().level());
    world.refreshPlayerAppearance();
    inventory.update(
        {false, false, true, 350, 328},
        world.playerInventory(),
        world.playerEquipment(),
        world.itemDatabase(),
        world.playerData().level());
    if (!check(
            unequipped.equipment_changed &&
                inventory.active() &&
                !inventory.holdingItem() &&
                !world.playerEquipment().mainHand() &&
                world.playerInventory().items().size() == 1 &&
                !world.playerPartEnabled(12),
            "The Short Sword did not return from the main hand to "
            "the backpack.")) {
        return false;
    }

    inventory.update(
        {false, false, true, 350, 300},
        world.playerInventory(),
        world.playerEquipment(),
        world.itemDatabase(),
        world.playerData().level());
    RecordingBackend held_renderer;
    osf::renderGameplayInventory(
        held_renderer,
        status,
        font,
        inventory,
        world);
    osf::renderHeldInventoryItem(
        held_renderer,
        inventory,
        world);
    return check(
        inventory.holdingItem() &&
            held_renderer.patterns.size() == 5 &&
            held_renderer.patterns.back().index == 0 &&
            held_renderer.patterns.back().draw.x == 334 &&
            held_renderer.patterns.back().draw.y == 236,
        "The held icon was not centered above the pointer.");
}

}  // namespace

int main() {
    return testInventoryState() &&
                   testInventoryResourcesAndRendering()
        ? 0
        : 1;
}
