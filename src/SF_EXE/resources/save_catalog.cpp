#include "save_catalog.hpp"

#include <array>
#include <cstdio>
#include <fstream>
#include <utility>

namespace osf {
namespace {

std::int32_t readI32(
    const std::array<std::uint8_t, 0x160>& bytes,
    std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        return 0;
    }
    const std::uint32_t value =
        static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
        (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) |
        (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u);
    return static_cast<std::int32_t>(value);
}

std::string readName(
    const std::array<std::uint8_t, 0x160>& bytes) {
    std::size_t length = 0;
    while (length < 16 && bytes[length] != 0) {
        ++length;
    }
    return std::string(
        reinterpret_cast<const char*>(bytes.data()),
        length);
}

}  // namespace

std::vector<RetailSaveSummary> loadRetailSaveCatalog(
    const std::filesystem::path& game_root) {
    std::vector<RetailSaveSummary> catalog;
    for (std::int32_t slot = 0; slot < 6; ++slot) {
        char filename[16]{};
        std::snprintf(
            filename,
            sizeof(filename),
            "%04d.Ssv",
            static_cast<int>(slot));
        const std::filesystem::path savePath =
            game_root / "Save" / filename;
        std::ifstream stream(savePath, std::ios::binary);
        if (!stream) {
            continue;
        }

        std::array<char, 16> signature{};
        std::array<std::uint8_t, 0x160> data{};
        if (!stream.read(signature.data(), signature.size()) ||
            !stream.read(
                reinterpret_cast<char*>(data.data()),
                data.size())) {
            continue;
        }

        std::snprintf(
            filename,
            sizeof(filename),
            "%04d.Bmp",
            static_cast<int>(slot));
        RetailSaveSummary summary;
        summary.slot = slot;
        summary.save_path = savePath;
        summary.preview_path =
            game_root / "Save" / filename;
        summary.name = readName(data);
        summary.gender = readI32(data, 0x18);
        summary.job = readI32(data, 0x1c);
        summary.level = readI32(data, 0x24);
        catalog.push_back(std::move(summary));
    }
    return catalog;
}

}  // namespace osf
