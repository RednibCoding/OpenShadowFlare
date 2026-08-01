#include "retail_filesystem.hpp"

#include <cctype>
#include <cstdio>
#include <string>

namespace osf {
namespace {

std::filesystem::path nativePath(std::string_view retail_path) {
    std::string path(retail_path);
    for (char& character : path) {
        if (character == '\\') {
            character = '/';
        }
    }
    return std::filesystem::path(path);
}

bool equalsIgnoreCase(
    std::string_view first,
    std::string_view second) {
    if (first.size() != second.size()) {
        return false;
    }
    for (std::size_t index = 0; index < first.size(); ++index) {
        const auto left =
            static_cast<unsigned char>(first[index]);
        const auto right =
            static_cast<unsigned char>(second[index]);
        if (std::tolower(left) != std::tolower(right)) {
            return false;
        }
    }
    return true;
}

}  // namespace

std::filesystem::path resolveRetailPath(
    const std::filesystem::path& root,
    std::string_view retail_path) {
    const std::filesystem::path requested =
        nativePath(retail_path);
    std::filesystem::path resolved = root;
    for (const std::filesystem::path& component : requested) {
        const std::filesystem::path exact = resolved / component;
        std::error_code error;
        if (std::filesystem::exists(exact, error)) {
            resolved = exact;
            continue;
        }

        const std::filesystem::path directory =
            resolved.empty() ? std::filesystem::path(".") : resolved;
        bool matched = false;
        for (std::filesystem::directory_iterator iterator(
                 directory, error);
             !error &&
             iterator != std::filesystem::directory_iterator();
             iterator.increment(error)) {
            if (equalsIgnoreCase(
                    iterator->path().filename().string(),
                    component.string())) {
                resolved /= iterator->path().filename();
                matched = true;
                break;
            }
        }
        if (!matched) {
            return root / requested;
        }
    }
    return resolved;
}

bool retailFileExists(
    const std::filesystem::path& root,
    std::string_view retail_path) {
    std::error_code error;
    return std::filesystem::is_regular_file(
        resolveRetailPath(root, retail_path), error);
}

std::int32_t countRetailSaves(
    const std::filesystem::path& root) {
    std::int32_t count = 0;
    for (std::int32_t index = 0; index < 6; ++index) {
        char path[32]{};
        std::snprintf(
            path,
            sizeof(path),
            "Save\\%04d.Ssv",
            static_cast<int>(index));
        if (retailFileExists(root, path)) {
            ++count;
        }
    }
    return count;
}

bool deleteRetailSave(
    const std::filesystem::path& root,
    std::int32_t logical_index) {
    if (logical_index < 0) {
        return false;
    }
    for (std::int32_t slot = 0; slot < 6; ++slot) {
        char save_path[32]{};
        std::snprintf(
            save_path,
            sizeof(save_path),
            "Save\\%04d.Ssv",
            static_cast<int>(slot));
        if (!retailFileExists(root, save_path)) {
            continue;
        }
        if (logical_index-- != 0) {
            continue;
        }

        std::error_code error;
        std::filesystem::remove(
            resolveRetailPath(root, save_path), error);
        char preview_path[32]{};
        std::snprintf(
            preview_path,
            sizeof(preview_path),
            "Save\\%04d.Bmp",
            static_cast<int>(slot));
        error.clear();
        std::filesystem::remove(
            resolveRetailPath(root, preview_path), error);
        return true;
    }
    return false;
}

}  // namespace osf
