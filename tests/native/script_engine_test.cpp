#include "libs/RKC_RPG_SCRIPT/rkc_rpg_script.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <iterator>
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
    std::int32_t companion_type_queries = 0;
    std::int32_t play_mode_queries = 0;
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
        [&native_commands, &external_values](
            std::int32_t opcode,
            const std::vector<std::int32_t>& arguments) {
            native_commands.emplace_back(opcode, arguments);
            if (opcode == 62 && arguments.size() >= 2) {
                external_values.insert_or_assign(
                    operandKey({12, arguments[0]}),
                    arguments[1]);
            }
            return true;
        },
        [&player_level_queries,
         &companion_type_queries,
         &play_mode_queries](
            osf::script::ValueQuery query,
            std::int32_t& value) {
            switch (query) {
            case osf::script::ValueQuery::local_player_level:
                ++player_level_queries;
                value = 1;
                return true;
            case osf::script::ValueQuery::
                    local_player_companion_type:
                ++companion_type_queries;
                value = 0;
                return true;
            case osf::script::ValueQuery::play_mode:
                ++play_mode_queries;
                value = 0;
                return true;
            case osf::script::ValueQuery::
                    local_player_current_life:
            case osf::script::ValueQuery::
                    local_player_maximum_life:
            case osf::script::ValueQuery::
                    local_player_current_mana:
            case osf::script::ValueQuery::
                    local_player_maximum_mana:
                value = 100;
                return true;
            case osf::script::ValueQuery::
                    local_player_condition_current:
            case osf::script::ValueQuery::
                    local_player_condition_maximum:
                value = -1;
                return true;
            }
            return false;
        },
        {},
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
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::complete &&
                !interpreter.waitingForMessage() &&
                native_commands.back() ==
                    std::make_pair(
                        std::int32_t{19},
                        std::vector<std::int32_t>{12000002}),
            "Syria's opening conversation did not release the actor.")) {
        return false;
    }
    const std::size_t syria_repeat_command =
        native_commands.size();
    if (!check(
            interpreter.startStatus(0, 12000002) ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000038 &&
                native_commands.size() ==
                    syria_repeat_command + 2 &&
                native_commands[syria_repeat_command] ==
                    std::make_pair(
                        std::int32_t{18},
                        std::vector<std::int32_t>{12000002}) &&
                native_commands[syria_repeat_command + 1] ==
                    std::make_pair(
                        std::int32_t{21},
                        std::vector<std::int32_t>{12000002, 0}) &&
                std::none_of(
                    native_commands.begin() +
                        static_cast<std::ptrdiff_t>(
                            syria_repeat_command),
                    native_commands.end(),
                    [](const auto& command) {
                        return command.first == 62 ||
                               command.first == 48;
                    }),
            "Syria's repeat interaction restarted the Red Goblin quest "
            "instead of entering her normal blessing dialogue.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::complete &&
                native_commands.back() ==
                    std::make_pair(
                        std::int32_t{19},
                        std::vector<std::int32_t>{12000002}),
            "Syria's repeat blessing did not release the actor.")) {
        return false;
    }

    constexpr std::int32_t companion_characters[] = {
        12010000,
        12010001,
        12010002,
        12010003,
    };
    constexpr std::int32_t companion_sentences[] = {
        158,
        173,
        188,
        203,
    };
    for (std::size_t index = 0;
         index < std::size(companion_characters);
         ++index) {
        if (!check(
                interpreter.startSentence(
                    companion_sentences[index], -1) ==
                    osf::script::StepResult::complete,
                "A Remote Town companion activation sentence failed.")) {
            return false;
        }
        const std::int32_t expected = index == 0 ? 0 : 1;
        for (const std::int32_t base :
             {100000000, 200000000, 300000000}) {
            const osf::script::Operand state_operand{
                5, base + companion_characters[index]};
            if (!check(
                    external_values[operandKey(state_operand)] ==
                        expected,
                    "A companion script did not update all three "
                    "retail entity state channels.")) {
                return false;
            }
        }
    }
    if (!check(
            companion_type_queries == 4 &&
                play_mode_queries == 4,
            "Companion activation did not query the retail player "
            "companion and single-player mode values.")) {
        return false;
    }

    if (!check(
            interpreter.startStatus(0, 12010000) ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000047 &&
                messages.back().selection_required &&
                messages.back().initial_selection == 3 &&
                messages.back().text.find("~Check Status~") !=
                    std::string::npos,
            "Kerberos's status did not open the retail choice message.")) {
        return false;
    }
    if (!check(
            interpreter.resume(3) ==
                    osf::script::StepResult::complete &&
                !interpreter.waitingForMessage() &&
                interpreter.readTemporaryFlag(1000001) == 3,
            "Kerberos's QUIT choice was not written back to the script.")) {
        return false;
    }

    if (!check(
            interpreter.startStatus(0, 12010003) ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000056 &&
                messages.back().selection_required,
            "Harley's status did not open its retail choice message.")) {
        return false;
    }
    if (!check(
            interpreter.resume(1) ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000057 &&
                !messages.back().selection_required,
            "Harley's first explanation line remained a choice message.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000058 &&
                !messages.back().selection_required,
            "Harley's explanation did not advance to its second line.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::complete &&
                !interpreter.waitingForMessage() &&
                native_commands.back() ==
                    std::make_pair(
                        std::int32_t{19},
                        std::vector<std::int32_t>{12010003}),
            "Harley's completed explanation did not release him.")) {
        return false;
    }

    if (!check(
            interpreter.startStatus(3, 10000000) ==
                    osf::script::StepResult::complete &&
                native_commands.back() ==
                    std::make_pair(
                        std::int32_t{17},
                        std::vector<std::int32_t>{1, 0}),
            "The Remote Town south-gate trigger did not emit its "
            "authored scenario and entry.")) {
        return false;
    }

    if (!check(
            interpreter.startStatus(0, 10000200) ==
                    osf::script::StepResult::complete &&
                native_commands.back() ==
                    std::make_pair(
                        std::int32_t{37},
                        std::vector<std::int32_t>{0}),
            "The Remote Town transport object did not emit opcode 37.")) {
        return false;
    }
    return check(
        interpreter.startStatus(0, 10000300) ==
                osf::script::StepResult::complete &&
            native_commands.back() ==
                std::make_pair(
                    std::int32_t{41},
                    std::vector<std::int32_t>{0}),
        "The Warehouse object did not emit opcode 41.");
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

bool testRetailOutdoorChestScript() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path path =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Scenario" / "00000001" /
        "Scenario.Scs";
    if (!std::filesystem::is_regular_file(path)) {
        return true;
    }
    osf::script::ScriptData script;
    std::string error;
    if (!script.load(path, &error)) {
        std::cerr << error << '\n';
        return false;
    }

    std::unordered_map<std::uint64_t, std::int32_t> values;
    std::vector<std::pair<
        std::int32_t,
        std::vector<std::int32_t>>> native_commands;
    osf::script::Interpreter interpreter({
        [&values](const osf::script::Operand& operand) {
            if (operand.type == 6) {
                return std::int32_t{80801};
            }
            if (operand.type == 7) {
                return std::int32_t{1832};
            }
            const auto found = values.find(operandKey(operand));
            return found == values.end()
                ? std::int32_t{0}
                : found->second;
        },
        [&values](
            const osf::script::Operand& operand,
            std::int32_t value) {
            values.insert_or_assign(
                operandKey(operand), value);
            return true;
        },
        {},
        [&native_commands](
            std::int32_t opcode,
            const std::vector<std::int32_t>& arguments) {
            native_commands.emplace_back(opcode, arguments);
            return true;
        },
        [](osf::script::ValueQuery, std::int32_t& value) {
            value = 0;
            return true;
        },
        {},
    });
    interpreter.bind(&script);
    const osf::script::StepResult result =
        interpreter.startStatus(0, 10020000);
    const bool opened =
        values[operandKey({5, 110020000})] == 0 &&
        values[operandKey({5, 110020001})] == 1;
    const bool played_sound =
        std::any_of(
            native_commands.begin(),
            native_commands.end(),
            [](const auto& command) {
                return command.first == 16 &&
                       command.second ==
                           std::vector<std::int32_t>{
                               77, 0, 80801, 1832};
            });
    const bool created_loot =
        std::any_of(
            native_commands.begin(),
            native_commands.end(),
            [](const auto& command) {
                return command.first == 24 &&
                       command.second ==
                           std::vector<std::int32_t>{
                               4, 81001, 2032};
            });
    return check(
        result == osf::script::StepResult::complete &&
            opened &&
            played_sound &&
            created_loot,
        "The first outdoor chest did not swap its authored objects, "
        "play its sound, and emit its item callback.");
#else
    return true;
#endif
}

bool testRetailRedGoblinDeathStatus() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path path =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Scenario" / "00000001" /
        "Scenario.Scs";
    if (!std::filesystem::is_regular_file(path)) {
        return true;
    }
    osf::script::ScriptData script;
    std::string error;
    if (!script.load(path, &error)) {
        std::cerr << error << '\n';
        return false;
    }

    std::vector<std::pair<
        std::int32_t,
        std::vector<std::int32_t>>> native_commands;
    osf::script::Interpreter interpreter({
        [](const osf::script::Operand&) {
            return std::int32_t{0};
        },
        {},
        {},
        [&native_commands](
            std::int32_t opcode,
            const std::vector<std::int32_t>& arguments) {
            native_commands.emplace_back(opcode, arguments);
            return true;
        },
        [](osf::script::ValueQuery query, std::int32_t& value) {
            if (query != osf::script::ValueQuery::play_mode) {
                return false;
            }
            value = 0;
            return true;
        },
        {},
    });
    interpreter.bind(&script);
    return check(
        interpreter.startStatus(4, 14010000) ==
                osf::script::StepResult::complete &&
            native_commands ==
                std::vector<std::pair<
                    std::int32_t,
                    std::vector<std::int32_t>>>{
                    {62, {0, 2, 1}},
                },
        "The Red Goblin death status did not emit its authored quest "
        "completion update.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    return testRetailRemoteTown() &&
                   testRetailOutdoorChestScript() &&
                   testRetailRedGoblinDeathStatus() &&
                   testMalformedScript()
               ? 0
               : 1;
}
