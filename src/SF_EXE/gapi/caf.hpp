#ifndef OPENSHADOWFLARE_CAF_HPP
#define OPENSHADOWFLARE_CAF_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace osf::gapi {

struct CafCell {
    std::int16_t status = 0;
    std::int16_t priority = 0;
    std::int16_t transparency = 0;
    std::int32_t pattern_index = 0;
};

struct CafDirection {
    std::int16_t frame_count = 0;
    std::vector<std::vector<CafCell>> parts;
};

struct CafChart {
    std::int16_t status = 0;
    std::array<CafDirection, 9> directions;
};

class CafAnimation {
public:
    bool decode(
        const std::vector<std::uint8_t>& bytes,
        std::string* error = nullptr);
    bool load(
        const std::filesystem::path& path,
        std::string* error = nullptr);

    void clear();
    std::int32_t version() const;
    std::int32_t palette_mode() const;
    std::int32_t chart_priority_stride() const;
    std::size_t maxPartCount() const;
    const std::vector<CafChart>& charts() const;

private:
    std::int32_t version_ = 0;
    std::int32_t palette_mode_ = 0;
    std::int32_t chart_priority_stride_ = 0;
    std::vector<CafChart> charts_;
};

}  // namespace osf::gapi

#endif
