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
            if (operand.type == 6 &&
                operand.value == 12000000) {
                return std::int32_t{91467};
            }
            if (operand.type == 7 &&
                operand.value == 12000000) {
                return std::int32_t{1532};
            }
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
                    osf::script::StepResult::waiting_for_message &&
                interpreter.waitingForMessage() &&
                messages.size() == 2 &&
                messages.back().id == 1000001 &&
                interpreter.readTemporaryFlag(1000002) == 2,
            "Ostare's first message did not enter its status-one callback.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::waiting_for_message &&
                messages.size() == 3 &&
                messages.back().id == 1000002 &&
                interpreter.readTemporaryFlag(1000002) == 3,
            "Ostare's second opening message did not follow retail.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::waiting_for_message &&
                messages.size() == 4 &&
                messages.back().id == 1000003 &&
                interpreter.readTemporaryFlag(1000002) == 4 &&
                native_commands.size() == 6 &&
                native_commands[2] ==
                    std::make_pair(
                        std::int32_t{10},
                        std::vector<std::int32_t>{
                            0, 0, 91667, 1532, -1, -1}) &&
                native_commands[3] ==
                    std::make_pair(
                        std::int32_t{10},
                        std::vector<std::int32_t>{
                            1, 1000000, 91467, 1732, -1, -1}) &&
                native_commands[4] ==
                    std::make_pair(
                        std::int32_t{10},
                        std::vector<std::int32_t>{
                            0, 100, 91667, 1332, -1, -1}) &&
                native_commands[5] ==
                    std::make_pair(
                        std::int32_t{10},
                        std::vector<std::int32_t>{
                            4, 0, 91467, 1532, 200, 200}),
            "Ostare's third opening message did not place its retail items.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::waiting_for_message &&
                messages.size() == 5 &&
                messages.back().id == 1000004 &&
                interpreter.readTemporaryFlag(1000002) == 0,
            "Ostare's final opening message did not follow retail.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::complete &&
                !interpreter.waitingForMessage() &&
                native_commands.size() == 7 &&
                native_commands.back() ==
                    std::make_pair(
                        std::int32_t{19},
                        std::vector<std::int32_t>{12000000}),
            "Ostare's opening conversation did not release the actor.")) {
        return false;
    }

    const osf::script::Message* repeated_message =
        script.findMessage(1000005);
    if (!check(
        repeated_message &&
            interpreter.startStatus(0, 12000000) ==
                osf::script::StepResult::waiting_for_message &&
            interpreter.waitingForMessage() &&
            player_level_queries == 1 &&
            messages.size() == 6 &&
            messages.back().id == 1000005 &&
            messages.back().text == repeated_message->text &&
            interpreter.readTemporaryFlag(1000000) ==
                1000005 &&
            interpreter.readTemporaryFlag(1000002) == 30,
        "Ostare's level-one repeat interaction did not follow retail.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::waiting_for_message &&
                messages.size() == 7 &&
                messages.back().id == 1000006 &&
                interpreter.readTemporaryFlag(1000002) == 0,
            "Ostare's repeat callback did not show its second message.")) {
        return false;
    }
    if (!check(
            interpreter.resume() == osf::script::StepResult::complete &&
            !interpreter.waitingForMessage() &&
            native_commands.back() ==
                std::make_pair(
                    std::int32_t{19},
                    std::vector<std::int32_t>{12000000}),
            "Ostare's repeat conversation did not release the actor.")) {
        return false;
    }

    const std::size_t malse_first_command =
        native_commands.size();
    const osf::script::StepResult malse_start =
        interpreter.startStatus(0, 12000001);
    if (!check(
            malse_start ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000019 &&
                messages.back().character_number == 12000001 &&
                interpreter.readTemporaryFlag(1000002) == 20 &&
                native_commands.size() ==
                    malse_first_command + 2 &&
                native_commands[malse_first_command] ==
                    std::make_pair(
                        std::int32_t{18},
                        std::vector<std::int32_t>{12000001}) &&
                native_commands[malse_first_command + 1] ==
                    std::make_pair(
                        std::int32_t{21},
                        std::vector<std::int32_t>{
                            12000001, 0}),
            "Malse's first interaction did not enter its retail script.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000020 &&
                interpreter.readTemporaryFlag(1000002) == 0,
            "Malse's first callback did not follow retail.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::complete &&
                !interpreter.waitingForMessage() &&
                native_commands.back() ==
                    std::make_pair(
                        std::int32_t{19},
                        std::vector<std::int32_t>{12000001}),
            "Malse's opening conversation did not release the actor.")) {
        return false;
    }

    const std::size_t syria_first_command =
        native_commands.size();
    if (!check(
            interpreter.startStatus(0, 12000002) ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000040 &&
                messages.back().character_number == 12000002 &&
                interpreter.readTemporaryFlag(1000002) == 40 &&
                native_commands.size() ==
                    syria_first_command,
            "Syria's first interaction did not enter its retail script.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000041 &&
                interpreter.readTemporaryFlag(1000002) == 0 &&
                native_commands.size() ==
                    syria_first_command + 2 &&
                native_commands[syria_first_command] ==
                    std::make_pair(
                        std::int32_t{62},
                        std::vector<std::int32_t>{0, 1, 0}) &&
                native_commands[syria_first_command + 1] ==
                    std::make_pair(
                        std::int32_t{48},
                        std::vector<std::int32_t>{0}),
            "Syria's callback did not start the retail quest.")) {
        return false;
    }
    return check(
        interpreter.resume() == osf::script::StepResult::complete &&
            !interpreter.waitingForMessage() &&
            native_commands.back() ==
                std::make_pair(
                    std::int32_t{19},
                    std::vector<std::int32_t>{12000002}),
        "Syria's opening conversation did not release the actor.");
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
