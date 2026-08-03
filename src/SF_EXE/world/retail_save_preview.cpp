#include "retail_save_preview.hpp"

#include "libs/RKC_DIB/rkc_dib.hpp"

#include <algorithm>
#include <cstddef>

namespace osf {

void RetailSavePreview::requestCapture() {
    capture_requested_ = true;
}

bool RetailSavePreview::captureRequested() const {
    return capture_requested_;
}

void RetailSavePreview::captureIfRequested(
    gapi::SurfaceView world_surface) {
    if (!capture_requested_) {
        return;
    }
    capture(world_surface);
}

void RetailSavePreview::capture(
    gapi::SurfaceView world_surface) {
    capture_requested_ = false;
    if (!world_surface.pixels ||
        world_surface.width < width ||
        world_surface.height < height) {
        clear();
        return;
    }

    const std::int32_t source_x =
        (world_surface.width - width) / 2;
    const std::int32_t source_y =
        (world_surface.height - height) / 2;
    pixels_.resize(
        static_cast<std::size_t>(width) *
        static_cast<std::size_t>(height));
    for (std::int32_t y = 0; y < height; ++y) {
        const gapi::Color* source =
            world_surface.pixels +
            static_cast<std::size_t>(source_y + y) *
                static_cast<std::size_t>(world_surface.width) +
            static_cast<std::size_t>(source_x);
        gapi::Color* destination =
            pixels_.data() +
            static_cast<std::size_t>(y) *
                static_cast<std::size_t>(width);
        std::copy(source, source + width, destination);
    }
}

void RetailSavePreview::clear() {
    capture_requested_ = false;
    pixels_.clear();
}

bool RetailSavePreview::valid() const {
    return pixels_.size() ==
        static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height);
}

bool RetailSavePreview::writeForSave(
    const std::filesystem::path& save_path,
    std::string* error) const {
    if (!valid()) {
        if (error) {
            *error =
                "No world frame is available for the save preview.";
        }
        return false;
    }
    std::filesystem::path preview_path = save_path;
    preview_path.replace_extension(".Bmp");
    return gapi::writeBitmapFile(
        preview_path,
        {pixels_.data(), width, height},
        error);
}

}  // namespace osf
