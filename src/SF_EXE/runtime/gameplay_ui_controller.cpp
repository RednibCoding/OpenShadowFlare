#include "gameplay_ui_controller.hpp"

#include "core/game_config.hpp"
#include "core/retail_random.hpp"
#include "items/item_audio.hpp"
#include "runtime/audio_system.hpp"
#include "runtime/input_adapter.hpp"
#include "states/game_state.hpp"
#include "states/gameplay_state.hpp"
#include "ui/quest_notice_layout.hpp"
#include "world/player_data.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_preview.hpp"
#include "world/world_scene.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

namespace osf::runtime {
namespace {

std::int32_t gameplayCameraAnchorX(
    bool left_panel_active,
    bool right_panel_active) {
    if (left_panel_active == right_panel_active) {
        return 320;
    }
    return left_panel_active ? 480 : 160;
}

GameplayMagicModel gameplayMagicModel(
    const PlayerMagic& magic) {
    GameplayMagicModel model;
    for (std::size_t spell = 0;
         spell < model.availability.size();
         ++spell) {
        model.availability[spell] =
            magic.availability(
                static_cast<std::int32_t>(spell));
    }
    for (std::size_t slot = 0;
         slot < model.bar_slots.size();
         ++slot) {
        model.bar_slots[slot] =
            magic.barSlot(
                static_cast<std::int32_t>(slot));
    }
    model.selected_spell =
        magic.selectedSpell();
    model.targeting = magic.targeting();
    return model;
}

}  // namespace

void GameplayUiController::reset() {
    options_.close();
    debug_.close();
    inventory_.close();
    map_.close();
    magic_.close();
    status_.close();
    mission_list_.close();
    transport_.close();
    vendor_.close();
    pending_action_ = GameplayOptionsAction::none;
}

bool GameplayUiController::gameplayPanelsActive() const {
    return inventory_.anyItemPanelActive() ||
           map_.active() ||
           magic_.active() ||
           status_.active() ||
           mission_list_.active() ||
           transport_.active() ||
           vendor_.active();
}

void GameplayUiController::closeVendor(WorldScene& world) {
    if (!vendor_.active()) {
        return;
    }
    if (VendorInventory* stock =
            world.vendorInventory(vendor_.inventoryIndex())) {
        vendor_.update(
            {true, false, 0, 0},
            *stock,
            inventory_,
            world.playerInventory(),
            world.itemDatabase());
    } else {
        vendor_.close();
    }
}

void GameplayUiController::closeGameplayPanels(WorldScene& world) {
    closeVendor(world);
    inventory_.close();
    map_.close();
    magic_.close();
    status_.close();
    mission_list_.close();
    transport_.close();
}

bool GameplayUiController::update(
    const GameplayFrameResult& gameplay_frame,
    InputAdapter& input,
    WorldScene& world,
    AudioSystem& audio,
    GameConfig& game_config,
    bool& config_dirty,
    RetailRandom& random,
    PlayerLoadRequest& player,
    RetailSavePreview& save_preview,
    GameStateDispatcher& game_state,
    bool& running,
    std::int32_t& shadow_opacity) {
    if (gameplay_frame.phase != GameplayPhase::world) {
        return false;
    }

    world.playerMagic().setAllSpellsAvailable(
        debug_.allSpellsEnabled());
    world.configurePlayerDebugResources(
        debug_.infiniteLifeEnabled(),
        debug_.infiniteManaEnabled());

    // Retail routes a dead player directly through its locked death action.
    // Menus cannot pause that action or expose save commands before revival.
    if (world.playerMotion() == PlayerMotion::defeated) {
        closeVendor(world);
        reset();
        world.setCameraAnchor(320, 240);
        return false;
    }

    // A successful save deliberately leaves the retail saving page visible
    // for one frame. Execute its deferred transition before any independently
    // open left or right panel can consume the following UI update.
    if (pending_action_ != GameplayOptionsAction::none) {
        const GameplayOptionsAction action = pending_action_;
        pending_action_ = GameplayOptionsAction::none;
        if (action ==
            GameplayOptionsAction::save_and_return_to_title) {
            game_state.transition(GameState::title);
        } else {
            running = false;
        }
        return true;
    }

    const bool debug_was_active = debug_.active();
    const bool debug_toggle = input.gameplayDebugPressed();
    if (debug_toggle && !debug_was_active) {
        options_.close();
        world.cancelPlayerMovement();
    }
    if (debug_was_active || debug_toggle) {
        const GameplayDebugResult result =
            debug_.update({
                debug_toggle,
                input.gameplayOptionsPressed(),
                input.menu().pointer_primary_pressed,
                input.menu().pointer_x,
                input.menu().pointer_y,
            });
        world.playerMagic().setAllSpellsAvailable(
            debug_.allSpellsEnabled());
        world.configurePlayerDebugResources(
            debug_.infiniteLifeEnabled(),
            debug_.infiniteManaEnabled());
        if (result.play_click_sound) {
            audio.playOptionsClick();
        }
        if (result.play_confirm_sound) {
            audio.playOptionsConfirm();
        }
        return true;
    }

    // Escape belongs to the visible gameplay panels before it belongs to
    // Settings. Left and right panels can be open together, so one press
    // closes the complete panel pair and a later press opens Settings.
    if (input.gameplayOptionsPressed() &&
        !options_.active() &&
        gameplayPanelsActive()) {
        closeGameplayPanels(world);
        world.cancelPlayerIdentifyMode();
        world.setCameraAnchor(320, 240);
        return true;
    }

    // The options and confirmation pages are modal. Process them before
    // inventory, status, magic, and other panels so an open panel cannot
    // claim a click intended for the confirmation dialog.
    if (updateOptions(
            input,
            world,
            audio,
            game_config,
            config_dirty,
            random,
            player,
            save_preview,
            shadow_opacity)) {
        return true;
    }

    const bool increased_power_hud_click =
        input.menu().pointer_primary_pressed &&
        input.menu().pointer_x > 24 &&
        input.menu().pointer_x < 59 &&
        input.menu().pointer_y > 407 &&
        input.menu().pointer_y < 431;
    if (!world.conversationActive() &&
        increased_power_hud_click &&
        world.activatePlayerIncreasedPower()) {
        audio.playGameplayEffect(58);
        world.cancelPlayerMovement();
        return true;
    }

    const bool quest_notice_hidden =
        world.conversationActive() ||
        inventory_.anyItemPanelActive() ||
        map_.active() ||
        magic_.active() ||
        status_.active() ||
        mission_list_.active() ||
        transport_.active() ||
        vendor_.active();
    if (!quest_notice_hidden &&
        input.menu().pointer_primary_pressed) {
        const ActiveQuestShortcutLayout shortcut;
        const bool shortcut_clicked =
            activeQuestShortcutVisible(world.quests()) &&
            activeQuestShortcutContains(
                shortcut,
                input.menu().pointer_x,
                input.menu().pointer_y);
        QuestNoticeLayout layout;
        const bool notice_clicked =
            buildQuestNoticeLayout(
                world.quests(), world.missions(), layout) &&
            questNoticeContains(
                layout,
                input.menu().pointer_x,
                input.menu().pointer_y);
        if (shortcut_clicked || notice_clicked) {
            mission_list_.open();
            world.cancelPlayerMovement();
            return true;
        }
    }

    const GameplayServiceRequest service =
        world.takeGameplayServiceRequest();
    if (service.kind != GameplayServiceKind::none) {
        if (service.kind ==
            GameplayServiceKind::identify_item) {
            if (!inventory_.active()) {
                inventory_.open();
            }
            const bool left_panel_active =
                magic_.active() ||
                status_.active() ||
                map_.active() ||
                mission_list_.active() ||
                transport_.active() ||
                vendor_.active() ||
                inventory_.specialItemsActive();
            world.setCameraAnchor(
                gameplayCameraAnchorX(
                    left_panel_active,
                    inventory_.active()),
                240);
            return false;
        }
        options_.close();
        map_.close();
        magic_.close();
        status_.close();
        mission_list_.close();
        closeVendor(world);
        world.cancelPlayerMovement();
        if (service.kind == GameplayServiceKind::transport) {
            transport_.open();
            world.setCameraAnchor(
                gameplayCameraAnchorX(
                    true, inventory_.active()),
                240);
        } else if (
            service.kind ==
            GameplayServiceKind::toggle_special_items) {
            transport_.close();
            if (inventory_.specialItemsActive()) {
                inventory_.closeSpecialItems();
                world.setCameraAnchor(
                    gameplayCameraAnchorX(
                        false, inventory_.active()),
                    240);
            } else {
                inventory_.openSpecialItems();
                world.setCameraAnchor(
                    gameplayCameraAnchorX(
                        true, inventory_.active()),
                    240);
            }
        } else if (service.kind == GameplayServiceKind::vendor) {
            transport_.close();
            inventory_.closeSpecialItems();
            if (world.vendorInventory(service.argument)) {
                vendor_.open(service.argument);
                inventory_.open();
                world.setCameraAnchor(320, 240);
            }
        }
        return false;
    }

    if (vendor_.active()) {
        VendorInventory* stock =
            world.vendorInventory(vendor_.inventoryIndex());
        if (!stock) {
            vendor_.close();
        } else {
            const GameplayVendorResult result = vendor_.update(
                {
                    input.pointerSecondaryPressed() &&
                        input.menu().pointer_y < 412,
                    input.menu().pointer_primary_pressed,
                    input.menu().pointer_x,
                    input.menu().pointer_y,
                },
                *stock,
                inventory_,
                world.playerInventory(),
                world.itemDatabase());
            if (result.item_sound_sample >= 0) {
                audio.playGameplayEffect(result.item_sound_sample);
            }
            if (!vendor_.active()) {
                world.setCameraAnchor(
                    gameplayCameraAnchorX(
                        false, inventory_.active()),
                    240);
            }
            if (result.pointer_consumed) {
                return true;
            }
        }
    }

    const bool transport_was_active = transport_.active();
    if (transport_was_active) {
        const GameplayTransportResult result =
            transport_.update(
                {
                    input.gameplayOptionsPressed() ||
                        (input.pointerSecondaryPressed() &&
                         input.menu().pointer_y < 412),
                    input.menu().pointer_primary_pressed,
                    input.menu().pointer_x,
                    input.menu().pointer_y,
                },
                world.transports().enabledRows());
        if (result.play_move_sound) {
            audio.playGameplayMenuMove();
        }
        if (result.selected_destination >= 0) {
            std::string error;
            const ScenarioTravelResult travel =
                world.activateTransportDestination(
                    result.selected_destination,
                    &error);
            if (travel == ScenarioTravelResult::failed) {
                std::fprintf(
                    stderr,
                    "Could not travel to the selected scenario: %s\n",
                    error.c_str());
            }
        }
        world.setCameraAnchor(
            gameplayCameraAnchorX(
                transport_.active(),
                inventory_.active()),
            240);
        return result.pointer_consumed;
    }

    const std::int32_t belt_pocket =
        input.gameplayBeltPocketPressed();
    if (belt_pocket >= 0 &&
        !world.conversationActive() &&
        !options_.active() &&
        !map_.active() &&
        !mission_list_.active()) {
        const PlayerItemUseResult used =
            world.usePlayerBeltPocket(
                belt_pocket);
        if (used.consumed) {
            audio.playGameplayEffect(
                used.sound_sample);
        }
    }

    const bool status_was_active = status_.active();
    const bool status_hud_toggle =
        input.menu().pointer_primary_pressed &&
        input.menu().pointer_x >= 524 &&
        input.menu().pointer_x < 584 &&
        input.menu().pointer_y >= 440 &&
        input.menu().pointer_y < 464;
    const bool status_toggle =
        (input.gameplayStatusPressed() ||
         status_hud_toggle) &&
        (!world.conversationActive() ||
         status_was_active) &&
        !options_.active();
    if (status_toggle && !status_was_active) {
        map_.close();
        magic_.close();
        mission_list_.close();
        transport_.close();
        closeVendor(world);
        inventory_.closeSpecialItems();
        world.cancelPlayerMovement();
    } else if (
        status_was_active &&
        (input.gameplayMagicPressed() ||
         input.gameplayMapPressed() ||
         input.gameplayMissionListPressed() ||
         input.gameplaySpecialItemsPressed())) {
        status_.close();
    }
    const GameplayStatusResult status_result =
        status_.update({
            status_toggle,
            status_was_active &&
                (input.gameplayOptionsPressed() ||
                 (input.pointerSecondaryPressed() &&
                  input.menu().pointer_y < 412)),
            input.menu().pointer_primary_pressed,
            input.menu().pointer_x,
            input.menu().pointer_y,
        });
    if (status_result.switch_to_magic) {
        magic_.open();
    }
    if (status_result.play_move_sound) {
        audio.playGameplayEffect(58);
    }
    if (status_result.pointer_consumed) {
        world.setCameraAnchor(
            gameplayCameraAnchorX(
                status_.active() || magic_.active(),
                inventory_.active()),
            240);
        return true;
    }

    const bool magic_was_active = magic_.active();
    const bool magic_toggle =
        input.gameplayMagicPressed() &&
        (!world.conversationActive() ||
         magic_was_active) &&
        !options_.active();
    if (magic_toggle && !magic_was_active) {
        map_.close();
        status_.close();
        mission_list_.close();
        transport_.close();
        closeVendor(world);
        inventory_.closeSpecialItems();
        world.cancelPlayerMovement();
    } else if (
        magic_was_active &&
        (input.gameplayMapPressed() ||
         input.gameplayMissionListPressed() ||
         input.gameplaySpecialItemsPressed())) {
        magic_.close();
    }
    const bool other_left_panel_active =
        status_.active() ||
        map_.active() ||
        mission_list_.active() ||
        transport_.active() ||
        vendor_.active() ||
        inventory_.specialItemsActive();
    const GameplayMagicResult magic_result =
        magic_.update(
            {
                magic_toggle,
                magic_was_active &&
                    (input.gameplayOptionsPressed() ||
                     (input.pointerSecondaryPressed() &&
                      input.menu().pointer_y < 412)),
                input.menu().pointer_primary_pressed,
                input.pointerPrimaryDown(),
                input.menu().pointer_x,
                input.menu().pointer_y,
                magic_.active() ||
                    other_left_panel_active,
                inventory_.active(),
            },
            gameplayMagicModel(
                world.playerMagic()));
    if (magic_result.assign_bar_slot >= 0) {
        world.playerMagic().assignBarSlot(
            magic_result.assign_bar_slot,
            magic_result.assign_spell);
    }
    if (magic_result.select_spell != -2) {
        world.playerMagic().selectSpell(
            magic_result.select_spell);
    }
    if (magic_result.toggle_targeting) {
        world.playerMagic().setTargeting(
            !world.playerMagic().targeting());
    }
    if (magic_result.switch_to_status) {
        status_.open();
    }
    if (magic_result.play_pick_sound) {
        audio.playGameplayEffect(57);
    }
    if (magic_result.play_move_sound) {
        audio.playGameplayEffect(58);
    }
    if (magic_result.pointer_consumed) {
        const bool left_panel_active =
            magic_.active() ||
            status_.active() ||
            map_.active() ||
            mission_list_.active() ||
            transport_.active() ||
            vendor_.active() ||
            inventory_.specialItemsActive();
        world.setCameraAnchor(
            gameplayCameraAnchorX(
                left_panel_active,
                inventory_.active()),
            240);
        return true;
    }

    const bool inventory_was_active =
        inventory_.anyItemPanelActive();
    const bool inventory_hud_toggle =
        input.menu().pointer_primary_pressed &&
        input.menu().pointer_x >= 584 &&
        input.menu().pointer_x < 640 &&
        input.menu().pointer_y >= 440 &&
        input.menu().pointer_y < 464;
    const bool inventory_toggle =
        (input.gameplayInventoryPressed() ||
         inventory_hud_toggle) &&
        (!world.conversationActive() ||
         inventory_was_active) &&
        !options_.active();
    const bool special_items_toggle =
        input.gameplaySpecialItemsPressed() &&
        (!world.conversationActive() ||
         inventory_.specialItemsActive()) &&
        !options_.active();
    const bool belt_pointer_pressed =
        (input.menu().pointer_primary_pressed ||
         input.pointerSecondaryPressed()) &&
        !world.conversationActive() &&
        !options_.active() &&
        GameplayInventory::beltPocketAt(
            input.menu().pointer_x,
            input.menu().pointer_y).has_value();
    if ((map_.active() || input.gameplayMapPressed()) &&
        (inventory_was_active || inventory_toggle)) {
        map_.update({
            input.gameplayMapPressed(),
            false,
            input.leftHeld(),
            input.upHeld(),
            input.rightHeld(),
            input.downHeld(),
            input.menu().confirm_pressed,
        });
    }
    if (inventory_was_active ||
        inventory_toggle ||
        special_items_toggle ||
        inventory_.holdingItem() ||
        belt_pointer_pressed) {
        const bool identification_was_active =
            world.playerIdentifyModeActive();
        const GameplayInventoryResult result =
            inventory_.update(
                {
                    inventory_toggle,
                    input.gameplayOptionsPressed() ||
                        (input.pointerSecondaryPressed() &&
                         input.menu().pointer_y < 412),
                    input.menu().pointer_primary_pressed,
                    input.menu().pointer_x,
                    input.menu().pointer_y,
                    special_items_toggle,
                    input.pointerSecondaryPressed(),
                    identification_was_active,
                },
                world.playerInventory(),
                world.playerEquipment(),
                world.playerBelt(),
                world.playerSpecialItems(),
                world.itemDatabase(),
                world.playerData().level());
        if (result.cancel_identification_requested) {
            world.cancelPlayerIdentifyMode();
        } else if (
            result.inventory_item_identify_requested >= 0) {
            world.identifyPlayerInventoryItem(
                result.inventory_item_identify_requested);
        }
        if (identification_was_active &&
            !inventory_.active()) {
            world.cancelPlayerIdentifyMode();
        }
        PlayerItemUseResult used;
        if (result.inventory_item_use_requested >= 0) {
            used = world.usePlayerInventoryItem(
                result.inventory_item_use_requested);
            inventory_.completeItemUse(
                used.consumed);
        } else if (result.belt_pocket_use_requested >= 0) {
            used = world.usePlayerBeltPocket(
                result.belt_pocket_use_requested);
        }
        if (used.consumed) {
            audio.playGameplayEffect(
                used.sound_sample);
        }
        if (result.equipment_changed) {
            world.refreshPlayerAppearance();
        }
        if (result.item_sound_sample >= 0) {
            audio.playGameplayEffect(
                result.item_sound_sample);
        }
        if (result.world_drop_requested) {
            const InventoryItem* held_item =
                inventory_.heldItem();
            const ItemDefinition* definition =
                held_item
                    ? world.itemDatabase().find(
                          held_item->category,
                          held_item->definition_id)
                    : nullptr;
            const bool dropped =
                held_item &&
                world.dropInventoryItem(
                    *held_item,
                    result.world_drop_screen_x,
                    result.world_drop_screen_y);
            inventory_.completeWorldDrop(
                dropped,
                world.playerInventory());
            if (dropped && definition) {
                audio.playGameplayEffect(
                    retailItemMoveSound(
                        *definition));
            }
        }
        const bool left_panel_active =
            inventory_.specialItemsActive() ||
            magic_.active() ||
            status_.active() ||
            map_.active() ||
            mission_list_.active() ||
            transport_.active() ||
            vendor_.active();
        world.setCameraAnchor(
            gameplayCameraAnchorX(
                left_panel_active,
                inventory_.active()),
            240);
        return result.pointer_consumed ||
               inventory_hud_toggle;
    }

    const bool map_was_active = map_.active();
    const bool map_toggle =
        input.gameplayMapPressed() &&
        (!world.conversationActive() ||
         map_was_active) &&
        !options_.active() &&
        !mission_list_.active();
    if (map_was_active || map_toggle) {
        map_.update({
            map_toggle,
            input.gameplayOptionsPressed() ||
                (input.pointerSecondaryPressed() &&
                 input.menu().pointer_y < 412),
            input.leftHeld(),
            input.upHeld(),
            input.rightHeld(),
            input.downHeld(),
            input.menu().confirm_pressed,
        });
        world.setCameraAnchor(
            gameplayCameraAnchorX(
                map_.active(),
                inventory_.active()),
            240);
        return false;
    }

    const bool mission_was_active = mission_list_.active();
    const bool mission_toggle =
        input.gameplayMissionListPressed() &&
        (!world.conversationActive() ||
         mission_was_active) &&
        !options_.active();
    if (mission_was_active || mission_toggle) {
        const GameplayMissionListResult result =
            mission_list_.update(
                {
                    mission_toggle,
                    input.gameplayOptionsPressed(),
                    input.menu().pointer_primary_pressed,
                    input.menu().pointer_x,
                    input.menu().pointer_y,
                },
                [&world](std::int32_t mission_id) {
                    return world.quests().state(mission_id) != 0;
                });
        if (!mission_was_active &&
            mission_list_.active()) {
            world.cancelPlayerMovement();
        }
        if (result.play_move_sound) {
            audio.playGameplayMenuMove();
        }
        world.setCameraAnchor(
            gameplayCameraAnchorX(
                mission_list_.active(),
                inventory_.active()),
            240);
        return true;
    }

    return false;
}

bool GameplayUiController::updateOptions(
    InputAdapter& input,
    WorldScene& world,
    AudioSystem& audio,
    GameConfig& game_config,
    bool& config_dirty,
    RetailRandom& random,
    PlayerLoadRequest& player,
    RetailSavePreview& save_preview,
    std::int32_t& shadow_opacity) {
    const bool was_active = options_.active();
    const bool toggle =
        input.gameplayOptionsPressed() &&
        (!world.conversationActive() || was_active);
    const GameplayOptionsResult result =
        options_.update(
            {
                toggle,
                input.menu().pointer_primary_pressed,
                input.pointerPrimaryDown(),
                input.menu().pointer_x,
                input.menu().pointer_y,
                input.gameplayHelpPressed(),
            },
            game_config);
    if (!was_active && options_.active()) {
        world.cancelPlayerMovement();
    }
    if (result.config_changed) {
        config_dirty = true;
        applyConfig(
            game_config, world, audio, shadow_opacity);
    }
    if (result.play_click_sound) {
        audio.playOptionsClick();
    }
    if (result.play_confirm_sound) {
        audio.playOptionsConfirm();
    }
    if (result.action ==
        GameplayOptionsAction::open_mission_list) {
        options_.close();
        mission_list_.open();
        return true;
    }
    if (result.action == GameplayOptionsAction::open_map) {
        options_.close();
        map_.open();
        world.setCameraAnchor(480, 240);
        return true;
    }
    if (result.action != GameplayOptionsAction::none) {
        std::string error;
        const bool saved =
            !player.save_path.empty() &&
            writeRetailSave(
                player.save_path,
                world.playerData(),
                world.itemDatabase(),
                world.playerInventory(),
                world.playerEquipment(),
                world.playerBelt(),
                world.playerSpecialItems(),
                world.retailSaveProgress(),
                world.playerMagic(),
                static_cast<std::uint8_t>(
                    random.next() & 0xff),
                &error);
        if (!saved) {
            options_.restoreConfirmation(result.action);
            std::fprintf(
                stderr,
                "Could not save the current game: %s\n",
                error.empty()
                    ? "no save slot is assigned"
                    : error.c_str());
        } else {
            player.source = PlayerDataSource::retail_save;
            if (game_config.save_image_at_game_end) {
                std::string preview_error;
                if (!save_preview.writeForSave(
                        player.save_path,
                        &preview_error)) {
                    std::fprintf(
                        stderr,
                        "Could not save the character preview: %s\n",
                        preview_error.c_str());
                }
            }
            pending_action_ = result.action;
        }
    }
    return was_active ||
           options_.active() ||
           (input.gameplayOptionsPressed() &&
            !world.conversationActive()) ||
           input.gameplayHelpPressed();
}

const GameplayOptionsMenu&
GameplayUiController::options() const {
    return options_;
}

const GameplayDebugMenu&
GameplayUiController::debug() const {
    return debug_;
}

const GameplayInventory&
GameplayUiController::inventory() const {
    return inventory_;
}

const GameplayMap& GameplayUiController::map() const {
    return map_;
}

const GameplayMagic& GameplayUiController::magic() const {
    return magic_;
}

const GameplayStatus& GameplayUiController::status() const {
    return status_;
}

const GameplayMissionList&
GameplayUiController::missionList() const {
    return mission_list_;
}

const GameplayTransport&
GameplayUiController::transport() const {
    return transport_;
}

const GameplayVendor& GameplayUiController::vendor() const {
    return vendor_;
}

void GameplayUiController::applyConfig(
    const GameConfig& config,
    WorldScene& world,
    AudioSystem& audio,
    std::int32_t& shadow_opacity) {
    world.configurePointer({
        config.click_range,
        config.click_range_enabled,
        config.click_priority,
    });
    audio.setEffectVolume(config.effect_volume);
    audio.setBgmVolume(config.bgm_volume);
    shadow_opacity =
        config.semi_transparent_shadow ? 500 : 1000;
}

}  // namespace osf::runtime
