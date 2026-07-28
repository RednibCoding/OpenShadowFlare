#include "openshadowflare/command_line.hpp"
#include "openshadowflare/game_config.hpp"
#include "openshadowflare/game_state.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using openshadowflare::GameConfig;

std::string encode(const std::array<std::int32_t, 16>& values) {
    std::string bytes;
    bytes.reserve(openshadowflare::kGameConfigByteSize);
    for (const std::int32_t value : values) {
        const auto raw = static_cast<std::uint32_t>(value);
        bytes.push_back(static_cast<char>(raw & 0xffU));
        bytes.push_back(static_cast<char>((raw >> 8U) & 0xffU));
        bytes.push_back(static_cast<char>((raw >> 16U) & 0xffU));
        bytes.push_back(static_cast<char>((raw >> 24U) & 0xffU));
    }
    return bytes;
}

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool testRetailDefaultsAndFixture() {
    GameConfig config;
    if (!check(
            !config.windowed_at_start &&
                config.semi_transparent_shadow &&
                config.semi_transparent_objects &&
                config.display_darkness &&
                config.unknown_48d528 &&
                config.unknown_48d540 &&
                config.save_image_at_game_end &&
                config.click_range == 2 &&
                config.click_range_enabled &&
                config.click_priority ==
                    std::array<std::int32_t, 5>{{4, 2, 3, 1, 0}} &&
                config.effect_volume == 0 &&
                config.bgm_volume == 0,
            "Retail constructor defaults do not match 0x00401000.")) {
        return false;
    }

    const std::array<std::int32_t, 16> shippedValues{{
        1, 1, 1, 1, 1, 1, 1, 2, 1, 4, 2, 3, 1, 0, 0, 0,
    }};
    std::istringstream input(encode(shippedValues), std::ios::binary);
    if (!check(
            openshadowflare::loadGameConfig(input, config),
            "The shipped SFlare.Cfg values were rejected.")) {
        return false;
    }
    return check(
        config.windowed_at_start &&
            config.click_priority ==
                std::array<std::int32_t, 5>{{4, 2, 3, 1, 0}},
        "The shipped SFlare.Cfg values were decoded incorrectly.");
}

bool testConfigValidationAndWriting() {
    std::array<std::int32_t, 16> values{{
        -9, -5, 0, 2, 1, 0, 9, 8, -2, 0, 1, 2, 3, 4, -12000, 100,
    }};
    GameConfig config;
    config.unknown_48d540 = false;
    std::istringstream input(encode(values), std::ios::binary);
    if (!check(
            openshadowflare::loadGameConfig(input, config),
            "Validatable config values unexpectedly failed to load.")) {
        return false;
    }
    if (!check(
            config.windowed_at_start &&
                config.semi_transparent_shadow &&
                !config.semi_transparent_objects &&
                config.display_darkness &&
                config.unknown_48d540 &&
                config.save_image_at_game_end &&
                config.click_range == 2 &&
                config.click_range_enabled &&
                config.effect_volume == -10000 &&
                config.bgm_volume == 0,
            "Retail config normalization differs from 0x00401eb0.")) {
        return false;
    }

    std::ostringstream output(std::ios::binary);
    if (!check(
            openshadowflare::saveGameConfig(output, config),
            "Could not write a game config.")) {
        return false;
    }
    return check(
        output.str().size() == openshadowflare::kGameConfigByteSize,
        "SFlare.Cfg output is not exactly 64 bytes.");
}

bool testConfigFailureSideEffects() {
    GameConfig truncated;
    truncated.windowed_at_start = false;
    const std::string onlyScreen = encode(
        std::array<std::int32_t, 16>{{
            7, 1, 1, 1, 1, 1, 1, 2, 1, 4, 2, 3, 1, 0, 0, 0,
        }}).substr(0, 4);
    std::istringstream shortInput(onlyScreen, std::ios::binary);
    if (!check(
            !openshadowflare::loadGameConfig(shortInput, truncated) &&
                truncated.windowed_at_start,
            "A short config did not retain the retail partial mutation.")) {
        return false;
    }

    GameConfig invalidPriority;
    const auto originalPriority = invalidPriority.click_priority;
    std::array<std::int32_t, 16> values{{
        1, 1, 1, 1, 1, 1, 1, 2, 1, 0, 0, 2, 3, 4, -10, -20,
    }};
    std::istringstream invalidInput(encode(values), std::ios::binary);
    return check(
        !openshadowflare::loadGameConfig(invalidInput, invalidPriority) &&
            invalidPriority.click_priority == originalPriority,
        "An invalid click-priority permutation was accepted or copied.");
}

bool testCommandLine() {
    GameConfig config;
    config.windowed_at_start = false;
    openshadowflare::applyRetailCommandLine("/W anything /f /w", config);
    if (!check(
            config.windowed_at_start,
            "Later /w and case-insensitive parsing do not match retail.")) {
        return false;
    }

    config.windowed_at_start = false;
    std::string shiftJis;
    shiftJis.push_back(static_cast<char>(0x81));
    shiftJis += "/w";
    openshadowflare::applyRetailCommandLine(shiftJis, config);
    if (!check(
            !config.windowed_at_start,
            "A Shift-JIS trail byte was incorrectly parsed as a switch.")) {
        return false;
    }

    constexpr char withEmbeddedNul[] = "ignored\0/w";
    openshadowflare::applyRetailCommandLine(
        std::string_view(withEmbeddedNul, sizeof(withEmbeddedNul) - 1),
        config);
    return check(
        !config.windowed_at_start,
        "Command-line parsing continued beyond the first NUL.");
}

bool testStateDispatcher() {
    std::vector<std::string> calls;
    openshadowflare::GameStateDispatcherCallbacks callbacks;
    callbacks.wait_until_renderer_idle =
        [&calls] { calls.emplace_back("wait"); };
    callbacks.title.enter =
        [&calls](std::int32_t value) {
            calls.emplace_back("title enter " + std::to_string(value));
        };
    callbacks.title.leave =
        [&calls] { calls.emplace_back("title leave"); };
    callbacks.loading.enter =
        [&calls](std::int32_t value) {
            calls.emplace_back("loading enter " + std::to_string(value));
        };
    callbacks.loading.leave =
        [&calls] { calls.emplace_back("loading leave"); };
    callbacks.gameplay.enter =
        [&calls](std::int32_t value) {
            calls.emplace_back("gameplay enter " + std::to_string(value));
        };
    callbacks.gameplay.leave =
        [&calls] { calls.emplace_back("gameplay leave"); };

    openshadowflare::GameStateDispatcher dispatcher(std::move(callbacks));
    if (!check(
            dispatcher.currentRetailState() == -1,
            "The retail state dispatcher did not start at -1.")) {
        return false;
    }
    dispatcher.transition(openshadowflare::GameState::title, 99);
    dispatcher.transition(openshadowflare::GameState::loading, 77);
    dispatcher.transition(openshadowflare::GameState::loading, 8);
    dispatcher.transition(9, 123);
    dispatcher.transition(openshadowflare::GameState::gameplay, 42);

    const std::vector<std::string> expected{
        "wait",
        "title enter 0",
        "wait",
        "title leave",
        "loading enter 77",
        "wait",
        "loading leave",
        "loading enter 8",
        "wait",
        "loading leave",
        "wait",
        "gameplay enter 0",
    };
    return check(
        calls == expected &&
            dispatcher.currentState() ==
                openshadowflare::GameState::gameplay,
        "State transition call order differs from 0x004023d0.");
}

}  // namespace

int main() {
    if (!testRetailDefaultsAndFixture() ||
        !testConfigValidationAndWriting() ||
        !testConfigFailureSideEffects() ||
        !testCommandLine() ||
        !testStateDispatcher()) {
        return 1;
    }
    return 0;
}
