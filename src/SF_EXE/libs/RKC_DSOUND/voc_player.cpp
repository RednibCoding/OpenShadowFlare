#include "libs/RKC_DSOUND/rkc_dsound.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace osf {
namespace {

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

float directSoundVolume(std::int32_t volume) {
    if (volume <= -10000) {
        return 0.0f;
    }
    if (volume >= 0) {
        return 1.0f;
    }
    return std::pow(10.0f, static_cast<float>(volume) / 2000.0f);
}

}  // namespace

VocPlayer::~VocPlayer() {
    clear();
}

bool VocPlayer::load(
    const std::filesystem::path& path,
    std::string* error) {
    clear();

    VocFile file;
    if (!file.load(path, error)) {
        return false;
    }
    if (file.variant_count() < 0 ||
        file.variant_count() > 1024) {
        setError(error, "The VOC variant count is unsupported.");
        return false;
    }

    variant_count_ = file.variant_count();
    sounds_.resize(file.samples().size(), nullptr);
    references_.resize(file.samples().size(), -1);
    voices_.resize(file.samples().size());
    for (std::size_t index = 0;
         index < file.samples().size();
         ++index) {
        const VocSample& sample = file.samples()[index];
        references_[index] = sample.reference_index;
        voices_[index].assign(
            static_cast<std::size_t>(variant_count_) + 1,
            LAL_INVALID_VOICE);
        if (sample.reference_index >= 0) {
            continue;
        }
        if (sample.format.format_tag != 1 ||
            sample.pcm.empty()) {
            setError(error, "The VOC contains unsupported audio data.");
            clear();
            return false;
        }

        const LalPcmFormat format{
            sample.format.sample_rate,
            sample.format.channels,
            sample.format.bits_per_sample,
            sample.format.frame_stride_bytes,
        };
        sounds_[index] = lal_sound_create_pcm(
            sample.pcm.data(), sample.pcm.size(), &format);
        if (!sounds_[index]) {
            setError(error, lal_last_error());
            clear();
            return false;
        }
    }

    if (error) {
        error->clear();
    }
    return true;
}

void VocPlayer::clear() {
    for (const std::vector<LalVoice>& sampleVoices : voices_) {
        for (const LalVoice voice : sampleVoices) {
            if (voice != LAL_INVALID_VOICE) {
                lal_stop(voice);
            }
        }
    }
    for (LalSound* sound : sounds_) {
        lal_sound_destroy(sound);
    }
    sounds_.clear();
    references_.clear();
    voices_.clear();
    variant_count_ = 0;
}

bool VocPlayer::play(
    std::size_t sample_index,
    bool loop,
    std::int32_t direct_sound_volume) {
    if (sample_index >= voices_.size()) {
        return false;
    }
    const LalSound* sound = resolveSound(sample_index);
    if (!sound) {
        return false;
    }

    for (LalVoice& voice : voices_[sample_index]) {
        if (lal_is_playing(voice)) {
            continue;
        }
        LalPlayOptions options = lal_play_options_default();
        options.volume = directSoundVolume(direct_sound_volume);
        options.loop = loop;
        voice = lal_play_ex(sound, &options);
        if (voice != LAL_INVALID_VOICE) {
            return true;
        }
    }
    return false;
}

void VocPlayer::setVolume(
    std::int32_t direct_sound_volume) {
    const float volume =
        directSoundVolume(direct_sound_volume);
    for (const std::vector<LalVoice>& sample_voices :
         voices_) {
        for (const LalVoice voice : sample_voices) {
            if (voice != LAL_INVALID_VOICE &&
                lal_is_playing(voice)) {
                lal_set_voice_volume(voice, volume);
            }
        }
    }
}

bool VocPlayer::isPlaying(std::size_t sample_index) const {
    if (sample_index >= voices_.size()) {
        return false;
    }
    return std::any_of(
        voices_[sample_index].begin(),
        voices_[sample_index].end(),
        [](LalVoice voice) {
            return lal_is_playing(voice);
        });
}

bool VocPlayer::loaded() const {
    return !sounds_.empty();
}

const LalSound* VocPlayer::resolveSound(
    std::size_t sample_index) const {
    if (sample_index >= sounds_.size()) {
        return nullptr;
    }
    for (std::size_t steps = 0;
         steps < sounds_.size();
         ++steps) {
        if (sounds_[sample_index]) {
            return sounds_[sample_index];
        }
        const std::int32_t reference = references_[sample_index];
        if (reference < 0 ||
            static_cast<std::size_t>(reference) >= sounds_.size()) {
            return nullptr;
        }
        sample_index = static_cast<std::size_t>(reference);
    }
    return nullptr;
}

}  // namespace osf
