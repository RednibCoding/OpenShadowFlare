#ifndef OPENSHADOWFLARE_RETAIL_SAVE_PREVIEW_HPP
#define OPENSHADOWFLARE_RETAIL_SAVE_PREVIEW_HPP

#include "gapi/gapi.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace osf {

class RetailSavePreview {
public:
    static constexpr std::int32_t width = 0x187;
    static constexpr std::int32_t height = 0x72;

    void capture(gapi::SurfaceView world_surface);
    void clear();
    bool valid() const;
    bool writeForSave(
        const std::filesystem::path& save_path,
        std::string* error = nullptr) const;

private:
    std::vector<gapi::Color> pixels_;
};

}  // namespace osf

#endif
