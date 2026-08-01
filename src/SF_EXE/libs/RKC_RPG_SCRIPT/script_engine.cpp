#include "libs/RKC_RPG_SCRIPT/rkc_rpg_script.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <utility>

namespace osf::script {
namespace {

std::int32_t wrappedArithmetic(
    std::int32_t left,
    std::int32_t right,
    bool subtract) {
    std::int64_t result = subtract
        ? static_cast<std::int64_t>(left) - right
        : static_cast<std::int64_t>(left) + right;
    constexpr std::int64_t range =
        std::int64_t{1} << 32;
    if (result > std::numeric_limits<std::int32_t>::max()) {
        result -= range;
    } else if (
        result < std::numeric_limits<std::int32_t>::min()) {
        result += range;
    }
    return static_cast<std::int32_t>(result);
}

std::string formatMessage(
    const std::string& message,
    const std::array<std::int32_t, 20>& parameters) {
    std::string formatted;
    formatted.reserve(message.size());
    std::size_t parameter = 0;
    for (std::size_t index = 0; index < message.size();) {
        if (message[index] != '%' || index + 1 >= message.size()) {
            formatted.push_back(message[index++]);
            continue;
        }
        if (message[index + 1] == '%') {
            formatted.push_back('%');
            index += 2;
            continue;
        }
        std::size_t cursor = index + 1;
        std::int32_t width = 0;
        while (cursor < message.size() &&
               std::isdigit(
                   static_cast<unsigned char>(message[cursor]))) {
            width = width * 10 + (message[cursor] - '0');
            ++cursor;
        }
        if (cursor >= message.size() || message[cursor] != 'd' ||
            parameter >= parameters.size()) {
            formatted.push_back(message[index++]);
            continue;
        }
        const std::string value =
            std::to_string(parameters[parameter++]);
        if (width > static_cast<std::int32_t>(value.size())) {
            formatted.append(
                static_cast<std::size_t>(width) - value.size(), ' ');
        }
        formatted.append(value);
        index = cursor + 1;
    }
    return formatted;
}

}  // namespace

Interpreter::Interpreter(InterpreterHooks hooks)
    : hooks_(std::move(hooks)) {}

void Interpreter::bind(const ScriptData* script) {
    script_ = script;
    reset();
    if (!script_) {
        return;
    }
    for (const FlagDefinition& flag :
         script_->temporaryFlags()) {
        temporary_flags_.insert_or_assign(
            flag.id, flag.initial_value);
    }
}

void Interpreter::reset() {
    temporary_flags_.clear();
    frames_.clear();
    waiting_for_message_ = false;
    message_callback_pending_ = false;
    message_selection_pending_ = false;
    message_selection_operand_ = {};
    message_initial_selection_ = -1;
    current_character_number_ = -1;
    message_callback_character_number_ = -1;
    unsupported_opcode_ = -1;
    message_parameters_.fill(-1);
}

StepResult Interpreter::startStatus(
    std::int32_t kind,
    std::int32_t character_number) {
    frames_.clear();
    waiting_for_message_ = false;
    message_callback_pending_ = false;
    message_selection_pending_ = false;
    message_selection_operand_ = {};
    message_initial_selection_ = -1;
    message_callback_character_number_ = -1;
    current_character_number_ = -1;
    unsupported_opcode_ = -1;
    return enterStatus(kind, character_number, false);
}

StepResult Interpreter::startSentence(
    std::int32_t sentence,
    std::int32_t character_number) {
    frames_.clear();
    waiting_for_message_ = false;
    message_callback_pending_ = false;
    message_selection_pending_ = false;
    message_selection_operand_ = {};
    message_initial_selection_ = -1;
    message_callback_character_number_ = -1;
    current_character_number_ = character_number;
    unsupported_opcode_ = -1;
    if (!script_ || !pushSentence(sentence)) {
        return StepResult::invalid_script;
    }
    return run();
}

StepResult Interpreter::enterStatus(
    std::int32_t kind,
    std::int32_t character_number,
    bool missing_is_complete) {
    if (!script_) {
        return StepResult::invalid_script;
    }
    const Status* status =
        script_->findStatus(kind, character_number);
    if (!status) {
        return missing_is_complete
                   ? StepResult::complete
                   : StepResult::invalid_script;
    }
    if (!pushSentence(status->sentence)) {
        return StepResult::invalid_script;
    }
    current_character_number_ = character_number;
    return run();
}

StepResult Interpreter::resume(std::int32_t selection) {
    if (!waiting_for_message_) {
        return frames_.empty()
                   ? StepResult::complete
                   : run();
    }
    waiting_for_message_ = false;
    if (message_selection_pending_) {
        const std::int32_t selected =
            selection >= 0
                ? selection
                : message_initial_selection_;
        if (selected < 0 ||
            !writeOperand(
                message_selection_operand_, selected)) {
            waiting_for_message_ = true;
            return StepResult::waiting_for_message;
        }
        message_selection_pending_ = false;
        message_selection_operand_ = {};
        message_initial_selection_ = -1;
    }
    if (!message_callback_pending_) {
        return frames_.empty()
                   ? StepResult::complete
                   : run();
    }

    const std::int32_t character_number =
        message_callback_character_number_;
    message_callback_pending_ = false;
    message_callback_character_number_ = -1;
    frames_.clear();
    unsupported_opcode_ = -1;
    return enterStatus(1, character_number, true);
}

bool Interpreter::waitingForMessage() const {
    return waiting_for_message_;
}

std::int32_t Interpreter::unsupportedOpcode() const {
    return unsupported_opcode_;
}

std::int32_t Interpreter::readTemporaryFlag(
    std::int32_t id) const {
    const auto found = temporary_flags_.find(id);
    return found == temporary_flags_.end()
               ? -1
               : found->second;
}

StepResult Interpreter::run() {
    if (!script_) {
        return StepResult::invalid_script;
    }
    while (!frames_.empty()) {
        Frame& frame = frames_.back();
        const Sentence* sentence =
            script_->findSentence(frame.sentence);
        if (!sentence) {
            return StepResult::invalid_script;
        }
        if (frame.command >= sentence->commands.size()) {
            frames_.pop_back();
            continue;
        }

        const Command& command =
            sentence->commands[frame.command++];
        const StepResult result = execute(command);
        if (result != StepResult::complete) {
            return result;
        }
    }
    return waiting_for_message_
               ? StepResult::waiting_for_message
               : StepResult::complete;
}

StepResult Interpreter::execute(const Command& command) {
    const auto executeNative =
        [this, &command](std::size_t argument_count) {
            if (command.operands.size() < argument_count) {
                return StepResult::invalid_script;
            }
            std::vector<std::int32_t> arguments;
            arguments.reserve(argument_count);
            for (std::size_t index = 0;
                 index < argument_count;
                 ++index) {
                arguments.push_back(
                    readOperand(command.operands[index]));
            }
            if (!hooks_.native_command ||
                !hooks_.native_command(
                    command.opcode, arguments)) {
                unsupported_opcode_ = command.opcode;
                return StepResult::unsupported_command;
            }
            return StepResult::complete;
        };

    switch (command.opcode) {
    case 0: {
        if (command.operands.size() < 4) {
            return StepResult::invalid_script;
        }
        const std::int32_t left =
            readOperand(command.operands[0]);
        const std::int32_t right =
            readOperand(command.operands[2]);
        bool condition = false;
        switch (command.operands[1].value) {
        case 0:
            condition = left == right;
            break;
        case 1:
            condition = left != right;
            break;
        case 2:
            condition = left > right;
            break;
        case 3:
            condition = left < right;
            break;
        default:
            return StepResult::invalid_script;
        }
        if (condition &&
            !pushSentence(command.operands[3].value)) {
            return StepResult::invalid_script;
        }
        return StepResult::complete;
    }
    case 1:
        if (command.operands.size() < 2 ||
            !writeOperand(
                command.operands[0],
                readOperand(command.operands[1]))) {
            return StepResult::invalid_script;
        }
        return StepResult::complete;
    case 2: {
        if (command.operands.size() < 5 || !script_) {
            return StepResult::invalid_script;
        }
        const std::int32_t id =
            readOperand(command.operands[0]);
        const Message* message = script_->findMessage(id);
        if (!message) {
            return StepResult::invalid_script;
        }
        const std::int32_t mode =
            readOperand(command.operands[2]);
        const bool mode_one = mode == 1;
        const std::int32_t initial_selection =
            readOperand(command.operands[3]);
        // The third operand controls the speech presentation, independently
        // of the fourth operand which marks a choice menu when non-negative.
        // Malse's retail service menu uses presentation mode zero.
        const bool selection_required = initial_selection >= 0;
        if (hooks_.show_message) {
            hooks_.show_message({
                message->id,
                formatMessage(message->text, message_parameters_),
                current_character_number_,
                selection_required,
                initial_selection,
            });
        }
        waiting_for_message_ = true;
        if (mode == 0 || mode_one) {
            const std::int32_t target =
                readOperand(command.operands[4]);
            message_callback_pending_ = true;
            message_callback_character_number_ =
                target == -1
                    ? current_character_number_
                    : target;
        }
        if (selection_required) {
            message_selection_pending_ = true;
            message_selection_operand_ = command.operands[1];
            message_initial_selection_ = initial_selection;
        }
        return StepResult::complete;
    }
    case 10:
        return executeNative(6);
    case 4:
        return executeNative(0);
    case 16:
        return command.operands.empty()
            ? StepResult::invalid_script
            : executeNative(
                  std::min<std::size_t>(
                      command.operands.size(), 4));
    case 11:
    case 12: {
        if (command.operands.size() < 2) {
            return StepResult::invalid_script;
        }
        const std::int32_t left =
            readOperand(command.operands[0]);
        const std::int32_t right =
            readOperand(command.operands[1]);
        const std::int32_t result = wrappedArithmetic(
            left, right, command.opcode == 12);
        if (!writeOperand(command.operands[0], result)) {
            return StepResult::invalid_script;
        }
        return StepResult::complete;
    }
    case 5:
    case 9:
    case 18:
    case 19:
    case 37:
    case 41:
    case 48:
        return executeNative(1);
    case 54:
        return executeNative(1);
    case 6:
        return executeNative(2);
    case 24:
        return executeNative(3);
    case 34: {
        if (command.operands.size() < 2) {
            return StepResult::invalid_script;
        }
        std::int32_t distance = 0;
        if (!hooks_.measure_character_distance ||
            !hooks_.measure_character_distance(
                readOperand(command.operands[0]), distance)) {
            unsupported_opcode_ = command.opcode;
            return StepResult::unsupported_command;
        }
        if (!writeOperand(command.operands[1], distance)) {
            return StepResult::invalid_script;
        }
        return StepResult::complete;
    }
    case 17:
    case 21:
        return executeNative(2);
    case 22:
    case 23: {
        if (command.operands.empty()) {
            return StepResult::invalid_script;
        }
        const std::int32_t character_number =
            readOperand(command.operands[0]);
        const std::int32_t state =
            command.opcode == 22 ? 1 : 0;
        constexpr std::int32_t state_bases[] = {
            100000000,
            200000000,
            300000000,
        };
        for (std::int32_t base : state_bases) {
            if (!writeOperand(
                    {5, base + character_number},
                    state)) {
                return StepResult::invalid_script;
            }
        }
        return StepResult::complete;
    }
    case 44: {
        if (command.operands.empty()) {
            return StepResult::invalid_script;
        }
        std::int32_t value = 0;
        if (!hooks_.query_value ||
            !hooks_.query_value(
                ValueQuery::local_player_companion_type,
                value)) {
            unsupported_opcode_ = command.opcode;
            return StepResult::unsupported_command;
        }
        if (!writeOperand(command.operands[0], value)) {
            return StepResult::invalid_script;
        }
        return StepResult::complete;
    }
    case 51:
        if (command.operands.size() < message_parameters_.size()) {
            return StepResult::invalid_script;
        }
        for (std::size_t index = 0;
             index < message_parameters_.size();
             ++index) {
            message_parameters_[index] =
                readOperand(command.operands[index]);
        }
        return StepResult::complete;
    case 52: {
        if (command.operands.size() < 2) {
            return StepResult::invalid_script;
        }
        std::int32_t value = 0;
        if (!hooks_.query_indexed_value ||
            !hooks_.query_indexed_value(
                ValueQuery::local_player_repair_price,
                readOperand(command.operands[0]),
                value)) {
            unsupported_opcode_ = command.opcode;
            return StepResult::unsupported_command;
        }
        if (!writeOperand(command.operands[1], value)) {
            return StepResult::invalid_script;
        }
        return StepResult::complete;
    }
    case 53:
    case 55: {
        if (command.operands.empty()) {
            return StepResult::invalid_script;
        }
        const ValueQuery query =
            command.opcode == 53
                ? ValueQuery::local_player_gold
                : ValueQuery::local_player_has_unidentified_items;
        std::int32_t value = 0;
        if (!hooks_.query_value ||
            !hooks_.query_value(query, value)) {
            unsupported_opcode_ = command.opcode;
            return StepResult::unsupported_command;
        }
        if (!writeOperand(command.operands[0], value)) {
            return StepResult::invalid_script;
        }
        return StepResult::complete;
    }
    case 42:
    case 43:
    case 63: {
        if (command.operands.size() < 2) {
            return StepResult::invalid_script;
        }
        ValueQuery current_query =
            ValueQuery::local_player_current_life;
        ValueQuery maximum_query =
            ValueQuery::local_player_maximum_life;
        if (command.opcode == 43) {
            current_query =
                ValueQuery::local_player_current_mana;
            maximum_query =
                ValueQuery::local_player_maximum_mana;
        } else if (command.opcode == 63) {
            current_query =
                ValueQuery::local_player_condition_current;
            maximum_query =
                ValueQuery::local_player_condition_maximum;
        }
        std::int32_t current = 0;
        std::int32_t maximum = 0;
        if (!hooks_.query_value ||
            !hooks_.query_value(current_query, current) ||
            !hooks_.query_value(maximum_query, maximum) ||
            !writeOperand(command.operands[0], current) ||
            !writeOperand(command.operands[1], maximum)) {
            unsupported_opcode_ = command.opcode;
            return StepResult::unsupported_command;
        }
        return StepResult::complete;
    }
    case 61: {
        if (command.operands.empty()) {
            return StepResult::invalid_script;
        }
        std::int32_t value = 0;
        if (!hooks_.query_value ||
            !hooks_.query_value(
                ValueQuery::local_player_level, value)) {
            unsupported_opcode_ = command.opcode;
            return StepResult::unsupported_command;
        }
        if (!writeOperand(command.operands[0], value)) {
            return StepResult::invalid_script;
        }
        return StepResult::complete;
    }
    case 62:
        return executeNative(3);
    default:
        unsupported_opcode_ = command.opcode;
        return StepResult::unsupported_command;
    }
}

std::int32_t Interpreter::readOperand(
    const Operand& operand) const {
    if (operand.type >= 0 && operand.type <= 2) {
        return operand.value;
    }
    if (operand.type == 4) {
        const auto found =
            temporary_flags_.find(operand.value);
        return found == temporary_flags_.end()
                   ? -1
                   : found->second;
    }
    if (operand.type == 8) {
        std::int32_t value = 0;
        if (hooks_.query_value &&
            hooks_.query_value(ValueQuery::play_mode, value)) {
            return value;
        }
    }
    return hooks_.read_operand
               ? hooks_.read_operand(operand)
               : 0;
}

bool Interpreter::writeOperand(
    const Operand& operand,
    std::int32_t value) {
    if (operand.type == 4) {
        const auto found =
            temporary_flags_.find(operand.value);
        if (found == temporary_flags_.end()) {
            return false;
        }
        found->second = value;
        return true;
    }
    return hooks_.write_operand &&
           hooks_.write_operand(operand, value);
}

bool Interpreter::pushSentence(std::int32_t sentence) {
    if (!script_ || !script_->findSentence(sentence)) {
        return false;
    }
    frames_.push_back({sentence, 0});
    return true;
}

}  // namespace osf::script
