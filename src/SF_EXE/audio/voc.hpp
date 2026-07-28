#ifndef OPENSHADOWFLARE_VOC_HPP
#define OPENSHADOWFLARE_VOC_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace osf {

struct VocPcmFormat {
    std::uint16_t format_tag = 0;
    std::uint16_t channels = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t average_bytes_per_second = 0;
    std::uint16_t frame_stride_bytes = 0;
    std::uint16_t bits_per_sample = 0;
    std::uint16_t extra_size = 0;
};

struct VocSample {
    std::string name;
    std::int32_t reference_index = -1;
    VocPcmFormat format;
    std::vector<std::uint8_t> pcm;
};

class VocFile {
public:
    bool decode(
        const std::vector<std::uint8_t>& bytes,
        std::string* error = nullptr);
    bool load(
        const std::filesystem::path& path,
        std::string* error = nullptr);

    void clear();
    std::int32_t version() const;
    std::int32_t variant_count() const;
    const std::vector<VocSample>& samples() const;

private:
    std::int32_t version_ = 0;
    std::int32_t variant_count_ = 0;
    std::vector<VocSample> samples_;
};

}  // namespace osf

#endif
