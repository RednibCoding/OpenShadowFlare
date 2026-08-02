#include "runtime/presentation/surface_presenter.hpp"

#include "gapi/gapi.hpp"

#include <3ds.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace {

void clearFramebuffer(
    std::uint8_t* framebuffer,
    std::int32_t screen_width,
    std::int32_t screen_height,
    std::int32_t framebuffer_stride) {
    if (!framebuffer) {
        return;
    }
    for (std::int32_t x = 0; x < screen_width; ++x) {
        for (std::int32_t y = 0; y < screen_height; ++y) {
            const std::size_t offset = static_cast<std::size_t>(3) *
                static_cast<std::size_t>(
                    x * framebuffer_stride + framebuffer_stride - y - 1);
            framebuffer[offset] = 0;
            framebuffer[offset + 1] = 0;
            framebuffer[offset + 2] = 0;
        }
    }
}

void drawSurface(
    std::uint8_t* framebuffer,
    std::int32_t screen_width,
    std::int32_t screen_height,
    std::int32_t framebuffer_stride,
    osf::gapi::SurfaceView surface) {
    clearFramebuffer(
        framebuffer, screen_width, screen_height, framebuffer_stride);
    if (!framebuffer || !surface.pixels || surface.width <= 0 ||
        surface.height <= 0) {
        return;
    }

    const osf::gapi::Viewport viewport = osf::gapi::fitViewport(
        surface.width,
        surface.height,
        screen_width,
        screen_height);
    for (std::int32_t y = 0; y < viewport.height; ++y) {
        const std::int32_t source_y =
            y * surface.height / viewport.height;
        for (std::int32_t x = 0; x < viewport.width; ++x) {
            const std::int32_t source_x =
                x * surface.width / viewport.width;
            const osf::gapi::Color color = surface.pixels[
                source_y * surface.width + source_x];
            const std::int32_t target_x = viewport.x + x;
            const std::int32_t target_y = viewport.y + y;
            const std::size_t offset = static_cast<std::size_t>(3) *
                static_cast<std::size_t>(
                    target_x * framebuffer_stride + framebuffer_stride -
                    target_y - 1);
            framebuffer[offset] = color.blue;
            framebuffer[offset + 1] = color.green;
            framebuffer[offset + 2] = color.red;
        }
    }
}

class Nintendo3DSSurfacePresenter final
    : public osf::runtime::SurfacePresenter {
public:
    bool initialize(LwlWindow*, std::string* error) override {
        if (error) {
            error->clear();
        }
        return true;
    }

    void present(osf::gapi::SurfaceView surface) override {
        static_assert(
            sizeof(osf::gapi::Color) == 4,
            "GAPI colors must be tightly packed RGBA bytes.");

        u16 width = 0;
        u16 height = 0;
        auto* const bottom_framebuffer = gfxGetFramebuffer(
            GFX_BOTTOM, GFX_LEFT, &width, &height);
        drawSurface(
            bottom_framebuffer,
            static_cast<std::int32_t>(height),
            static_cast<std::int32_t>(width),
            static_cast<std::int32_t>(width),
            surface);
        if (!auxiliary_presented_) {
            auto* const top_framebuffer = gfxGetFramebuffer(
                GFX_TOP, GFX_LEFT, &width, &height);
            clearFramebuffer(
                top_framebuffer,
                static_cast<std::int32_t>(height),
                static_cast<std::int32_t>(width),
                static_cast<std::int32_t>(width));
        }
        gfxFlushBuffers();
        gfxScreenSwapBuffers(GFX_TOP, false);
        gfxScreenSwapBuffers(GFX_BOTTOM, false);
        gspWaitForVBlank();
        auxiliary_presented_ = false;
    }

    void presentAuxiliary(osf::gapi::SurfaceView surface) override {
        u16 width = 0;
        u16 height = 0;
        auto* const top_framebuffer = gfxGetFramebuffer(
            GFX_TOP, GFX_LEFT, &width, &height);
        // libctru exposes the framebuffer in its physical portrait
        // orientation: 240 pixels tall by 400 pixels wide. The display is
        // rotated in memory, so the visible top screen is height by width and
        // each visible column advances by the raw framebuffer width.
        drawSurface(
            top_framebuffer,
            static_cast<std::int32_t>(height),
            static_cast<std::int32_t>(width),
            static_cast<std::int32_t>(width),
            surface);
        auxiliary_presented_ = true;
    }

private:
    bool auxiliary_presented_ = false;
};

}  // namespace

std::unique_ptr<osf::runtime::SurfacePresenter>
osf::runtime::createSurfacePresenter() {
    return std::make_unique<Nintendo3DSSurfacePresenter>();
}
