#include "runtime/presentation/surface_presenter.hpp"

#include "lwl.h"

#include <switch.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

namespace {

constexpr std::int32_t kScreenWidth = 1280;
constexpr std::int32_t kScreenHeight = 720;

class SwitchSurfacePresenter final
    : public osf::runtime::SurfacePresenter {
public:
    ~SwitchSurfacePresenter() override {
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

    Framebuffer framebuffer_{};
    bool initialized_ = false;
    bool frame_prepared_ = false;
};

bool SwitchSurfacePresenter::initialize(
    LwlWindow* window,
    std::string* error) {
    (void) window;
    shutdown();

    if (R_FAILED(framebufferCreate(
            &framebuffer_,
            nwindowGetDefault(),
            kScreenWidth,
            kScreenHeight,
            PIXEL_FORMAT_RGBA_8888,
            2)) ||
        R_FAILED(framebufferMakeLinear(&framebuffer_))) {
        framebufferClose(&framebuffer_);
        std::memset(&framebuffer_, 0, sizeof(framebuffer_));
        if (error) {
            *error = "Could not create the Switch framebuffer.";
        }
        return false;
    }

    initialized_ = true;
    if (error) {
        error->clear();
    }
    return true;
}

void SwitchSurfacePresenter::prepareFrame(
    osf::gapi::SurfaceView surface) {
    frame_prepared_ = false;
    static_assert(
        sizeof(osf::gapi::Color) == sizeof(std::uint32_t),
        "GAPI colors must be tightly packed RGBA bytes.");
    if (!initialized_ || !surface.pixels ||
        surface.width <= 0 || surface.height <= 0) {
        return;
    }

    std::uint32_t stride = 0;
    auto* const output = static_cast<std::uint32_t*>(
        framebufferBegin(&framebuffer_, &stride));
    if (!output || stride <
        static_cast<std::uint32_t>(kScreenWidth * sizeof(*output))) {
        if (output) {
            framebufferEnd(&framebuffer_);
        }
        return;
    }

    const osf::gapi::Viewport viewport = osf::gapi::fitViewport(
        surface.width,
        surface.height,
        kScreenWidth,
        kScreenHeight);
    const std::uint32_t outputStride = stride / sizeof(*output);
    for (std::int32_t y = 0; y < kScreenHeight; ++y) {
        std::uint32_t* const row = output + y * outputStride;
        for (std::int32_t x = 0; x < kScreenWidth; ++x) {
            row[x] = RGBA8(0, 0, 0, 255);
        }
    }

    for (std::int32_t y = 0; y < viewport.height; ++y) {
        const std::int32_t sourceY =
            y * surface.height / viewport.height;
        std::uint32_t* const row = output +
            (viewport.y + y) * outputStride + viewport.x;
        for (std::int32_t x = 0; x < viewport.width; ++x) {
            const std::int32_t sourceX =
                x * surface.width / viewport.width;
            const osf::gapi::Color color = surface.pixels[
                sourceY * surface.width + sourceX];
            row[x] = RGBA8(
                color.red, color.green, color.blue, color.alpha);
        }
    }
    frame_prepared_ = true;
}

void SwitchSurfacePresenter::displayFrame() {
    if (!initialized_ || !frame_prepared_) {
        return;
    }
    framebufferEnd(&framebuffer_);
    frame_prepared_ = false;
}

#if OSF_ENABLE_DEBUG_TOOLS
std::optional<std::uint64_t>
SwitchSurfacePresenter::videoMemoryUsageBytes() const {
    if (!initialized_) {
        return std::nullopt;
    }
    constexpr std::uint64_t kBufferCount = 2;
    return static_cast<std::uint64_t>(kScreenWidth) *
           static_cast<std::uint64_t>(kScreenHeight) *
           sizeof(std::uint32_t) * kBufferCount;
}
#endif

void SwitchSurfacePresenter::shutdown() {
    if (initialized_ && frame_prepared_) {
        framebufferEnd(&framebuffer_);
    }
    frame_prepared_ = false;
    if (initialized_) {
        framebufferClose(&framebuffer_);
        initialized_ = false;
    }
    std::memset(&framebuffer_, 0, sizeof(framebuffer_));
}

}

std::unique_ptr<osf::runtime::SurfacePresenter>
osf::runtime::createSurfacePresenter() {
    return std::make_unique<SwitchSurfacePresenter>();
}
