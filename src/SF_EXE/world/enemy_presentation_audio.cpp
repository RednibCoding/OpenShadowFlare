#include "enemy_presentation_audio.hpp"

#include <array>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kResourceCount = 25;
constexpr std::int32_t kChartCount = 10;
constexpr std::int32_t kMarkerSlotCount = 3;

struct SampleOverride {
    std::int32_t resource_id;
    std::int32_t marker_slot;
    std::int32_t animation_chart;
    std::int32_t sample;
};

constexpr std::array<SampleOverride, 59> kSampleOverrides{{
    {3, 0, 7, 91},
    {3, 0, 8, 91},
    {3, 1, 0, 90},
    {3, 1, 1, 90},
    {3, 1, 2, 90},
    {3, 1, 3, 90},
    {3, 1, 4, 90},
    {3, 1, 5, 90},
    {3, 1, 6, 90},
    {3, 1, 7, 90},
    {3, 1, 8, 90},
    {4, 0, 1, 92},
    {4, 0, 3, 87},
    {4, 1, 0, 89},
    {4, 1, 1, 89},
    {4, 1, 2, 89},
    {4, 1, 3, 89},
    {4, 1, 4, 89},
    {4, 1, 5, 89},
    {4, 1, 6, 89},
    {4, 1, 7, 89},
    {4, 1, 8, 89},
    {7, 0, 1, 108},
    {8, 0, 4, 113},
    {8, 0, 5, 112},
    {8, 0, 7, 112},
    {8, 0, 9, 110},
    {9, 0, 9, 110},
    {12, 0, 4, 116},
    {12, 0, 7, 115},
    {12, 0, 8, 115},
    {13, 0, 4, 113},
    {13, 0, 5, 112},
    {14, 0, 4, 131},
    {15, 0, 4, 129},
    {15, 0, 7, 129},
    {16, 0, 4, 146},
    {16, 0, 5, 146},
    {16, 1, 5, 147},
    {17, 0, 4, 123},
    {17, 0, 7, 124},
    {17, 0, 8, 124},
    {18, 0, 4, 140},
    {18, 0, 7, 141},
    {19, 0, 4, 3},
    {19, 0, 5, 3},
    {20, 0, 1, 108},
    {20, 0, 4, 3},
    {20, 0, 5, 3},
    {21, 0, 4, 155},
    {21, 0, 7, 155},
    {22, 0, 9, 110},
    {23, 0, 4, 147},
    {23, 0, 7, 106},
    {24, 0, 0, 90},
    {24, 0, 1, 90},
    {24, 0, 4, 134},
    {24, 0, 7, 134},
    {24, 0, 8, 135},
}};

}  // namespace

std::int32_t retailEnemyPresentationSample(
    std::int32_t resource_id,
    std::int32_t animation_chart,
    std::int32_t marker_slot) {
    if (animation_chart < 0 ||
        animation_chart >= kChartCount ||
        marker_slot < 0 ||
        marker_slot >= kMarkerSlotCount) {
        return kNoEnemyPresentationSample;
    }

    if (resource_id >= 0 &&
        resource_id < kResourceCount) {
        for (const SampleOverride& entry :
             kSampleOverrides) {
            if (entry.resource_id == resource_id &&
                entry.marker_slot == marker_slot &&
                entry.animation_chart ==
                    animation_chart) {
                return entry.sample;
            }
        }
    }

    // DAT_004809a8 contains the three ten-chart fallback
    // rows. Only chart three has a fallback, and every
    // marker slot maps it to sample 86.
    return animation_chart == 3
        ? 86
        : kNoEnemyPresentationSample;
}

}  // namespace osf
