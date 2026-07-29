#include "libs/RKC_RPG_SCRIPT/rkc_rpg_script.hpp"

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
            mode_one
                ? readOperand(command.operands[3])
                : -1;
        // Mode one also carries the chained informational messages used by
        // companion explanations. A non-negative initial range distinguishes
        // the actual choice menus from those ordinary acknowledgement steps.
        const bool selection_required =
            mode_one && initial_selection >= 0;
        if (hooks_.show_message) {
            hooks_.show_message({
                message->id,
                message->text,
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
    case 18:
    case 19:
    case 48:
        return executeNative(1);
    case 21:
        return executeNative(2);
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
