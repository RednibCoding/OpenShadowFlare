#ifndef OPENSHADOWFLARE_WORLD_SCENE_HPP
#define OPENSHADOWFLARE_WORLD_SCENE_HPP

#include "gapi/caf.hpp"
#include "gapi/njp.hpp"
#include "ground_map.hpp"
#include "object_map.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace osf {

class WorldScene {
public:
    bool loadInitialScenario(
        const std::filesystem::path& data_root,
        std::int32_t character_gender,
        std::string* error = nullptr);
    void clear();

    const GroundMap& ground() const;
    const ObjectMap& objectMap() const;
    const std::vector<std::unique_ptr<gapi::NjpImage>>&
        mapPatterns() const;
    const gapi::NjpImage& playerPatterns() const;
    const gapi::NjpImage& playerShadowPatterns() const;
    const gapi::CafAnimation& playerAnimation() const;
    bool playerPartEnabled(std::size_t part) const;
    bool hasPlayer() const;
    std::int32_t playerWorldX() const;
    std::int32_t playerWorldY() const;
    std::int32_t playerDirection() const;
    std::int32_t musicTrack() const;

private:
    GroundMap ground_;
    ObjectMap object_map_;
    std::vector<std::unique_ptr<gapi::NjpImage>> map_patterns_;
    gapi::NjpImage player_patterns_;
    gapi::NjpImage player_shadow_patterns_;
    gapi::CafAnimation player_animation_;
    std::vector<std::uint8_t> player_parts_enabled_;
    bool has_player_ = false;
    std::int32_t player_world_x_ = 0;
    std::int32_t player_world_y_ = 0;
    std::int32_t player_direction_ = 0;
    std::int32_t music_track_ = -1;
};

}  // namespace osf

#endif
