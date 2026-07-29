#pragma once

#include "libs/RKC_DSOUND/rkc_dsound.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace osf {

struct CharacterSelectFrameResult;
struct TitleFrameResult;

namespace runtime {

class AudioSystem {
public:
    AudioSystem() = default;
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    bool initialize(
        const std::filesystem::path& data_root,
        std::int32_t effect_volume,
        std::int32_t bgm_volume,
        std::string* error = nullptr);
    void shutdown();

    void playTitleFrame(const TitleFrameResult& frame);
    void playCharacterSelectFrame(
        const CharacterSelectFrameResult& frame);

    bool loadMenuMusic(std::string_view retail_path);
    bool menuMusicIsPlaying() const;
    void playMenuMusic(bool loop);
    void releaseMenuMusic();

    void startWorldMusic(std::int32_t track);
    void stopWorldMusic();

private:
    bool loadVoc(VocPlayer& player, std::string_view retail_path);
    void playRepeatedEffect(
        std::size_t sample,
        std::int32_t count);

    bool initialized_ = false;
    std::int32_t effect_volume_ = 0;
    std::int32_t bgm_volume_ = 0;
    std::filesystem::path data_root_;
    VocPlayer effect_audio_;
    VocPlayer menu_music_;
    VocPlayer world_music_;
};

}  // namespace runtime
}  // namespace osf
