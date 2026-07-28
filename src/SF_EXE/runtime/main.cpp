#include "lal.h"
#include "lgl.h"
#include "lwl.h"
#include "core/command_line.hpp"
#include "core/game_config.hpp"
#include "core/retail_random.hpp"
#include "libs/RKC_DIB/rkc_dib.hpp"
#include "libs/RKC_DBFCONTROL/rkc_dbfcontrol.hpp"
#include "libs/RKC_DSOUND/rkc_dsound.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "render/character_select_renderer.hpp"
#include "render/gameplay_renderer.hpp"
#include "render/title_renderer.hpp"
#include "runtime/lgl_surface_presenter.hpp"
#include "states/game_state.hpp"
#include "states/gameplay_state.hpp"
#include "states/menu_states.hpp"
#include "states/save_catalog.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr int kVirtualWidth = 640;
constexpr int kVirtualHeight = 480;
constexpr std::size_t kMenuConfirmSound = 55;
constexpr std::size_t kTitleConfirmSound = 56;
constexpr std::size_t kMenuMoveSound = 58;
constexpr std::size_t kTitleCueSound = 62;

void* loadOpenGlFunction(const char* name, void*) {
    return lwl_gl_get_proc_address(name);
}

bool isSmokeTest(int argc, char** argv) {
    for (int index = 1; index < argc; ++index) {
        if (std::strcmp(argv[index], "--smoke-test") == 0) {
            return true;
        }
    }
    return false;
}

std::filesystem::path nativePath(std::string_view retailPath) {
    std::string path(retailPath);
    for (char& character : path) {
        if (character == '\\') {
            character = '/';
        }
    }
    return std::filesystem::path(path);
}

bool equalsIgnoreCase(
    std::string_view first,
    std::string_view second) {
    if (first.size() != second.size()) {
        return false;
    }
    for (std::size_t index = 0; index < first.size(); ++index) {
        const auto left = static_cast<unsigned char>(first[index]);
        const auto right = static_cast<unsigned char>(second[index]);
        if (std::tolower(left) != std::tolower(right)) {
            return false;
        }
    }
    return true;
}

std::filesystem::path resolveRetailPath(
    const std::filesystem::path& root,
    std::string_view retailPath) {
    const std::filesystem::path requested = nativePath(retailPath);
    std::filesystem::path resolved = root;
    for (const std::filesystem::path& component : requested) {
        const std::filesystem::path exact = resolved / component;
        std::error_code error;
        if (std::filesystem::exists(exact, error)) {
            resolved = exact;
            continue;
        }

        const std::filesystem::path directory =
            resolved.empty() ? std::filesystem::path(".") : resolved;
        bool matched = false;
        for (std::filesystem::directory_iterator iterator(
                 directory, error);
             !error &&
             iterator != std::filesystem::directory_iterator();
             iterator.increment(error)) {
            if (equalsIgnoreCase(
                    iterator->path().filename().string(),
                    component.string())) {
                resolved /= iterator->path().filename();
                matched = true;
                break;
            }
        }
        if (!matched) {
            return root / requested;
        }
    }
    return resolved;
}

std::filesystem::path findDataRoot() {
    const auto isDataRoot =
        [](const std::filesystem::path& candidate) {
            std::error_code error;
            const bool hasConfig = std::filesystem::is_regular_file(
                candidate / "SFlare.Cfg", error);
            error.clear();
            const bool hasTitle = std::filesystem::is_regular_file(
                candidate / "System" / "Title" / "Pattern" /
                    "Title.njp",
                error);
            return hasConfig && hasTitle;
        };
    const auto searchParents =
        [&isDataRoot](std::filesystem::path directory) {
            for (;;) {
                const std::filesystem::path candidates[] = {
                    directory,
                    directory / "ShadowFlare",
                    directory / "tmp" / "ShadowFlare",
                };
                for (const std::filesystem::path& candidate :
                     candidates) {
                    if (isDataRoot(candidate)) {
                        return candidate;
                    }
                }

                const std::filesystem::path parent =
                    directory.parent_path();
                if (parent.empty() || parent == directory) {
                    break;
                }
                directory = parent;
            }
            return std::filesystem::path{};
        };

    std::error_code error;
    const std::filesystem::path fromWorkingDirectory =
        searchParents(std::filesystem::current_path(error));
    if (!fromWorkingDirectory.empty()) {
        return fromWorkingDirectory;
    }

    char executablePath[4096]{};
    if (lwl_exe_path(
            executablePath,
            static_cast<int>(sizeof(executablePath)))) {
        const std::filesystem::path fromExecutable =
            searchParents(
                std::filesystem::absolute(
                    executablePath, error).parent_path());
        if (!fromExecutable.empty()) {
            return fromExecutable;
        }
    }

    const std::filesystem::path fallbacks[] = {
        ".",
        std::filesystem::path("tmp") / "ShadowFlare",
    };
    for (const std::filesystem::path& candidate : fallbacks) {
        if (isDataRoot(candidate)) {
            return candidate;
        }
    }
    return ".";
}

bool fileExists(
    const std::filesystem::path& root,
    std::string_view retailPath) {
    std::error_code error;
    return std::filesystem::is_regular_file(
        resolveRetailPath(root, retailPath), error);
}

std::int32_t countRetailSaves(
    const std::filesystem::path& root) {
    std::int32_t count = 0;
    for (std::int32_t index = 0; index < 6; ++index) {
        char path[32]{};
        std::snprintf(path, sizeof(path), "Save\\%04d.Ssv", index);
        if (fileExists(root, path)) {
            ++count;
        }
    }
    return count;
}

bool deleteRetailSave(
    const std::filesystem::path& root,
    std::int32_t logicalIndex) {
    if (logicalIndex < 0) {
        return false;
    }

    for (std::int32_t slot = 0; slot < 6; ++slot) {
        char savePath[32]{};
        std::snprintf(
            savePath, sizeof(savePath), "Save\\%04d.Ssv", slot);
        if (!fileExists(root, savePath)) {
            continue;
        }
        if (logicalIndex-- != 0) {
            continue;
        }

        std::error_code error;
        std::filesystem::remove(
            resolveRetailPath(root, savePath), error);

        char previewPath[32]{};
        std::snprintf(
            previewPath, sizeof(previewPath), "Save\\%04d.Bmp", slot);
        error.clear();
        std::filesystem::remove(
            resolveRetailPath(root, previewPath), error);
        return true;
    }
    return false;
}

class Runtime {
public:
    explicit Runtime(std::filesystem::path dataRoot)
        : dataRoot_(std::move(dataRoot)),
          renderer_(
              kVirtualWidth,
              kVirtualHeight,
              [this](osf::gapi::SurfaceView surface) {
                  presentSurface(surface);
              }),
          titleState_(random_, makeTitleStateHooks()),
          characterSelectState_(makeCharacterSelectStateHooks()),
          gameplayState_(makeGameplayStateHooks()),
          gameState_(makeGameStateCallbacks()) {}

    ~Runtime() {
        surfacePresenter_.shutdown();
        lgl_reset();
        lwl_gl_context_destroy(context_);
        lwl_window_destroy(window_);
        if (audioInitialized_) {
            effectAudio_.clear();
            menuMusicAudio_.clear();
            worldMusicAudio_.clear();
            lal_shutdown();
        }
        if (windowingInitialized_) {
            lwl_shutdown();
        }
    }

    bool initialize(const osf::GameConfig& gameConfig) {
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
        lwl_window_set_mode(
            window_,
            gameConfig.windowed_at_start
                ? LWL_WINDOW_NORMAL
                : LWL_WINDOW_FULLSCREEN);
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

        audioInitialized_ = lal_init();
        if (!audioInitialized_) {
            std::fprintf(
                stderr,
                "Warning: audio is unavailable: %s\n",
                lal_last_error());
        } else {
            effectVolume_ = gameConfig.effect_volume;
            bgmVolume_ = gameConfig.bgm_volume;
            loadVoc(
                effectAudio_,
                "System\\Game\\Voice\\Voice00.Voc");
        }
        shadowOpacity_ =
            gameConfig.semi_transparent_shadow ? 500 : 1000;

        if (!loadPattern(
                0, "System\\Common\\Pattern\\Font00.njp") ||
            !loadPattern(
                1, "System\\Common\\Pattern\\Font01.njp") ||
            !loadPattern(
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
                if (event.type == LWL_EVENT_QUIT) {
                    running = false;
                } else {
                    handleEvent(event, running);
                }
            }

            while (running && gameAccumulator >= gameStep) {
                updateGame(running);
                gameAccumulator -= gameStep;
            }

            renderGame();

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

    void setPointerPosition(std::int32_t x, std::int32_t y) {
        int width = kVirtualWidth;
        int height = kVirtualHeight;
        lwl_window_get_size(window_, &width, &height);
        if (width > 0 && height > 0) {
            const osf::gapi::Viewport viewport =
                osf::gapi::fitViewport(
                    kVirtualWidth,
                    kVirtualHeight,
                    width,
                    height);
            menuInput_.pointer_x =
                (x - viewport.x) * kVirtualWidth / viewport.width;
            menuInput_.pointer_y =
                (y - viewport.y) * kVirtualHeight / viewport.height;
            characterInput_.pointer_x = menuInput_.pointer_x;
            characterInput_.pointer_y = menuInput_.pointer_y;
        }
    }

    void handleEvent(const LwlEvent& event, bool& running) {
        if (event.type == LWL_EVENT_RESIZED) {
            return;
        }
        if (event.type == LWL_EVENT_MOUSE_MOVE) {
            setPointerPosition(event.x, event.y);
            return;
        }
        if (event.type == LWL_EVENT_MOUSE_DOWN && event.button == 1) {
            setPointerPosition(event.x, event.y);
            menuInput_.pointer_primary_pressed = true;
            characterInput_.pointer_primary_pressed = true;
            pointerPrimaryDown_ = true;
            return;
        }
        if (event.type == LWL_EVENT_MOUSE_UP && event.button == 1) {
            setPointerPosition(event.x, event.y);
            pointerPrimaryDown_ = false;
            return;
        }
        if (event.type == LWL_EVENT_TEXT_INPUT) {
            characterInput_.text_input += event.text;
            return;
        }
        if (event.type == LWL_EVENT_KEY_UP) {
            if (std::strcmp(event.key, "up") == 0) {
                upHeld_ = false;
            } else if (std::strcmp(event.key, "down") == 0) {
                downHeld_ = false;
            } else if (std::strcmp(event.key, "left") == 0) {
                leftHeld_ = false;
            } else if (std::strcmp(event.key, "right") == 0) {
                rightHeld_ = false;
            } else if (
                std::strcmp(event.key, "return") == 0 ||
                std::strcmp(event.key, "keypad enter") == 0) {
                confirmHeld_ = false;
            } else if (std::strcmp(event.key, "escape") == 0) {
                backHeld_ = false;
            } else if (std::strcmp(event.key, "delete") == 0) {
                deleteHeld_ = false;
            } else if (std::strcmp(event.key, "backspace") == 0) {
                backspaceHeld_ = false;
            } else if (std::strcmp(event.key, "r") == 0) {
                runHeld_ = false;
            }
            return;
        }
        if (event.type != LWL_EVENT_KEY_DOWN) {
            return;
        }

        if (std::strcmp(event.key, "escape") == 0) {
            if (gameState_.currentState() ==
                osf::GameState::character_select) {
                if (!backHeld_) {
                    characterInput_.back_pressed = true;
                }
                backHeld_ = true;
            } else {
                running = false;
            }
        } else if (std::strcmp(event.key, "up") == 0) {
            if (!upHeld_) {
                menuInput_.up_pressed = true;
                characterInput_.up_pressed = true;
            }
            upHeld_ = true;
        } else if (std::strcmp(event.key, "down") == 0) {
            if (!downHeld_) {
                menuInput_.down_pressed = true;
                characterInput_.down_pressed = true;
            }
            downHeld_ = true;
        } else if (std::strcmp(event.key, "left") == 0) {
            if (!leftHeld_) {
                characterInput_.left_pressed = true;
            }
            leftHeld_ = true;
        } else if (std::strcmp(event.key, "right") == 0) {
            if (!rightHeld_) {
                characterInput_.right_pressed = true;
            }
            rightHeld_ = true;
        } else if (
            std::strcmp(event.key, "return") == 0 ||
            std::strcmp(event.key, "keypad enter") == 0) {
            if (!confirmHeld_) {
                menuInput_.confirm_pressed = true;
                characterInput_.confirm_pressed = true;
            }
            confirmHeld_ = true;
        } else if (std::strcmp(event.key, "delete") == 0) {
            if (!deleteHeld_) {
                characterInput_.delete_pressed = true;
            }
            deleteHeld_ = true;
        } else if (std::strcmp(event.key, "backspace") == 0) {
            if (!backspaceHeld_) {
                characterInput_.backspace_pressed = true;
            }
            backspaceHeld_ = true;
        } else if (std::strcmp(event.key, "r") == 0) {
            if (!runHeld_) {
                runTogglePressed_ = true;
            }
            runHeld_ = true;
        }
    }

    void updateGame(bool& running) {
        switch (gameState_.currentState()) {
        case osf::GameState::title: {
            for (std::size_t index = 0;
                 index < titleAnimations_.size();
                 ++index) {
                const auto& charts =
                    titleAnimations_[index].charts();
                menuInput_.smoke_frame_counts[index] =
                    charts.empty()
                        ? 0
                        : charts.front().directions[8].frame_count;
            }
            titleFrame_ =
                titleState_.update(menuInput_);
            playTitleAudio(titleFrame_);
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
            characterInput_.saved_game_count = savedGameCount_;
            characterFrame_ =
                characterSelectState_.update(characterInput_);
            playCharacterSelectAudio(characterFrame_);
            if (characterFrame_.action ==
                osf::CharacterSelectAction::return_to_title) {
                gameState_.transition(osf::GameState::title);
            } else if (
                characterFrame_.action ==
                osf::CharacterSelectAction::enter_gameplay) {
                gameplayGender_ =
                    characterSelectState_.data().character_gender;
                gameState_.transition(osf::GameState::gameplay);
            } else if (
                characterFrame_.action ==
                osf::CharacterSelectAction::exit_game) {
                running = false;
            }
            break;
        }
        case osf::GameState::gameplay:
            gameplayFrame_ = gameplayState_.update({
                menuInput_.confirm_pressed,
                menuInput_.pointer_primary_pressed,
                menuInput_.pointer_x,
                menuInput_.pointer_y,
                pointerPrimaryDown_,
                runTogglePressed_,
            });
            break;
        default:
            break;
        }

        menuInput_.pointer_primary_pressed = false;
        menuInput_.confirm_pressed = false;
        menuInput_.up_pressed = false;
        menuInput_.down_pressed = false;
        characterInput_.pointer_primary_pressed = false;
        characterInput_.confirm_pressed = false;
        characterInput_.back_pressed = false;
        characterInput_.delete_pressed = false;
        characterInput_.up_pressed = false;
        characterInput_.down_pressed = false;
        characterInput_.left_pressed = false;
        characterInput_.right_pressed = false;
        characterInput_.backspace_pressed = false;
        characterInput_.text_input.clear();
        runTogglePressed_ = false;
    }

    void renderCharacterSelect() {
        const auto pattern = patterns_.find(4);
        if (pattern == patterns_.end()) {
            return;
        }
        const auto font = patterns_.find(0);
        osf::renderCharacterSelect(
            renderer_,
            pattern->second,
            font == patterns_.end() ? nullptr : &font->second,
            characterSelectState_.data(),
            characterFrame_,
            characterInput_,
            savedGames_,
            savedPreviews_);
    }

    void renderGame() {
        renderer_.beginFrame({0, 0, 0, 255});
        if (gameState_.currentState() == osf::GameState::title) {
            const auto pattern = patterns_.find(4);
            if (pattern != patterns_.end()) {
                std::array<osf::TitleSmokeAsset, 10> smoke{};
                for (std::size_t index = 0;
                     index < smoke.size();
                     ++index) {
                    const auto smokePattern =
                        patterns_.find(
                            5 + static_cast<std::int32_t>(index) * 2);
                    if (smokePattern != patterns_.end()) {
                        smoke[index] = {
                            &smokePattern->second,
                            &titleAnimations_[index],
                        };
                    }
                }
                osf::renderTitle(
                    renderer_,
                    pattern->second,
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
                const auto waiting = patterns_.find(2);
                if (waiting != patterns_.end()) {
                    osf::renderInitialLoadingScreen(
                        renderer_,
                        waiting->second,
                        gameplayFrame_.loading_counter,
                        gameplayFrame_.ready_to_continue);
                }
            } else {
                const auto font = patterns_.find(1);
                osf::renderWorld(
                    renderer_,
                    world_,
                    shadowOpacity_,
                    font == patterns_.end()
                        ? nullptr
                        : &font->second);
            }
        }
        renderer_.endFrame();
    }

    bool loadPattern(
        std::int32_t id,
        std::string_view retailPath) {
        osf::gapi::NjpImage image;
        std::string error;
        const std::filesystem::path path =
            resolveRetailPath(dataRoot_, retailPath);
        if (!image.load(path, &error)) {
            std::fprintf(
                stderr,
                "Could not load %s: %s\n",
                path.string().c_str(),
                error.c_str());
            return false;
        }
        patterns_.insert_or_assign(id, std::move(image));
        return true;
    }

    bool loadVoc(
        osf::VocPlayer& player,
        std::string_view retailPath) {
        if (!audioInitialized_) {
            return false;
        }
        std::string error;
        const std::filesystem::path path =
            resolveRetailPath(dataRoot_, retailPath);
        if (!player.load(path, &error)) {
            std::fprintf(
                stderr,
                "Could not load %s: %s\n",
                path.string().c_str(),
                error.c_str());
            return false;
        }
        return true;
    }

    void playRepeatedEffect(
        std::size_t sample,
        std::int32_t count) {
        for (std::int32_t index = 0; index < count; ++index) {
            effectAudio_.play(sample, false, effectVolume_);
        }
    }

    void playTitleAudio(const osf::TitleFrameResult& frame) {
        if (frame.play_title_sound) {
            effectAudio_.play(
                kTitleCueSound, false, effectVolume_);
        }
        playRepeatedEffect(
            kMenuMoveSound, frame.play_move_sound_count);
        if (frame.play_confirm_sound) {
            effectAudio_.play(
                kTitleConfirmSound, false, effectVolume_);
        }
        if (frame.start_menu_music) {
            menuMusicAudio_.play(0, true, bgmVolume_);
        }
    }

    void playCharacterSelectAudio(
        const osf::CharacterSelectFrameResult& frame) {
        playRepeatedEffect(
            kMenuMoveSound, frame.play_move_sound_count);
        playRepeatedEffect(
            kMenuConfirmSound,
            frame.play_selection_sound_count);
    }

    osf::TitleStateHooks makeTitleStateHooks() {
        osf::TitleStateHooks hooks;
        hooks.load_pattern =
            [this](
                std::int32_t id,
                std::string_view path) {
                return loadPattern(id, path);
            };
        hooks.load_animation =
            [this](
                std::size_t index,
                std::int32_t,
                std::string_view path) {
                if (index >= titleAnimations_.size()) {
                    return false;
                }
                std::string error;
                const std::filesystem::path resolved =
                    resolveRetailPath(dataRoot_, path);
                if (!titleAnimations_[index].load(
                        resolved, &error)) {
                    std::fprintf(
                        stderr,
                        "Could not load %s: %s\n",
                        resolved.string().c_str(),
                        error.c_str());
                    return false;
                }
                return true;
            };
        hooks.release_pattern = [this](std::int32_t id) {
            patterns_.erase(id);
        };
        hooks.release_animation = [this](std::size_t index) {
            if (index < titleAnimations_.size()) {
                titleAnimations_[index].clear();
            }
        };
        hooks.load_voice =
            [this](
                std::string_view path,
                std::int32_t slot) {
                if (slot == 500) {
                    loadVoc(menuMusicAudio_, path);
                }
            };
        hooks.files_exist = [this](std::string_view pattern) {
            return pattern == "Save\\*.Ssv" &&
                   countRetailSaves(dataRoot_) != 0;
        };
        hooks.file_exists =
            [this](std::string_view path) {
                return fileExists(dataRoot_, path);
            };
        return hooks;
    }

    osf::CharacterSelectStateHooks makeCharacterSelectStateHooks() {
        osf::CharacterSelectStateHooks hooks;
        hooks.load_pattern =
            [this](
                std::int32_t id,
                std::string_view path) {
                return loadPattern(id, path);
            };
        hooks.release_pattern = [this](std::int32_t id) {
            patterns_.erase(id);
        };
        hooks.file_exists =
            [this](std::string_view path) {
                return fileExists(dataRoot_, path);
            };
        hooks.load_saved_characters = [this] {
            loadSavedCharacters();
        };
        hooks.delete_saved_character = [this](std::int32_t index) {
            deleteRetailSave(dataRoot_, index);
            loadSavedCharacters();
        };
        hooks.read_clipboard = [this] {
            char* text = lwl_clipboard_get(window_);
            std::string result = text ? text : "";
            lwl_free(text);
            return result;
        };
        hooks.voice_is_playing = [this](std::int32_t slot) {
            return slot == 500 &&
                   menuMusicAudio_.isPlaying(0);
        };
        hooks.play_voice =
            [this](std::int32_t slot, bool loop) {
                if (slot == 500) {
                    menuMusicAudio_.play(0, loop, bgmVolume_);
                }
            };
        hooks.release_voice = [this](std::int32_t slot) {
            if (slot == 500) {
                menuMusicAudio_.clear();
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
            gameplayState_.enter();
        };
        callbacks.gameplay.leave = [this] {
            gameplayState_.leave();
        };
        return callbacks;
    }

    osf::GameplayStateHooks makeGameplayStateHooks() {
        osf::GameplayStateHooks hooks;
        hooks.prepare_world = [this] {
            std::string error;
            const bool worldReady =
                world_.loadInitialScenario(
                    dataRoot_, gameplayGender_, &error);
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
            const std::int32_t track = world_.musicTrack();
            if (!audioInitialized_ || track < 0 || track > 99) {
                return;
            }
            char path[48]{};
            std::snprintf(
                path,
                sizeof(path),
                "System\\Game\\Music\\BGM%02d.Voc",
                track);
            if (loadVoc(worldMusicAudio_, path)) {
                worldMusicAudio_.play(0, true, bgmVolume_);
            }
        };
        hooks.stop_world_music = [this] {
            worldMusicAudio_.clear();
        };
        hooks.command_player_movement =
            [this](std::int32_t x, std::int32_t y) {
                world_.commandPlayerMovement(x, y);
            };
        hooks.update_pointer_hover =
            [this](std::int32_t x, std::int32_t y) {
                world_.updatePointerHover(x, y);
            };
        hooks.command_world_interaction =
            [this](std::int32_t x, std::int32_t y) {
                return world_.commandWorldInteraction(x, y);
            };
        hooks.conversation_active = [this] {
            return world_.conversationActive();
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

    void loadSavedCharacters() {
        savedGames_ = osf::loadRetailSaveCatalog(dataRoot_);
        savedPreviews_.clear();
        savedPreviews_.reserve(savedGames_.size());
        for (const osf::RetailSaveSummary& save : savedGames_) {
            osf::gapi::BitmapImage preview;
            std::string error;
            preview.load(save.preview_path, &error);
            savedPreviews_.push_back(std::move(preview));
        }
        savedGameCount_ =
            static_cast<std::int32_t>(savedGames_.size());
    }

    LwlWindow* window_ = nullptr;
    LwlGlContext* context_ = nullptr;
    bool windowingInitialized_ = false;
    bool audioInitialized_ = false;
    bool upHeld_ = false;
    bool downHeld_ = false;
    bool leftHeld_ = false;
    bool rightHeld_ = false;
    bool confirmHeld_ = false;
    bool backHeld_ = false;
    bool deleteHeld_ = false;
    bool backspaceHeld_ = false;
    bool pointerPrimaryDown_ = false;
    bool runHeld_ = false;
    bool runTogglePressed_ = false;
    std::int32_t effectVolume_ = 0;
    std::int32_t bgmVolume_ = 0;
    std::int32_t shadowOpacity_ = 500;
    std::int32_t savedGameCount_ = 0;
    std::int32_t gameplayGender_ = 0;
    std::filesystem::path dataRoot_;
    std::unordered_map<std::int32_t, osf::gapi::NjpImage> patterns_;
    std::array<osf::gapi::CafAnimation, 10> titleAnimations_;
    std::vector<osf::RetailSaveSummary> savedGames_;
    std::vector<osf::gapi::BitmapImage> savedPreviews_;
    osf::VocPlayer effectAudio_;
    osf::VocPlayer menuMusicAudio_;
    osf::VocPlayer worldMusicAudio_;
    LglSurfacePresenter surfacePresenter_;
    osf::gapi::SoftwareBackend renderer_;
    osf::TitleFrameResult titleFrame_;
    osf::CharacterSelectFrameResult characterFrame_;
    osf::GameplayFrameResult gameplayFrame_;
    osf::MenuFrameInput menuInput_;
    osf::CharacterSelectFrameInput characterInput_;
    osf::RetailRandom random_;
    osf::TitleState titleState_;
    osf::CharacterSelectState characterSelectState_;
    osf::WorldScene world_;
    osf::GameplayState gameplayState_;
    osf::GameStateDispatcher gameState_;
};

}  // namespace

int main(int argc, char** argv) {
    const std::filesystem::path dataRoot = findDataRoot();
    osf::GameConfig gameConfig;

    // Retail ignores config-load failure and retains its constructor defaults.
    osf::loadGameConfigFile(
        (dataRoot / "SFlare.Cfg").string(), gameConfig);
    for (int index = 1; index < argc; ++index) {
        osf::applyRetailCommandLine(argv[index], gameConfig);
    }

    Runtime runtime(dataRoot);
    if (!runtime.initialize(gameConfig)) {
        return 1;
    }
    return runtime.run(isSmokeTest(argc, argv));
}
