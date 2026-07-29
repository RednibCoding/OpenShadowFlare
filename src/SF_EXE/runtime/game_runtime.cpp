#include "game_runtime.hpp"

#include "lgl.h"
#include "lwl.h"
#include "core/retail_random.hpp"
#include "libs/RKC_DIB/rkc_dib.hpp"
#include "libs/RKC_DBFCONTROL/rkc_dbfcontrol.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "render/character_select_renderer.hpp"
#include "render/gameplay_hud_renderer.hpp"
#include "render/gameplay_help_renderer.hpp"
#include "render/gameplay_options_renderer.hpp"
#include "render/gameplay_renderer.hpp"
#include "render/gameplay_overlay_renderer.hpp"
#include "render/loading_renderer.hpp"
#include "render/title_renderer.hpp"
#include "resources/retail_filesystem.hpp"
#include "runtime/audio_system.hpp"
#include "runtime/frontend_assets.hpp"
#include "runtime/input_adapter.hpp"
#include "runtime/lgl_surface_presenter.hpp"
#include "states/game_state.hpp"
#include "states/gameplay_options_menu.hpp"
#include "states/gameplay_state.hpp"
#include "states/character_select_state.hpp"
#include "states/save_catalog.hpp"
#include "states/title_state.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_preview.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <utility>

namespace {

constexpr int kVirtualWidth = 640;
constexpr int kVirtualHeight = 480;

void* loadOpenGlFunction(const char* name, void*) {
    return lwl_gl_get_proc_address(name);
}

class Runtime {
public:
    explicit Runtime(std::filesystem::path dataRoot)
        : dataRoot_(std::move(dataRoot)),
          frontendAssets_(dataRoot_),
          renderer_(
              kVirtualWidth,
              kVirtualHeight,
              [this](osf::gapi::SurfaceView surface) {
                  presentSurface(surface);
              }),
          input_(kVirtualWidth, kVirtualHeight),
          titleState_(random_, makeTitleStateHooks()),
          characterSelectState_(makeCharacterSelectStateHooks()),
          gameplayState_(makeGameplayStateHooks()),
          gameState_(makeGameStateCallbacks()) {}

    ~Runtime() {
        surfacePresenter_.shutdown();
        lgl_reset();
        lwl_gl_context_destroy(context_);
        lwl_window_destroy(window_);
        if (windowingInitialized_) {
            lwl_shutdown();
        }
    }

    bool initialize(const osf::GameConfig& gameConfig) {
        gameConfig_ = gameConfig;
        if (!lwl_init()) {
            std::fprintf(stderr, "Could not initialize LWL.\n");
            return false;
        }
        windowingInitialized_ = true;

        window_ = lwl_window_create(
            "OpenShadowFlare", kVirtualWidth, kVirtualHeight);
        if (!window_) {
            std::fprintf(stderr, "Could not create the game window.\n");
            return false;
        }
        lwl_window_set_mode(window_, LWL_WINDOW_NORMAL);
        lwl_window_show(window_);

        const LwlGlConfig config = lwl_gl_config_default();
        context_ = lwl_gl_context_create(window_, &config);
        if (!context_ || !lwl_gl_context_make_current(context_)) {
            std::fprintf(
                stderr,
                "Could not create an OpenGL %d.%d core context.\n",
                config.major_version,
                config.minor_version);
            return false;
        }
        if (!lgl_load(loadOpenGlFunction, nullptr)) {
            std::fprintf(
                stderr,
                "Could not load OpenGL: %s\n",
                lgl_last_error());
            return false;
        }
        std::string presenterError;
        if (!surfacePresenter_.initialize(&presenterError)) {
            std::fprintf(
                stderr,
                "Could not initialize surface presentation: %s\n",
                presenterError.c_str());
            return false;
        }
        if (!lwl_gl_context_set_swap_interval(context_, 1)) {
            std::fprintf(
                stderr,
                "Warning: display synchronization is unavailable.\n");
        }

        std::string audioError;
        if (!audio_.initialize(
                dataRoot_,
                gameConfig_.effect_volume,
                gameConfig_.bgm_volume,
                &audioError)) {
            std::fprintf(
                stderr,
                "Warning: audio is unavailable: %s\n",
                audioError.c_str());
        }
        shadowOpacity_ =
            gameConfig_.semi_transparent_shadow ? 500 : 1000;
        world_.configurePointer({
            gameConfig_.click_range,
            gameConfig_.click_range_enabled,
            gameConfig_.click_priority,
        });

        if (!frontendAssets_.loadPattern(
                0, "System\\Common\\Pattern\\Font00.njp") ||
            !frontendAssets_.loadPattern(
                1, "System\\Common\\Pattern\\Font01.njp") ||
            !frontendAssets_.loadPattern(
                2,
                "System\\Common\\Pattern\\Waiting.njp")) {
            return false;
        }
        gameState_.transition(osf::GameState::title);
        return true;
    }

    int run(bool smokeTest) {
        constexpr double renderStep = 1.0 / 60.0;
        constexpr double gameStep = 1.0 / 30.0;
        constexpr double maximumElapsed = 0.25;

        bool running = true;
        int renderedFrames = 0;
        double previousTime = lwl_time_seconds();
        double nextFrame = previousTime;
        double gameAccumulator = gameStep;

        while (running) {
            const double currentTime = lwl_time_seconds();
            gameAccumulator += std::clamp(
                currentTime - previousTime,
                0.0,
                maximumElapsed);
            previousTime = currentTime;

            LwlEvent event{};
            while (lwl_poll_event(window_, &event)) {
                if (!input_.handleEvent(
                        window_,
                        event,
                        gameState_.currentState())) {
                    running = false;
                }
            }

            while (running && gameAccumulator >= gameStep) {
                updateGame(running);
                gameAccumulator -= gameStep;
            }

            const double interpolation =
                std::clamp(
                    gameAccumulator / gameStep,
                    0.0,
                    1.0);
            renderGame(interpolation);

            ++renderedFrames;
            if (smokeTest && renderedFrames >= 3) {
                running = false;
            }

            nextFrame += renderStep;
            if (nextFrame < currentTime - maximumElapsed) {
                nextFrame = currentTime;
            }
            lwl_sleep_until_seconds(nextFrame);
        }
        if (configDirty_) {
            osf::saveGameConfigFile(
                (dataRoot_ / "SFlare.Cfg").string(),
                gameConfig_);
        }
        return 0;
    }

private:
    void presentSurface(osf::gapi::SurfaceView source) {
        int width = 0;
        int height = 0;
        lwl_window_get_size(window_, &width, &height);
        surfacePresenter_.present(source, width, height);
        lwl_gl_context_swap_buffers(context_);
    }


    void updateGame(bool& running) {
        switch (gameState_.currentState()) {
        case osf::GameState::title: {
            for (std::size_t index = 0;
                 index < 10;
                 ++index) {
                const auto* animation =
                    frontendAssets_.titleAnimation(index);
                const auto& charts = animation->charts();
                input_.menu().smoke_frame_counts[index] =
                    charts.empty()
                        ? 0
                        : charts.front().directions[8].frame_count;
            }
            titleFrame_ =
                titleState_.update(input_.menu());
            audio_.playTitleFrame(titleFrame_);
            if (titleFrame_.action ==
                osf::TitleAction::open_character_select) {
                gameState_.transition(
                    osf::GameState::character_select,
                    titleFrame_.character_select_argument);
            } else if (
                titleFrame_.action == osf::TitleAction::exit_game) {
                running = false;
            }
            break;
        }
        case osf::GameState::character_select: {
            input_.characterSelect().saved_game_count =
                frontendAssets_.savedGameCount();
            characterFrame_ =
                characterSelectState_.update(
                    input_.characterSelect());
            audio_.playCharacterSelectFrame(characterFrame_);
            if (characterFrame_.action ==
                osf::CharacterSelectAction::return_to_title) {
                gameState_.transition(osf::GameState::title);
            } else if (
                characterFrame_.action ==
                osf::CharacterSelectAction::enter_gameplay) {
                const auto& selection =
                    characterSelectState_.data();
                gameplayPlayer_ = {};
                if (selection.mode ==
                    osf::CharacterSelectMode::saved_game) {
                    gameplayPlayer_.source =
                        osf::PlayerDataSource::retail_save;
                    const auto& saved_games =
                        frontendAssets_.savedGames();
                    if (selection.selected_saved_game >= 0 &&
                        static_cast<std::size_t>(
                            selection.selected_saved_game) <
                            saved_games.size()) {
                        gameplayPlayer_.save_path =
                            saved_games[
                                static_cast<std::size_t>(
                                    selection.selected_saved_game)]
                                .save_path;
                    }
                } else {
                    gameplayPlayer_.source =
                        osf::PlayerDataSource::new_character;
                    gameplayPlayer_.name =
                        selection.character_name;
                    gameplayPlayer_.gender =
                        selection.character_gender;
                    gameplayPlayer_.save_path =
                        osf::resolveRetailPath(
                            dataRoot_,
                            selection.next_save_path);
                }
                gameState_.transition(osf::GameState::gameplay);
            } else if (
                characterFrame_.action ==
                osf::CharacterSelectAction::exit_game) {
                running = false;
            }
            break;
        }
        case osf::GameState::gameplay: {
            if (!updateGameplayOptions(running)) {
                gameplayFrame_ = gameplayState_.update({
                    input_.menu().confirm_pressed,
                    input_.menu()
                        .pointer_primary_pressed,
                    input_.menu().pointer_x,
                    input_.menu().pointer_y,
                    input_.pointerPrimaryDown(),
                    input_.runTogglePressed(),
                });
            }
            break;
        }
        default:
            break;
        }

        input_.clearTransientInput();
    }

    void renderCharacterSelect() {
        const auto* pattern = frontendAssets_.pattern(4);
        if (!pattern) {
            return;
        }
        const auto* font = frontendAssets_.pattern(0);
        osf::renderCharacterSelect(
            renderer_,
            *pattern,
            font,
            characterSelectState_.data(),
            characterFrame_,
            frontendAssets_.savedGames(),
            frontendAssets_.savedPreviews());
    }

    void renderGame(double interpolation) {
        renderer_.beginFrame({0, 0, 0, 255});
        if (gameState_.currentState() == osf::GameState::title) {
            const auto* pattern = frontendAssets_.pattern(4);
            if (pattern) {
                std::array<osf::TitleSmokeAsset, 10> smoke{};
                for (std::size_t index = 0;
                     index < smoke.size();
                     ++index) {
                    const auto* smokePattern =
                        frontendAssets_.pattern(
                            5 + static_cast<std::int32_t>(index) * 2);
                    if (smokePattern) {
                        smoke[index] = {
                            smokePattern,
                            frontendAssets_.titleAnimation(index),
                        };
                    }
                }
                osf::renderTitle(
                    renderer_,
                    *pattern,
                    smoke,
                    titleFrame_);
            }
        } else if (
            gameState_.currentState() ==
            osf::GameState::character_select) {
            renderCharacterSelect();
        } else if (
            gameState_.currentState() ==
            osf::GameState::gameplay) {
            if (gameplayFrame_.phase ==
                osf::GameplayPhase::loading) {
                const auto* waiting = frontendAssets_.pattern(2);
                if (waiting) {
                    osf::renderInitialLoadingScreen(
                        renderer_,
                        *waiting,
                        gameplayFrame_.loading_counter,
                        gameplayFrame_.ready_to_continue);
                }
            } else {
                const auto* font = frontendAssets_.pattern(1);
                osf::renderWorldGeometry(
                    renderer_,
                    world_,
                    shadowOpacity_,
                    interpolation,
                    gameConfig_.semi_transparent_objects);
                savePreview_.capture(renderer_.surface());
                const auto* bar = frontendAssets_.pattern(5);
                if (bar) {
                    osf::renderGameplayHud(
                        renderer_,
                        *bar,
                        osf::gameplayHudValues(
                            world_.playerData(),
                            world_.playerMovementPace()));
                }
                if (!gameplayOptions_.active()) {
                    osf::renderGameplayOverlay(
                        renderer_,
                        world_,
                        font,
                        world_.renderCameraScreenX(
                            interpolation),
                        world_.renderCameraScreenY(
                            interpolation),
                        interpolation);
                }
                const auto* status =
                    frontendAssets_.pattern(6);
                if (status && font) {
                    if (gameplayOptions_.page() ==
                        osf::GameplayOptionsPage::help) {
                        osf::renderGameplayHelp(
                            renderer_,
                            *status,
                            *font,
                            world_,
                            gameplayOptions_
                                .animationCounter(),
                            gameplayOptions_
                                .helpCloseVisible(),
                            gameplayOptions_
                                .helpCloseAnimationCounter());
                    } else {
                        osf::renderGameplayOptions(
                            renderer_,
                            *status,
                            *font,
                            gameplayOptions_,
                            gameConfig_);
                    }
                }
            }
        }
        renderer_.endFrame();
    }

    osf::TitleStateHooks makeTitleStateHooks() {
        osf::TitleStateHooks hooks;
        hooks.load_pattern =
            [this](
                std::int32_t id,
                std::string_view path) {
                return frontendAssets_.loadPattern(id, path);
            };
        hooks.load_animation =
            [this](
                std::size_t index,
                std::int32_t,
                std::string_view path) {
                return frontendAssets_.loadTitleAnimation(
                    index, path);
            };
        hooks.release_pattern = [this](std::int32_t id) {
            frontendAssets_.releasePattern(id);
        };
        hooks.release_animation = [this](std::size_t index) {
            frontendAssets_.releaseTitleAnimation(index);
        };
        hooks.load_voice =
            [this](
                std::string_view path,
                std::int32_t slot) {
                if (slot == 500) {
                    audio_.loadMenuMusic(path);
                }
            };
        hooks.files_exist = [this](std::string_view pattern) {
            return pattern == "Save\\*.Ssv" &&
                   osf::countRetailSaves(dataRoot_) != 0;
        };
        hooks.file_exists =
            [this](std::string_view path) {
                return osf::retailFileExists(dataRoot_, path);
            };
        return hooks;
    }

    osf::CharacterSelectStateHooks makeCharacterSelectStateHooks() {
        osf::CharacterSelectStateHooks hooks;
        hooks.load_pattern =
            [this](
                std::int32_t id,
                std::string_view path) {
                return frontendAssets_.loadPattern(id, path);
            };
        hooks.release_pattern = [this](std::int32_t id) {
            frontendAssets_.releasePattern(id);
        };
        hooks.file_exists =
            [this](std::string_view path) {
                return osf::retailFileExists(dataRoot_, path);
            };
        hooks.load_saved_characters = [this] {
            frontendAssets_.loadSavedCharacters();
        };
        hooks.delete_saved_character = [this](std::int32_t index) {
            osf::deleteRetailSave(dataRoot_, index);
            frontendAssets_.loadSavedCharacters();
        };
        hooks.read_clipboard = [this] {
            char* text = lwl_clipboard_get(window_);
            std::string result = text ? text : "";
            lwl_free(text);
            return result;
        };
        hooks.voice_is_playing = [this](std::int32_t slot) {
            return slot == 500 &&
                   audio_.menuMusicIsPlaying();
        };
        hooks.play_voice =
            [this](std::int32_t slot, bool loop) {
                if (slot == 500) {
                    audio_.playMenuMusic(loop);
                }
            };
        hooks.release_voice = [this](std::int32_t slot) {
            if (slot == 500) {
                audio_.releaseMenuMusic();
            }
        };
        return hooks;
    }

    osf::GameStateDispatcherCallbacks makeGameStateCallbacks() {
        osf::GameStateDispatcherCallbacks callbacks;
        callbacks.title.enter = [this](std::int32_t) {
            titleState_.enter();
        };
        callbacks.title.leave = [this] {
            titleState_.leave();
        };
        callbacks.character_select.enter = [this](std::int32_t argument) {
            characterSelectState_.enter(argument);
        };
        callbacks.character_select.leave = [this] {
            characterSelectState_.leave();
        };
        callbacks.gameplay.enter = [this](std::int32_t) {
            gameplayFrame_ = {};
            gameplayOptions_.close();
            savePreview_.clear();
            pendingGameplayOptionsAction_ =
                osf::GameplayOptionsAction::none;
            gameplayState_.enter();
        };
        callbacks.gameplay.leave = [this] {
            gameplayOptions_.close();
            gameplayState_.leave();
        };
        return callbacks;
    }

    osf::GameplayStateHooks makeGameplayStateHooks() {
        osf::GameplayStateHooks hooks;
        hooks.prepare_interface = [this] {
            if (!frontendAssets_.loadPattern(
                    5,
                    "System\\Game\\Pattern\\Bar.njp")) {
                return false;
            }
            if (!frontendAssets_.loadPattern(
                    6,
                    "System\\Game\\Pattern\\Status.njp")) {
                frontendAssets_.releasePattern(5);
                return false;
            }
            return true;
        };
        hooks.release_interface = [this] {
            frontendAssets_.releasePattern(5);
            frontendAssets_.releasePattern(6);
        };
        hooks.prepare_world = [this] {
            std::string error;
            const bool worldReady =
                world_.loadInitialScenario(
                    dataRoot_, gameplayPlayer_, &error);
            if (!worldReady) {
                std::fprintf(
                    stderr,
                    "Could not load the initial world: %s\n",
                    error.c_str());
            }
            return worldReady;
        };
        hooks.release_world = [this] {
            world_.clear();
        };
        hooks.start_world_music = [this] {
            audio_.startWorldMusic(world_.musicTrack());
        };
        hooks.stop_world_music = [this] {
            audio_.stopWorldMusic();
        };
        hooks.command_player_movement =
            [this](std::int32_t x, std::int32_t y) {
                world_.commandPlayerMovement(x, y);
            };
        hooks.cancel_player_movement = [this] {
            world_.cancelPlayerMovement();
        };
        hooks.update_pointer_hover =
            [this](std::int32_t x, std::int32_t y) {
                world_.updatePointerHover(x, y);
                const auto* font = frontendAssets_.pattern(1);
                if (!font ||
                    !world_.conversationRequiresSelection()) {
                    return;
                }
                const std::int32_t option =
                    osf::conversationChoiceAtScreenPosition(
                        world_,
                        *font,
                        world_.cameraScreenX(),
                        world_.cameraScreenY(),
                        x,
                        y);
                if (option >= 0) {
                    world_.selectConversationOption(option);
                }
            };
        hooks.command_world_interaction =
            [this](std::int32_t x, std::int32_t y) {
                return world_.commandWorldInteraction(x, y);
            };
        hooks.world_interaction_pending = [this] {
            return world_.interactionPending();
        };
        hooks.conversation_active = [this] {
            return world_.conversationActive();
        };
        hooks.conversation_requires_selection = [this] {
            return world_.conversationRequiresSelection();
        };
        hooks.choose_conversation_option =
            [this](std::int32_t x, std::int32_t y) {
                const auto* font = frontendAssets_.pattern(1);
                if (!font) {
                    return false;
                }
                const std::int32_t option =
                    osf::conversationChoiceAtScreenPosition(
                        world_,
                        *font,
                        world_.cameraScreenX(),
                        world_.cameraScreenY(),
                        x,
                        y);
                if (option < 0) {
                    return false;
                }
                world_.chooseConversationOption(option);
                return true;
            };
        hooks.advance_conversation = [this] {
            world_.advanceConversation();
        };
        hooks.toggle_player_run = [this] {
            world_.togglePlayerRun();
        };
        hooks.update_world = [this] {
            world_.update();
        };
        return hooks;
    }

    bool updateGameplayOptions(bool& running) {
        if (gameplayFrame_.phase !=
            osf::GameplayPhase::world) {
            return false;
        }
        if (pendingGameplayOptionsAction_ !=
            osf::GameplayOptionsAction::none) {
            const osf::GameplayOptionsAction action =
                pendingGameplayOptionsAction_;
            pendingGameplayOptionsAction_ =
                osf::GameplayOptionsAction::none;
            if (action ==
                osf::GameplayOptionsAction::
                    save_and_return_to_title) {
                gameState_.transition(osf::GameState::title);
            } else {
                running = false;
            }
            return true;
        }
        const bool was_active =
            gameplayOptions_.active();
        const bool toggle =
            input_.gameplayOptionsPressed() &&
            (!world_.conversationActive() || was_active);
        const osf::GameplayOptionsResult result =
            gameplayOptions_.update(
                {
                    toggle,
                    input_.menu().pointer_primary_pressed,
                    input_.pointerPrimaryDown(),
                    input_.menu().pointer_x,
                    input_.menu().pointer_y,
                    input_.gameplayHelpPressed(),
                },
                gameConfig_);
        if (!was_active && gameplayOptions_.active()) {
            world_.cancelPlayerMovement();
        }
        if (result.config_changed) {
            configDirty_ = true;
            applyGameplayConfig();
        }
        if (result.play_click_sound) {
            audio_.playOptionsClick();
        }
        if (result.play_confirm_sound) {
            audio_.playOptionsConfirm();
        }
        if (result.action !=
            osf::GameplayOptionsAction::none) {
            std::string error;
            const bool saved =
                !gameplayPlayer_.save_path.empty() &&
                osf::writeRetailSave(
                    gameplayPlayer_.save_path,
                    world_.playerData(),
                    static_cast<std::uint8_t>(
                        random_.next() & 0xff),
                    &error);
            if (!saved) {
                gameplayOptions_.restoreConfirmation(
                    result.action);
                std::fprintf(
                    stderr,
                    "Could not save the current game: %s\n",
                    error.empty()
                        ? "no save slot is assigned"
                        : error.c_str());
            } else {
                gameplayPlayer_.source =
                    osf::PlayerDataSource::retail_save;
                if (gameConfig_.save_image_at_game_end) {
                    std::string preview_error;
                    if (!savePreview_.writeForSave(
                            gameplayPlayer_.save_path,
                            &preview_error)) {
                        std::fprintf(
                            stderr,
                            "Could not save the character preview: %s\n",
                            preview_error.c_str());
                    }
                }
                pendingGameplayOptionsAction_ =
                    result.action;
            }
        }
        return was_active ||
               gameplayOptions_.active() ||
               (input_.gameplayOptionsPressed() &&
                !world_.conversationActive());
    }

    void applyGameplayConfig() {
        shadowOpacity_ =
            gameConfig_.semi_transparent_shadow
                ? 500
                : 1000;
        world_.configurePointer({
            gameConfig_.click_range,
            gameConfig_.click_range_enabled,
            gameConfig_.click_priority,
        });
        audio_.setEffectVolume(gameConfig_.effect_volume);
        audio_.setBgmVolume(gameConfig_.bgm_volume);
    }

    LwlWindow* window_ = nullptr;
    LwlGlContext* context_ = nullptr;
    bool windowingInitialized_ = false;
    bool configDirty_ = false;
    std::int32_t shadowOpacity_ = 500;
    osf::GameConfig gameConfig_;
    osf::PlayerLoadRequest gameplayPlayer_;
    std::filesystem::path dataRoot_;
    osf::runtime::FrontendAssets frontendAssets_;
    LglSurfacePresenter surfacePresenter_;
    osf::gapi::SoftwareBackend renderer_;
    osf::TitleFrameResult titleFrame_;
    osf::CharacterSelectFrameResult characterFrame_;
    osf::GameplayFrameResult gameplayFrame_;
    osf::runtime::AudioSystem audio_;
    osf::runtime::InputAdapter input_;
    osf::RetailRandom random_;
    osf::TitleState titleState_;
    osf::CharacterSelectState characterSelectState_;
    osf::WorldScene world_;
    osf::RetailSavePreview savePreview_;
    osf::GameplayOptionsMenu gameplayOptions_;
    osf::GameplayOptionsAction
        pendingGameplayOptionsAction_ =
            osf::GameplayOptionsAction::none;
    osf::GameplayState gameplayState_;
    osf::GameStateDispatcher gameState_;
};

}  // namespace

int osf::runtime::runGame(
    const std::filesystem::path& data_root,
    const GameConfig& game_config,
    bool smoke_test) {
    Runtime runtime(data_root);
    if (!runtime.initialize(game_config)) {
        return 1;
    }
    return runtime.run(smoke_test);
}
