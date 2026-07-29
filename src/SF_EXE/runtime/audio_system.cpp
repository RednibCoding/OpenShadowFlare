#include "audio_system.hpp"

#include "lal.h"
#include "resources/retail_filesystem.hpp"
#include "states/character_select_state.hpp"
#include "states/title_state.hpp"

#include <cstdio>

namespace osf::runtime {
namespace {

constexpr std::size_t kMenuConfirmSound = 55;
constexpr std::size_t kTitleConfirmSound = 56;
constexpr std::size_t kMenuMoveSound = 58;
constexpr std::size_t kTitleCueSound = 62;

}  // namespace

AudioSystem::~AudioSystem() {
    shutdown();
}

bool AudioSystem::initialize(
    const std::filesystem::path& data_root,
    std::int32_t effect_volume,
    std::int32_t bgm_volume,
    std::string* error) {
    shutdown();
    if (!lal_init()) {
        if (error) {
            *error = lal_last_error();
        }
        return false;
    }

    initialized_ = true;
    data_root_ = data_root;
    effect_volume_ = effect_volume;
    bgm_volume_ = bgm_volume;
    loadVoc(effect_audio_, "System\\Game\\Voice\\Voice00.Voc");
    return true;
}

void AudioSystem::shutdown() {
    if (!initialized_) {
        return;
    }
    effect_audio_.clear();
    menu_music_.clear();
    world_music_.clear();
    lal_shutdown();
    initialized_ = false;
}

void AudioSystem::playTitleFrame(const TitleFrameResult& frame) {
    if (frame.play_title_sound) {
        effect_audio_.play(
            kTitleCueSound, false, effect_volume_);
    }
    playRepeatedEffect(
        kMenuMoveSound, frame.play_move_sound_count);
    if (frame.play_confirm_sound) {
        effect_audio_.play(
            kTitleConfirmSound, false, effect_volume_);
    }
    if (frame.start_menu_music) {
        playMenuMusic(true);
    }
}

void AudioSystem::playCharacterSelectFrame(
    const CharacterSelectFrameResult& frame) {
    playRepeatedEffect(
        kMenuMoveSound, frame.play_move_sound_count);
    playRepeatedEffect(
        kMenuConfirmSound,
        frame.play_selection_sound_count);
}

bool AudioSystem::loadMenuMusic(std::string_view retail_path) {
    return loadVoc(menu_music_, retail_path);
}

bool AudioSystem::menuMusicIsPlaying() const {
    return initialized_ && menu_music_.isPlaying(0);
}

void AudioSystem::playMenuMusic(bool loop) {
    if (initialized_) {
        menu_music_.play(0, loop, bgm_volume_);
    }
}

void AudioSystem::releaseMenuMusic() {
    menu_music_.clear();
}

void AudioSystem::startWorldMusic(std::int32_t track) {
    if (!initialized_ || track < 0 || track > 99) {
        return;
    }

    char path[48]{};
    std::snprintf(
        path,
        sizeof(path),
        "System\\Game\\Music\\BGM%02d.Voc",
        track);
    if (loadVoc(world_music_, path)) {
        world_music_.play(0, true, bgm_volume_);
    }
}

void AudioSystem::stopWorldMusic() {
    world_music_.clear();
}

void AudioSystem::setEffectVolume(std::int32_t volume) {
    effect_volume_ = volume;
    effect_audio_.setVolume(effect_volume_);
}

void AudioSystem::setBgmVolume(std::int32_t volume) {
    bgm_volume_ = volume;
    menu_music_.setVolume(bgm_volume_);
    world_music_.setVolume(bgm_volume_);
}

void AudioSystem::playOptionsClick() {
    if (initialized_) {
        effect_audio_.play(
            kMenuConfirmSound, false, effect_volume_);
    }
}

bool AudioSystem::loadVoc(
    VocPlayer& player,
    std::string_view retail_path) {
    if (!initialized_) {
        return false;
    }

    std::string error;
    const std::filesystem::path path =
        resolveRetailPath(data_root_, retail_path);
    if (!player.load(path, &error)) {
        std::fprintf(
            stderr,
            "Could not load %s: %s\n",
            path.string().c_str(),
            error.c_str());
        return false;
    }
    return true;
}

void AudioSystem::playRepeatedEffect(
    std::size_t sample,
    std::int32_t count) {
    for (std::int32_t index = 0; index < count; ++index) {
        effect_audio_.play(
            sample, false, effect_volume_);
    }
}

}  // namespace osf::runtime
