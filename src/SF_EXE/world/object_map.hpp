#ifndef OPENSHADOWFLARE_OBJECT_MAP_HPP
#define OPENSHADOWFLARE_OBJECT_MAP_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace osf {

struct ObjectBounds {
    std::int32_t left = 0;
    std::int32_t top = 0;
    std::int32_t right = 0;
    std::int32_t bottom = 0;
};

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

private:
    std::int32_t version_ = 0;
    std::vector<MapObject> objects_;
};

}  // namespace osf

#endif
