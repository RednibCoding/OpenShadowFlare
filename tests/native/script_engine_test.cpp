#include "libs/RKC_RPG_SCRIPT/rkc_rpg_script.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

std::uint64_t operandKey(const osf::script::Operand& operand) {
    return
        (static_cast<std::uint64_t>(
             static_cast<std::uint32_t>(operand.type))
         << 32u) |
        static_cast<std::uint32_t>(operand.value);
}

bool testRetailRemoteTown() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path path =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Scenario" / "00000000" /
        "Scenario.Scs";
    if (!std::filesystem::is_regular_file(path)) {
        return true;
    }

    osf::script::ScriptData script;
    std::string error;
    if (!check(
            script.load(path, &error),
            "The retail Remote Town script no longer decodes.")) {
        std::cerr << error << '\n';
        return false;
    }
    const std::size_t command_count = std::accumulate(
        script.sentences().begin(),
        script.sentences().end(),
        std::size_t{0},
        [](std::size_t count, const osf::script::Sentence& sentence) {
            return count + sentence.commands.size();
        });
    const osf::script::Status* ostare =
        script.findStatus(0, 12000000);
    const osf::script::Message* introduction =
        script.findMessage(1000000);
    if (!check(
            script.version() == "000" &&
                script.temporaryFlags().size() == 66 &&
                script.networkFlags().empty() &&
                script.messages().size() == 61 &&
                script.statuses().size() == 23 &&
                script.sentences().size() == 220 &&
                command_count == 608 &&
                ostare &&
                ostare->sentence == 4 &&
                introduction &&
                introduction->text.rfind(
                    "Thank you for coming. I am Ostare", 0) == 0,
            "The Remote Town script inventory differs from retail.")) {
        return false;
    }

    std::vector<osf::script::MessageEvent> messages;
    std::vector<std::pair<
        std::int32_t,
        std::vector<std::int32_t>>> native_commands;
    std::unordered_map<std::uint64_t, std::int32_t>
        external_values;
    std::int32_t player_level_queries = 0;
    osf::script::Interpreter interpreter({
        [&external_values](const osf::script::Operand& operand) {
            const auto found =
                external_values.find(operandKey(operand));
            return found == external_values.end()
                       ? 0
                       : found->second;
        },
        [&external_values](
            const osf::script::Operand& operand,
            std::int32_t value) {
            external_values.insert_or_assign(
                operandKey(operand), value);
            return true;
        },
        [&messages](const osf::script::MessageEvent& message) {
            messages.push_back(message);
        },
        [&native_commands](
            std::int32_t opcode,
            const std::vector<std::int32_t>& arguments) {
            native_commands.emplace_back(opcode, arguments);
            return true;
        },
        [&player_level_queries](
            osf::script::ValueQuery query,
            std::int32_t& value) {
            if (query !=
                osf::script::ValueQuery::local_player_level) {
                return false;
            }
            ++player_level_queries;
            value = 1;
            return true;
        },
    });
    interpreter.bind(&script);
    if (!check(
            interpreter.startStatus(0, 12000000) ==
                    osf::script::StepResult::waiting_for_message &&
                interpreter.waitingForMessage() &&
                messages.size() == 1 &&
                messages.front().id == 1000000 &&
                messages.front().text == introduction->text &&
                interpreter.readTemporaryFlag(1000000) ==
                    1000000 &&
                interpreter.readTemporaryFlag(1000002) == 1 &&
                native_commands.size() == 2 &&
                native_commands[0].first == 18 &&
                native_commands[0].second ==
                    std::vector<std::int32_t>{12000000} &&
                native_commands[1].first == 21 &&
                native_commands[1].second ==
                    std::vector<std::int32_t>{12000000, 0},
            "Ostare's status-zero sentence did not emit its retail message.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::complete &&
                !interpreter.waitingForMessage(),
            "The first Remote Town conversation did not resume cleanly.")) {
        return false;
    }

    const osf::script::Message* repeated_message =
        script.findMessage(1000005);
    return check(
        repeated_message &&
            interpreter.startStatus(0, 12000000) ==
                osf::script::StepResult::waiting_for_message &&
            interpreter.waitingForMessage() &&
            player_level_queries == 1 &&
            messages.size() == 2 &&
            messages.back().id == 1000005 &&
            messages.back().text == repeated_message->text &&
            interpreter.readTemporaryFlag(1000000) ==
                1000005 &&
            interpreter.readTemporaryFlag(1000002) == 30,
        "Ostare's level-one repeat interaction did not follow retail.");
#else
    return true;
#endif
}

bool testMalformedScript() {
    osf::script::ScriptData script;
    std::vector<std::uint8_t> bytes(16, 0);
    return check(
        !script.decode(bytes),
        "A script with the wrong header was accepted.");
}

}  // namespace

int main() {
    return testRetailRemoteTown() && testMalformedScript() ? 0 : 1;
}
