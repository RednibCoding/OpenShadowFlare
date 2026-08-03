#include "runtime/presentation/surface_presenter.hpp"

#include <SDL.h>

#include <cstdint>
#include <memory>
#include <string>

namespace {

constexpr std::int32_t kFrameWidth = 640;
constexpr std::int32_t kFrameHeight = 480;

class WiiUSurfacePresenter final : public osf::runtime::SurfacePresenter {
public:
    ~WiiUSurfacePresenter() override {
        shutdown();
    }

    bool initialize(LwlWindow* window, std::string* error) override;
    void prepareFrame(osf::gapi::SurfaceView surface) override;
    void displayFrame() override;
#if OSF_ENABLE_DEBUG_TOOLS
    std::optional<std::uint64_t>
        videoMemoryUsageBytes() const override;
#endif

private:
    void shutdown();

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    bool videoInitialized_ = false;
    bool frame_prepared_ = false;
};

bool WiiUSurfacePresenter::initialize(LwlWindow* window, std::string* error) {
    (void) window;
    shutdown();

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        if (error) {
            *error = std::string("Could not initialize Wii U video: ") +
                SDL_GetError();
        }
        return false;
    }
    videoInitialized_ = true;

    window_ = SDL_CreateWindow(
        "OpenShadowFlare", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        kFrameWidth, kFrameHeight, SDL_WINDOW_SHOWN);
    if (!window_) {
        if (error) {
            *error = std::string("Could not create the Wii U video surface: ") +
                SDL_GetError();
        }
        shutdown();
        return false;
    }

    renderer_ = SDL_CreateRenderer(
        window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        if (error) {
            *error = std::string("Could not create the Wii U GPU renderer: ") +
                SDL_GetError();
        }
        shutdown();
        return false;
    }

    texture_ = SDL_CreateTexture(
        renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
        kFrameWidth, kFrameHeight);
    if (!texture_) {
        if (error) {
            *error = std::string("Could not create the Wii U frame texture: ") +
                SDL_GetError();
        }
        shutdown();
        return false;
    }

    if (error) {
        error->clear();
    }
    return true;
}

void WiiUSurfacePresenter::prepareFrame(osf::gapi::SurfaceView surface) {
    frame_prepared_ = false;
    static_assert(
        sizeof(osf::gapi::Color) == sizeof(std::uint32_t),
        "GAPI colors must be tightly packed RGBA bytes.");
    if (!texture_ || !renderer_ || !surface.pixels || surface.width != kFrameWidth ||
        surface.height != kFrameHeight) {
        return;
    }

    if (SDL_UpdateTexture(
            texture_, nullptr, surface.pixels,
            kFrameWidth *
                static_cast<std::int32_t>(sizeof(*surface.pixels))) != 0 ||
        SDL_RenderClear(renderer_) != 0 ||
        SDL_RenderCopy(renderer_, texture_, nullptr, nullptr) != 0) {
        return;
    }
    frame_prepared_ = true;
}

void WiiUSurfacePresenter::displayFrame() {
    if (!renderer_ || !frame_prepared_) {
        return;
    }
    SDL_RenderPresent(renderer_);
    frame_prepared_ = false;
}

#if OSF_ENABLE_DEBUG_TOOLS
std::optional<std::uint64_t>
WiiUSurfacePresenter::videoMemoryUsageBytes() const {
    if (!texture_) {
        return std::nullopt;
    }
    return static_cast<std::uint64_t>(kFrameWidth) *
           static_cast<std::uint64_t>(kFrameHeight) *
           sizeof(std::uint32_t);
}
#endif

void WiiUSurfacePresenter::shutdown() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    if (videoInitialized_) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        videoInitialized_ = false;
    }
    frame_prepared_ = false;
}

}  // namespace

std::unique_ptr<osf::runtime::SurfacePresenter>
osf::runtime::createSurfacePresenter() {
    return std::make_unique<WiiUSurfacePresenter>();
}
