#include "world/enemy_presentation_audio.hpp"

#include <cstdint>
#include <iostream>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

std::uint64_t hashWord(
    std::uint64_t hash,
    std::int32_t value) {
    const std::uint32_t bits =
        static_cast<std::uint32_t>(value);
    for (std::uint32_t shift = 0;
         shift < 32;
         shift += 8) {
        hash ^= (bits >> shift) & 0xffu;
        hash *= 1099511628211ull;
    }
    return hash;
}

bool testCompleteRetailOverrideTable() {
    std::int32_t override_count = 0;
    std::int32_t sample_sum = 0;
    std::uint64_t hash = 1469598103934665603ull;
    for (std::int32_t resource = 0;
         resource < 25;
         ++resource) {
        for (std::int32_t marker = 0;
             marker < 3;
             ++marker) {
            for (std::int32_t chart = 0;
                 chart < 10;
                 ++chart) {
                const std::int32_t resolved =
                    osf::retailEnemyPresentationSample(
                        resource, chart, marker);
                const std::int32_t raw_override =
                    resolved == 86
                    ? osf::kNoEnemyPresentationSample
                    : resolved;
                if (raw_override !=
                    osf::kNoEnemyPresentationSample) {
                    ++override_count;
                    sample_sum += raw_override;
                }
                hash = hashWord(hash, raw_override);
            }
        }
    }
    return check(
        override_count == 59 &&
            sample_sum == 6064 &&
            hash == 0x70d141614d95e6a9ull,
        "The 25-by-3-by-10 enemy sound override table "
        "differs from DAT_00480a20.");
}

bool testOverrideAndFallbackOrder() {
    return check(
        osf::retailEnemyPresentationSample(4, 3, 0) ==
                87 &&
            osf::retailEnemyPresentationSample(4, 3, 1) ==
                89 &&
            osf::retailEnemyPresentationSample(0, 3, 0) ==
                86 &&
            osf::retailEnemyPresentationSample(24, 4, 0) ==
                134 &&
            osf::retailEnemyPresentationSample(16, 5, 1) ==
                147 &&
            osf::retailEnemyPresentationSample(19, 4, 0) ==
                3 &&
            osf::retailEnemyPresentationSample(0, 4, 0) ==
                osf::kNoEnemyPresentationSample,
        "Enemy sound lookup did not prefer an override and then "
        "fall back through DAT_004809a8.");
}

bool testInvalidIndicesAreContained() {
    return check(
        osf::retailEnemyPresentationSample(-1, 3, 2) ==
                86 &&
            osf::retailEnemyPresentationSample(25, 3, 2) ==
                86 &&
            osf::retailEnemyPresentationSample(0, -1, 0) ==
                osf::kNoEnemyPresentationSample &&
            osf::retailEnemyPresentationSample(0, 10, 0) ==
                osf::kNoEnemyPresentationSample &&
            osf::retailEnemyPresentationSample(0, 3, -1) ==
                osf::kNoEnemyPresentationSample &&
            osf::retailEnemyPresentationSample(0, 3, 3) ==
                osf::kNoEnemyPresentationSample,
        "Portable bounds checks changed a valid fallback or "
        "accepted an invalid chart/marker index.");
}

bool testCompleteDeathSampleTable() {
    std::int32_t sample_count = 0;
    std::int32_t sample_sum = 0;
    std::uint64_t hash = 1469598103934665603ull;
    for (std::int32_t resource = 0;
         resource < 25;
         ++resource) {
        const std::int32_t sample =
            osf::retailEnemyDeathSample(resource);
        if (sample >= 0) {
            ++sample_count;
            sample_sum += sample;
        }
        hash = hashWord(hash, sample);
    }
    return check(
        sample_count == 20 &&
            sample_sum == 2431 &&
            hash == 0x086ee0f4b8c1f596ull &&
            osf::retailEnemyDeathSample(-1) == -1 &&
            osf::retailEnemyDeathSample(25) == -1,
        "The resource-specific enemy death samples differ from "
        "DAT_004815d8.");
}

}  // namespace

int main() {
    return testCompleteRetailOverrideTable() &&
                   testOverrideAndFallbackOrder() &&
                   testInvalidIndicesAreContained() &&
                   testCompleteDeathSampleTable()
        ? 0
        : 1;
}
