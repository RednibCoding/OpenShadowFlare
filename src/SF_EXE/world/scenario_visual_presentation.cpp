#include "scenario_visual_presentation.hpp"

#include <algorithm>

namespace osf {
namespace {

constexpr std::int32_t kFadeUpdates = 120;
constexpr std::int32_t kMinimumPageUpdates = 300;

}  // namespace

void ScenarioVisualPresentation::clear() {
    visual_id_ = -1;
    page_ = 0;
    page_count_ = 0;
    counter_ = 0;
    advance_requested_ = false;
    closing_ = false;
}

void ScenarioVisualPresentation::begin(
    std::int32_t visual_id,
    std::size_t page_count) {
    visual_id_ = visual_id;
    page_ = 0;
    page_count_ = std::max<std::size_t>(page_count, 1);
    counter_ = 0;
    advance_requested_ = false;
    closing_ = false;
}

void ScenarioVisualPresentation::requestAdvance() {
    if (active()) {
        advance_requested_ = true;
    }
}

void ScenarioVisualPresentation::advanceFrame() {
    if (closing_) {
        clear();
        return;
    }
    if (!active()) {
        advance_requested_ = false;
        return;
    }

    ++counter_;
    const bool advance = advance_requested_;
    advance_requested_ = false;
    if (counter_ < kMinimumPageUpdates || !advance) {
        return;
    }

    ++page_;
    if (page_ < page_count_) {
        // Retail restarts subsequent pages at one rather than zero.
        counter_ = 1;
        return;
    }
    --page_;
    closing_ = true;
}

bool ScenarioVisualPresentation::active() const {
    return visual_id_ != -1 && !closing_;
}

std::int32_t ScenarioVisualPresentation::visualId() const {
    return visual_id_;
}

std::size_t ScenarioVisualPresentation::page() const {
    return page_;
}

std::int32_t ScenarioVisualPresentation::counter() const {
    return counter_;
}

std::int32_t ScenarioVisualPresentation::fadeStrength() const {
    if (!active()) {
        return 0;
    }
    if (counter_ >= kFadeUpdates) {
        return 1000;
    }
    return std::max(counter_, 0) * 1000 / kFadeUpdates;
}

bool ScenarioVisualPresentation::continueVisible() const {
    return active() &&
           counter_ + 1 >= kMinimumPageUpdates;
}

std::int32_t ScenarioVisualPresentation::continueOffset() const {
    const std::int32_t phase =
        (std::max(counter_, 0) + 1) % 15;
    if (phase < 5) {
        return 0;
    }
    return phase < 10 ? 8 : 16;
}

}  // namespace osf
