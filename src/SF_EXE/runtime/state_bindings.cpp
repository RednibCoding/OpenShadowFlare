#include "state_bindings.hpp"

#include "lwl.h"
#include "resources/resource_manager.hpp"
#include "ui/conversation_layout.hpp"
#include "resources/retail_filesystem.hpp"
#include "runtime/audio_system.hpp"
#include "world/player_data.hpp"
#include "world/world_scene.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace osf::runtime {

TitleStateHooks makeTitleStateHooks(
    const std::filesystem::path& data_root,
    ResourceManager& resources,
    AudioSystem& audio) {
    TitleStateHooks hooks;
    hooks.load_pattern =
        [&resources](
            std::int32_t id,
            std::string_view path) {
            return resources.loadTitlePattern(id, path);
        };
    hooks.load_animation =
        [&resources](
            std::size_t index,
            std::int32_t,
            std::string_view path) {
            return resources.loadTitleAnimation(index, path);
        };
    hooks.release_pattern = [&resources](std::int32_t id) {
        resources.releaseTitlePattern(id);
    };
    hooks.release_animation = [&resources](std::size_t index) {
        resources.releaseTitleAnimation(index);
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
    ResourceManager& resources,
    AudioSystem& audio,
    LwlWindow*& window) {
    CharacterSelectStateHooks hooks;
    hooks.begin_scene = [&resources] {
        resources.loadCommonPattern(
            0,
            "System\\Common\\Pattern\\Font00.njp",
            std::vector<std::uint8_t>{1});
    };
    hooks.clear_scene = [&resources] {
        resources.releaseCommonPattern(0);
    };
    hooks.load_pattern =
        [&resources](
            std::int32_t id,
            std::string_view path) {
            return resources.loadCharacterSelectPattern(id, path);
        };
    hooks.release_pattern = [&resources](std::int32_t id) {
        resources.releaseCharacterSelectPattern(id);
    };
    hooks.file_exists =
        [&data_root](std::string_view path) {
            return retailFileExists(data_root, path);
        };
    hooks.load_saved_characters = [&resources] {
        resources.loadSavedCharacters();
    };
    hooks.delete_saved_character =
        [&data_root, &resources](std::int32_t index) {
            deleteRetailSave(data_root, index);
            resources.loadSavedCharacters();
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
    ResourceManager& resources,
    AudioSystem& audio,
    WorldScene& world) {
    GameplayStateHooks hooks;
    hooks.prepare_interface = [&resources] {
        const bool ready = resources.loadCommonPattern(
                1,
                "System\\Common\\Pattern\\Font01.njp",
                std::vector<std::uint8_t>{1}) &&
            resources.loadCommonPattern(
                2,
                "System\\Common\\Pattern\\Waiting.njp") &&
            resources.loadGameplayPattern(
                5, "System\\Game\\Pattern\\Bar.njp") &&
            resources.loadGameplayPattern(
                8, "System\\Game\\Pattern\\StatusIcon.njp") &&
            resources.loadGameplayPattern(
                9, "System\\Game\\Pattern\\MagicIcon.njp") &&
            resources.loadGameplayPattern(
                10,
                "System\\Game\\Pattern\\MagicBarIcon.njp");
        if (!ready) {
            resources.releaseCommonPattern(1);
            resources.releaseCommonPattern(2);
            resources.releaseGameplayResources();
            return false;
        }
        return true;
    };
    hooks.release_loading_artwork = [&resources] {
        resources.releaseCommonPattern(2);
    };
    hooks.release_interface = [&resources] {
        resources.releaseCommonPattern(1);
        resources.releaseCommonPattern(2);
        resources.releaseGameplayResources();
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
        [&resources, &world](
            std::int32_t x,
            std::int32_t y) {
            world.updatePointerHover(x, y);
            const auto* font = resources.pattern(1);
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
    hooks.scenario_visual_active = [&world] {
        return world.scenarioVisualActive();
    };
    hooks.advance_scenario_visual = [&world] {
        world.requestScenarioVisualAdvance();
    };
    hooks.conversation_requires_selection = [&world] {
        return world.conversationRequiresSelection();
    };
    hooks.choose_conversation_option =
        [&resources, &world](
            std::int32_t x,
            std::int32_t y) {
            const auto* font = resources.pattern(1);
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
    hooks.toggle_companion_activity = [&world] {
        world.toggleOwnedCompanionActivity();
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
