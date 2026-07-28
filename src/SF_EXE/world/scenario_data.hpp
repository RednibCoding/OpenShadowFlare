#ifndef OPENSHADOWFLARE_SCENARIO_DATA_HPP
#define OPENSHADOWFLARE_SCENARIO_DATA_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace osf {

struct ScenarioEntry {
    std::int32_t key = 0;
    std::int32_t world_x = 0;
    std::int32_t world_y = 0;
    std::int32_t direction = 0;
};

class ScenarioData {
public:
    bool load(
        const std::filesystem::path& path,
        std::string* error = nullptr);
    bool decode(
        const std::vector<std::uint8_t>& bytes,
        std::string* error = nullptr);
    void clear();

    const std::string& controllerPath() const;
    const std::string& mapPath() const;
    const std::string& title() const;
    std::int32_t musicTrack() const;
    const std::vector<ScenarioEntry>& entries() const;
    const ScenarioEntry* findEntry(std::int32_t key) const;

private:
    std::string controller_path_;
    std::string map_path_;
    std::string title_;
    std::int32_t music_track_ = -1;
    std::vector<ScenarioEntry> entries_;
};

}  // namespace osf

#endif
