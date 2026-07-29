#include "game_runtime.hpp"

#include "lwl.h"
#include "core/retail_random.hpp"
#include "resources/retail_filesystem.hpp"
#include "runtime/application_loop.hpp"
#include "runtime/audio_system.hpp"
#include "runtime/frontend_assets.hpp"
#include "runtime/gameplay_ui_controller.hpp"
#include "runtime/input_adapter.hpp"
#include "runtime/presentation/surface_presenter.hpp"
#include "runtime/runtime_renderer.hpp"
#include "runtime/state_bindings.hpp"
#include "states/game_state.hpp"
#include "states/gameplay_state.hpp"
#include "states/character_select_state.hpp"
#include "states/title_state.hpp"
#include "world/retail_save_preview.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>

namespace {

constexpr int kVirtualWidth = 640;
constexpr int kVirtualHeight = 480;

class Runtime final : public osf::runtime::FrameApplication {
public:
    explicit Runtime(std::filesystem::path dataRoot)
        : dataRoot_(std::move(dataRoot)),
          frontendAssets_(dataRoot_),
          surfacePresenter_(
              osf::runtime::createSurfacePresenter()),
          renderer_(
              kVirtualWidth,
              kVirtualHeight,
              [this](osf::gapi::SurfaceView surface) {
                  presentSurface(surface);
              }),
          input_(kVirtualWidth, kVirtualHeight),
          titleState_(
              random_,
              osf::runtime::makeTitleStateHooks(
                  dataRoot_, frontendAssets_, audio_)),
          characterSelectState_(
              osf::runtime::makeCharacterSelectStateHooks(
                  dataRoot_,
                  frontendAssets_,
                  audio_,
                  window_)),
          gameplayState_(
              osf::runtime::makeGameplayStateHooks(
                  dataRoot_,
                  gameplayPlayer_,
                  frontendAssets_,
                  audio_,
                  world_)),
          gameState_(makeGameStateCallbacks()) {}

    ~Runtime() {
        saveConfigIfDirty();
        surfacePresenter_.reset();
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

        std::string presenterError;
        if (!surfacePresenter_ ||
            !surfacePresenter_->initialize(
                window_, &presenterError)) {
            std::fprintf(
                stderr,
                "Could not initialize surface presentation: %s\n",
                presenterError.c_str());
            return false;
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

    void start(bool smokeTest) {
        smokeTest_ = smokeTest;
        running_ = true;
        renderedFrames_ = 0;
        previousTime_ = lwl_time_seconds();
        nextFrame_ = previousTime_;
        gameAccumulator_ = kGameStep;
    }

    bool frame() override {
        tickOnce();
        if (!running_) {
            saveConfigIfDirty();
        }
        return running_;
    }

private:
    static constexpr double kRenderStep = 1.0 / 60.0;
    static constexpr double kGameStep = 1.0 / 30.0;
    static constexpr double kMaximumElapsed = 0.25;

    void tickOnce() {
        const double currentTime = lwl_time_seconds();
        gameAccumulator_ += std::clamp(
            currentTime - previousTime_,
            0.0,
            kMaximumElapsed);
        previousTime_ = currentTime;

        LwlEvent event{};
        while (lwl_poll_event(window_, &event)) {
            if (!input_.handleEvent(
                    window_,
                    event,
                    gameState_.currentState())) {
                running_ = false;
            }
        }

        while (running_ && gameAccumulator_ >= kGameStep) {
            updateGame(running_);
            gameAccumulator_ -= kGameStep;
        }

        const double interpolation =
            std::clamp(
                gameAccumulator_ / kGameStep,
                0.0,
                1.0);
        renderer_.render(
            {
                gameState_.currentState(),
                titleFrame_,
                characterFrame_,
                gameplayFrame_,
                characterSelectState_,
                world_,
                frontendAssets_,
                savePreview_,
                gameplayUi_.options(),
                gameplayUi_.inventory(),
                gameplayUi_.map(),
                gameplayUi_.missionList(),
                gameConfig_,
                shadowOpacity_,
                gameplayCounter_,
            },
            interpolation);

        ++renderedFrames_;
        if (smokeTest_ && renderedFrames_ >= 3) {
            running_ = false;
        }

        nextFrame_ += kRenderStep;
        if (nextFrame_ < currentTime - kMaximumElapsed) {
            nextFrame_ = currentTime;
        }
        lwl_sleep_until_seconds(nextFrame_);
    }

    void saveConfigIfDirty() {
        if (configDirty_) {
            osf::saveGameConfigFile(
                (dataRoot_ / "SFlare.Cfg").string(),
                gameConfig_);
            configDirty_ = false;
        }
    }

    void presentSurface(osf::gapi::SurfaceView source) {
        surfacePresenter_->present(source);
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
            ++gameplayCounter_;
            if (!gameplayUi_.update(
                    gameplayFrame_,
                    input_,
                    world_,
                    audio_,
                    gameConfig_,
                    configDirty_,
                    random_,
                    gameplayPlayer_,
                    savePreview_,
                    gameState_,
                    running,
                    shadowOpacity_)) {
                const bool map_active =
                    gameplayUi_.map().active();
                const bool inventory_active =
                    gameplayUi_.inventory().active();
                gameplayFrame_ = gameplayState_.update({
                    input_.menu().confirm_pressed &&
                        !map_active,
                    input_.menu()
                        .pointer_primary_pressed,
                    input_.menu().pointer_x,
                    input_.menu().pointer_y,
                    input_.pointerPrimaryDown(),
                    input_.runTogglePressed(),
                    map_active ? 320 : 0,
                    0,
                    inventory_active ? 320 : 640,
                    400,
                });
            }
            break;
        }
        default:
            break;
        }

        input_.clearTransientInput();
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
            gameplayCounter_ = 0;
            gameplayUi_.reset();
            savePreview_.clear();
            gameplayState_.enter();
        };
        callbacks.gameplay.leave = [this] {
            gameplayUi_.reset();
            gameplayState_.leave();
        };
        return callbacks;
    }

    LwlWindow* window_ = nullptr;
    bool windowingInitialized_ = false;
    bool configDirty_ = false;
    bool running_ = false;
    bool smokeTest_ = false;
    int renderedFrames_ = 0;
    double previousTime_ = 0.0;
    double nextFrame_ = 0.0;
    double gameAccumulator_ = 0.0;
    std::int32_t shadowOpacity_ = 500;
    std::uint32_t gameplayCounter_ = 0;
    osf::GameConfig gameConfig_;
    osf::PlayerLoadRequest gameplayPlayer_;
    std::filesystem::path dataRoot_;
    osf::runtime::FrontendAssets frontendAssets_;
    std::unique_ptr<osf::runtime::SurfacePresenter>
        surfacePresenter_;
    osf::runtime::RuntimeRenderer renderer_;
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
    osf::runtime::GameplayUiController gameplayUi_;
    osf::GameplayState gameplayState_;
    osf::GameStateDispatcher gameState_;
};

}  // namespace

int osf::runtime::runGame(
    const std::filesystem::path& data_root,
    const GameConfig& game_config,
    bool smoke_test) {
    auto runtime = std::make_unique<Runtime>(data_root);
    if (!runtime->initialize(game_config)) {
        return 1;
    }
    runtime->start(smoke_test);
    return runApplicationLoop(std::move(runtime));
}
