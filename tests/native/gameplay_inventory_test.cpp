#include "gapi/gapi.hpp"
#include "items/item_database.hpp"
#include "items/item_information.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "render/gameplay_inventory_renderer.hpp"
#include "render/item_information_renderer.hpp"
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
                world.playerEquipment().item(
                    osf::EquipmentSlot::main_hand) &&
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
                !world.playerEquipment().item(
                    osf::EquipmentSlot::main_hand) &&
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
    if (!check(
            inventory.holdingItem() &&
                held_renderer.patterns.size() == 5 &&
                held_renderer.patterns.back().index == 0 &&
                held_renderer.patterns.back().draw.x == 334 &&
                held_renderer.patterns.back().draw.y == 236,
            "The held icon was not centered above the pointer.")) {
        return false;
    }

    inventory.update(
        {false, false, true, 350, 328},
        world.playerInventory(),
        world.playerEquipment(),
        world.itemDatabase(),
        world.playerData().level());
    const osf::ItemDefinition* round_shield =
        world.itemDatabase().find(1, 1000000);
    if (!check(
            round_shield &&
                world.playerInventory().add(*round_shield),
            "The Round Shield equipment fixture could not be prepared.")) {
        return false;
    }
    const osf::InventoryItem shield =
        world.playerInventory().items().back();
    inventory.update(
        {
            false,
            false,
            true,
            osf::GameplayInventory::backpack_left +
                shield.grid_x *
                    osf::GameplayInventory::cell_size + 8,
            osf::GameplayInventory::backpack_top +
                shield.grid_y *
                    osf::GameplayInventory::cell_size + 8,
        },
        world.playerInventory(),
        world.playerEquipment(),
        world.itemDatabase(),
        world.playerData().level());
    inventory.update(
        {false, false, true, 590, 40},
        world.playerInventory(),
        world.playerEquipment(),
        world.itemDatabase(),
        world.playerData().level());
    if (!check(
            inventory.holdingItem() &&
                !world.playerEquipment().item(
                    osf::EquipmentSlot::helmet),
            "The helmet region accepted an off-hand item.")) {
        return false;
    }
    const osf::GameplayInventoryResult shield_equipped =
        inventory.update(
            {false, false, true, 510, 200},
            world.playerInventory(),
            world.playerEquipment(),
            world.itemDatabase(),
            world.playerData().level());
    world.refreshPlayerAppearance();
    RecordingBackend shield_renderer;
    osf::renderGameplayInventory(
        shield_renderer,
        status,
        font,
        inventory,
        world);
    if (!check(
        shield_equipped.equipment_changed &&
            !inventory.holdingItem() &&
            world.playerEquipment().item(
                osf::EquipmentSlot::off_hand) &&
            world.playerEquipment().totalWeight(
                world.itemDatabase()) == 40 &&
            world.playerPartEnabled(9) &&
            world.playerPartRedStrength(9) == 900 &&
            world.playerPartGreenStrength(9) == 800 &&
            world.playerPartBlueStrength(9) == 500 &&
            shield_renderer.patterns.size() == 6 &&
            shield_renderer.patterns[3].index == 45 &&
            shield_renderer.patterns[3].draw.x == 480 &&
            shield_renderer.patterns[3].draw.y == 176 &&
            shield_renderer.texts[4].text == "40",
        "The Round Shield did not use its retail off-hand region, "
        "weight, icon placement, and CAF colors.")) {
        return false;
    }
    for (std::int32_t update = 0; update < 3; ++update) {
        inventory.update(
            {false, false, false, 510, 200},
            world.playerInventory(),
            world.playerEquipment(),
            world.itemDatabase(),
            world.playerData().level());
    }
    if (!check(
            inventory.hoveredEquipmentSlot() ==
                osf::EquipmentSlot::off_hand &&
                inventory.informationItem(
                    world.playerInventory(),
                    world.playerEquipment()) ==
                    world.playerEquipment().item(
                        osf::EquipmentSlot::off_hand),
            "The equipped-item region did not share the retail "
            "information path.")) {
        return false;
    }

    const osf::ItemDefinition* gold =
        world.itemDatabase().find(4, 0);
    if (!check(
            gold &&
                world.playerInventory().add(*gold, 200),
            "The Gold information fixture could not be prepared.")) {
        return false;
    }
    const osf::InventoryItem& gold_item =
        world.playerInventory().items().back();
    const std::int32_t gold_pointer_x =
        osf::GameplayInventory::backpack_left +
        gold_item.grid_x *
            osf::GameplayInventory::cell_size + 8;
    const std::int32_t gold_pointer_y =
        osf::GameplayInventory::backpack_top +
        gold_item.grid_y *
            osf::GameplayInventory::cell_size + 8;
    for (std::int32_t update = 0; update < 3; ++update) {
        inventory.update(
            {
                false,
                false,
                false,
                gold_pointer_x,
                gold_pointer_y,
            },
            world.playerInventory(),
            world.playerEquipment(),
            world.itemDatabase(),
            world.playerData().level());
    }
    RecordingBackend gold_information_renderer;
    osf::renderItemInformation(
        gold_information_renderer,
        font,
        inventory,
        world);
    constexpr std::string_view expected_gold_information =
        "[Gold]\n"
        "\n"
        "Price                     :      200\n";
    if (!check(
            osf::itemInformationText(
                gold_item, *gold) ==
                expected_gold_information &&
            gold_information_renderer.texts.size() == 2 &&
            gold_information_renderer.texts[1].text ==
                expected_gold_information &&
            gold_information_renderer.rectangles.size() == 5 &&
            gold_information_renderer.rectangles[0].x ==
                gold_pointer_x - 112 &&
            gold_information_renderer.rectangles[0].y ==
                gold_pointer_y + 8 &&
            gold_information_renderer.rectangles[0].width == 224 &&
            gold_information_renderer.rectangles[0].height == 44,
            "The Gold information price, faded backing, or "
            "retail dimensions differ.")) {
        return false;
    }

    const osf::ItemDefinition* dagger =
        world.itemDatabase().find(0, 100);
    if (!check(
            dagger &&
                world.playerInventory().add(*dagger),
            "The Dagger information fixture could not be prepared.")) {
        return false;
    }
    const osf::InventoryItem& dagger_item =
        world.playerInventory().items().back();
    const std::int32_t pointer_x =
        osf::GameplayInventory::backpack_left +
        dagger_item.grid_x *
            osf::GameplayInventory::cell_size + 8;
    const std::int32_t pointer_y =
        osf::GameplayInventory::backpack_top +
        dagger_item.grid_y *
            osf::GameplayInventory::cell_size + 8;
    inventory.update(
        {false, false, false, pointer_x, pointer_y},
        world.playerInventory(),
        world.playerEquipment(),
        world.itemDatabase(),
        world.playerData().level());
    inventory.update(
        {false, false, false, pointer_x, pointer_y},
        world.playerInventory(),
        world.playerEquipment(),
        world.itemDatabase(),
        world.playerData().level());
    if (!check(
            !inventory.informationItem(
                world.playerInventory(),
                world.playerEquipment()),
            "The item information appeared before retail's hover delay.")) {
        return false;
    }
    inventory.update(
        {false, false, false, pointer_x, pointer_y},
        world.playerInventory(),
        world.playerEquipment(),
        world.itemDatabase(),
        world.playerData().level());
    RecordingBackend information_renderer;
    osf::renderItemInformation(
        information_renderer,
        font,
        inventory,
        world);
    osf::InventoryItem damaged_dagger = dagger_item;
    damaged_dagger.durability = 150;
    constexpr std::string_view expected_information =
        "[Dagger]\n"
        "\n"
        "Attack                    :       10\n"
        "Hit Rate                  :      120\n"
        "Speed of Attack           :       50\n"
        "Durability                :      300\n"
        "Weight                    :       10\n"
        "Required Level            :        1\n"
        "Sale Price                :      100\n"
        "\n"
        "Fire   :  0 Water  :  0 Earth  :  0 Thunder:  0\n"
        "Holy   :  0 Dark   :  0 Gel    :  0 Metal  :  0\n";
    return check(
        dagger_item.durability == 300 &&
            osf::itemSalePrice(
                dagger_item, *dagger) == 100 &&
            osf::itemSalePrice(
                damaged_dagger, *dagger) == 50 &&
            osf::itemInformationText(
                dagger_item, *dagger) ==
                expected_information &&
            inventory.informationItem(
                world.playerInventory(),
                world.playerEquipment()) ==
                &dagger_item &&
            information_renderer.texts.size() == 2 &&
            information_renderer.texts[0].text ==
                expected_information &&
            information_renderer.texts[0].draw.color.red == 0 &&
            information_renderer.texts[1].draw.x ==
                pointer_x - 141 &&
            information_renderer.texts[1].draw.y ==
                pointer_y + 12 &&
            information_renderer.texts[1].draw.letter_spacing == 0 &&
            information_renderer.texts[1].draw.color.red == 224 &&
            information_renderer.texts[1].draw.color.green == 224 &&
            information_renderer.texts[1].draw.color.blue == 224 &&
            information_renderer.rectangles.size() == 5 &&
            information_renderer.rectangles[0].x ==
                pointer_x - 145 &&
            information_renderer.rectangles[0].y ==
                pointer_y + 8 &&
            information_renderer.rectangles[0].width == 290 &&
            information_renderer.rectangles[0].height == 152 &&
            information_renderer.rectangles[0].color.red == 0 &&
            information_renderer.rectangles[0].opacity == 600 &&
            information_renderer.rectangles[1].x ==
                pointer_x - 146 &&
            information_renderer.rectangles[1].y ==
                pointer_y + 7 &&
            information_renderer.rectangles[1].width == 291 &&
            information_renderer.rectangles[1].height == 1 &&
            information_renderer.rectangles[1].color.red == 255 &&
            information_renderer.rectangles[1].opacity == 500 &&
            information_renderer.rectangles[2].width == 1 &&
            information_renderer.rectangles[2].height == 153 &&
            information_renderer.rectangles[3].x ==
                pointer_x + 144 &&
            information_renderer.rectangles[3].height == 152 &&
            information_renderer.rectangles[4].y ==
                pointer_y + 159 &&
            information_renderer.rectangles[4].width == 290,
        "The Dagger information text, delay, color, faded "
        "backing, frame, or pointer placement differs from "
        "retail.");
}

}  // namespace

int main() {
    return testInventoryState() &&
                   testInventoryResourcesAndRendering()
        ? 0
        : 1;
}
