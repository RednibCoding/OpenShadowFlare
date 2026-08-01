#include "libs/RKC_RPG_SCRIPT/rkc_rpg_script.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
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

void appendI32(
    std::vector<std::uint8_t>& bytes,
    std::int32_t value) {
    const std::uint32_t raw = static_cast<std::uint32_t>(value);
    for (std::uint32_t shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::uint8_t>(raw >> shift));
    }
}

osf::script::ScriptData makeRandomCommandScript() {
    std::vector<std::uint8_t> bytes{
        'S', 'c', 'e', 'n', 'a', 'S', 'c', 'r',
        'i', 'p', 't', 'V', '0', '0', '0', '\0',
    };
    appendI32(bytes, 3);
    for (std::int32_t id = 1000000; id <= 1000002; ++id) {
        appendI32(bytes, id);
        appendI32(bytes, -1);
    }
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 1);
    appendI32(bytes, 3);
    for (std::int32_t destination = 1000000;
         destination <= 1000002;
         ++destination) {
        appendI32(bytes, 39);
        appendI32(bytes, 3);
        appendI32(bytes, 1);
        appendI32(bytes, 20);
        appendI32(bytes, 1);
        appendI32(bytes, 40);
        appendI32(bytes, 4);
        appendI32(bytes, destination);
    }
    osf::script::ScriptData script;
    script.decode(bytes);
    return script;
}

osf::script::ScriptData makeArithmeticCommandScript() {
    std::vector<std::uint8_t> bytes{
        'S', 'c', 'e', 'n', 'a', 'S', 'c', 'r',
        'i', 'p', 't', 'V', '0', '0', '0', '\0',
    };
    const std::array<std::int32_t, 4> initial_values{{
        std::numeric_limits<std::int32_t>::max(),
        -7,
        -7,
        123,
    }};
    appendI32(bytes, static_cast<std::int32_t>(initial_values.size()));
    for (std::size_t index = 0;
         index < initial_values.size(); ++index) {
        appendI32(bytes, 1000000 + static_cast<std::int32_t>(index));
        appendI32(bytes, initial_values[index]);
    }
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 1);
    appendI32(bytes, 5);
    const std::array<std::array<std::int32_t, 3>, 5> commands{{
        {{13, 1000000, 2}},
        {{14, 1000001, 3}},
        {{15, 1000002, 3}},
        {{14, 1000003, 0}},
        {{15, 1000003, 0}},
    }};
    for (const auto& command : commands) {
        appendI32(bytes, command[0]);
        appendI32(bytes, 2);
        appendI32(bytes, 4);
        appendI32(bytes, command[1]);
        appendI32(bytes, 1);
        appendI32(bytes, command[2]);
    }
    osf::script::ScriptData script;
    script.decode(bytes);
    return script;
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
    std::int32_t companion_status_queries = 0;
    std::int32_t companion_status_type = -1;
    std::int32_t player_gold = 200;
    std::int32_t player_current_life = 100;
    std::int32_t player_maximum_life = 100;
    std::int32_t player_current_mana = 100;
    std::int32_t player_maximum_mana = 100;
    bool has_unidentified_items = true;
    std::int32_t teleporter_distance = 0;
    std::array<std::int32_t, 6> repair_prices{{
        30, 10, 20, 15, 5, 40,
    }};
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
         &play_mode_queries,
         &player_gold,
         &player_current_life,
         &player_maximum_life,
         &player_current_mana,
         &player_maximum_mana,
         &has_unidentified_items](
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
                value = player_current_life;
                return true;
            case osf::script::ValueQuery::
                    local_player_maximum_life:
                value = player_maximum_life;
                return true;
            case osf::script::ValueQuery::
                    local_player_current_mana:
                value = player_current_mana;
                return true;
            case osf::script::ValueQuery::
                    local_player_maximum_mana:
                value = player_maximum_mana;
                return true;
            case osf::script::ValueQuery::
                    local_player_condition_current:
            case osf::script::ValueQuery::
                    local_player_condition_maximum:
                value = -1;
                return true;
            case osf::script::ValueQuery::local_player_gold:
                value = player_gold;
                return true;
            case osf::script::ValueQuery::
                    local_player_has_unidentified_items:
                value = has_unidentified_items ? 1 : 0;
                return true;
            case osf::script::ValueQuery::
                    local_player_repair_price:
            case osf::script::ValueQuery::
                    local_player_spell_learned:
            case osf::script::ValueQuery::
                    local_player_job_selection:
            case osf::script::ValueQuery::scenario_entry_value:
            case osf::script::ValueQuery::blackjack_result:
                return false;
            }
            return false;
        },
        [&repair_prices](
            osf::script::ValueQuery query,
            std::int32_t index,
            std::int32_t& value) {
            if (query != osf::script::ValueQuery::
                    local_player_repair_price ||
                index < -1 || index > 4) {
                return false;
            }
            value = repair_prices[
                index < 0
                    ? repair_prices.size() - 1u
                    : static_cast<std::size_t>(index)];
            return true;
        },
        [&teleporter_distance](
            std::int32_t character_number,
            std::int32_t& distance) {
            if (character_number != 10000202) {
                return false;
            }
            distance = teleporter_distance;
            return true;
        },
        {},
        {},
        [&companion_status_queries, &companion_status_type](
            std::int32_t type,
            std::string& message) {
            ++companion_status_queries;
            companion_status_type = type;
            message = "Kerberos\n\nLevel              1\n";
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

    external_values.insert_or_assign(
        operandKey({12, 0}), 2);
    if (!check(
            interpreter.startStatus(0, 12000001) ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000021,
            "Malse did not enter his authored merchant branch after the "
            "Red Goblin quest completed.")) {
        return false;
    }

    external_values.insert_or_assign(
        operandKey({12, 3}), 2);
    const auto malse_menu_result =
        interpreter.startSentence(45, 12000001);
    if (!check(
            malse_menu_result ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000013 &&
                messages.back().selection_required &&
                messages.back().initial_selection == 3 &&
                messages.back().text.find("~Trade") !=
                    std::string::npos,
            "Malse's post-quest interaction did not open his authored "
            "service choices.")) {
        return false;
    }
    const std::size_t malse_trade_command =
        native_commands.size();
    const auto malse_trade_result = interpreter.resume(0);
    if (!check(
            malse_trade_result ==
                    osf::script::StepResult::complete &&
                native_commands.size() == malse_trade_command + 2 &&
                native_commands[malse_trade_command] ==
                    std::make_pair(
                        std::int32_t{5},
                        std::vector<std::int32_t>{0}) &&
                native_commands[malse_trade_command + 1] ==
                    std::make_pair(
                        std::int32_t{19},
                        std::vector<std::int32_t>{12000001}),
            "Malse's Trade choice did not open vendor inventory zero and "
            "release the actor.")) {
        return false;
    }

    const std::size_t identify_command = native_commands.size();
    if (!check(
            interpreter.startSentence(45, 12000001) ==
                    osf::script::StepResult::waiting_for_message &&
                interpreter.resume(1) ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000017 &&
                messages.back().text.find(
                    "It costs 100 gold") != std::string::npos &&
                messages.back().selection_required &&
                messages.back().initial_selection == 1,
            "Malse's Identify choice did not query owned items and "
            "format its retail confirmation.")) {
        return false;
    }
    if (!check(
            interpreter.resume(0) ==
                    osf::script::StepResult::complete &&
                native_commands.size() == identify_command + 3 &&
                native_commands[identify_command] ==
                    std::make_pair(
                        std::int32_t{54},
                        std::vector<std::int32_t>{100}) &&
                native_commands[identify_command + 1] ==
                    std::make_pair(
                        std::int32_t{4},
                        std::vector<std::int32_t>{}) &&
                native_commands[identify_command + 2] ==
                    std::make_pair(
                        std::int32_t{19},
                        std::vector<std::int32_t>{12000001}),
            "Confirming Malse's Identify service did not spend 100 Gold, "
            "identify the owned items, and release him.")) {
        return false;
    }

    player_gold = 200;
    has_unidentified_items = true;
    const std::size_t repair_command = native_commands.size();
    if (!check(
            interpreter.startSentence(45, 12000001) ==
                    osf::script::StepResult::waiting_for_message &&
                interpreter.resume(2) ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000014 &&
                messages.back().selection_required &&
                messages.back().initial_selection == 7 &&
                messages.back().text.find("30 Gold") !=
                    std::string::npos &&
                messages.back().text.find("40 Gold") !=
                    std::string::npos,
            "Malse's Repair choice did not format the seven retail "
            "repair prices.")) {
        return false;
    }
    const osf::script::StepResult arms_repair_result =
        interpreter.resume(0);
    if (!check(
            arms_repair_result ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000014 &&
                native_commands.size() == repair_command + 2 &&
                native_commands[repair_command] ==
                    std::make_pair(
                        std::int32_t{9},
                        std::vector<std::int32_t>{0}) &&
                native_commands[repair_command + 1] ==
                    std::make_pair(
                        std::int32_t{54},
                        std::vector<std::int32_t>{30}),
            "Malse's Arms repair did not repair the retail equipment "
            "group and spend its quoted Gold.")) {
        return false;
    }

    repair_prices[1] = 0;
    const std::size_t repaired_item_command = native_commands.size();
    if (!check(
            interpreter.resume(1) ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000016 &&
                native_commands.size() == repaired_item_command,
            "Malse did not reject an already repaired equipment group "
            "without spending Gold.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000014,
            "Malse did not return from the already-repaired message.")) {
        return false;
    }

    player_gold = 4;
    const std::size_t poor_repair_command = native_commands.size();
    if (!check(
            interpreter.resume(4) ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000015 &&
                native_commands.size() == poor_repair_command,
            "Malse repaired Leg Armor when the player could not afford "
            "the quoted price.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000014,
            "Malse did not return from the insufficient-Gold message.")) {
        return false;
    }

    player_gold = 200;
    repair_prices[1] = 10;
    const std::size_t all_repair_command = native_commands.size();
    if (!check(
            interpreter.resume(5) ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000014 &&
                native_commands.size() == all_repair_command + 6 &&
                std::equal(
                    native_commands.begin() +
                        static_cast<std::ptrdiff_t>(all_repair_command),
                    native_commands.begin() +
                        static_cast<std::ptrdiff_t>(all_repair_command + 5),
                    std::array<std::pair<
                        std::int32_t,
                        std::vector<std::int32_t>>, 5>{{
                        {9, {0}}, {9, {1}}, {9, {2}},
                        {9, {3}}, {9, {4}},
                    }}.begin()) &&
                native_commands.back() ==
                    std::make_pair(
                        std::int32_t{54},
                        std::vector<std::int32_t>{80}),
            "Malse's All Equipped service did not repair every retail "
            "group and charge their sum.")) {
        return false;
    }

    const std::size_t backpack_repair_command = native_commands.size();
    if (!check(
            interpreter.resume(6) ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000014 &&
                native_commands.size() == backpack_repair_command + 2 &&
                native_commands[backpack_repair_command] ==
                    std::make_pair(
                        std::int32_t{9},
                        std::vector<std::int32_t>{-1}) &&
                native_commands.back() ==
                    std::make_pair(
                        std::int32_t{54},
                        std::vector<std::int32_t>{40}),
            "Malse's Non-Equipped service did not repair the backpack "
            "and charge its quoted price.")) {
        return false;
    }
    if (!check(
            interpreter.resume(7) ==
                    osf::script::StepResult::complete &&
                native_commands.back() ==
                    std::make_pair(
                        std::int32_t{19},
                        std::vector<std::int32_t>{12000001}),
            "Malse's Repair QUIT choice did not release the actor.")) {
        return false;
    }
    player_gold = 99;
    const std::size_t poor_identify_command = native_commands.size();
    if (!check(
            interpreter.startSentence(45, 12000001) ==
                    osf::script::StepResult::waiting_for_message &&
                interpreter.resume(1) ==
                    osf::script::StepResult::waiting_for_message &&
                interpreter.resume(0) ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000015 &&
                interpreter.resume() ==
                    osf::script::StepResult::complete &&
                native_commands.size() == poor_identify_command + 1 &&
                native_commands.back() ==
                    std::make_pair(
                        std::int32_t{19},
                        std::vector<std::int32_t>{12000001}),
            "Malse's Identify confirmation did not reject a player with "
            "less than 100 Gold before mutating items.")) {
        return false;
    }

    has_unidentified_items = false;
    const std::size_t no_identify_command = native_commands.size();
    if (!check(
            interpreter.startSentence(45, 12000001) ==
                    osf::script::StepResult::waiting_for_message &&
                interpreter.resume(1) ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000018 &&
                interpreter.resume() ==
                    osf::script::StepResult::complete &&
                native_commands.size() == no_identify_command + 1 &&
                native_commands.back() ==
                    std::make_pair(
                        std::int32_t{19},
                        std::vector<std::int32_t>{12000001}),
            "Malse's already-identified branch did not show its retail "
            "message and leave Gold untouched.")) {
        return false;
    }

    external_values.insert_or_assign(
        operandKey({12, 0}), 0);

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

    player_current_life = 50;
    player_current_mana = 75;
    const std::size_t syria_blessing_command =
        native_commands.size();
    if (!check(
            interpreter.startStatus(0, 12000002) ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().id == 1000037 &&
                native_commands.size() ==
                    syria_blessing_command + 2 &&
                native_commands[syria_blessing_command] ==
                    std::make_pair(
                        std::int32_t{18},
                        std::vector<std::int32_t>{12000002}) &&
                native_commands[syria_blessing_command + 1] ==
                    std::make_pair(
                        std::int32_t{21},
                        std::vector<std::int32_t>{12000002, 0}),
            "Syria did not select her retail recovery message for a "
            "wounded player.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::complete &&
                native_commands.size() ==
                    syria_blessing_command + 7 &&
                native_commands[syria_blessing_command + 2] ==
                    std::make_pair(
                        std::int32_t{19},
                        std::vector<std::int32_t>{12000002}) &&
                native_commands[syria_blessing_command + 3] ==
                    std::make_pair(
                        std::int32_t{20},
                        std::vector<std::int32_t>{
                            12000002, 4, -1, -1, -1, -1}) &&
                native_commands[syria_blessing_command + 4] ==
                    std::make_pair(
                        std::int32_t{7},
                        std::vector<std::int32_t>{}) &&
                native_commands[syria_blessing_command + 5] ==
                    std::make_pair(
                        std::int32_t{8},
                        std::vector<std::int32_t>{}) &&
                native_commands[syria_blessing_command + 6].first == 16,
            "Syria's callback did not emit the retail PEOPLE action, "
            "life recovery, mana recovery, and sample commands.")) {
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
            interpreter.resume(0) ==
                    osf::script::StepResult::waiting_for_message &&
                interpreter.waitingForMessage() &&
                messages.back().id == -1 &&
                messages.back().text ==
                    "Kerberos\n\nLevel              1\n" &&
                !messages.back().selection_required &&
                messages.back().character_number == 12010000 &&
                companion_status_queries == 1 &&
                companion_status_type == 0,
            "Kerberos's Check Status choice did not run retail opcode 3.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::complete &&
                !interpreter.waitingForMessage() &&
                interpreter.readTemporaryFlag(1000001) == -1 &&
                native_commands.back() ==
                    std::make_pair(
                        std::int32_t{19},
                        std::vector<std::int32_t>{12010000}),
            "Closing Kerberos's status did not write its result and "
            "release the actor through status one.")) {
        return false;
    }

    const std::size_t companion_swap_command =
        native_commands.size();
    const osf::script::StepResult gravity_swap_start =
        interpreter.startStatus(0, 12010001);
    const osf::script::StepResult gravity_swap_result =
        interpreter.resume(2);
    if (!check(
            gravity_swap_start ==
                    osf::script::StepResult::waiting_for_message &&
                messages.back().selection_required &&
                gravity_swap_result ==
                    osf::script::StepResult::complete &&
                native_commands.size() ==
                    companion_swap_command + 4 &&
                native_commands[companion_swap_command + 2] ==
                    std::make_pair(
                        std::int32_t{45},
                        std::vector<std::int32_t>{1}) &&
                native_commands[companion_swap_command + 3] ==
                    std::make_pair(
                        std::int32_t{19},
                        std::vector<std::int32_t>{12010001}),
            "Gravity's Swap Dogs choice did not invoke retail opcode 45 "
            "before releasing the actor.")) {
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
                !messages.back().selection_required &&
                interpreter.readTemporaryFlag(1000001) == -1,
            "Harley's explanation did not advance to its second line.")) {
        return false;
    }
    if (!check(
            interpreter.resume() ==
                    osf::script::StepResult::complete &&
                !interpreter.waitingForMessage() &&
                interpreter.readTemporaryFlag(1000001) == -1 &&
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

    const osf::script::Message* transport_label =
        script.findMessage(1000060);
    const std::size_t transport_near_command =
        native_commands.size();
    if (!check(
            transport_label &&
                transport_label->text == "Remote Town\n" &&
                interpreter.startStatus(5, -1) ==
                    osf::script::StepResult::complete &&
                interpreter.readTemporaryFlag(1000039) == 50 &&
                interpreter.readTemporaryFlag(1000040) == 1 &&
                external_values[operandKey({10, 0})] == 1 &&
                native_commands.size() ==
                    transport_near_command + 4 &&
                native_commands[transport_near_command] ==
                    std::make_pair(
                        std::int32_t{16},
                        std::vector<std::int32_t>{80, 0, 0, 0}) &&
                native_commands[transport_near_command + 1] ==
                    std::make_pair(
                        std::int32_t{27},
                        std::vector<std::int32_t>{
                            10000200, 0, -160, 1000060,
                            224, 224, 224, 1000}) &&
                native_commands[transport_near_command + 2] ==
                    std::make_pair(
                        std::int32_t{46},
                        std::vector<std::int32_t>{10000203, 50}) &&
                native_commands[transport_near_command + 3] ==
                    std::make_pair(
                        std::int32_t{46},
                        std::vector<std::int32_t>{10000204, 50}),
            "Remote Town's transport point did not enable, sound, label, "
            "and begin both authored object fades.")) {
        return false;
    }
    teleporter_distance = 10;
    const std::size_t transport_away_command =
        native_commands.size();
    if (!check(
            interpreter.startStatus(5, -1) ==
                    osf::script::StepResult::complete &&
                interpreter.readTemporaryFlag(1000039) == 0 &&
                interpreter.readTemporaryFlag(1000040) == 0 &&
                native_commands.size() ==
                    transport_away_command + 3 &&
                native_commands[transport_away_command] ==
                    std::make_pair(
                        std::int32_t{38},
                        std::vector<std::int32_t>{0}) &&
                native_commands[transport_away_command + 1] ==
                    std::make_pair(
                        std::int32_t{46},
                        std::vector<std::int32_t>{10000203, 0}) &&
                native_commands[transport_away_command + 2] ==
                    std::make_pair(
                        std::int32_t{46},
                        std::vector<std::int32_t>{10000204, 0}),
            "Leaving Remote Town's transport point did not reset its "
            "sound latch, fade objects, and close the matching service.")) {
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
        {},
        {},
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
        {},
        {},
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

bool testRetailGiantWarehouseScript() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path path =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Scenario" / "99000013" /
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
        [](const osf::script::Operand&) { return std::int32_t{0}; },
        [](const osf::script::Operand&, std::int32_t) { return true; },
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
        {},
        {},
        {},
    });
    interpreter.bind(&script);
    return check(
        interpreter.startStatus(0, 10000900) ==
                osf::script::StepResult::complete &&
            native_commands ==
                std::vector<std::pair<
                    std::int32_t,
                    std::vector<std::int32_t>>>{{41, {1}}},
        "Tower of Ordeal 12F's Giant Warehouse did not emit the "
        "nonzero opcode-41 owner branch.");
#else
    return true;
#endif
}

bool testRetailItemOwnershipCommands() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Scenario";
    if (!std::filesystem::is_regular_file(
            root / "00000000" / "Scenario.Scs") ||
        !std::filesystem::is_regular_file(
            root / "04900001" / "Scenario.Scs") ||
        !std::filesystem::is_regular_file(
            root / "04100000" / "Scenario.Scs")) {
        return true;
    }

    osf::script::ScriptData remote_town;
    osf::script::ScriptData angel_memory;
    osf::script::ScriptData spell_reward;
    std::string error;
    if (!remote_town.load(
            root / "00000000" / "Scenario.Scs", &error) ||
        !angel_memory.load(
            root / "04900001" / "Scenario.Scs", &error) ||
        !spell_reward.load(
            root / "04100000" / "Scenario.Scs", &error)) {
        std::cerr << error << '\n';
        return false;
    }

    std::vector<std::pair<
        std::int32_t,
        std::vector<std::int32_t>>> native_commands;
    std::vector<std::pair<std::int32_t, std::int32_t>> queries;
    std::vector<std::int32_t> spell_queries;
    osf::script::Interpreter interpreter({
        {},
        [](const osf::script::Operand&, std::int32_t) {
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
        [&spell_queries](
            osf::script::ValueQuery query,
            std::int32_t index,
            std::int32_t& value) {
            if (query != osf::script::ValueQuery::
                    local_player_spell_learned) {
                return false;
            }
            spell_queries.push_back(index);
            value = 1;
            return true;
        },
        {},
        [&queries](
            std::int32_t category,
            std::int32_t definition_id,
            bool& present) {
            queries.emplace_back(category, definition_id);
            present = true;
            return true;
        },
        {},
    });
    interpreter.bind(&remote_town);
    const osf::script::StepResult query_result =
        interpreter.startSentence(36, -1);
    const std::int32_t query_flag =
        interpreter.readTemporaryFlag(1000005);
    const osf::script::StepResult remove_result =
        interpreter.startSentence(37, -1);
    const auto removed = std::find(
        native_commands.begin(),
        native_commands.end(),
        std::pair<std::int32_t, std::vector<std::int32_t>>{
            59, {4, 99000000}});
    if (!check(
            query_result !=
                    osf::script::StepResult::unsupported_command &&
                query_result !=
                    osf::script::StepResult::invalid_script &&
                queries == std::vector<std::pair<
                    std::int32_t, std::int32_t>>{{4, 99000000}} &&
                query_flag == 1 &&
                remove_result !=
                    osf::script::StepResult::unsupported_command &&
                remove_result !=
                    osf::script::StepResult::invalid_script &&
                removed != native_commands.end(),
            "Remote Town's item query/remove commands differ from retail.")) {
        return false;
    }

    native_commands.clear();
    interpreter.bind(&angel_memory);
    const osf::script::StepResult grant_result =
        interpreter.startSentence(30, -1);
    const auto granted = std::find(
        native_commands.begin(),
        native_commands.end(),
        std::pair<std::int32_t, std::vector<std::int32_t>>{
            75, {4, 98000001}});
    const auto experience = std::find(
        native_commands.begin(),
        native_commands.end(),
        std::pair<std::int32_t, std::vector<std::int32_t>>{
            68, {50}});
    if (!check(
            grant_result == osf::script::StepResult::complete &&
            granted != native_commands.end() &&
            experience != native_commands.end(),
            "The shipped Spirit Stone reward did not emit retail opcodes "
            "75 and 68.")) {
        return false;
    }

    native_commands.clear();
    interpreter.bind(&spell_reward);
    const osf::script::StepResult query_spell_result =
        interpreter.startSentence(3, -1);
    const std::int32_t learned_flag =
        interpreter.readTemporaryFlag(1000005);
    const osf::script::StepResult learn_spell_result =
        interpreter.startSentence(13, -1);
    const bool emitted_learn = std::any_of(
        native_commands.begin(),
        native_commands.end(),
        [](const auto& command) {
            return command.first == 67 &&
                   command.second.size() == 1;
        });
    return check(
        query_spell_result !=
                osf::script::StepResult::unsupported_command &&
            query_spell_result !=
                osf::script::StepResult::invalid_script &&
            !spell_queries.empty() &&
            learned_flag == 1 &&
            learn_spell_result == osf::script::StepResult::complete &&
            emitted_learn,
        "The shipped spell query/reward path did not execute retail "
        "opcodes 69 and 67.");
#else
    return true;
#endif
}

bool testRetailJobCommands() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path path =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Scenario" / "03900003" /
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
    std::int32_t job_queries = 0;
    osf::script::Interpreter interpreter({
        {},
        {},
        {},
        [&native_commands](
            std::int32_t opcode,
            const std::vector<std::int32_t>& arguments) {
            native_commands.emplace_back(opcode, arguments);
            return true;
        },
        [&job_queries](
            osf::script::ValueQuery query,
            std::int32_t& value) {
            if (query != osf::script::ValueQuery::
                    local_player_job_selection) {
                return false;
            }
            ++job_queries;
            value = 2;
            return true;
        },
        {},
        {},
        {},
        {},
    });
    interpreter.bind(&script);
    const osf::script::StepResult query_result =
        interpreter.startSentence(366, -1);
    if (!check(
            query_result == osf::script::StepResult::complete &&
                job_queries == 1 &&
                interpreter.readTemporaryFlag(1000014) == 2,
            "The shipped job query did not execute retail opcode 71.")) {
        return false;
    }

    native_commands.clear();
    const osf::script::StepResult change_result =
        interpreter.startSentence(381, -1);
    return check(
        change_result == osf::script::StepResult::complete &&
            native_commands.size() == 2 &&
            native_commands[0] ==
                std::make_pair(
                    std::int32_t{70},
                    std::vector<std::int32_t>{1}) &&
            native_commands[1].first == 16,
        "The shipped job-change path did not emit retail opcode 70.");
#else
    return true;
#endif
}

bool testRetailEquipmentColorCommand() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path path =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Scenario" / "01000000" /
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
        std::vector<std::int32_t>>> commands;
    osf::script::Interpreter interpreter({
        {},
        {},
        {},
        [&commands](
            std::int32_t opcode,
            const std::vector<std::int32_t>& arguments) {
            commands.emplace_back(opcode, arguments);
            return true;
        },
        {},
        {},
        {},
        {},
        {},
    });
    interpreter.bind(&script);
    return check(
        interpreter.startSentence(212, -1) ==
                osf::script::StepResult::complete &&
            commands == std::vector<std::pair<
                std::int32_t,
                std::vector<std::int32_t>>>{{72, {}}},
        "The shipped equipment-color service did not emit retail "
        "opcode 72 without operands.");
#else
    return true;
#endif
}

bool testRetailBlackjackCommands() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path path =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Scenario" / "99000018" /
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
        std::vector<std::int32_t>>> commands;
    std::unordered_map<std::uint64_t, std::int32_t> values;
    std::int32_t result_queries = 0;
    osf::script::Interpreter interpreter({
        [&values](const osf::script::Operand& operand) {
            const auto found = values.find(operandKey(operand));
            return found == values.end() ? 0 : found->second;
        },
        [&values](
            const osf::script::Operand& operand,
            std::int32_t value) {
            values.insert_or_assign(operandKey(operand), value);
            return true;
        },
        {},
        [&commands](
            std::int32_t opcode,
            const std::vector<std::int32_t>& arguments) {
            commands.emplace_back(opcode, arguments);
            return true;
        },
        [&result_queries](
            osf::script::ValueQuery query,
            std::int32_t& value) {
            if (query !=
                osf::script::ValueQuery::blackjack_result) {
                return false;
            }
            ++result_queries;
            value = 2;
            return true;
        },
        {},
        {},
        {},
        {},
    });
    interpreter.bind(&script);
    if (!check(
            interpreter.startSentence(22, -1) ==
                    osf::script::StepResult::complete &&
                commands.size() == 2 &&
                commands[0].first == 54 &&
                commands[1] ==
                    std::make_pair(
                        std::int32_t{73},
                        std::vector<std::int32_t>{}),
            "The shipped Blackjack launch did not emit opcode 73 "
            "without operands.")) {
        return false;
    }
    const osf::script::StepResult result =
        interpreter.startSentence(31, -1);
    return check(
        result_queries == 1 &&
            interpreter.readTemporaryFlag(1000004) == 2 &&
            (result == osf::script::StepResult::complete ||
             result ==
                 osf::script::StepResult::waiting_for_message),
        "Opcode 74 did not publish the dealer-win result to the "
        "shipped outcome branch.");
#else
    return true;
#endif
}

bool testRetailScenarioEntryAndCaptionCommands() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path path =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Scenario" / "00010000" /
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
    osf::script::InterpreterHooks hooks;
    hooks.read_operand = [&values](
                             const osf::script::Operand& operand) {
        const auto found = values.find(operandKey(operand));
        return found == values.end() ? 0 : found->second;
    };
    hooks.write_operand = [&values](
                              const osf::script::Operand& operand,
                              std::int32_t value) {
        values.insert_or_assign(operandKey(operand), value);
        return true;
    };
    hooks.query_value = [](osf::script::ValueQuery query,
                           std::int32_t& value) {
        if (query !=
            osf::script::ValueQuery::scenario_entry_value) {
            return false;
        }
        value = 2;
        return true;
    };
    osf::script::Interpreter interpreter(std::move(hooks));
    interpreter.bind(&script);
    const osf::script::StepResult result =
        interpreter.startStatus(7, -1);
    const osf::script::ScenarioCaptionEvent& caption =
        interpreter.caption();
    return check(
        result == osf::script::StepResult::complete &&
            interpreter.readTemporaryFlag(1000003) == 2 &&
            caption.id == 1000001 &&
            caption.text == "Dusty Ruins, B2F\n",
        "Opcodes 49 and 50 did not select the retail Dusty Ruins "
        "caption from entry two.");
#else
    return true;
#endif
}

bool testRetailScenarioEntityOverrideCommand() {
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
    using NativeCall =
        std::pair<std::int32_t, std::vector<std::int32_t>>;
    std::vector<NativeCall> calls;
    osf::script::InterpreterHooks hooks;
    hooks.native_command = [&calls](
                               std::int32_t opcode,
                               const std::vector<std::int32_t>& arguments) {
        calls.emplace_back(opcode, arguments);
        return true;
    };
    osf::script::Interpreter interpreter(std::move(hooks));
    interpreter.bind(&script);
    if (!check(
            interpreter.startSentence(2, -1) ==
                    osf::script::StepResult::complete &&
                calls == std::vector<NativeCall>{
                    {56, {10001030, 0, 0, 0}},
                    {56, {10001031, 1, 0, 0}},
                },
            "Opcode 56 did not evaluate Near Remote Town's first "
            "object-override branch.")) {
        return false;
    }
    calls.clear();
    return check(
        interpreter.startSentence(3, -1) ==
                osf::script::StepResult::complete &&
            calls == std::vector<NativeCall>{
                {56, {10001030, 1, 0, 0}},
                {56, {10001031, 0, 0, 0}},
            },
        "Opcode 56 did not evaluate Near Remote Town's opposite "
        "object-override branch.");
#else
    return true;
#endif
}

bool testRetailScenarioEffectCommand() {
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
    using NativeCall =
        std::pair<std::int32_t, std::vector<std::int32_t>>;
    std::vector<NativeCall> calls;
    osf::script::InterpreterHooks hooks;
    hooks.read_operand = [](
                             const osf::script::Operand& operand) {
        if (operand.type == 6) {
            return 111;
        }
        if (operand.type == 7) {
            return 222;
        }
        return 0;
    };
    hooks.native_command = [&calls](
                               std::int32_t opcode,
                               const std::vector<std::int32_t>& arguments) {
        calls.emplace_back(opcode, arguments);
        return true;
    };
    osf::script::Interpreter interpreter(std::move(hooks));
    interpreter.bind(&script);
    if (!check(
            interpreter.startSentence(17, -1) ==
                    osf::script::StepResult::complete &&
                calls.size() == 2 &&
                calls[0].first == 30 &&
                calls[0].second.size() == 14 &&
                calls[0].second[0] == 111 &&
                calls[0].second[1] == 222 &&
                calls[0].second[2] == 2 &&
                calls[0].second[6] == 150 &&
                calls[0].second[8] == 1 &&
                calls[1].first == 16,
            "Opcode 30 did not evaluate Near Remote Town's first "
            "authored effect sentence.")) {
        return false;
    }

    std::size_t call_count = 0;
    std::size_t scenario_count = 0;
    const std::filesystem::path root = path.parent_path().parent_path();
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(root)) {
        const std::filesystem::path scenario_path =
            entry.path() / "Scenario.Scs";
        if (!std::filesystem::is_regular_file(scenario_path)) {
            continue;
        }
        osf::script::ScriptData data;
        if (!data.load(scenario_path)) {
            return check(
                false,
                "A shipped script failed during the opcode-30 audit.");
        }
        std::size_t scenario_calls = 0;
        for (const osf::script::Sentence& sentence :
             data.sentences()) {
            for (const osf::script::Command& command :
                 sentence.commands) {
                if (command.opcode != 30) {
                    continue;
                }
                if (!check(
                        command.operands.size() == 14,
                        "A shipped opcode-30 call changed shape.")) {
                    return false;
                }
                ++call_count;
                ++scenario_calls;
            }
        }
        scenario_count += scenario_calls != 0 ? 1u : 0u;
    }
    return check(
        call_count == 411 && scenario_count == 33,
        "The shipped opcode-30 inventory differs from the audited "
        "retail scripts.");
#else
    return true;
#endif
}

bool testRetailPlacedEffectCommand() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path near_town =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Scenario" / "00000001" /
        "Scenario.Scs";
    if (!std::filesystem::is_regular_file(near_town)) {
        return true;
    }
    osf::script::ScriptData near_town_script;
    std::string error;
    if (!near_town_script.load(near_town, &error)) {
        std::cerr << error << '\n';
        return false;
    }
    using NativeCall =
        std::pair<std::int32_t, std::vector<std::int32_t>>;
    std::vector<NativeCall> calls;
    osf::script::InterpreterHooks hooks;
    hooks.read_operand = [](
                             const osf::script::Operand& operand) {
        if (operand.type == 6) {
            return 111;
        }
        if (operand.type == 7) {
            return 222;
        }
        return 0;
    };
    hooks.native_command = [&calls](
                               std::int32_t opcode,
                               const std::vector<std::int32_t>& arguments) {
        calls.emplace_back(opcode, arguments);
        return true;
    };
    osf::script::Interpreter interpreter(std::move(hooks));
    interpreter.bind(&near_town_script);
    const osf::script::StepResult near_town_result =
        interpreter.startSentence(18, -1);
    if (!check(
            near_town_result ==
                    osf::script::StepResult::complete &&
                calls.size() == 1 && calls[0].first == 36 &&
                calls[0].second ==
                    std::vector<std::int32_t>{
                        20009, 111, 222, 150, 1, 1, 1,
                    },
            "Opcode 36 did not evaluate Near Remote Town's second "
            "authored effect sentence.")) {
        return false;
    }

    const std::filesystem::path root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Scenario";
    if (!std::filesystem::is_directory(root)) {
        return true;
    }
    std::size_t call_count = 0;
    std::size_t scenario_count = 0;
    std::map<std::int32_t, std::size_t> effects;
    std::map<std::int32_t, std::size_t> directions;
    std::map<std::int32_t, std::size_t> heights;
    std::map<std::int32_t, std::size_t> right_bounds;
    std::map<std::int32_t, std::size_t> bottom_bounds;
    std::map<std::string, std::size_t> operand_shapes;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(root)) {
        const std::filesystem::path path =
            entry.path() / "Scenario.Scs";
        if (!std::filesystem::is_regular_file(path)) {
            continue;
        }
        osf::script::ScriptData data;
        if (!data.load(path)) {
            return check(
                false,
                "A shipped script failed during the opcode-36 audit.");
        }
        std::size_t scenario_calls = 0;
        for (const osf::script::Sentence& sentence :
             data.sentences()) {
            for (const osf::script::Command& command :
                 sentence.commands) {
                if (command.opcode != 36) {
                    continue;
                }
                ++call_count;
                ++scenario_calls;
                std::string shape;
                for (const osf::script::Operand& operand :
                     command.operands) {
                    shape += std::to_string(operand.type);
                    shape += ',';
                }
                ++operand_shapes[shape];
                if (command.operands.size() == 7 &&
                    command.operands[0].type == 1) {
                    ++effects[command.operands[0].value];
                }
                if (command.operands.size() == 7) {
                    ++directions[command.operands[4].value];
                    ++heights[command.operands[3].value];
                    ++right_bounds[command.operands[5].value];
                    ++bottom_bounds[command.operands[6].value];
                }
            }
        }
        scenario_count += scenario_calls != 0 ? 1u : 0u;
    }
    return check(
        call_count == 353 && scenario_count == 26 &&
            effects ==
                std::map<std::int32_t, std::size_t>{
                    {20007, 34},
                    {20008, 34},
                    {20009, 285},
                } &&
            directions ==
                std::map<std::int32_t, std::size_t>{
                    {-1, 68}, {1, 108}, {3, 48},
                    {5, 48}, {7, 81},
                } &&
            heights ==
                std::map<std::int32_t, std::size_t>{
                    {0, 68}, {150, 285},
                } &&
            right_bounds ==
                std::map<std::int32_t, std::size_t>{
                    {-1, 96}, {0, 68}, {1, 189},
                } &&
            bottom_bounds == right_bounds &&
            operand_shapes ==
                std::map<std::string, std::size_t>{
                    {"1,6,7,0,0,0,0,", 68},
                    {"1,6,7,1,1,1,1,", 285},
                },
        "The shipped opcode-36 inventory differs from the audited "
        "retail scripts.");
#else
    return true;
#endif
}

bool testRetailInclusiveRandomCommand() {
    osf::script::ScriptData script = makeRandomCommandScript();
    if (!check(
            script.sentences().size() == 1,
            "The synthetic random-command script did not decode.")) {
        return false;
    }
    const std::array<std::int32_t, 3> draws{{0, 20, 21}};
    std::size_t next_draw = 0;
    osf::script::InterpreterHooks hooks;
    hooks.next_random = [&draws, &next_draw](std::int32_t& value) {
        if (next_draw >= draws.size()) {
            return false;
        }
        value = draws[next_draw++];
        return true;
    };
    osf::script::Interpreter interpreter(std::move(hooks));
    interpreter.bind(&script);
    if (!check(
            interpreter.startSentence(0, -1) ==
                    osf::script::StepResult::complete &&
                next_draw == draws.size() &&
                interpreter.readTemporaryFlag(1000000) == 20 &&
                interpreter.readTemporaryFlag(1000001) == 40 &&
                interpreter.readTemporaryFlag(1000002) == 20,
            "Opcode 39 did not preserve retail inclusive bounds or "
            "random-call order.")) {
        return false;
    }

    osf::script::Interpreter missing_random;
    missing_random.bind(&script);
    return check(
        missing_random.startSentence(0, -1) ==
                osf::script::StepResult::unsupported_command &&
            missing_random.unsupportedOpcode() == 39,
        "Opcode 39 ran without an attached retail random stream.");
}

bool testRetailArithmeticCommands() {
    osf::script::ScriptData script =
        makeArithmeticCommandScript();
    if (!check(
            script.sentences().size() == 1,
            "The synthetic arithmetic-command script did not decode.")) {
        return false;
    }
    osf::script::Interpreter interpreter;
    interpreter.bind(&script);
    return check(
        interpreter.startSentence(0, -1) ==
                osf::script::StepResult::complete &&
            interpreter.readTemporaryFlag(1000000) == -2 &&
            interpreter.readTemporaryFlag(1000001) == -2 &&
            interpreter.readTemporaryFlag(1000002) == -1 &&
            interpreter.readTemporaryFlag(1000003) == 123,
        "Opcodes 13 through 15 lost retail overflow, signed division, "
        "remainder, or zero-divisor behavior.");
}

bool testRetailArithmeticCommandInventory() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Scenario";
    if (!std::filesystem::is_directory(root)) {
        return true;
    }
    std::map<std::int32_t, std::size_t> call_counts;
    std::map<std::int32_t, std::size_t> scenario_counts;
    std::map<std::int32_t, std::map<std::string, std::size_t>>
        operand_shapes;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(root)) {
        const std::filesystem::path path =
            entry.path() / "Scenario.Scs";
        if (!std::filesystem::is_regular_file(path)) {
            continue;
        }
        osf::script::ScriptData data;
        if (!data.load(path)) {
            return check(
                false,
                "A shipped script failed during the arithmetic audit.");
        }
        std::array<bool, 3> scenario_has_opcode{};
        for (const osf::script::Sentence& sentence :
             data.sentences()) {
            for (const osf::script::Command& command :
                 sentence.commands) {
                if (command.opcode < 13 || command.opcode > 15) {
                    continue;
                }
                if (!check(
                        command.operands.size() == 2,
                        "A shipped arithmetic command changed shape.")) {
                    return false;
                }
                ++call_counts[command.opcode];
                scenario_has_opcode[
                    static_cast<std::size_t>(command.opcode - 13)] = true;
                std::string shape;
                for (const osf::script::Operand& operand :
                     command.operands) {
                    shape += std::to_string(operand.type);
                    shape += ',';
                }
                ++operand_shapes[command.opcode][shape];
            }
        }
        for (std::size_t index = 0;
             index < scenario_has_opcode.size(); ++index) {
            if (scenario_has_opcode[index]) {
                ++scenario_counts[
                    13 + static_cast<std::int32_t>(index)];
            }
        }
    }
    return check(
        call_counts ==
                std::map<std::int32_t, std::size_t>{
                    {13, 67}, {14, 126}, {15, 195},
                } &&
            scenario_counts ==
                std::map<std::int32_t, std::size_t>{
                    {13, 34}, {14, 45}, {15, 27},
                } &&
            operand_shapes[13] ==
                std::map<std::string, std::size_t>{{"4,1,", 67}} &&
            operand_shapes[14] ==
                std::map<std::string, std::size_t>{
                    {"4,1,", 116}, {"4,4,", 10},
                } &&
            operand_shapes[15] ==
                std::map<std::string, std::size_t>{
                    {"4,1,", 185}, {"4,4,", 10},
                },
        "The shipped opcode-13-through-15 inventory differs from the "
        "audited retail scripts.");
#else
    return true;
#endif
}

bool testRetailInclusiveRandomCommandInventory() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Scenario";
    if (!std::filesystem::is_directory(root)) {
        return true;
    }
    std::size_t call_count = 0;
    std::size_t scenario_count = 0;
    std::size_t binary_choice_count = 0;
    std::size_t delay_count = 0;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(root)) {
        const std::filesystem::path path =
            entry.path() / "Scenario.Scs";
        if (!std::filesystem::is_regular_file(path)) {
            continue;
        }
        osf::script::ScriptData data;
        if (!data.load(path)) {
            return check(
                false,
                "A shipped script failed during the opcode-39 audit.");
        }
        std::size_t scenario_calls = 0;
        for (const osf::script::Sentence& sentence :
             data.sentences()) {
            for (const osf::script::Command& command :
                 sentence.commands) {
                if (command.opcode != 39) {
                    continue;
                }
                if (!check(
                        command.operands.size() == 3,
                        "A shipped opcode-39 call changed shape.")) {
                    return false;
                }
                ++call_count;
                ++scenario_calls;
                const osf::script::Operand& lower =
                    command.operands[0];
                const osf::script::Operand& upper =
                    command.operands[1];
                if (lower.type == 1 && lower.value == 0 &&
                    upper.type == 1 && upper.value == 1) {
                    ++binary_choice_count;
                }
                if (lower.type == 1 && lower.value == 20 &&
                    upper.type == 1 && upper.value == 40) {
                    ++delay_count;
                }
            }
        }
        scenario_count += scenario_calls != 0 ? 1u : 0u;
    }
    return check(
        call_count == 611 && scenario_count == 55 &&
            binary_choice_count == 285 && delay_count == 41,
        "The shipped opcode-39 inventory differs from the audited retail "
        "scripts.");
#else
    return true;
#endif
}

bool testRetailCompanionStatusCommandInventory() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Scenario";
    if (!std::filesystem::is_directory(root)) {
        return true;
    }
    std::size_t call_count = 0;
    std::size_t scenario_count = 0;
    std::map<std::int32_t, std::size_t> type_counts;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(root)) {
        const std::filesystem::path path =
            entry.path() / "Scenario.Scs";
        if (!std::filesystem::is_regular_file(path)) {
            continue;
        }
        osf::script::ScriptData data;
        if (!data.load(path)) {
            return check(
                false,
                "A shipped script failed during the opcode-3 audit.");
        }
        std::size_t scenario_calls = 0;
        for (const osf::script::Sentence& sentence :
             data.sentences()) {
            for (const osf::script::Command& command :
                 sentence.commands) {
                if (command.opcode != 3) {
                    continue;
                }
                if (!check(
                        command.operands.size() == 2 &&
                            command.operands[0].type == 1 &&
                            command.operands[1].type == 4,
                        "A shipped opcode-3 call changed shape.")) {
                    return false;
                }
                ++call_count;
                ++scenario_calls;
                ++type_counts[command.operands[0].value];
            }
        }
        scenario_count += scenario_calls != 0 ? 1u : 0u;
    }
    return check(
        call_count == 6 && scenario_count == 3 &&
            type_counts ==
                std::map<std::int32_t, std::size_t>{
                    {0, 1}, {1, 1}, {2, 1},
                    {3, 1}, {4, 1}, {5, 1},
                },
        "The shipped opcode-3 inventory differs from the audited retail "
        "scripts.");
#else
    return true;
#endif
}

bool testRetailCompanionSwapCommandInventory() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Scenario";
    if (!std::filesystem::is_directory(root)) {
        return true;
    }
    std::size_t call_count = 0;
    std::size_t scenario_count = 0;
    std::map<std::int32_t, std::size_t> type_counts;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(root)) {
        const std::filesystem::path path =
            entry.path() / "Scenario.Scs";
        if (!std::filesystem::is_regular_file(path)) {
            continue;
        }
        osf::script::ScriptData data;
        if (!data.load(path)) {
            return check(
                false,
                "A shipped script failed during the opcode-45 audit.");
        }
        std::size_t scenario_calls = 0;
        for (const osf::script::Sentence& sentence :
             data.sentences()) {
            for (const osf::script::Command& command :
                 sentence.commands) {
                if (command.opcode != 45) {
                    continue;
                }
                if (!check(
                        command.operands.size() == 1 &&
                            command.operands[0].type == 1,
                        "A shipped opcode-45 call changed shape.")) {
                    return false;
                }
                ++call_count;
                ++scenario_calls;
                ++type_counts[command.operands[0].value];
            }
        }
        scenario_count += scenario_calls != 0 ? 1u : 0u;
    }
    return check(
        call_count == 6 && scenario_count == 3 &&
            type_counts ==
                std::map<std::int32_t, std::size_t>{
                    {0, 1}, {1, 1}, {2, 1},
                    {3, 1}, {4, 1}, {5, 1},
                },
        "The shipped opcode-45 inventory differs from the audited retail "
        "scripts.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    return testRetailRemoteTown() &&
                   testRetailOutdoorChestScript() &&
                   testRetailRedGoblinDeathStatus() &&
                   testRetailGiantWarehouseScript() &&
                   testRetailItemOwnershipCommands() &&
                   testRetailJobCommands() &&
                   testRetailEquipmentColorCommand() &&
                   testRetailBlackjackCommands() &&
                   testRetailScenarioEntryAndCaptionCommands() &&
                   testRetailScenarioEntityOverrideCommand() &&
                   testRetailScenarioEffectCommand() &&
                   testRetailPlacedEffectCommand() &&
                   testRetailArithmeticCommands() &&
                   testRetailArithmeticCommandInventory() &&
                   testRetailInclusiveRandomCommand() &&
                   testRetailInclusiveRandomCommandInventory() &&
                   testRetailCompanionStatusCommandInventory() &&
                   testRetailCompanionSwapCommandInventory() &&
                   testMalformedScript()
               ? 0
               : 1;
}
