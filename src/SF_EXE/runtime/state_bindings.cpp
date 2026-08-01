#include "state_bindings.hpp"

#include "lwl.h"
#include "ui/conversation_layout.hpp"
#include "resources/retail_filesystem.hpp"
#include "runtime/audio_system.hpp"
#include "runtime/frontend_assets.hpp"
#include "world/player_data.hpp"
#include "world/world_scene.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace osf::runtime {

TitleStateHooks makeTitleStateHooks(
    const std::filesystem::path& data_root,
    FrontendAssets& assets,
    AudioSystem& audio) {
    TitleStateHooks hooks;
    hooks.load_pattern =
        [&assets](
            std::int32_t id,
            std::string_view path) {
            return assets.loadPattern(id, path);
        };
    hooks.load_animation =
        [&assets](
            std::size_t index,
            std::int32_t,
            std::string_view path) {
            return assets.loadTitleAnimation(index, path);
        };
    hooks.release_pattern = [&assets](std::int32_t id) {
        assets.releasePattern(id);
    };
    hooks.release_animation = [&assets](std::size_t index) {
        assets.releaseTitleAnimation(index);
    };
    hooks.load_voice =
        [&audio](
            std::string_view path,
            std::int32_t slot) {
            if (slot == 500) {
                audio.loadMenuMusic(path);
            }
        };
    hooks.files_exist =
        [&data_root](std::string_view pattern) {
            return pattern == "Save\\*.Ssv" &&
                   countRetailSaves(data_root) != 0;
        };
    hooks.file_exists =
        [&data_root](std::string_view path) {
            return retailFileExists(data_root, path);
        };
    return hooks;
}

CharacterSelectStateHooks makeCharacterSelectStateHooks(
    const std::filesystem::path& data_root,
    FrontendAssets& assets,
    AudioSystem& audio,
    LwlWindow*& window) {
    CharacterSelectStateHooks hooks;
    hooks.load_pattern =
        [&assets](
            std::int32_t id,
            std::string_view path) {
            return assets.loadPattern(id, path);
        };
    hooks.release_pattern = [&assets](std::int32_t id) {
        assets.releasePattern(id);
    };
    hooks.file_exists =
        [&data_root](std::string_view path) {
            return retailFileExists(data_root, path);
        };
    hooks.load_saved_characters = [&assets] {
        assets.loadSavedCharacters();
    };
    hooks.delete_saved_character =
        [&data_root, &assets](std::int32_t index) {
            deleteRetailSave(data_root, index);
            assets.loadSavedCharacters();
        };
    hooks.read_clipboard = [&window] {
        char* text = lwl_clipboard_get(window);
        std::string result = text ? text : "";
        lwl_free(text);
        return result;
    };
    hooks.voice_is_playing =
        [&audio](std::int32_t slot) {
            return slot == 500 &&
                   audio.menuMusicIsPlaying();
        };
    hooks.play_voice =
        [&audio](std::int32_t slot, bool loop) {
            if (slot == 500) {
                audio.playMenuMusic(loop);
            }
        };
    hooks.release_voice =
        [&audio](std::int32_t slot) {
            if (slot == 500) {
                audio.releaseMenuMusic();
            }
        };
    return hooks;
}

GameplayStateHooks makeGameplayStateHooks(
    const std::filesystem::path& data_root,
    PlayerLoadRequest& player,
    FrontendAssets& assets,
    AudioSystem& audio,
    WorldScene& world) {
    GameplayStateHooks hooks;
    hooks.prepare_interface = [&assets] {
        if (!assets.loadPattern(
                5, "System\\Game\\Pattern\\Bar.njp")) {
            return false;
        }
        if (!assets.loadPattern(
                6, "System\\Game\\Pattern\\Status.njp")) {
            assets.releasePattern(5);
            return false;
        }
        if (!assets.loadPattern(
                7, "System\\Game\\Pattern\\MapIcon.njp")) {
            assets.releasePattern(5);
            assets.releasePattern(6);
            return false;
        }
        if (!assets.loadPattern(
                8, "System\\Game\\Pattern\\StatusIcon.njp")) {
            assets.releasePattern(5);
            assets.releasePattern(6);
            assets.releasePattern(7);
            return false;
        }
        if (!assets.loadPattern(
                9, "System\\Game\\Pattern\\MagicIcon.njp")) {
            assets.releasePattern(5);
            assets.releasePattern(6);
            assets.releasePattern(7);
            assets.releasePattern(8);
            return false;
        }
        if (!assets.loadPattern(
                10,
                "System\\Game\\Pattern\\MagicBarIcon.njp")) {
            assets.releasePattern(5);
            assets.releasePattern(6);
            assets.releasePattern(7);
            assets.releasePattern(8);
            assets.releasePattern(9);
            return false;
        }
        if (!assets.loadPattern(
                11, "System\\Game\\Pattern\\Card.njp")) {
            assets.releasePattern(5);
            assets.releasePattern(6);
            assets.releasePattern(7);
            assets.releasePattern(8);
            assets.releasePattern(9);
            assets.releasePattern(10);
            return false;
        }
        return true;
    };
    hooks.release_interface = [&assets] {
        assets.releasePattern(5);
        assets.releasePattern(6);
        assets.releasePattern(7);
        assets.releasePattern(8);
        assets.releasePattern(9);
        assets.releasePattern(10);
        assets.releasePattern(11);
    };
    hooks.prepare_world =
        [&data_root, &player, &world] {
            std::string error;
            const bool ready =
                world.loadInitialScenario(
                    data_root, player, &error);
            if (!ready) {
                std::fprintf(
                    stderr,
                    "Could not load the initial world: %s\n",
                    error.c_str());
            }
            return ready;
        };
    hooks.release_world = [&world] {
        world.clear();
    };
    hooks.start_world_music = [&audio, &world] {
        audio.startWorldMusic(world.musicTrack());
    };
    hooks.stop_world_music = [&audio] {
        audio.stopWorldMusic();
    };
    hooks.command_player_movement =
        [&world](std::int32_t x, std::int32_t y) {
            world.commandPlayerMovement(x, y);
        };
    hooks.cancel_player_movement = [&world] {
        world.cancelPlayerMovement();
    };
    hooks.update_pointer_hover =
        [&assets, &world](
            std::int32_t x,
            std::int32_t y) {
            world.updatePointerHover(x, y);
            const auto* font = assets.pattern(1);
            if (!font ||
                !world.conversationRequiresSelection()) {
                return;
            }
            const std::int32_t option =
                conversationChoiceAtScreenPosition(
                    world,
                    *font,
                    world.cameraScreenX(),
                    world.cameraScreenY(),
                    x,
                    y);
            if (option >= 0) {
                world.selectConversationOption(option);
            }
        };
    hooks.clear_pointer_hover = [&world] {
        world.clearPointerHover();
    };
    hooks.command_world_interaction =
        [&world](std::int32_t x, std::int32_t y) {
            return world.commandWorldInteraction(x, y);
        };
    hooks.command_player_magic =
        [&world](std::int32_t x, std::int32_t y) {
            return world.commandPlayerMagic(x, y);
        };
    hooks.world_interaction_pending = [&world] {
        return world.interactionPending();
    };
    hooks.conversation_active = [&world] {
        return world.conversationActive();
    };
    hooks.conversation_requires_selection = [&world] {
        return world.conversationRequiresSelection();
    };
    hooks.choose_conversation_option =
        [&assets, &world](
            std::int32_t x,
            std::int32_t y) {
            const auto* font = assets.pattern(1);
            if (!font) {
                return false;
            }
            const std::int32_t option =
                conversationChoiceAtScreenPosition(
                    world,
                    *font,
                    world.cameraScreenX(),
                    world.cameraScreenY(),
                    x,
                    y);
            if (option < 0) {
                return false;
            }
            world.chooseConversationOption(option);
            return true;
        };
    hooks.advance_conversation = [&world] {
        world.advanceConversation();
    };
    hooks.toggle_player_run = [&world] {
        world.togglePlayerRun();
    };
    hooks.activate_increased_power = [&world] {
        world.activatePlayerIncreasedPower();
    };
    hooks.place_land_mine = [&world] {
        world.placePlayerLandMine();
    };
    hooks.update_world = [&audio, &world] {
        world.update();
        for (const std::int32_t sample :
             world.takeAudioSamples()) {
            audio.playGameplayEffect(sample);
        }
    };
    return hooks;
}

}  // namespace osf::runtime
