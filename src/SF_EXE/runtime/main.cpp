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

class Runtime {
public:
    Runtime()
        : titleState_(random_),
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
                } else if (
                    event.type == LWL_EVENT_KEY_DOWN &&
                    std::strcmp(event.key, "escape") == 0) {
                    running = false;
                } else if (event.type == LWL_EVENT_RESIZED) {
                    lglViewport(0, 0, event.x, event.y);
                }
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
