#include "gameplay_ui_controller.hpp"

#include "core/game_config.hpp"
#include "core/retail_random.hpp"
#include "items/item_audio.hpp"
#include "runtime/audio_system.hpp"
#include "runtime/input_adapter.hpp"
#include "states/game_state.hpp"
#include "states/gameplay_state.hpp"
#include "world/player_data.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_preview.hpp"
#include "world/world_scene.hpp"

#include <cstdint>
#include <cstdio>
#include <string>

namespace osf::runtime {

void GameplayUiController::reset() {
    options_.close();
    inventory_.close();
    map_.close();
    mission_list_.close();
    pending_action_ = GameplayOptionsAction::none;
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

    const std::int32_t belt_pocket =
        input.gameplayBeltPocketPressed();
    if (belt_pocket >= 0 &&
        !world.conversationActive() &&
        !options_.active() &&
        !map_.active() &&
        !mission_list_.active()) {
        const BeltItemUseResult used =
            world.usePlayerBeltPocket(
                belt_pocket);
        if (used.consumed) {
            audio.playGameplayEffect(
                used.sound_sample);
        }
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
        !options_.active() &&
        !map_.active() &&
        !mission_list_.active();
    const bool special_items_toggle =
        input.gameplaySpecialItemsPressed() &&
        (!world.conversationActive() ||
         inventory_.specialItemsActive()) &&
        !options_.active() &&
        !map_.active() &&
        !mission_list_.active();
    const bool belt_pointer_pressed =
        input.menu().pointer_primary_pressed &&
        !world.conversationActive() &&
        !options_.active() &&
        !map_.active() &&
        !mission_list_.active() &&
        GameplayInventory::beltPocketAt(
            input.menu().pointer_x,
            input.menu().pointer_y).has_value();
    if (inventory_was_active ||
        inventory_toggle ||
        special_items_toggle ||
        inventory_.holdingItem() ||
        belt_pointer_pressed) {
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
                },
                world.playerInventory(),
                world.playerEquipment(),
                world.playerBelt(),
                world.playerSpecialItems(),
                world.itemDatabase(),
                world.playerData().level());
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
                dropped);
            if (dropped && definition) {
                audio.playGameplayEffect(
                    retailItemMoveSound(
                        *definition));
            }
        }
        const std::int32_t camera_anchor_x =
            inventory_.active()
                ? 160
                : inventory_.specialItemsActive()
                    ? 480
                    : 320;
        world.setCameraAnchor(camera_anchor_x, 240);
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
            map_.active() ? 480 : 320,
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
        return true;
    }

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
            !world.conversationActive());
}

const GameplayOptionsMenu&
GameplayUiController::options() const {
    return options_;
}

const GameplayInventory&
GameplayUiController::inventory() const {
    return inventory_;
}

const GameplayMap& GameplayUiController::map() const {
    return map_;
}

const GameplayMissionList&
GameplayUiController::missionList() const {
    return mission_list_;
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
