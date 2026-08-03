#include "runtime/presentation/surface_presenter.hpp"

#include "lwl.h"

#include <psp2/display.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <memory>
#include <string>

namespace {

constexpr std::int32_t kScreenWidth = 960;
constexpr std::int32_t kScreenHeight = 544;
constexpr std::size_t kBufferBytes =
    static_cast<std::size_t>(kScreenWidth) * kScreenHeight *
    sizeof(std::uint32_t);

class VitaSurfacePresenter final
    : public osf::runtime::SurfacePresenter {
public:
    ~VitaSurfacePresenter() override {
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

    std::uint32_t* buffers_ = nullptr;
    int back_buffer_ = 0;
    bool initialized_ = false;
    bool frame_prepared_ = false;
};

bool VitaSurfacePresenter::initialize(
    LwlWindow* window,
    std::string* error) {
    (void) window;
    shutdown();

    // The display module requires each framebuffer to be 256 KiB aligned.
    buffers_ = static_cast<std::uint32_t*>(
        memalign(0x40000, kBufferBytes * 2));
    if (!buffers_) {
        if (error) {
            *error = "Could not allocate Vita display buffers.";
        }
        return false;
    }
    std::memset(buffers_, 0, kBufferBytes * 2);

    SceDisplayFrameBuf frameBuffer{};
    frameBuffer.size = sizeof(frameBuffer);
    frameBuffer.base = buffers_;
    frameBuffer.pitch = kScreenWidth;
    frameBuffer.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    frameBuffer.width = kScreenWidth;
    frameBuffer.height = kScreenHeight;
    if (sceDisplaySetFrameBuf(
            &frameBuffer, SCE_DISPLAY_SETBUF_NEXTFRAME) < 0) {
        shutdown();
        if (error) {
            *error = "Could not configure the Vita display framebuffer.";
        }
        return false;
    }

    initialized_ = true;
    if (error) {
        error->clear();
    }
    return true;
}

void VitaSurfacePresenter::prepareFrame(osf::gapi::SurfaceView surface) {
    frame_prepared_ = false;
    static_assert(
        sizeof(osf::gapi::Color) == sizeof(std::uint32_t),
        "GAPI colors must be tightly packed RGBA bytes.");
    if (!initialized_ || !surface.pixels ||
        surface.width <= 0 || surface.height <= 0) {
        return;
    }

    std::uint32_t* const output = buffers_ +
        static_cast<std::size_t>(back_buffer_) *
        kScreenWidth * kScreenHeight;
    std::memset(output, 0, kBufferBytes);

    const osf::gapi::Viewport viewport = osf::gapi::fitViewport(
        surface.width, surface.height, kScreenWidth, kScreenHeight);
    for (std::int32_t y = 0; y < viewport.height; ++y) {
        const std::int32_t sourceY = y * surface.height / viewport.height;
        std::uint32_t* const row = output +
            (viewport.y + y) * kScreenWidth + viewport.x;
        for (std::int32_t x = 0; x < viewport.width; ++x) {
            const std::int32_t sourceX =
                x * surface.width / viewport.width;
            const osf::gapi::Color color = surface.pixels[
                sourceY * surface.width + sourceX];
            // A8B8G8R8 is stored as RGBA bytes on Vita's little-endian CPU.
            row[x] = (static_cast<std::uint32_t>(color.alpha) << 24) |
                (static_cast<std::uint32_t>(color.blue) << 16) |
                (static_cast<std::uint32_t>(color.green) << 8) |
                color.red;
        }
    }

    SceDisplayFrameBuf frameBuffer{};
    frameBuffer.size = sizeof(frameBuffer);
    frameBuffer.base = output;
    frameBuffer.pitch = kScreenWidth;
    frameBuffer.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    frameBuffer.width = kScreenWidth;
    frameBuffer.height = kScreenHeight;
    if (sceDisplaySetFrameBuf(
            &frameBuffer, SCE_DISPLAY_SETBUF_NEXTFRAME) < 0) {
        return;
    }
    frame_prepared_ = true;
}

void VitaSurfacePresenter::displayFrame() {
    if (!initialized_ || !frame_prepared_) {
        return;
    }
    sceDisplayWaitVblankStart();
    back_buffer_ = 1 - back_buffer_;
    frame_prepared_ = false;
}

#if OSF_ENABLE_DEBUG_TOOLS
std::optional<std::uint64_t>
VitaSurfacePresenter::videoMemoryUsageBytes() const {
    if (!initialized_) {
        return std::nullopt;
    }
    return static_cast<std::uint64_t>(kBufferBytes) * 2;
}
#endif

void VitaSurfacePresenter::shutdown() {
    if (buffers_) {
        free(buffers_);
        buffers_ = nullptr;
    }
    back_buffer_ = 0;
    initialized_ = false;
    frame_prepared_ = false;
}

}  // namespace

std::unique_ptr<osf::runtime::SurfacePresenter>
osf::runtime::createSurfacePresenter() {
    return std::make_unique<VitaSurfacePresenter>();
}
