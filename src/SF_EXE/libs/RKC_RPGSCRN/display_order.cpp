#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace osf {
namespace {

std::int32_t topDepth(const DisplayOrderEntry& entry) {
    return calculateRealPosition({
        entry.position.x + entry.judgement.left,
        entry.position.y + entry.judgement.top,
    }).y;
}

bool blocks(
    const DisplayOrderEntry& other,
    const DisplayOrderEntry& candidate) {
    const std::int32_t other_class =
        displayClassForStatus(other.status);
    const std::int32_t candidate_class =
        displayClassForStatus(candidate.status);
    if (other_class != candidate_class) {
        return candidate_class < other_class;
    }

    const std::int32_t candidate_right =
        candidate.position.x + candidate.judgement.right;
    const std::int32_t candidate_bottom =
        candidate.position.y + candidate.judgement.bottom;
    const std::int32_t other_left =
        other.position.x + other.judgement.left;
    const std::int32_t other_top =
        other.position.y + other.judgement.top;
    const std::int32_t other_right =
        other.position.x + other.judgement.right;
    const std::int32_t other_bottom =
        other.position.y + other.judgement.bottom;

    return other_left < candidate_right &&
           other_top < candidate_bottom &&
           (other_right < candidate_right ||
            other_bottom < candidate_bottom);
}

}  // namespace

std::int32_t displayClassForStatus(std::int16_t status) {
    std::int32_t result = (status & 0x100) != 0 ? 1 : 0;
    if ((status & 0x80) != 0) {
        result = 2;
    }
    if ((status & 0x20) != 0) {
        result = 3;
    }
    return result;
}

void sortDisplayObjects(
    std::vector<DisplayOrderEntry>& entries) {
    // InsertSort first establishes the class and top-left order used by
    // OBJECTBLOCK::SetDisplayObject. Equal entries retain insertion order.
    std::stable_sort(
        entries.begin(),
        entries.end(),
        [](const DisplayOrderEntry& left,
           const DisplayOrderEntry& right) {
            const std::int32_t left_class =
                displayClassForStatus(left.status);
            const std::int32_t right_class =
                displayClassForStatus(right.status);
            if (left_class != right_class) {
                return left_class < right_class;
            }
            return topDepth(left) < topDepth(right);
        });

    // SortDisplayObject then selects the first entry that has no remaining
    // object which must be drawn behind it. Its strict rectangle comparisons
    // are important: touching edges do not establish a depth dependency.
    for (std::size_t target = 0;
         target < entries.size();
         ++target) {
        std::size_t candidate = target;
        for (; candidate < entries.size(); ++candidate) {
            bool blocked = false;
            for (std::size_t other = target;
                 other < entries.size();
                 ++other) {
                if (other != candidate &&
                    blocks(entries[other], entries[candidate])) {
                    blocked = true;
                    break;
                }
            }
            if (!blocked) {
                break;
            }
        }
        if (candidate != target &&
            candidate < entries.size()) {
            DisplayOrderEntry selected =
                std::move(entries[candidate]);
            entries.erase(
                entries.begin() +
                static_cast<std::ptrdiff_t>(candidate));
            entries.insert(
                entries.begin() +
                    static_cast<std::ptrdiff_t>(target),
                std::move(selected));
        }
    }
}

}  // namespace osf
