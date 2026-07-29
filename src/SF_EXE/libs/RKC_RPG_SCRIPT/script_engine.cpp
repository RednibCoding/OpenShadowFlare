#include "libs/RKC_RPG_SCRIPT/rkc_rpg_script.hpp"

#include <utility>

namespace osf::script {

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
    unsupported_opcode_ = -1;
}

StepResult Interpreter::startStatus(
    std::int32_t kind,
    std::int32_t character_number) {
    frames_.clear();
    waiting_for_message_ = false;
    unsupported_opcode_ = -1;
    if (!script_) {
        return StepResult::invalid_script;
    }
    const Status* status =
        script_->findStatus(kind, character_number);
    if (!status || !pushSentence(status->sentence)) {
        return StepResult::invalid_script;
    }
    return run();
}

StepResult Interpreter::resume() {
    if (!waiting_for_message_) {
        return frames_.empty()
                   ? StepResult::complete
                   : run();
    }
    waiting_for_message_ = false;
    return run();
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
    return StepResult::complete;
}

StepResult Interpreter::execute(const Command& command) {
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
        if (command.operands.empty() || !script_) {
            return StepResult::invalid_script;
        }
        const std::int32_t id =
            readOperand(command.operands[0]);
        const Message* message = script_->findMessage(id);
        if (!message) {
            return StepResult::invalid_script;
        }
        if (hooks_.show_message) {
            hooks_.show_message({message->id, message->text});
        }
        waiting_for_message_ = true;
        return StepResult::waiting_for_message;
    }
    case 18: {
        if (command.operands.empty()) {
            return StepResult::invalid_script;
        }
        if (!hooks_.native_command ||
            !hooks_.native_command(
                command.opcode,
                {readOperand(command.operands[0])})) {
            unsupported_opcode_ = command.opcode;
            return StepResult::unsupported_command;
        }
        return StepResult::complete;
    }
    case 21: {
        if (command.operands.size() < 2) {
            return StepResult::invalid_script;
        }
        if (!hooks_.native_command ||
            !hooks_.native_command(
                command.opcode,
                {
                    readOperand(command.operands[0]),
                    readOperand(command.operands[1]),
                })) {
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
