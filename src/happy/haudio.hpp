/*
 * haudio.hpp - Happy Audio - Simple Cross-Platform Audio Library
 * Part of the Happy Library (hwl, h2d, haudio)
 * by Rednib Coding (Michael Binder)
 * 
 * Minimal cross-platform audio playback and mixing
 * Supports: Windows (waveOut), Linux (ALSA)
 * Modern C++17, memory-safe
 * 
 * Usage:
 *   #define HAUDIO_IMPLEMENTATION in ONE .cpp file before including
 *   Link: -lasound (Linux), -lwinmm (Windows)
 * 
 * Features:
 *   - WAV file loading (8/16 bit, mono/stereo)
 *   - Multiple simultaneous sounds (software mixing)
 *   - Volume control per sound and master
 *   - Looping support
 */

#ifndef HAUDIO_HPP
#define HAUDIO_HPP

#include <array>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <atomic>
#include <mutex>

/*==============================================================================
 * Platform Detection
 *============================================================================*/
#if defined(_WIN32) || defined(_WIN64)
    #define HAUDIO_WINDOWS
#elif defined(__linux__)
    #define HAUDIO_LINUX
#else
    #error "Unsupported platform (only Windows and Linux supported)"
#endif

namespace haudio {

/*==============================================================================
 * Audio Format
 *============================================================================*/

struct AudioFormat {
    int sampleRate = 44100;     // Samples per second (22050, 44100, 48000)
    int channels = 2;           // 1 = mono, 2 = stereo
    int bitsPerSample = 16;     // 8 or 16
    
    int bytesPerSample() const { return bitsPerSample / 8; }
    int bytesPerFrame() const { return bytesPerSample() * channels; }
};

/*==============================================================================
 * Sound - Audio sample data (like a loaded WAV file)
 *============================================================================*/

class Sound {
public:
    Sound();
    ~Sound();
    
    // Non-copyable, movable
    Sound(const Sound&) = delete;
    Sound& operator=(const Sound&) = delete;
    Sound(Sound&& other) noexcept;
    Sound& operator=(Sound&& other) noexcept;
    
    // Load from WAV file
    bool loadWAV(const std::string& filename);
    
    // Load from raw PCM data
    bool loadRaw(const void* data, size_t size, const AudioFormat& format);
    
    // Create empty buffer (for streaming/procedural audio)
    bool create(size_t frames, const AudioFormat& format);
    
    void release();
    
    // Properties
    bool valid() const { return !m_data.empty(); }
    const AudioFormat& format() const { return m_format; }
    size_t frames() const { return m_data.size() / m_format.bytesPerFrame(); }
    double duration() const { return (double)frames() / m_format.sampleRate; }
    
    // Raw data access
    const uint8_t* data() const { return m_data.data(); }
    uint8_t* data() { return m_data.data(); }
    size_t size() const { return m_data.size(); }
    
private:
    std::vector<uint8_t> m_data;
    AudioFormat m_format;
};

/*==============================================================================
 * Voice - A playing instance of a Sound
 *============================================================================*/

class Voice {
public:
    Voice();
    
    void setVolume(float vol);    // 0.0 to 1.0
    float volume() const { return m_volume; }
    
    void setLooping(bool loop);
    bool looping() const { return m_looping; }
    
    void setPaused(bool paused);
    bool paused() const { return m_paused; }
    
    void stop();
    bool playing() const { return m_playing && !m_paused; }
    bool active() const { return m_playing; }
    
    // Position in frames
    size_t position() const { return m_position; }
    void setPosition(size_t frame);
    
private:
    friend class Mixer;
    
    const Sound* m_sound = nullptr;
    size_t m_position = 0;
    float m_volume = 1.0f;
    bool m_looping = false;
    bool m_paused = false;
    bool m_playing = false;
};

/*==============================================================================
 * Mixer - Audio output and mixing engine
 *============================================================================*/

// Forward declare Impl - full definition in HAUDIO_IMPLEMENTATION section
struct MixerImpl;

class Mixer {
    friend struct MixerImpl;  // Allow MixerImpl to access private members
public:
    Mixer();
    ~Mixer();
    
    // Non-copyable
    Mixer(const Mixer&) = delete;
    Mixer& operator=(const Mixer&) = delete;
    
    // Initialize audio output
    // format: desired output format (default: 44100 Hz, stereo, 16-bit)
    // bufferMs: buffer size in milliseconds (lower = less latency, more CPU)
    bool init(const AudioFormat& format = AudioFormat(), int bufferMs = 50);
    
    void shutdown();
    
    bool initialized() const { return m_initialized; }
    const AudioFormat& format() const { return m_format; }
    
    // Master volume (0.0 to 1.0)
    void setMasterVolume(float vol);
    float masterVolume() const { return m_masterVolume; }
    
    // Play a sound, returns a Voice handle for control
    // Returns nullptr if no free voice slots
    Voice* play(const Sound& sound, float volume = 1.0f, bool loop = false);
    
    // Stop all sounds
    void stopAll();
    
    // Max simultaneous voices (default 32)
    static constexpr int MAX_VOICES = 32;
    
private:
    // Platform-specific implementation (defined in HAUDIO_IMPLEMENTATION)
    MixerImpl* m_impl = nullptr;
    
    AudioFormat m_format;
    float m_masterVolume = 1.0f;
    bool m_initialized = false;
    
    std::array<Voice, MAX_VOICES> m_voices;
    std::mutex m_mutex;
    
    // Called by platform code to fill audio buffer
    void mixAudio(int16_t* buffer, size_t frames);
};

} // namespace haudio

/*==============================================================================
 * Implementation
 *============================================================================*/

#ifdef HAUDIO_IMPLEMENTATION

#include <cstring>
#include <algorithm>
#include <cmath>

#ifdef HAUDIO_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
    #include <mmsystem.h>
    #pragma comment(lib, "winmm.lib")
#endif

#ifdef HAUDIO_LINUX
    #include <alsa/asoundlib.h>
    #include <thread>
#endif

namespace haudio {

/*==============================================================================
 * Sound Implementation
 *============================================================================*/

Sound::Sound() = default;
Sound::~Sound() { release(); }

Sound::Sound(Sound&& other) noexcept
    : m_data(std::move(other.m_data))
    , m_format(other.m_format)
{
    other.m_format = AudioFormat();
}

Sound& Sound::operator=(Sound&& other) noexcept {
    if (this != &other) {
        m_data = std::move(other.m_data);
        m_format = other.m_format;
        other.m_format = AudioFormat();
    }
    return *this;
}

void Sound::release() {
    m_data.clear();
    m_format = AudioFormat();
}

bool Sound::create(size_t frames, const AudioFormat& format) {
    release();
    m_format = format;
    m_data.resize(frames * format.bytesPerFrame());
    return true;
}

bool Sound::loadRaw(const void* data, size_t size, const AudioFormat& format) {
    release();
    m_format = format;
    m_data.resize(size);
    std::memcpy(m_data.data(), data, size);
    return true;
}

bool Sound::loadWAV(const std::string& filename) {
    release();
    
    FILE* f = fopen(filename.c_str(), "rb");
    if (!f) return false;
    
    // Read RIFF header
    char riff[4];
    uint32_t fileSize;
    char wave[4];
    
    if (fread(riff, 1, 4, f) != 4 || std::memcmp(riff, "RIFF", 4) != 0) {
        fclose(f);
        return false;
    }
    fread(&fileSize, 4, 1, f);
    if (fread(wave, 1, 4, f) != 4 || std::memcmp(wave, "WAVE", 4) != 0) {
        fclose(f);
        return false;
    }
    
    // Find fmt and data chunks
    bool foundFmt = false;
    bool foundData = false;
    
    while (!foundData) {
        char chunkId[4];
        uint32_t chunkSize;
        
        if (fread(chunkId, 1, 4, f) != 4) break;
        if (fread(&chunkSize, 4, 1, f) != 1) break;
        
        if (std::memcmp(chunkId, "fmt ", 4) == 0) {
            uint16_t audioFormat, numChannels;
            uint32_t sampleRate, byteRate;
            uint16_t blockAlign, bitsPerSample;
            
            fread(&audioFormat, 2, 1, f);
            fread(&numChannels, 2, 1, f);
            fread(&sampleRate, 4, 1, f);
            fread(&byteRate, 4, 1, f);
            fread(&blockAlign, 2, 1, f);
            fread(&bitsPerSample, 2, 1, f);
            
            // Only support PCM (format 1)
            if (audioFormat != 1) {
                fclose(f);
                return false;
            }
            
            m_format.sampleRate = sampleRate;
            m_format.channels = numChannels;
            m_format.bitsPerSample = bitsPerSample;
            
            // Skip any extra format bytes
            if (chunkSize > 16) {
                fseek(f, chunkSize - 16, SEEK_CUR);
            }
            foundFmt = true;
        }
        else if (std::memcmp(chunkId, "data", 4) == 0) {
            if (!foundFmt) {
                fclose(f);
                return false;
            }
            
            m_data.resize(chunkSize);
            if (fread(m_data.data(), 1, chunkSize, f) != chunkSize) {
                release();
                fclose(f);
                return false;
            }
            foundData = true;
        }
        else {
            // Skip unknown chunk
            fseek(f, chunkSize, SEEK_CUR);
        }
    }
    
    fclose(f);
    return foundFmt && foundData;
}

/*==============================================================================
 * Voice Implementation
 *============================================================================*/

Voice::Voice() = default;

void Voice::setVolume(float vol) {
    m_volume = std::clamp(vol, 0.0f, 1.0f);
}

void Voice::setLooping(bool loop) {
    m_looping = loop;
}

void Voice::setPaused(bool paused) {
    m_paused = paused;
}

void Voice::stop() {
    m_playing = false;
    m_position = 0;
}

void Voice::setPosition(size_t frame) {
    if (m_sound) {
        m_position = std::min(frame, m_sound->frames());
    }
}

/*==============================================================================
 * Mixer - Platform-specific Implementation
 *============================================================================*/

#ifdef HAUDIO_WINDOWS

struct MixerImpl {
    HWAVEOUT hWaveOut = nullptr;
    WAVEHDR waveHeaders[2];
    std::vector<int16_t> buffers[2];
    int currentBuffer = 0;
    std::atomic<bool> running{false};
    Mixer* mixer = nullptr;
    
    static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance,
                                      DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
        if (uMsg == WOM_DONE) {
            MixerImpl* impl = (MixerImpl*)dwInstance;
            if (impl->running) {
                // Refill the completed buffer
                WAVEHDR* hdr = (WAVEHDR*)dwParam1;
                int bufIdx = (hdr == &impl->waveHeaders[0]) ? 0 : 1;
                
                impl->mixer->mixAudio(impl->buffers[bufIdx].data(),
                                       impl->buffers[bufIdx].size() / impl->mixer->m_format.channels);
                
                waveOutWrite(impl->hWaveOut, hdr, sizeof(WAVEHDR));
            }
        }
    }
};

bool Mixer::init(const AudioFormat& format, int bufferMs) {
    if (m_initialized) return true;
    
    m_format = format;
    m_impl = new MixerImpl();
    m_impl->mixer = this;
    
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = format.channels;
    wfx.nSamplesPerSec = format.sampleRate;
    wfx.wBitsPerSample = format.bitsPerSample;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    
    MMRESULT result = waveOutOpen(&m_impl->hWaveOut, WAVE_MAPPER, &wfx,
                                   (DWORD_PTR)MixerImpl::waveOutProc,
                                   (DWORD_PTR)m_impl, CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        delete m_impl; m_impl = nullptr;
        return false;
    }
    
    // Allocate double-buffers
    size_t bufferFrames = (format.sampleRate * bufferMs) / 1000;
    size_t bufferSamples = bufferFrames * format.channels;
    
    for (int i = 0; i < 2; i++) {
        m_impl->buffers[i].resize(bufferSamples, 0);
        
        WAVEHDR& hdr = m_impl->waveHeaders[i];
        std::memset(&hdr, 0, sizeof(WAVEHDR));
        hdr.lpData = (LPSTR)m_impl->buffers[i].data();
        hdr.dwBufferLength = bufferSamples * sizeof(int16_t);
        
        waveOutPrepareHeader(m_impl->hWaveOut, &hdr, sizeof(WAVEHDR));
    }
    
    m_impl->running = true;
    m_initialized = true;
    
    // Start playback by queueing both buffers
    for (int i = 0; i < 2; i++) {
        mixAudio(m_impl->buffers[i].data(), bufferFrames);
        waveOutWrite(m_impl->hWaveOut, &m_impl->waveHeaders[i], sizeof(WAVEHDR));
    }
    
    return true;
}

void Mixer::shutdown() {
    if (!m_initialized) return;
    
    m_impl->running = false;
    
    if (m_impl->hWaveOut) {
        waveOutReset(m_impl->hWaveOut);
        
        for (int i = 0; i < 2; i++) {
            waveOutUnprepareHeader(m_impl->hWaveOut, &m_impl->waveHeaders[i], sizeof(WAVEHDR));
        }
        
        waveOutClose(m_impl->hWaveOut);
    }
    
    delete m_impl; m_impl = nullptr;
    m_initialized = false;
}

#endif // HAUDIO_WINDOWS

#ifdef HAUDIO_LINUX

struct MixerImpl {
    snd_pcm_t* pcm = nullptr;
    std::thread audioThread;
    std::atomic<bool> running{false};
    std::vector<int16_t> buffer;
    size_t bufferFrames = 0;
    Mixer* mixer = nullptr;
    
    void audioLoop() {
        while (running) {
            mixer->mixAudio(buffer.data(), bufferFrames);
            
            int16_t* ptr = buffer.data();
            snd_pcm_uframes_t remaining = bufferFrames;
            
            while (remaining > 0 && running) {
                snd_pcm_sframes_t written = snd_pcm_writei(pcm, ptr, remaining);
                
                if (written < 0) {
                    // Handle underrun
                    snd_pcm_recover(pcm, written, 0);
                } else {
                    ptr += written * mixer->m_format.channels;
                    remaining -= written;
                }
            }
        }
    }
};

bool Mixer::init(const AudioFormat& format, int bufferMs) {
    if (m_initialized) return true;
    
    m_format = format;
    m_impl = new MixerImpl();
    m_impl->mixer = this;
    
    // Open default playback device
    int err = snd_pcm_open(&m_impl->pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        delete m_impl; m_impl = nullptr;
        return false;
    }
    
    // Set hardware parameters
    snd_pcm_hw_params_t* hwParams;
    snd_pcm_hw_params_alloca(&hwParams);
    snd_pcm_hw_params_any(m_impl->pcm, hwParams);
    
    snd_pcm_hw_params_set_access(m_impl->pcm, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    
    snd_pcm_format_t pcmFormat = (format.bitsPerSample == 16) 
        ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_U8;
    snd_pcm_hw_params_set_format(m_impl->pcm, hwParams, pcmFormat);
    
    unsigned int rate = format.sampleRate;
    snd_pcm_hw_params_set_rate_near(m_impl->pcm, hwParams, &rate, 0);
    
    snd_pcm_hw_params_set_channels(m_impl->pcm, hwParams, format.channels);
    
    snd_pcm_uframes_t bufferSize = (format.sampleRate * bufferMs) / 1000;
    snd_pcm_hw_params_set_buffer_size_near(m_impl->pcm, hwParams, &bufferSize);
    
    snd_pcm_uframes_t periodSize = bufferSize / 4;
    snd_pcm_hw_params_set_period_size_near(m_impl->pcm, hwParams, &periodSize, 0);
    
    err = snd_pcm_hw_params(m_impl->pcm, hwParams);
    if (err < 0) {
        snd_pcm_close(m_impl->pcm);
        delete m_impl; m_impl = nullptr;
        return false;
    }
    
    // Allocate buffer
    m_impl->bufferFrames = periodSize;
    m_impl->buffer.resize(periodSize * format.channels, 0);
    
    m_impl->running = true;
    m_initialized = true;
    
    // Start audio thread
    m_impl->audioThread = std::thread(&MixerImpl::audioLoop, m_impl);
    
    return true;
}

void Mixer::shutdown() {
    if (!m_initialized) return;
    
    m_impl->running = false;
    
    if (m_impl->audioThread.joinable()) {
        m_impl->audioThread.join();
    }
    
    if (m_impl->pcm) {
        snd_pcm_drain(m_impl->pcm);
        snd_pcm_close(m_impl->pcm);
    }
    
    delete m_impl; m_impl = nullptr;
    m_initialized = false;
}

#endif // HAUDIO_LINUX

/*==============================================================================
 * Mixer - Common Implementation
 *============================================================================*/

Mixer::Mixer() = default;

Mixer::~Mixer() {
    shutdown();
}

void Mixer::setMasterVolume(float vol) {
    m_masterVolume = std::clamp(vol, 0.0f, 1.0f);
}

Voice* Mixer::play(const Sound& sound, float volume, bool loop) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find a free voice slot
    for (auto& voice : m_voices) {
        if (!voice.m_playing) {
            voice.m_sound = &sound;
            voice.m_position = 0;
            voice.m_volume = std::clamp(volume, 0.0f, 1.0f);
            voice.m_looping = loop;
            voice.m_paused = false;
            voice.m_playing = true;
            return &voice;
        }
    }
    
    return nullptr;  // No free voice slots
}

void Mixer::stopAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto& voice : m_voices) {
        voice.stop();
    }
}

void Mixer::mixAudio(int16_t* buffer, size_t frames) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Clear buffer
    std::memset(buffer, 0, frames * m_format.channels * sizeof(int16_t));
    
    // Mix all active voices
    for (auto& voice : m_voices) {
        if (!voice.m_playing || voice.m_paused || !voice.m_sound) {
            continue;
        }
        
        const Sound& sound = *voice.m_sound;
        const AudioFormat& srcFmt = sound.format();
        
        // Simple mixing: assumes same format as output for now
        // TODO: Add sample rate and format conversion
        if (srcFmt.sampleRate != m_format.sampleRate ||
            srcFmt.channels != m_format.channels ||
            srcFmt.bitsPerSample != m_format.bitsPerSample) {
            // Skip incompatible sounds
            continue;
        }
        
        float vol = voice.m_volume * m_masterVolume;
        const int16_t* src = (const int16_t*)sound.data();
        size_t srcFrames = sound.frames();
        
        for (size_t i = 0; i < frames; i++) {
            if (voice.m_position >= srcFrames) {
                if (voice.m_looping) {
                    voice.m_position = 0;
                } else {
                    voice.m_playing = false;
                    break;
                }
            }
            
            for (int ch = 0; ch < m_format.channels; ch++) {
                size_t srcIdx = voice.m_position * m_format.channels + ch;
                size_t dstIdx = i * m_format.channels + ch;
                
                // Mix with clipping
                int32_t sample = buffer[dstIdx] + (int32_t)(src[srcIdx] * vol);
                buffer[dstIdx] = (int16_t)std::clamp(sample, -32768, 32767);
            }
            
            voice.m_position++;
        }
    }
}

} // namespace haudio

#endif // HAUDIO_IMPLEMENTATION

#endif // HAUDIO_HPP
