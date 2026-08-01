#include "gapi/gapi.hpp"
#include "items/item_database.hpp"
#include "items/item_information.hpp"
#include "items/player_belt.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"
#include "items/player_special_items.hpp"
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
#include <utility>
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
    osf::PlayerBelt belt;
    osf::PlayerSpecialItems special_items;
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
        owned, equipment, belt, special_items, database, 1);
    inventory.update(
        {false, false, false, 350, 350},
        owned, equipment, belt, special_items, database, 1);
    if (!check(
            inventory.active() &&
                inventory.hoveredItemIndex() == 0,
            "The inventory did not use the item's full grid footprint.")) {
        return false;
    }
    const osf::GameplayInventoryResult pickup =
        inventory.update(
            {false, false, true, 350, 350},
            owned, equipment, belt, special_items, database, 1);
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
        owned, equipment, belt, special_items, database, 1);
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
        owned, equipment, belt, special_items, database, 1);
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
        owned, equipment, belt, special_items, database, 1);
    inventory.update(
        {true, false, false, 480, 300},
        owned, equipment, belt, special_items, database, 1);
    const osf::GameplayInventoryResult drop =
        inventory.update(
            {false, false, true, 100, 200},
            owned, equipment, belt, special_items, database, 1);
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
        owned, equipment, belt, special_items, database, 1);
    inventory.update(
        {false, false, true, 480, 328},
        owned, equipment, belt, special_items, database, 1);
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
        owned, equipment, belt, special_items, database, 1);
    if (!check(
            inventory.closeHovered(),
            "The authored inventory Close tab did not hover.")) {
        return false;
    }
    const osf::GameplayInventoryResult result =
        inventory.update(
            {false, false, true, 400, 398},
            owned, equipment, belt, special_items, database, 1);
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
    player.gender =
        osf::playerGenderValue(osf::PlayerGender::male);
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
        world,
        0);
    if (!check(
        renderer.patterns.size() == 14 &&
            renderer.rectangles.size() == 1 &&
            renderer.rectangles[0].x == 320 &&
            renderer.rectangles[0].width == 320 &&
            renderer.rectangles[0].height == 412 &&
            renderer.patterns[0].index == 2 &&
            renderer.patterns[1].index == 3 &&
            renderer.patterns[2].index == 0 &&
            renderer.patterns.back().index == 74 &&
            renderer.patterns[3].index == 67 &&
            renderer.texts.size() == 12 &&
            renderer.texts[0].text == "Total Gold" &&
            renderer.texts[1].text == "Total Gold" &&
            renderer.texts[2].text == "0" &&
            renderer.texts[2].draw.x == 464 &&
            renderer.texts[4].text == "5" &&
            renderer.texts[5].draw.color.red == 224 &&
            renderer.texts[5].draw.color.green == 224 &&
            renderer.texts[5].draw.color.blue == 224 &&
            renderer.texts[6].text == "/" &&
            renderer.texts[8].text == "10" &&
            renderer.texts[10].text == "70",
        "The authored inventory frame or live values differ.")) {
        return false;
    }
    world.playerInventory().clear();
    world.playerEquipment().clear();
    world.playerBelt().clear();
    world.refreshPlayerAppearance();

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
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    const osf::GameplayInventoryResult equipped =
        inventory.update(
            {false, false, true, 510, 80},
            world.playerInventory(),
            world.playerEquipment(),
            world.playerBelt(),
            world.playerSpecialItems(),
            world.itemDatabase(),
            world.playerData().level());
    world.refreshPlayerAppearance();
    RecordingBackend equipped_renderer;
    osf::renderGameplayInventory(
        equipped_renderer,
        status,
        font,
        inventory,
        world,
        0);
    if (!check(
            equipped.equipment_changed &&
                equipped.item_sound_sample == 49 &&
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
                equipped_renderer.patterns.size() == 6 &&
                equipped_renderer.patterns[4].index == 0 &&
                equipped_renderer.patterns[4].draw.x == 496 &&
                equipped_renderer.patterns[4].draw.y == 16 &&
                equipped_renderer.texts[10].text == "30",
            "Equipping the Short Sword did not update ownership, "
            "derived values, artwork, and the CAF mask together.")) {
        return false;
    }

    const osf::GameplayInventoryResult unequipped =
        inventory.update(
            {false, false, true, 510, 80},
            world.playerInventory(),
            world.playerEquipment(),
            world.playerBelt(),
            world.playerSpecialItems(),
            world.itemDatabase(),
            world.playerData().level());
    world.refreshPlayerAppearance();
    inventory.update(
        {false, false, true, 350, 328},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    if (!check(
            unequipped.equipment_changed &&
                unequipped.item_sound_sample == 48 &&
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
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    RecordingBackend held_renderer;
    osf::renderGameplayInventory(
        held_renderer,
        status,
        font,
        inventory,
        world,
        0);
    osf::renderHeldInventoryItem(
        held_renderer,
        status,
        inventory,
        world,
        0);
    if (!check(
            inventory.holdingItem() &&
                held_renderer.patterns.size() == 6 &&
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
        world.playerBelt(),
        world.playerSpecialItems(),
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
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    inventory.update(
        {false, false, true, 590, 40},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
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
            world.playerBelt(),
            world.playerSpecialItems(),
            world.itemDatabase(),
            world.playerData().level());
    world.refreshPlayerAppearance();
    RecordingBackend shield_renderer;
    osf::renderGameplayInventory(
        shield_renderer,
        status,
        font,
        inventory,
        world,
        0);
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
            shield_renderer.patterns.size() == 7 &&
            shield_renderer.patterns[4].index == 45 &&
            shield_renderer.patterns[4].draw.x == 480 &&
            shield_renderer.patterns[4].draw.y == 176 &&
            shield_renderer.texts[10].text == "40",
        "The Round Shield did not use its retail off-hand region, "
        "weight, icon placement, and CAF colors.")) {
        return false;
    }
    for (std::int32_t update = 0; update < 3; ++update) {
        inventory.update(
            {false, false, false, 510, 200},
            world.playerInventory(),
            world.playerEquipment(),
            world.playerBelt(),
            world.playerSpecialItems(),
            world.itemDatabase(),
            world.playerData().level());
    }
    if (!check(
            inventory.hoveredEquipmentSlot() ==
                osf::EquipmentSlot::off_hand &&
                inventory.informationItem(
                    world.playerInventory(),
                    world.playerEquipment(),
                    world.playerSpecialItems()) ==
                    world.playerEquipment().item(
                        osf::EquipmentSlot::off_hand),
            "The equipped-item region did not share the retail "
            "information path.")) {
        return false;
    }

    const osf::ItemDefinition* tablet =
        world.itemDatabase().find(3, 0);
    if (!check(
            tablet &&
                world.playerInventory().add(*tablet),
            "The Tablet information fixture could not be prepared.")) {
        return false;
    }
    const osf::InventoryItem& tablet_item =
        world.playerInventory().items().back();
    const std::int32_t tablet_pointer_x =
        osf::GameplayInventory::backpack_left +
        tablet_item.grid_x *
            osf::GameplayInventory::cell_size + 8;
    const std::int32_t tablet_pointer_y =
        osf::GameplayInventory::backpack_top +
        tablet_item.grid_y *
            osf::GameplayInventory::cell_size + 8;
    for (std::int32_t update = 0; update < 3; ++update) {
        inventory.update(
            {
                false,
                false,
                false,
                tablet_pointer_x,
                tablet_pointer_y,
            },
            world.playerInventory(),
            world.playerEquipment(),
            world.playerBelt(),
            world.playerSpecialItems(),
            world.itemDatabase(),
            world.playerData().level());
    }
    RecordingBackend tablet_information_renderer;
    osf::renderItemInformation(
        tablet_information_renderer,
        font,
        inventory,
        world);
    const std::string tablet_information =
        osf::itemInformationText(
            tablet_item, *tablet);
    if (!check(
            tablet_information.rfind(
                "[Tablet]\n\n"
                "Sale Price                :",
                0) == 0 &&
                tablet_information_renderer.texts.size() == 2 &&
                tablet_information_renderer.texts[1].text ==
                    tablet_information &&
                tablet_information_renderer.rectangles.size() == 5 &&
                tablet_information_renderer.rectangles[0].x ==
                    tablet_pointer_x - 112 &&
                tablet_information_renderer.rectangles[0].y ==
                    tablet_pointer_y + 8 &&
                tablet_information_renderer.rectangles[0].width == 224 &&
                tablet_information_renderer.rectangles[0].height == 44,
            "The one-cell Tablet information omitted its retail "
            "sale-price row or popup dimensions.")) {
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
            world.playerBelt(),
            world.playerSpecialItems(),
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
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    inventory.update(
        {false, false, false, pointer_x, pointer_y},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    if (!check(
            !inventory.informationItem(
                world.playerInventory(),
                world.playerEquipment(),
                    world.playerSpecialItems()),
            "The item information appeared before retail's hover delay.")) {
        return false;
    }
    inventory.update(
        {false, false, false, pointer_x, pointer_y},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
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
                world.playerEquipment(),
                    world.playerSpecialItems()) ==
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

bool testConditionArtwork() {
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
                ? "The condition-artwork world could not be prepared."
                : error.c_str())) {
        return false;
    }
    world.playerInventory().clear();
    world.playerEquipment().clear();
    world.playerBelt().clear();
    world.refreshPlayerAppearance();

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
                    &error) &&
                status.patterns().size() > 17 &&
                status.patterns()[16].width == 14 &&
                status.patterns()[16].height == 15,
            "The retail condition pattern is unavailable or changed.")) {
        return false;
    }

    const osf::ItemDefinition* dagger =
        world.itemDatabase().find(0, 100);
    if (!check(
            dagger &&
                world.playerInventory().add(*dagger),
            "The condition-artwork Dagger could not be created.")) {
        return false;
    }

    std::optional<osf::InventoryItem> low_dagger =
        world.playerInventory().take(0);
    if (!low_dagger) {
        return false;
    }
    low_dagger->durability = 29;
    const std::int32_t backpack_x =
        osf::GameplayInventory::backpack_left +
        low_dagger->grid_x *
            osf::GameplayInventory::cell_size;
    const std::int32_t backpack_y =
        osf::GameplayInventory::backpack_top +
        low_dagger->grid_y *
            osf::GameplayInventory::cell_size;
    const std::int32_t backpack_warning_x =
        backpack_x +
        low_dagger->width *
            osf::GameplayInventory::cell_size -
        16;
    const std::int32_t backpack_warning_y =
        backpack_y +
        low_dagger->height *
            osf::GameplayInventory::cell_size -
        16;
    if (!world.playerInventory()
             .place(
                 *low_dagger,
                 low_dagger->grid_x,
                 low_dagger->grid_y)
             .accepted) {
        return false;
    }

    osf::GameplayInventory inventory;
    inventory.open();
    RecordingBackend warning_on;
    osf::renderGameplayInventory(
        warning_on,
        status,
        font,
        inventory,
        world,
        7);
    if (!check(
            warning_on.patterns.size() == 7 &&
                warning_on.patterns[5].image == &status &&
                warning_on.patterns[5].index == 16 &&
                warning_on.patterns[5].draw.x ==
                    backpack_warning_x &&
                warning_on.patterns[5].draw.y ==
                    backpack_warning_y,
            "Low durability did not draw Status pattern 16 at "
            "the retail backpack corner.")) {
        return false;
    }

    RecordingBackend warning_off;
    osf::renderGameplayInventory(
        warning_off,
        status,
        font,
        inventory,
        world,
        8);
    if (!check(
            warning_off.patterns.size() == 6,
            "The low-durability warning did not blink off.")) {
        return false;
    }

    std::optional<osf::InventoryItem> broken_dagger =
        world.playerInventory().take(0);
    if (!broken_dagger) {
        return false;
    }
    broken_dagger->durability = 0;
    if (!world.playerInventory()
             .place(
                 *broken_dagger,
                 broken_dagger->grid_x,
                 broken_dagger->grid_y)
             .accepted) {
        return false;
    }
    RecordingBackend broken;
    osf::renderGameplayInventory(
        broken,
        status,
        font,
        inventory,
        world,
        8);
    if (!check(
            broken.patterns.size() == 7 &&
                broken.patterns[5].image == &status &&
                broken.patterns[5].index == 16,
            "A broken backpack item did not keep its warning visible.")) {
        return false;
    }

    const std::int32_t pointer_x = backpack_x + 8;
    const std::int32_t pointer_y = backpack_y + 8;
    inventory.update(
        {false, false, true, pointer_x, pointer_y},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    RecordingBackend held;
    osf::renderGameplayInventory(
        held,
        status,
        font,
        inventory,
        world,
        8);
    osf::renderHeldInventoryItem(
        held,
        status,
        inventory,
        world,
        8);
    if (!check(
            inventory.holdingItem() &&
                held.patterns.size() == 7 &&
                held.patterns.back().image == &status &&
                held.patterns.back().index == 16 &&
                held.patterns.back().draw.x ==
                    pointer_x +
                        broken_dagger->width *
                            osf::GameplayInventory::cell_size /
                            2 -
                        16 &&
                held.patterns.back().draw.y ==
                    pointer_y +
                        broken_dagger->height *
                            osf::GameplayInventory::cell_size /
                            2 -
                        16,
            "The held broken item lost its retail condition corner.")) {
        return false;
    }

    const osf::GameplayInventoryResult equipped =
        inventory.update(
            {false, false, true, 510, 80},
            world.playerInventory(),
            world.playerEquipment(),
            world.playerBelt(),
            world.playerSpecialItems(),
            world.itemDatabase(),
            world.playerData().level());
    const osf::InventoryItem* equipped_dagger =
        world.playerEquipment().item(
            osf::EquipmentSlot::main_hand);
    if (!equipped_dagger) {
        return false;
    }
    const osf::EquipmentRegion region =
        osf::GameplayInventory::equipmentRegion(
            osf::EquipmentSlot::main_hand);
    const std::int32_t equipped_x =
        region.left +
        (region.width_in_cells - equipped_dagger->width) *
            osf::GameplayInventory::cell_size / 2;
    const std::int32_t equipped_y =
        region.top +
        (region.height_in_cells - equipped_dagger->height) *
            osf::GameplayInventory::cell_size / 2;
    RecordingBackend equipped_renderer;
    osf::renderGameplayInventory(
        equipped_renderer,
        status,
        font,
        inventory,
        world,
        8);
    return check(
        equipped.equipment_changed &&
            !inventory.holdingItem() &&
            equipped_renderer.patterns.size() == 7 &&
            equipped_renderer.patterns[5].image == &status &&
            equipped_renderer.patterns[5].index == 16 &&
            equipped_renderer.patterns[5].draw.x ==
                equipped_x +
                    equipped_dagger->width *
                        osf::GameplayInventory::cell_size -
                    16 &&
            equipped_renderer.patterns[5].draw.y ==
                equipped_y +
                    equipped_dagger->height *
                        osf::GameplayInventory::cell_size -
                    16,
        "Equipped broken gear did not use the same retail "
        "condition placement.");
}

bool testAccessoryAndBeltOwnership() {
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
                ? "The accessory-and-belt world could not be prepared."
                : error.c_str())) {
        return false;
    }
    world.playerInventory().clear();
    world.playerEquipment().clear();
    world.playerBelt().clear();
    world.refreshPlayerAppearance();

    const osf::ItemDefinition* accessory =
        world.itemDatabase().find(2, 1000000);
    const osf::ItemDefinition* tablet =
        world.itemDatabase().find(3, 0);
    const osf::ItemDefinition* capsule =
        world.itemDatabase().find(3, 10000000);
    if (!check(
            accessory && tablet && capsule,
            "The retail accessory or belt fixtures are unavailable.")) {
        return false;
    }

    osf::GameplayInventory inventory;
    inventory.open();
    if (!world.playerInventory().add(*accessory)) {
        return false;
    }
    inventory.update(
        {false, false, true, 350, 278},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    const osf::GameplayInventoryResult equipped =
        inventory.update(
            {false, false, true, 410, 150},
            world.playerInventory(),
            world.playerEquipment(),
            world.playerBelt(),
            world.playerSpecialItems(),
            world.itemDatabase(),
            world.playerData().level());
    if (!check(
            equipped.equipment_changed &&
                equipped.item_sound_sample == 93 &&
                !inventory.holdingItem() &&
                world.playerEquipment().item(
                    osf::EquipmentSlot::accessory_1),
            "The first retail accessory cell did not accept category two.")) {
        return false;
    }

    for (std::int32_t update = 0; update < 3; ++update) {
        inventory.update(
            {false, false, false, 410, 150},
            world.playerInventory(),
            world.playerEquipment(),
            world.playerBelt(),
            world.playerSpecialItems(),
            world.itemDatabase(),
            world.playerData().level());
    }
    if (!check(
            inventory.hoveredEquipmentSlot() ==
                    osf::EquipmentSlot::accessory_1 &&
                inventory.informationItem(
                    world.playerInventory(),
                    world.playerEquipment(),
                    world.playerSpecialItems()) ==
                    world.playerEquipment().item(
                        osf::EquipmentSlot::accessory_1),
            "Accessory hover did not join the shared item-information path.")) {
        return false;
    }

    osf::gapi::NjpImage status;
    osf::gapi::NjpImage font;
    if (!status.load(
            data_root / "System" / "Game" / "Pattern" /
                "Status.njp",
            &error) ||
        !font.load(
            data_root / "System" / "Common" / "Pattern" /
                "Font01.njp",
            &error)) {
        return false;
    }
    RecordingBackend accessory_renderer;
    osf::renderGameplayInventory(
        accessory_renderer,
        status,
        font,
        inventory,
        world,
        0);
    bool accessory_drawn = false;
    for (const PatternCall& call :
         accessory_renderer.patterns) {
        accessory_drawn =
            accessory_drawn ||
            (call.image != &status &&
             call.index ==
                 static_cast<std::size_t>(
                     accessory->inventory_pattern) &&
             call.draw.x == 400 &&
             call.draw.y == 143);
    }
    if (!check(
            accessory_drawn,
            "Accessory artwork did not use its exact retail cell origin.")) {
        return false;
    }

    if (!world.playerInventory().add(*tablet)) {
        return false;
    }
    inventory.update(
        {false, false, true, 350, 278},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    const osf::GameplayInventoryResult belt_placement =
        inventory.update(
            {false, false, true, 360, 416},
            world.playerInventory(),
            world.playerEquipment(),
            world.playerBelt(),
            world.playerSpecialItems(),
            world.itemDatabase(),
            world.playerData().level());
    if (!check(
            belt_placement.pointer_consumed &&
                belt_placement.item_sound_sample == 48 &&
                !inventory.holdingItem() &&
                world.playerBelt().itemAt(0, 0) &&
                world.playerBelt().itemAt(0, 0)->definition_id ==
                    tablet->id,
            "A category-three item did not enter the first belt pocket.")) {
        return false;
    }

    RecordingBackend belt_renderer;
    osf::renderGameplayBeltItems(
        belt_renderer,
        world);
    if (!check(
            belt_renderer.patterns.size() == 1 &&
                belt_renderer.patterns[0].index ==
                    static_cast<std::size_t>(
                        tablet->inventory_pattern) &&
                belt_renderer.patterns[0].draw.x == 357 &&
                belt_renderer.patterns[0].draw.y == 413,
            "The first belt item did not use the retail HUD origin.")) {
        return false;
    }

    if (!world.playerInventory().add(*capsule)) {
        return false;
    }
    inventory.update(
        {false, false, true, 350, 278},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    inventory.update(
        {false, false, true, 360, 416},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    const auto first =
        osf::GameplayInventory::beltPocketAt(357, 413);
    const auto last =
        osf::GameplayInventory::beltPocketAt(532, 476);
    return check(
        inventory.holdingItem() &&
            inventory.heldItem()->definition_id == tablet->id &&
            world.playerBelt().itemAt(0, 0) &&
            world.playerBelt().itemAt(0, 0)->definition_id ==
                capsule->id &&
            first &&
            first->grid_x == 0 &&
            first->grid_y == 0 &&
            last &&
            last->grid_x == 3 &&
            last->grid_y == 1 &&
            !osf::GameplayInventory::beltPocketAt(356, 413) &&
            !osf::GameplayInventory::beltPocketAt(533, 476),
        "Belt swapping or exact staggered hit bounds differ from retail.");
}

bool testSpecialItemOwnershipAndRendering() {
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
                ? "The special-item world could not be prepared."
                : error.c_str())) {
        return false;
    }

    const osf::ItemDefinition* accessory =
        world.itemDatabase().find(2, 1000000);
    if (!check(
            accessory,
            "The special-item fixture is unavailable.")) {
        return false;
    }
    osf::InventoryItem special_item;
    special_item.category = accessory->category;
    special_item.definition_id = accessory->id;
    special_item.width = accessory->inventory_width;
    special_item.height = accessory->inventory_height;
    if (!world.playerSpecialItems()
             .place(special_item, 0, 0)
             .accepted) {
        return false;
    }

    osf::GameplayInventory inventory;
    inventory.update(
        {false, false, false, 20, 76, true},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    if (!check(
            inventory.specialItemsActive() &&
                !inventory.active() &&
                inventory.anyItemPanelActive(),
            "X did not open the separate special-item panel.")) {
        return false;
    }

    osf::gapi::NjpImage status;
    if (!status.load(
            data_root / "System" / "Game" / "Pattern" /
                "Status.njp",
            &error)) {
        return false;
    }
    RecordingBackend rendered;
    osf::renderGameplaySpecialItems(
        rendered,
        status,
        inventory,
        world,
        0);
    if (!check(
            rendered.rectangles.size() == 1 &&
                rendered.rectangles[0].x == 0 &&
                rendered.rectangles[0].y == 0 &&
                rendered.rectangles[0].width == 320 &&
                rendered.rectangles[0].height == 412 &&
                rendered.patterns.size() >= 3 &&
                rendered.patterns[0].image == &status &&
                rendered.patterns[0].index == 14 &&
                rendered.patterns[1].image == &status &&
                rendered.patterns[1].index == 15 &&
                rendered.patterns[2].draw.x == 16 &&
                rendered.patterns[2].draw.y == 72,
            "The special-item panel did not use its retail artwork "
            "and 9-by-10 grid origin.")) {
        return false;
    }

    for (std::int32_t update = 0; update < 3; ++update) {
        inventory.update(
            {false, false, false, 20, 76},
            world.playerInventory(),
            world.playerEquipment(),
            world.playerBelt(),
            world.playerSpecialItems(),
            world.itemDatabase(),
            world.playerData().level());
    }
    if (!check(
            inventory.informationItem(
                world.playerInventory(),
                world.playerEquipment(),
                world.playerSpecialItems()) ==
                &world.playerSpecialItems().items()[0],
            "Special items did not join the delayed hover-information path.")) {
        return false;
    }

    inventory.update(
        {false, false, true, 20, 76},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    inventory.update(
        {false, false, true, 64, 152},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    if (!check(
            !inventory.holdingItem() &&
                world.playerSpecialItems().items().size() == 1 &&
                world.playerSpecialItems().items()[0].grid_x == 1 &&
                world.playerSpecialItems().items()[0].grid_y == 2,
            "Special-item pointer placement did not use retail centering.")) {
        return false;
    }

    inventory.update(
        {true, false, false, 64, 152},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    if (!check(
            inventory.active() &&
                inventory.specialItemsActive(),
            "The left Special Item owner and right inventory owner "
            "could not remain open together.")) {
        return false;
    }
    world.playerInventory().clear();
    inventory.update(
        {false, false, true, 64, 152},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    inventory.update(
        {false, false, true, 352, 280},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    if (!check(
            !inventory.holdingItem() &&
                world.playerSpecialItems().items().empty() &&
                world.playerInventory().items().size() == 1,
            "An item could not move from the left Warehouse owner "
            "to the open right inventory.")) {
        return false;
    }
    inventory.update(
        {false, false, true, 352, 280},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    inventory.update(
        {false, false, true, 32, 88},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    if (!check(
            !inventory.holdingItem() &&
                world.playerInventory().items().empty() &&
                world.playerSpecialItems().items().size() == 1,
            "An item could not move back from the right inventory "
            "to the open left Warehouse owner.")) {
        return false;
    }
    inventory.update(
        {false, false, false, 64, 152, true},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    inventory.update(
        {false, false, false, 64, 152, true},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    inventory.update(
        {true, false, false, 64, 152},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    inventory.update(
        {false, false, false, 64, 152, true},
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        world.itemDatabase(),
        world.playerData().level());
    return check(
        !inventory.anyItemPanelActive(),
        "The independent left and right item panels did not close "
        "their own owners.");
}

bool testIdentificationSelection() {
    osf::PlayerInventory owned;
    osf::PlayerEquipment equipment;
    osf::PlayerBelt belt;
    osf::PlayerSpecialItems special_items;
    osf::ItemDatabase database;
    osf::ItemDefinition item_definition;
    item_definition.category = 0;
    item_definition.id = 10;
    item_definition.variant = 1;
    item_definition.inventory_width = 1;
    item_definition.inventory_height = 1;
    item_definition.name = "Speed Short Sword";
    item_definition.description = "Short Sword";
    osf::InventoryItem item =
        osf::makeInventoryItem(item_definition);
    item.retail_state.resize(49u * 4u);
    if (!check(
            item.identified == 0 &&
                owned.store(std::move(item)),
            "The unidentified backpack fixture could not be stored.")) {
        return false;
    }

    osf::GameplayInventory inventory;
    inventory.open();
    const std::int32_t pointer_x =
        osf::GameplayInventory::backpack_left + 8;
    const std::int32_t pointer_y =
        osf::GameplayInventory::backpack_top + 8;
    const osf::GameplayInventoryResult selection =
        inventory.update(
            {
                false,
                false,
                true,
                pointer_x,
                pointer_y,
                false,
                false,
                true,
            },
            owned,
            equipment,
            belt,
            special_items,
            database,
            1);
    if (!check(
            selection.pointer_consumed &&
                selection.inventory_item_identify_requested == 0 &&
                !selection.cancel_identification_requested &&
                !inventory.holdingItem() &&
                owned.items().size() == 1 &&
                osf::itemInformationText(
                    owned.items()[0], item_definition) ==
                    "[Short Sword]\n\n",
            "Identify mode picked up the item or exposed its hidden name "
            "and values.")) {
        return false;
    }

    if (!check(
            owned.identify(0) &&
                owned.items()[0].identified == 1 &&
                owned.items()[0].retail_state[48u * 4u] == 1 &&
                osf::itemInformationText(
                    owned.items()[0], item_definition)
                    .rfind("[Speed Short Sword]\n\n", 0) == 0 &&
                osf::itemInformationText(
                    owned.items()[0], item_definition)
                    .find("Durability") != std::string::npos,
            "The identified flag did not update the item and its retail "
            "state mirror.")) {
        return false;
    }

    const osf::GameplayInventoryResult known_selection =
        inventory.update(
            {
                false,
                false,
                true,
                pointer_x,
                pointer_y,
                false,
                false,
                true,
            },
            owned,
            equipment,
            belt,
            special_items,
            database,
            1);
    if (!check(
            known_selection.pointer_consumed &&
                known_selection.inventory_item_identify_requested == -1 &&
                inventory.active() &&
                !inventory.holdingItem(),
            "Identify mode accepted an already identified item.")) {
        return false;
    }

    const osf::GameplayInventoryResult outside_selection =
        inventory.update(
            {
                false,
                false,
                true,
                100,
                200,
                false,
                false,
                true,
            },
            owned,
            equipment,
            belt,
            special_items,
            database,
            1);
    if (!check(
            outside_selection.pointer_consumed &&
                outside_selection
                        .inventory_item_identify_requested == -1 &&
                inventory.active(),
            "Identify mode leaked an outside click into the world.")) {
        return false;
    }

    const osf::GameplayInventoryResult cancelled =
        inventory.update(
            {
                false,
                true,
                false,
                100,
                200,
                false,
                true,
                true,
            },
            owned,
            equipment,
            belt,
            special_items,
            database,
            1);
    return check(
        cancelled.pointer_consumed &&
            cancelled.cancel_identification_requested &&
            inventory.active(),
        "Right-clicking did not cancel Identify while leaving Inventory "
        "open.");
}

bool testMerchantIdentificationOwners() {
    osf::ItemDefinition weapon;
    weapon.category = 0;
    weapon.id = 10;
    weapon.variant = 1;
    weapon.inventory_width = 1;
    weapon.inventory_height = 1;
    osf::InventoryItem backpack_item =
        osf::makeInventoryItem(weapon);
    backpack_item.retail_state.resize(49u * 4u);

    osf::ItemDefinition armor;
    armor.category = 1;
    armor.id = 10;
    armor.subtype = 1;
    armor.variant = 1;
    armor.inventory_width = 1;
    armor.inventory_height = 1;
    osf::InventoryItem equipped_item =
        osf::makeInventoryItem(armor);
    equipped_item.retail_state.resize(49u * 4u);

    osf::ItemDefinition medicine;
    medicine.category = 3;
    medicine.id = 10;
    medicine.variant = 1;
    medicine.inventory_width = 1;
    medicine.inventory_height = 1;

    osf::PlayerInventory inventory;
    osf::PlayerEquipment equipment;
    osf::PlayerBelt belt;
    if (!inventory.store(std::move(backpack_item)) ||
        !equipment
             .place(
                 osf::EquipmentSlot::body,
                 std::move(equipped_item),
                 armor,
                 1)
             .accepted ||
        !belt
             .place(
                 osf::makeInventoryItem(medicine),
                 0,
                 0,
                 medicine)
             .accepted) {
        return false;
    }

    if (!check(
            inventory.hasUnidentifiedItems() &&
                equipment.hasUnidentifiedItems() &&
                belt.hasUnidentifiedItems(),
            "Malse's Identify scan missed an owned item container.")) {
        return false;
    }
    const std::int32_t identified =
        inventory.identifyAll() +
        equipment.identifyAll() +
        belt.identifyAll();
    return check(
        identified == 3 &&
            !inventory.hasUnidentifiedItems() &&
            !equipment.hasUnidentifiedItems() &&
            !belt.hasUnidentifiedItems() &&
            inventory.items()[0].retail_state[48u * 4u] == 1 &&
            equipment.item(osf::EquipmentSlot::body)
                    ->retail_state[48u * 4u] == 1,
        "Malse's Identify mutation did not cover every owner or its retail "
        "save mirror.");
}

bool testSecondaryUseRequests() {
    osf::PlayerInventory owned;
    osf::PlayerEquipment equipment;
    osf::PlayerBelt belt;
    osf::PlayerSpecialItems special_items;
    osf::ItemDatabase database;
    osf::ItemDefinition tablet;
    tablet.category = 3;
    tablet.id = 0;
    tablet.inventory_width = 1;
    tablet.inventory_height = 1;
    if (!owned.add(tablet) ||
        !belt.place(
                 osf::makeInventoryItem(tablet),
                 0,
                 0,
                 tablet)
             .accepted) {
        return false;
    }

    osf::GameplayInventory inventory;
    inventory.open();
    const osf::GameplayInventoryResult backpack_use =
        inventory.update(
            {
                false,
                true,
                false,
                osf::GameplayInventory::backpack_left + 8,
                osf::GameplayInventory::backpack_top + 8,
                false,
                true,
            },
            owned,
            equipment,
            belt,
            special_items,
            database,
            1);
    if (!check(
            backpack_use.pointer_consumed &&
                backpack_use.inventory_item_use_requested == 0 &&
                backpack_use.belt_pocket_use_requested == -1 &&
                inventory.active(),
            "Right-clicking a backpack item did not request its use "
            "before the panel-close path.")) {
        return false;
    }

    inventory.completeItemUse(false);
    const osf::GameplayInventoryResult belt_use =
        inventory.update(
            {
                false,
                false,
                false,
                357 + 8,
                413 + 8,
                false,
                true,
            },
            owned,
            equipment,
            belt,
            special_items,
            database,
            1);
    return check(
        belt_use.pointer_consumed &&
            belt_use.inventory_item_use_requested == -1 &&
            belt_use.belt_pocket_use_requested == 0,
        "Right-clicking a belt item did not request the retail "
        "1-8 pocket.");
}

}  // namespace

int main() {
    return testInventoryState() &&
                   testIdentificationSelection() &&
                   testMerchantIdentificationOwners() &&
                   testSecondaryUseRequests() &&
                   testInventoryResourcesAndRendering() &&
                   testConditionArtwork() &&
                   testAccessoryAndBeltOwnership() &&
                   testSpecialItemOwnershipAndRendering()
        ? 0
        : 1;
}
