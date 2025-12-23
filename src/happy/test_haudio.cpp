/*
 * test_haudio.cpp - Test the haudio library with a WAV file
 * 
 * Build (Linux):
 *   g++ -std=c++17 -O2 test_haudio.cpp -o test_haudio -lasound -lpthread
 */

#define HAUDIO_IMPLEMENTATION
#include "haudio.hpp"

#include <cstdio>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]) {
    const char* wavFile = "../../tmp/mothra.wav";
    if (argc > 1) {
        wavFile = argv[1];
    }
    
    printf("haudio test - loading %s\n", wavFile);
    
    // Load the WAV file
    haudio::Sound sound;
    if (!sound.loadWAV(wavFile)) {
        fprintf(stderr, "Failed to load WAV file: %s\n", wavFile);
        return 1;
    }
    
    printf("Loaded: %d Hz, %d channels, %d bits, %.2f seconds\n",
           sound.format().sampleRate,
           sound.format().channels,
           sound.format().bitsPerSample,
           sound.duration());
    
    // Initialize mixer
    haudio::Mixer mixer;
    haudio::AudioFormat fmt;
    fmt.sampleRate = sound.format().sampleRate;
    fmt.channels = sound.format().channels;
    fmt.bitsPerSample = sound.format().bitsPerSample;
    
    if (!mixer.init(fmt, 100)) {
        fprintf(stderr, "Failed to initialize audio mixer\n");
        return 1;
    }
    
    printf("Mixer initialized, playing sound...\n");
    
    // Play the sound
    haudio::Voice* voice = mixer.play(sound, 1.0f, false);
    if (!voice) {
        fprintf(stderr, "Failed to play sound\n");
        return 1;
    }
    
    printf("Playing... press Ctrl+C to stop\n");
    
    // Wait for playback to finish
    while (voice->active()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        printf("\r  Position: %.2f / %.2f sec", 
               (double)voice->position() / sound.format().sampleRate,
               sound.duration());
        fflush(stdout);
    }
    
    printf("\nDone!\n");
    
    mixer.shutdown();
    return 0;
}
