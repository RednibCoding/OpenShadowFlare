#include "world/enemy_presentation.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

void appendI16(
    std::vector<std::uint8_t>& bytes,
    std::int16_t value) {
    const std::uint16_t bits =
        static_cast<std::uint16_t>(value);
    bytes.push_back(static_cast<std::uint8_t>(bits));
    bytes.push_back(
        static_cast<std::uint8_t>(bits >> 8u));
}

void appendI32(
    std::vector<std::uint8_t>& bytes,
    std::int32_t value) {
    const std::uint32_t bits =
        static_cast<std::uint32_t>(value);
    for (std::uint32_t shift = 0;
         shift < 32;
         shift += 8) {
        bytes.push_back(
            static_cast<std::uint8_t>(
                bits >> shift));
    }
}

osf::gapi::CafAnimation animationWithCharts(
    const std::map<
        std::int32_t,
        std::vector<std::uint16_t>>& chart_cells) {
    std::vector<std::uint8_t> bytes;
    constexpr std::array<char, 16> header{{
        'C', 'H', 'R', 'A', 'n', 'i', 'm', 'a',
        't', 'i', 'o', 'n', '0', '0', '2', '\0',
    }};
    bytes.insert(bytes.end(), header.begin(), header.end());
    appendI32(bytes, 10);
    for (std::int32_t chart = 0; chart < 10; ++chart) {
        appendI16(bytes, 0);
        for (std::int32_t direction = 0;
             direction < 9;
             ++direction) {
            const auto cells = chart_cells.find(chart);
            const bool populated =
                direction == 1 &&
                cells != chart_cells.end();
            appendI32(bytes, populated ? 1 : 0);
            appendI16(
                bytes,
                populated
                    ? static_cast<std::int16_t>(
                          cells->second.size())
                    : 0);
            if (!populated) {
                continue;
            }
            appendI32(
                bytes,
                static_cast<std::int32_t>(
                    cells->second.size()));
            for (std::uint16_t status :
                 cells->second) {
                appendI16(
                    bytes,
                    static_cast<std::int16_t>(
                        status));
                appendI16(bytes, 0);
                appendI32(bytes, 0);
                appendI16(bytes, 0);
            }
        }
    }
    appendI32(bytes, 0);
    appendI32(bytes, 0);

    osf::gapi::CafAnimation animation;
    std::string error;
    if (!animation.decode(bytes, &error)) {
        std::cerr << "Could not build the CAF fixture: "
                  << error << '\n';
    }
    return animation;
}

osf::EnemyPresentationProfile profile() {
    osf::EnemyPresentationProfile result;
    result.direct_maximum_target_distance = {
        321, 654, 987};
    result.direct_animation_chart = {4, 5, 6};
    result.direct_animation_speed_index = {6, 0, 9};
    result.effect_animation_chart = {7, 8, 9};
    result.effect_animation_speed_index = {4, 4, 4};
    result.effect_type = {10, 11, 12};
    result.effect_subtype = {20, 21, 22};
    result.effect_parameter = {30, 31, 32};
    result.effect_additive = {40, 41, 42};
    return result;
}

bool testDirectTimingMarkersAndCompletion() {
    const osf::gapi::CafAnimation animation =
        animationWithCharts({
            {4, {
                0x400u,
                0u,
                0x40u | 0x800u,
                0x1000u,
                0u,
            }},
        });
    const osf::EnemyPresentationProfile values =
        profile();
    osf::EnemyPresentationController controller;
    controller.reset();
    controller.select(1);

    std::int32_t searches = 0;
    osf::EnemyPresentationContext context;
    context.position = {20, 30};
    context.direction = 6;
    context.resource_id = 3;
    context.profile = &values;
    context.animation = &animation;
    context.target_in_range =
        [&searches](
            std::int32_t minimum,
            std::int32_t maximum) {
            ++searches;
            if (minimum != 0 || maximum != 321) {
                return osf::EnemyAiTarget{};
            }
            return osf::EnemyAiTarget{
                true,
                osf::MovementTargetKind::player,
                0,
                80,
                {100, 30},
            };
        };

    osf::EnemyPresentationUpdate update =
        controller.update(context);
    if (!check(
            update.handled &&
                update.active &&
                update.presentation_action == 1 &&
                update.animation_chart == 4 &&
                update.animation_frame == 0 &&
                update.direction == 1 &&
                update.audio_markers ==
                    osf::kEnemyAudioMarkerZero &&
                !update.impact &&
                searches == 1,
            "Direct presentation entry did not select, face, "
            "draw, or scan frame zero like retail.")) {
        return false;
    }

    controller.select(1);
    update = controller.update(context);
    if (!check(
            update.active &&
                update.animation_frame == 2 &&
                update.audio_markers ==
                    osf::kEnemyAudioMarkerOne &&
                update.audio_samples[0] == -1 &&
                update.audio_samples[1] == 90 &&
                update.audio_samples[2] == -1 &&
                update.impact &&
                update.impact_family ==
                    osf::EnemyPresentationFamily::direct &&
                update.impact_variant == 0 &&
                update.target.found &&
                searches == 1,
            "Direct presentation did not scan every crossed "
            "frame or preserve its entry target.")) {
        return false;
    }

    update = controller.update(context);
    return check(
        update.handled &&
            !update.active &&
            update.animation_frame == 4 &&
            update.audio_markers ==
                osf::kEnemyAudioMarkerTwo &&
            update.completion_event == 2 &&
            controller.presentationAction() == 7 &&
            controller.animationChart() == 0,
        "Direct presentation did not clamp its last frame, emit "
        "event two, and return to idle.");
}

bool testSlowTimingDoesNotRestart() {
    const osf::gapi::CafAnimation animation =
        animationWithCharts({
            {5, {0u, 0x40u, 0u}},
        });
    const osf::EnemyPresentationProfile values =
        profile();
    osf::EnemyPresentationController controller;
    controller.reset();
    controller.select(2);

    osf::EnemyPresentationContext context;
    context.direction = 1;
    context.profile = &values;
    context.animation = &animation;

    if (!controller.update(context).active) {
        return check(
            false,
            "The slow presentation did not enter.");
    }
    for (std::int32_t update_index = 0;
         update_index < 3;
         ++update_index) {
        controller.select(2);
        const osf::EnemyPresentationUpdate update =
            controller.update(context);
        if (!check(
                update.animation_frame == 0 &&
                    controller.elapsedUpdates() ==
                        update_index + 1,
                "Repeated selection restarted a slow retail "
                "presentation instead of continuing it.")) {
            return false;
        }
    }
    const osf::EnemyPresentationUpdate update =
        controller.update(context);
    return check(
        update.animation_frame == 1 &&
            update.impact &&
            update.completion_event == -1,
        "The 0.3 speed tier did not use elapsed-update "
        "multiplication with truncation.");
}

bool testEffectTargetAndCompletion() {
    const osf::gapi::CafAnimation animation =
        animationWithCharts({
            {9, {0x40u | 0x400u}},
        });
    const osf::EnemyPresentationProfile values =
        profile();
    osf::EnemyPresentationController controller;
    controller.reset();
    controller.select(6);

    std::int32_t searches = 0;
    osf::EnemyPresentationContext context;
    context.position = {0, 0};
    context.direction = 5;
    context.resource_id = 8;
    context.profile = &values;
    context.animation = &animation;
    context.default_target = [&searches]() {
        ++searches;
        return osf::EnemyAiTarget{
            true,
            osf::MovementTargetKind::scenario_actor,
            osf::kFirstCompanionCharacterNumber,
            100,
            {100, 0},
        };
    };

    const osf::EnemyPresentationUpdate update =
        controller.update(context);
    return check(
        update.handled &&
            !update.active &&
            update.presentation_action == 6 &&
            update.animation_chart == 9 &&
            update.direction == 1 &&
            update.impact &&
            update.impact_family ==
                osf::EnemyPresentationFamily::effect &&
            update.impact_variant == 2 &&
            update.effect_type == 12 &&
            update.effect_subtype == 22 &&
            update.effect_parameter == 32 &&
            update.effect_additive == 42 &&
            update.audio_samples[0] == 110 &&
            update.completion_event == 7 &&
            searches == 1,
        "Effect presentation six did not use default targeting, "
        "its frame marker, or completion event seven.");
}

bool testSkippedFramesAndMissingVisual() {
    const osf::gapi::CafAnimation animation =
        animationWithCharts({
            {6, {
                0u,
                0x40u | 0x400u,
                0x800u,
                0x1000u,
            }},
        });
    const osf::EnemyPresentationProfile values =
        profile();
    osf::EnemyPresentationController controller;
    controller.reset();
    controller.select(3);

    osf::EnemyPresentationContext context;
    context.direction = 1;
    context.profile = &values;
    context.animation = &animation;
    if (!controller.update(context).active) {
        return check(
            false,
            "The fast presentation did not enter.");
    }
    const osf::EnemyPresentationUpdate skipped =
        controller.update(context);
    if (!check(
            !skipped.active &&
                skipped.animation_frame == 3 &&
                !skipped.impact &&
                skipped.audio_markers == 0 &&
                skipped.completion_event == 4,
            "A frame jump beyond the retail chart end scanned "
            "markers which the executable skips.")) {
        return false;
    }

    controller.reset();
    controller.select(4);
    context.animation = nullptr;
    const osf::EnemyPresentationUpdate missing =
        controller.update(context);
    return check(
        missing.handled &&
            !missing.active &&
            missing.completion_event == 5 &&
            controller.presentationAction() == 7,
        "A resource-less enemy presentation did not complete "
        "immediately with its retail event.");
}

bool testInvalidSpeedIsRejectedWithoutStateChange() {
    osf::EnemyPresentationProfile values = profile();
    values.direct_animation_speed_index[0] = 10;
    osf::EnemyPresentationController controller;
    controller.reset();
    controller.select(1);

    osf::EnemyPresentationContext context;
    context.profile = &values;
    const osf::EnemyPresentationUpdate update =
        controller.update(context);
    return check(
        !update.handled &&
            controller.presentationAction() == 7 &&
            controller.elapsedUpdates() == 0,
        "An out-of-table speed index partially entered an "
        "unsafe presentation.");
}

bool testExistingEventIsNotOverwritten() {
    const osf::EnemyPresentationProfile values =
        profile();
    osf::EnemyPresentationController controller;
    controller.reset();
    controller.select(1);

    osf::EnemyPresentationContext context;
    context.event_number = 42;
    context.profile = &values;
    const osf::EnemyPresentationUpdate update =
        controller.update(context);
    return check(
        update.handled &&
            !update.active &&
            update.completion_event == -1 &&
            controller.presentationAction() == 7,
        "Presentation completion overwrote a non-minus-one "
        "enemy event.");
}

}  // namespace

int main() {
    return testDirectTimingMarkersAndCompletion() &&
                   testSlowTimingDoesNotRestart() &&
                   testEffectTargetAndCompletion() &&
                   testSkippedFramesAndMissingVisual() &&
                   testInvalidSpeedIsRejectedWithoutStateChange() &&
                   testExistingEventIsNotOverwritten()
        ? 0
        : 1;
}
