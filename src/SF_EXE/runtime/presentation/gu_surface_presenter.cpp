// PSP surface presenter. The PSP has no OpenGL ES, so instead of the LGL blit
// this backend uploads GAPI's software 640x480 RGBA surface as sceGu textures
// and draws it, letterboxed, onto the 480x272 screen.
//
// Two seams matter here:
//   * PSP textures are capped at 512x512, so the 640-wide surface is split into
//     column tiles (512 + 128) and drawn as two sprites.
//   * The GE reads texture memory from RAM; CPU writes to the staging tiles must
//     be flushed out of the data cache before each draw.

#include "runtime/presentation/surface_presenter.hpp"

#include "gapi/gapi.hpp"
#include "lwl.h"

#include <pspdisplay.h>
#include <pspge.h>
#include <pspgu.h>
#include <pspkernel.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace osf::runtime {
namespace {

constexpr int kScreenWidth = 480;
constexpr int kScreenHeight = 272;
constexpr int kBufferWidth = 512;
constexpr int kMaxTextureSize = 512;
constexpr int kMaxTiles = 4;

static_assert(
    sizeof(osf::gapi::Color) == 4,
    "GAPI colors must be tightly packed RGBA bytes so they can be copied "
    "straight into a GU_PSM_8888 texture.");

// The presentation pass issues only a clear and up to four sprites.  The
// previous 1 MiB command list was unnecessarily expensive on PSP memory.
unsigned int g_display_list[16 * 1024] __attribute__((aligned(16)));

struct Vertex {
    float u;
    float v;
    float x;
    float y;
    float z;
};

unsigned int nextPowerOfTwo(unsigned int value) {
    unsigned int result = 1;
    while (result < value) {
        result <<= 1;
    }
    return result;
}

unsigned int vramBytesPerPixel(unsigned int pixel_format) {
    switch (pixel_format) {
    case GU_PSM_5650:
    case GU_PSM_5551:
    case GU_PSM_4444:
        return 2;
    case GU_PSM_8888:
    default:
        return 4;
    }
}

unsigned int g_vram_offset = 0;

void* allocateVramBuffer(
    unsigned int width, unsigned int height, unsigned int pixel_format) {
    const unsigned int size =
        width * height * vramBytesPerPixel(pixel_format);
    void* const result = reinterpret_cast<void*>(
        static_cast<std::uintptr_t>(g_vram_offset));
    g_vram_offset += size;
    return result;
}

void applyBlitState() {
    sceGuOffset(2048 - (kScreenWidth / 2), 2048 - (kScreenHeight / 2));
    sceGuViewport(2048, 2048, kScreenWidth, kScreenHeight);
    sceGuScissor(0, 0, kScreenWidth, kScreenHeight);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);
    sceGuDisable(GU_BLEND);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_8888, 0, 0, 0);
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    sceGuTexScale(1.0f, 1.0f);
    sceGuTexOffset(0.0f, 0.0f);
}

struct SurfaceTile {
    int source_x = 0;
    int used_width = 0;
    int texture_width = 0;
    std::uint32_t* pixels = nullptr;
    std::size_t byte_count = 0;
};

class GuSurfacePresenter final : public SurfacePresenter {
public:
    ~GuSurfacePresenter() override {
        shutdown();
    }

    bool initialize(LwlWindow* window, std::string* error) override;
    void prepareFrame(osf::gapi::SurfaceView surface) override;
    void displayFrame() override;
    void reset() override;

private:
    void shutdown();
    void releaseTiles();
    bool ensureTiles(int surface_width, int surface_height, std::string* error);

    bool initialized_ = false;
    void* draw_buffer_ = nullptr;
    void* display_buffer_ = nullptr;
    void* depth_buffer_ = nullptr;
    int surface_width_ = 0;
    int surface_height_ = 0;
    int texture_height_ = 0;
    int tile_count_ = 0;
    bool frame_prepared_ = false;
    SurfaceTile tiles_[kMaxTiles];
};

bool GuSurfacePresenter::initialize(
    LwlWindow* window, std::string* error) {
    (void) window;
    shutdown();

    g_vram_offset = 0;
    draw_buffer_ =
        allocateVramBuffer(kBufferWidth, kScreenHeight, GU_PSM_8888);
    display_buffer_ =
        allocateVramBuffer(kBufferWidth, kScreenHeight, GU_PSM_8888);
    depth_buffer_ =
        allocateVramBuffer(kBufferWidth, kScreenHeight, GU_PSM_4444);

    sceGuInit();
    sceGuStart(GU_DIRECT, g_display_list);
    sceGuDrawBuffer(GU_PSM_8888, draw_buffer_, kBufferWidth);
    sceGuDispBuffer(
        kScreenWidth, kScreenHeight, display_buffer_, kBufferWidth);
    sceGuDepthBuffer(depth_buffer_, kBufferWidth);
    applyBlitState();
    sceGuClearColor(0xff000000);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    initialized_ = true;
    if (error) {
        error->clear();
    }
    return true;
}

void GuSurfacePresenter::shutdown() {
    if (initialized_) {
        sceGuTerm();
        initialized_ = false;
    }
    releaseTiles();
    surface_width_ = 0;
    surface_height_ = 0;
    texture_height_ = 0;
    frame_prepared_ = false;
}

void GuSurfacePresenter::reset() {
    frame_prepared_ = false;
    if (!initialized_) {
        return;
    }
    sceGuStart(GU_DIRECT, g_display_list);
    sceGuDrawBuffer(GU_PSM_8888, draw_buffer_, kBufferWidth);
    sceGuDispBuffer(
        kScreenWidth, kScreenHeight, display_buffer_, kBufferWidth);
    sceGuDepthBuffer(depth_buffer_, kBufferWidth);
    applyBlitState();
    sceGuClearColor(0xff000000);
    sceGuClear(GU_COLOR_BUFFER_BIT);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

void GuSurfacePresenter::releaseTiles() {
    for (int index = 0; index < tile_count_; ++index) {
        std::free(tiles_[index].pixels);
        tiles_[index] = SurfaceTile{};
    }
    tile_count_ = 0;
}

bool GuSurfacePresenter::ensureTiles(
    int surface_width, int surface_height, std::string* error) {
    if (surface_width == surface_width_ &&
        surface_height == surface_height_ &&
        tile_count_ > 0) {
        return true;
    }

    releaseTiles();
    surface_width_ = 0;
    surface_height_ = 0;

    texture_height_ =
        static_cast<int>(nextPowerOfTwo(
            static_cast<unsigned int>(surface_height)));

    int column = 0;
    while (column < surface_width && tile_count_ < kMaxTiles) {
        const int used_width =
            (surface_width - column) < kMaxTextureSize
                ? (surface_width - column)
                : kMaxTextureSize;
        const int texture_width =
            static_cast<int>(nextPowerOfTwo(
                static_cast<unsigned int>(used_width)));
        const std::size_t byte_count =
            static_cast<std::size_t>(texture_width) *
            static_cast<std::size_t>(texture_height_) *
            sizeof(std::uint32_t);
        auto* const pixels =
            static_cast<std::uint32_t*>(std::malloc(byte_count));
        if (!pixels) {
            releaseTiles();
            if (error) {
                *error = "Could not allocate a PSP surface tile.";
            }
            return false;
        }
        std::memset(pixels, 0, byte_count);

        tiles_[tile_count_].source_x = column;
        tiles_[tile_count_].used_width = used_width;
        tiles_[tile_count_].texture_width = texture_width;
        tiles_[tile_count_].pixels = pixels;
        tiles_[tile_count_].byte_count = byte_count;
        ++tile_count_;
        column += used_width;
    }

    if (column < surface_width) {
        releaseTiles();
        if (error) {
            *error = "PSP surface is wider than the tiler supports.";
        }
        return false;
    }

    surface_width_ = surface_width;
    surface_height_ = surface_height;
    return true;
}

void GuSurfacePresenter::prepareFrame(osf::gapi::SurfaceView surface) {
    frame_prepared_ = false;
    if (!initialized_ || !surface.pixels ||
        surface.width <= 0 || surface.height <= 0) {
        return;
    }
    if (!ensureTiles(surface.width, surface.height, nullptr)) {
        return;
    }

    const auto* const source =
        reinterpret_cast<const std::uint32_t*>(surface.pixels);
    const int source_stride = surface.width;

    for (int index = 0; index < tile_count_; ++index) {
        SurfaceTile& tile = tiles_[index];
        for (int row = 0; row < surface.height; ++row) {
            std::memcpy(
                tile.pixels +
                    static_cast<std::size_t>(row) * tile.texture_width,
                source +
                    static_cast<std::size_t>(row) * source_stride +
                    tile.source_x,
                static_cast<std::size_t>(tile.used_width) *
                    sizeof(std::uint32_t));
        }
        sceKernelDcacheWritebackRange(tile.pixels, tile.byte_count);
    }

    frame_prepared_ = true;
}

void GuSurfacePresenter::displayFrame() {
    if (!initialized_ || !frame_prepared_) {
        return;
    }

    const osf::gapi::Viewport fit =
        osf::gapi::fitViewport(
            surface_width_, surface_height_, kScreenWidth, kScreenHeight);
    const float v_max = static_cast<float>(surface_height_);

    sceGuStart(GU_DIRECT, g_display_list);
    applyBlitState();
    sceGuClearColor(0xff000000);
    sceGuClear(GU_COLOR_BUFFER_BIT);

    for (int index = 0; index < tile_count_; ++index) {
        const SurfaceTile& tile = tiles_[index];
        const float left =
            static_cast<float>(fit.x) +
                static_cast<float>(fit.width) *
                static_cast<float>(tile.source_x) /
                static_cast<float>(surface_width_);
        const float right =
            static_cast<float>(fit.x) +
                static_cast<float>(fit.width) *
                static_cast<float>(tile.source_x + tile.used_width) /
                static_cast<float>(surface_width_);
        const float u_max = static_cast<float>(tile.used_width);

        sceGuTexImage(
            0,
            tile.texture_width,
            texture_height_,
            tile.texture_width,
            tile.pixels);

        auto* const vertices =
            static_cast<Vertex*>(sceGuGetMemory(2 * sizeof(Vertex)));
        vertices[0] = {0.0f, 0.0f, left, static_cast<float>(fit.y), 0.0f};
        vertices[1] = {
            u_max,
            v_max,
            right,
            static_cast<float>(fit.y + fit.height),
            0.0f};
        sceGuDrawArray(
            GU_SPRITES,
            GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
            2,
            nullptr,
            vertices);
    }

    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
    frame_prepared_ = false;
}

}  // namespace

std::unique_ptr<SurfacePresenter> createSurfacePresenter() {
    return std::make_unique<GuSurfacePresenter>();
}

}  // namespace osf::runtime
