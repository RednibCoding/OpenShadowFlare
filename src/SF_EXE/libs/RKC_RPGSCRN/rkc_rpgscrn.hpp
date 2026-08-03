#ifndef OPENSHADOWFLARE_LIBS_RKC_RPGSCRN_HPP
#define OPENSHADOWFLARE_LIBS_RKC_RPGSCRN_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace osf::gapi {

class NjpImage;

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

namespace osf {

struct WorldPosition {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct ScreenPosition {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

ScreenPosition calculateRealPosition(
    WorldPosition position,
    std::int32_t base_x = 15,
    std::int32_t base_y = 10);
WorldPosition calculateWorldPosition(
    ScreenPosition position,
    std::int32_t base_x = 15,
    std::int32_t base_y = 10);

struct GroundCell {
    std::int16_t status = 0;
    std::int16_t pattern_set = -1;
    std::int16_t pattern = -1;
};

class GroundMap {
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
    std::int32_t chipWidth() const;
    std::int32_t chipHeight() const;
    std::int32_t baseMagnificationX() const;
    std::int32_t baseMagnificationY() const;
    std::int32_t judgeWidth() const;
    std::int32_t judgeHeight() const;
    std::int32_t judgeOffsetX() const;
    std::int32_t judgeOffsetY() const;
    const GroundCell* cell(
        std::int32_t x,
        std::int32_t y) const;
    const std::int16_t* judge(
        std::int32_t x,
        std::int32_t y) const;
    std::uint64_t memoryUsageBytes() const;

private:
    std::int32_t width_ = 0;
    std::int32_t height_ = 0;
    std::int32_t chip_width_ = 0;
    std::int32_t chip_height_ = 0;
    std::int32_t base_magnification_x_ = 0;
    std::int32_t base_magnification_y_ = 0;
    std::int32_t judge_width_ = 0;
    std::int32_t judge_height_ = 0;
    std::int32_t judge_offset_x_ = 0;
    std::int32_t judge_offset_y_ = 0;
    std::vector<GroundCell> cells_;
    std::vector<std::int16_t> judgement_;
};

struct ObjectBounds {
    std::int32_t left = 0;
    std::int32_t top = 0;
    std::int32_t right = 0;
    std::int32_t bottom = 0;
};

struct DisplayOrderEntry {
    std::size_t source_index = 0;
    WorldPosition position;
    ObjectBounds judgement;
    std::int16_t status = 0;
};

std::int32_t displayClassForStatus(std::int16_t status);
void sortDisplayObjects(
    std::vector<DisplayOrderEntry>& entries);

struct MapObject {
    std::int32_t world_x = 0;
    std::int32_t world_y = 0;
    std::int16_t pattern_set = -1;
    std::int16_t pattern = -1;
    std::int16_t palette = -1;
    std::int16_t opacity = 1000;
    std::int16_t status = 0;
    std::int16_t height = 0;
    std::int16_t red_strength = 1000;
    std::int16_t green_strength = 1000;
    std::int16_t blue_strength = 1000;
    ObjectBounds judgement;
};

class ObjectMap {
public:
    bool decode(
        const std::vector<std::uint8_t>& bytes,
        std::string* error = nullptr);
    bool load(
        const std::filesystem::path& path,
        std::string* error = nullptr);
    void clear();

    std::int32_t version() const;
    const std::vector<MapObject>& objects() const;
    std::uint64_t memoryUsageBytes() const;

private:
    std::int32_t version_ = 0;
    std::vector<MapObject> objects_;
};

bool positionIsWalkable(
    const GroundMap& ground,
    const ObjectMap& objects,
    WorldPosition position,
    const ObjectBounds& bounds,
    bool exclude_special_objects = false);

using DisplayPartEnabled =
    std::function<bool(std::size_t)>;

struct DisplayHitRectangle {
    std::int32_t left = 0;
    std::int32_t top = 0;
    std::int32_t right = 0;
    std::int32_t bottom = 0;
};

bool displayPatternIntersectsRectangle(
    const gapi::NjpImage& image,
    std::size_t pattern_index,
    ScreenPosition anchor,
    DisplayHitRectangle rectangle,
    std::int32_t height = 0);

bool displayPatternContainsPoint(
    const gapi::NjpImage& image,
    std::size_t pattern_index,
    ScreenPosition anchor,
    ScreenPosition point,
    std::int32_t height = 0);

bool displayAnimationIntersectsRectangle(
    const gapi::CafAnimation& animation,
    const gapi::NjpImage& patterns,
    WorldPosition position,
    std::int32_t chart_index,
    std::int32_t direction_index,
    std::int32_t animation_frame,
    const DisplayPartEnabled& part_enabled,
    std::int32_t camera_x,
    std::int32_t camera_y,
    DisplayHitRectangle rectangle,
    std::int32_t height = 0);

bool displayAnimationContainsPoint(
    const gapi::CafAnimation& animation,
    const gapi::NjpImage& patterns,
    WorldPosition position,
    std::int32_t chart_index,
    std::int32_t direction_index,
    std::int32_t animation_frame,
    const DisplayPartEnabled& part_enabled,
    std::int32_t camera_x,
    std::int32_t camera_y,
    ScreenPosition point,
    std::int32_t height = 0);

}  // namespace osf

#endif
