#ifndef OPENSHADOWFLARE_VOC_PLAYER_HPP
#define OPENSHADOWFLARE_VOC_PLAYER_HPP

#include "lal.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct LalSound;

namespace osf {

class VocPlayer {
public:
    VocPlayer() = default;
    ~VocPlayer();

    VocPlayer(const VocPlayer&) = delete;
    VocPlayer& operator=(const VocPlayer&) = delete;

    bool load(
        const std::filesystem::path& path,
        std::string* error = nullptr);
    void clear();

    bool play(
        std::size_t sample_index,
        bool loop,
        std::int32_t direct_sound_volume);
    bool isPlaying(std::size_t sample_index) const;
    bool loaded() const;

private:
    const LalSound* resolveSound(std::size_t sample_index) const;

    std::vector<LalSound*> sounds_;
    std::vector<std::int32_t> references_;
    std::vector<std::vector<LalVoice>> voices_;
    std::int32_t variant_count_ = 0;
};

}  // namespace osf

#endif
