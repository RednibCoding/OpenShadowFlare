#include "lal.h"
#include "lgl.h"
#include "lwl.h"
#include "core/command_line.hpp"
#include "core/game_config.hpp"
#include "core/retail_random.hpp"
#include "states/game_state.hpp"
#include "states/menu_states.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

namespace {

constexpr int kVirtualWidth = 640;
constexpr int kVirtualHeight = 480;

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

bool fileExists(std::string_view retailPath) {
    std::error_code error;
    return std::filesystem::is_regular_file(
        nativePath(retailPath), error);
}

std::int32_t countRetailSaves() {
    std::int32_t count = 0;
    for (std::int32_t index = 0; index < 6; ++index) {
        char path[32]{};
        std::snprintf(path, sizeof(path), "Save\\%04d.Ssv", index);
        if (fileExists(path)) {
            ++count;
        }
    }
    return count;
}

class Runtime {
public:
    Runtime()
        : titleState_(random_, makeTitleStateHooks()),
          characterSelectState_(makeCharacterSelectStateHooks()),
          gameState_(makeGameStateCallbacks()) {}

    ~Runtime() {
        lgl_reset();
        lwl_gl_context_destroy(context_);
        lwl_window_destroy(window_);
        if (audioInitialized_) {
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
            std::fprintf(stderr, "Could not load OpenGL: %s\n", lgl_last_error());
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
        }

        lglViewport(0, 0, kVirtualWidth, kVirtualHeight);
        gameState_.transition(osf::GameState::title);
        return true;
    }

    int run(bool smokeTest) {
        bool running = true;
        int renderedFrames = 0;
        double nextFrame = lwl_time_seconds();

        while (running) {
            LwlEvent event{};
            while (lwl_poll_event(window_, &event)) {
                if (event.type == LWL_EVENT_QUIT) {
                    running = false;
                } else {
                    handleEvent(event, running);
                }
            }

            if (running) {
                updateGame(running);
            }

            lglClearColor(0.025f, 0.025f, 0.04f, 1.0f);
            lglClear(LGL_COLOR_BUFFER_BIT);
            lwl_gl_context_swap_buffers(context_);

            ++renderedFrames;
            if (smokeTest && renderedFrames >= 3) {
                running = false;
            }

            nextFrame += 1.0 / 60.0;
            lwl_sleep_until_seconds(nextFrame);
        }
        return 0;
    }

private:
    void setPointerPosition(std::int32_t x, std::int32_t y) {
        int width = kVirtualWidth;
        int height = kVirtualHeight;
        lwl_window_get_size(window_, &width, &height);
        if (width > 0 && height > 0) {
            menuInput_.pointer_x = x * kVirtualWidth / width;
            menuInput_.pointer_y = y * kVirtualHeight / height;
            characterInput_.pointer_x = menuInput_.pointer_x;
            characterInput_.pointer_y = menuInput_.pointer_y;
        }
    }

    void handleEvent(const LwlEvent& event, bool& running) {
        if (event.type == LWL_EVENT_RESIZED) {
            lglViewport(0, 0, event.x, event.y);
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
        }
    }

    void updateGame(bool& running) {
        switch (gameState_.currentState()) {
        case osf::GameState::title: {
            const osf::TitleFrameResult result =
                titleState_.update(menuInput_);
            if (result.action == osf::TitleAction::open_character_select) {
                gameState_.transition(
                    osf::GameState::character_select,
                    result.character_select_argument);
            } else if (result.action == osf::TitleAction::exit_game) {
                running = false;
            }
            break;
        }
        case osf::GameState::character_select: {
            characterInput_.saved_game_count = savedGameCount_;
            const osf::CharacterSelectFrameResult result =
                characterSelectState_.update(characterInput_);
            if (result.action ==
                osf::CharacterSelectAction::return_to_title) {
                gameState_.transition(osf::GameState::title);
            } else if (
                result.action ==
                osf::CharacterSelectAction::enter_gameplay) {
                gameState_.transition(osf::GameState::gameplay);
            } else if (
                result.action ==
                osf::CharacterSelectAction::exit_game) {
                running = false;
            }
            break;
        }
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
    }

    osf::TitleStateHooks makeTitleStateHooks() {
        osf::TitleStateHooks hooks;
        hooks.files_exist = [](std::string_view pattern) {
            return pattern == "Save\\*.Ssv" &&
                   countRetailSaves() != 0;
        };
        hooks.file_exists = fileExists;
        return hooks;
    }

    osf::CharacterSelectStateHooks makeCharacterSelectStateHooks() {
        osf::CharacterSelectStateHooks hooks;
        hooks.file_exists = fileExists;
        hooks.load_saved_characters = [this] {
            savedGameCount_ = countRetailSaves();
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
        return callbacks;
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
    std::int32_t savedGameCount_ = 0;
    osf::MenuFrameInput menuInput_;
    osf::CharacterSelectFrameInput characterInput_;
    osf::RetailRandom random_;
    osf::TitleState titleState_;
    osf::CharacterSelectState characterSelectState_;
    osf::GameStateDispatcher gameState_;
};

}  // namespace

int main(int argc, char** argv) {
    osf::GameConfig gameConfig;

    // Retail ignores config-load failure and retains its constructor defaults.
    osf::loadGameConfigFile("SFlare.Cfg", gameConfig);
    for (int index = 1; index < argc; ++index) {
        osf::applyRetailCommandLine(argv[index], gameConfig);
    }

    Runtime runtime;
    if (!runtime.initialize(gameConfig)) {
        return 1;
    }
    return runtime.run(isSmokeTest(argc, argv));
}
