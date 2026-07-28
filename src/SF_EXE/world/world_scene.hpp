#ifndef OPENSHADOWFLARE_WORLD_SCENE_HPP
#define OPENSHADOWFLARE_WORLD_SCENE_HPP

#include "gapi/caf.hpp"
#include "gapi/njp.hpp"
#include "ground_map.hpp"

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
    const std::vector<std::unique_ptr<gapi::NjpImage>>&
        groundPatterns() const;
    const gapi::NjpImage& playerPatterns() const;
    const gapi::CafAnimation& playerAnimation() const;
    bool hasPlayer() const;
    std::int32_t playerWorldX() const;
    std::int32_t playerWorldY() const;

private:
    GroundMap ground_;
    std::vector<std::unique_ptr<gapi::NjpImage>> ground_patterns_;
    gapi::NjpImage player_patterns_;
    gapi::CafAnimation player_animation_;
    bool has_player_ = false;
    std::int32_t player_world_x_ = 0;
    std::int32_t player_world_y_ = 0;
};

}  // namespace osf

#endif
