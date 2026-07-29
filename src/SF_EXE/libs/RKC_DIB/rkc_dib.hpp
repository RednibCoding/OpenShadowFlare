#ifndef OPENSHADOWFLARE_LIBS_RKC_DIB_HPP
#define OPENSHADOWFLARE_LIBS_RKC_DIB_HPP

#include "gapi/gapi.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace osf::gapi {

class BitmapImage {
public:
    bool decode(
        const std::vector<std::uint8_t>& bytes,
        std::string* error = nullptr);
    bool load(
        const std::filesystem::path& path,
        std::string* error = nullptr);

    void clear();
    std::int32_t width() const;
    std::int32_t height() const;
    const std::vector<Color>& pixels() const;

private:
    std::int32_t width_ = 0;
    std::int32_t height_ = 0;
    std::vector<Color> pixels_;
};

bool writeBitmapFile(
    const std::filesystem::path& path,
    SurfaceView surface,
    std::string* error = nullptr);

}  // namespace osf::gapi

#endif
