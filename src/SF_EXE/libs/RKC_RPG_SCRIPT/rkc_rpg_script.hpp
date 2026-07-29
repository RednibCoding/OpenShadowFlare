#ifndef OPENSHADOWFLARE_LIBS_RKC_RPG_SCRIPT_HPP
#define OPENSHADOWFLARE_LIBS_RKC_RPG_SCRIPT_HPP

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace osf::script {

struct FlagDefinition {
    std::int32_t id = 0;
    std::int32_t initial_value = 0;
};

struct Message {
    std::int32_t id = 0;
    std::string text;
};

struct Status {
    bool networked = false;
    std::int32_t kind = 0;
    std::int32_t character_number = -1;
    std::int32_t sentence = -1;
};

struct Operand {
    std::int32_t type = 0;
    std::int32_t value = 0;
};

struct Command {
    std::int32_t opcode = -1;
    std::vector<Operand> operands;
};

struct Sentence {
    std::vector<Command> commands;
};

class ScriptData {
public:
    bool load(
        const std::filesystem::path& path,
        std::string* error = nullptr);
    bool decode(
        const std::vector<std::uint8_t>& bytes,
        std::string* error = nullptr);
    void clear();

    const std::string& version() const;
    const std::vector<FlagDefinition>& temporaryFlags() const;
    const std::vector<FlagDefinition>& networkFlags() const;
    const std::vector<Message>& messages() const;
    const std::vector<Status>& statuses() const;
    const std::vector<Sentence>& sentences() const;
    const Message* findMessage(std::int32_t id) const;
    const Status* findStatus(
        std::int32_t kind,
        std::int32_t character_number) const;
    const Sentence* findSentence(std::int32_t index) const;

private:
    std::string version_;
    std::vector<FlagDefinition> temporary_flags_;
    std::vector<FlagDefinition> network_flags_;
    std::vector<Message> messages_;
    std::vector<Status> statuses_;
    std::vector<Sentence> sentences_;
};

enum class StepResult {
    complete,
    waiting_for_message,
    unsupported_command,
    invalid_script,
};

struct MessageEvent {
    std::int32_t id = -1;
    std::string text;
};

enum class ValueQuery {
    local_player_level,
};

struct InterpreterHooks {
    std::function<std::int32_t(const Operand&)> read_operand;
    std::function<bool(const Operand&, std::int32_t)> write_operand;
    std::function<void(const MessageEvent&)> show_message;
    std::function<bool(
        std::int32_t,
        const std::vector<std::int32_t>&)> native_command;
    std::function<bool(ValueQuery, std::int32_t&)> query_value;
};

class Interpreter {
public:
    explicit Interpreter(InterpreterHooks hooks = {});

    void bind(const ScriptData* script);
    void reset();
    StepResult startStatus(
        std::int32_t kind,
        std::int32_t character_number);
    StepResult resume();
    bool waitingForMessage() const;
    std::int32_t unsupportedOpcode() const;
    std::int32_t readTemporaryFlag(std::int32_t id) const;

private:
    struct Frame {
        std::int32_t sentence = -1;
        std::size_t command = 0;
    };

    StepResult run();
    StepResult execute(const Command& command);
    std::int32_t readOperand(const Operand& operand) const;
    bool writeOperand(
        const Operand& operand,
        std::int32_t value);
    bool pushSentence(std::int32_t sentence);

    const ScriptData* script_ = nullptr;
    InterpreterHooks hooks_;
    std::unordered_map<std::int32_t, std::int32_t>
        temporary_flags_;
    std::vector<Frame> frames_;
    bool waiting_for_message_ = false;
    std::int32_t unsupported_opcode_ = -1;
};

}  // namespace osf::script

#endif
