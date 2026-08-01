#include "runtime/presentation/surface_presenter.hpp"

#include "gsKit.h"
#include "dmaKit.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

namespace {

using osf::gapi::SurfaceView;

constexpr std::int32_t kTextureWidth = 640;
constexpr std::int32_t kTextureHeight = 512;

class Ps2SurfacePresenter final
    : public osf::runtime::SurfacePresenter {
public:
    ~Ps2SurfacePresenter() override {
        shutdown();
    }

    bool initialize(
        LwlWindow* window,
        std::string* error) override;
    void present(SurfaceView surface) override;

private:
    void shutdown();
    void uploadSurface(const SurfaceView& surface);

    GSGLOBAL* gsGlobal_ = nullptr;
    GSTEXTURE texture_{};
    std::unique_ptr<std::uint32_t[]> textureMemory_;
    std::int32_t textureWidth_ = 0;
    std::int32_t textureHeight_ = 0;
};

void setError(std::string* error, const char* message) {
    if (error) {
        *error = message;
    }
}

bool Ps2SurfacePresenter::initialize(
    LwlWindow* window,
    std::string* error) {
    (void) window;
    shutdown();

    gsGlobal_ = gsKit_init_global();
    if (!gsGlobal_) {
        setError(error, "Could not initialize gsKit.");
        return false;
    }
    gsGlobal_->Mode = GS_MODE_NTSC;
    gsGlobal_->Interlace = GS_INTERLACED;
    gsGlobal_->Field = GS_FIELD;
    gsGlobal_->Width = 640;
    gsGlobal_->Height = 448;
    gsGlobal_->PSM = GS_PSM_CT32;
    gsGlobal_->DoubleBuffering = GS_SETTING_ON;
    gsGlobal_->ZBuffering = GS_SETTING_OFF;

    dmaKit_init(
        D_CTRL_RELE_OFF,
        D_CTRL_MFD_OFF,
        D_CTRL_STS_UNSPEC,
        D_CTRL_STD_OFF,
        D_CTRL_RCYC_8,
        1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gsGlobal_);
    gsKit_mode_switch(gsGlobal_, GS_PERSISTENT);
    gsKit_set_clamp(gsGlobal_, GS_CMODE_CLAMP);

    textureMemory_ =
        std::make_unique<std::uint32_t[]>(
            static_cast<std::size_t>(kTextureWidth) *
            static_cast<std::size_t>(kTextureHeight));
    texture_.Width = static_cast<u32>(kTextureWidth);
    texture_.Height = static_cast<u32>(kTextureHeight);
    texture_.PSM = GS_PSM_CT32;
    texture_.Filter = GS_FILTER_NEAREST;
    texture_.Mem = reinterpret_cast<u32*>(textureMemory_.get());
    texture_.Vram = gsKit_vram_alloc(
        gsGlobal_,
        static_cast<u32>(kTextureWidth) *
            static_cast<u32>(kTextureHeight) * 4u,
        GSKIT_ALLOC_USERBUFFER);
    if (texture_.Vram == 0) {
        setError(error, "Could not allocate texture VRAM.");
        shutdown();
        return false;
    }
    textureWidth_ = kTextureWidth;
    textureHeight_ = kTextureHeight;
    return true;
}

void Ps2SurfacePresenter::shutdown() {
    textureWidth_ = 0;
    textureHeight_ = 0;
    textureMemory_.reset();
    gsGlobal_ = nullptr;
}

void Ps2SurfacePresenter::uploadSurface(const SurfaceView& surface) {
    if (surface.width > textureWidth_ ||
        surface.height > textureHeight_) {
        return;
    }
    const std::uint32_t* source =
        reinterpret_cast<const std::uint32_t*>(surface.pixels);
    std::uint32_t* destination = textureMemory_.get();
    for (std::int32_t row = 0; row < surface.height; ++row) {
        std::memcpy(
            destination,
            source,
            static_cast<std::size_t>(surface.width) * 4u);
        destination += textureWidth_;
        source += surface.width;
    }
    gsKit_texture_upload(gsGlobal_, &texture_);
}

void Ps2SurfacePresenter::present(SurfaceView surface) {
    if (!gsGlobal_ || !textureMemory_ || !surface.pixels ||
        surface.width <= 0 || surface.height <= 0) {
        return;
    }

    uploadSurface(surface);

    const float scale = std::min(
        static_cast<float>(gsGlobal_->Width) /
            static_cast<float>(surface.width),
        static_cast<float>(gsGlobal_->Height) /
            static_cast<float>(surface.height));
    const std::int32_t draw_width =
        static_cast<std::int32_t>(surface.width * scale);
    const std::int32_t draw_height =
        static_cast<std::int32_t>(surface.height * scale);
    const std::int32_t offset_x =
        (static_cast<std::int32_t>(gsGlobal_->Width) - draw_width) / 2;
    const std::int32_t offset_y =
        (static_cast<std::int32_t>(gsGlobal_->Height) - draw_height) / 2;
    gsKit_clear(
        gsGlobal_, GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x00, 0x00));
    gsKit_prim_sprite_striped_texture(
        gsGlobal_,
        &texture_,
        static_cast<float>(offset_x),
        static_cast<float>(offset_y),
        0.0f,
        0.0f,
        static_cast<float>(offset_x + draw_width),
        static_cast<float>(offset_y + draw_height),
        static_cast<float>(surface.width),
        static_cast<float>(surface.height),
        0,
        GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
    gsKit_queue_exec(gsGlobal_);
    gsKit_sync_flip(gsGlobal_);
}

}  // namespace

std::unique_ptr<osf::runtime::SurfacePresenter>
osf::runtime::createSurfacePresenter() {
    return std::make_unique<Ps2SurfacePresenter>();
}
