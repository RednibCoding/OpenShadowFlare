#include "scenario_script_runtime.hpp"

#include <utility>

namespace osf {
namespace {

std::uint64_t valueKey(const script::Operand& operand) {
    return
        (static_cast<std::uint64_t>(
             static_cast<std::uint32_t>(operand.type))
         << 32u) |
        static_cast<std::uint32_t>(operand.value);
}

}  // namespace

ScenarioScriptRuntime::ScenarioScriptRuntime(
    ScenarioScriptRuntimeHooks hooks)
    : hooks_(std::move(hooks)),
      interpreter_({
          [this](const script::Operand& operand) {
              return readOperand(operand);
          },
          [this](
              const script::Operand& operand,
              std::int32_t value) {
              return writeOperand(operand, value);
          },
          [this](const script::MessageEvent& message) {
              showMessage(message);
          },
          [this](
              std::int32_t opcode,
              const std::vector<std::int32_t>& arguments) {
              return hooks_.native_command &&
                     hooks_.native_command(opcode, arguments);
          },
          [this](
              script::ValueQuery query,
              std::int32_t& value) {
              return hooks_.query_value &&
                     hooks_.query_value(query, value);
          },
          [this](
              script::ValueQuery query,
              std::int32_t index,
              std::int32_t& value) {
              return hooks_.query_indexed_value &&
                     hooks_.query_indexed_value(
                         query, index, value);
          },
          [this](
              std::int32_t character_number,
              std::int32_t& distance) {
              return hooks_.measure_character_distance &&
                     hooks_.measure_character_distance(
                         character_number, distance);
          },
          [this](
              std::int32_t category,
              std::int32_t definition_id,
              bool& present) {
              return hooks_.query_item &&
                     hooks_.query_item(
                         category, definition_id, present);
          },
          [this](std::int32_t& value) {
              if (!hooks_.next_random) {
                  return false;
              }
              value = hooks_.next_random();
              return true;
          },
          [this](
              std::int32_t companion_type,
              std::string& message) {
              return hooks_.build_companion_status_message &&
                     hooks_.build_companion_status_message(
                         companion_type, message);
          },
          [this](
              std::int32_t character_number,
              std::int32_t& state) {
              return hooks_.query_enemy_lifecycle_state &&
                     hooks_.query_enemy_lifecycle_state(
                         character_number, state);
          },
      }) {}

bool ScenarioScriptRuntime::load(
    const std::filesystem::path& path,
    std::string* error) {
    clear();
    if (!data_.load(path, error)) {
        return false;
    }
    interpreter_.bind(&data_);
    return true;
}

void ScenarioScriptRuntime::adopt(script::ScriptData data) {
    interpreter_.bind(nullptr);
    data_.clear();
    clearMessage();
    data_ = std::move(data);
    interpreter_.bind(&data_);
}

void ScenarioScriptRuntime::clear() {
    interpreter_.bind(nullptr);
    data_.clear();
    values_.clear();
    clearMessage();
}

script::StepResult ScenarioScriptRuntime::startStatus(
    std::int32_t kind,
    std::int32_t character_number) {
    clearMessage();
    return interpreter_.startStatus(kind, character_number);
}

script::StepResult ScenarioScriptRuntime::runStatusKind(
    std::int32_t kind) {
    if (message_active_) {
        return script::StepResult::waiting_for_message;
    }
    script::StepResult first_failure =
        script::StepResult::complete;
    for (const script::Status& status : data_.statuses()) {
        if (status.kind != kind) {
            continue;
        }
        const script::StepResult result =
            interpreter_.startSentence(
                status.sentence,
                status.character_number);
        if (result ==
            script::StepResult::waiting_for_message) {
            return result;
        }
        if (result != script::StepResult::complete &&
            first_failure == script::StepResult::complete) {
            first_failure = result;
        }
    }
    return first_failure;
}

script::StepResult ScenarioScriptRuntime::resume(
    std::int32_t selection) {
    clearMessage();
    return interpreter_.resume(selection);
}

const script::ScriptData& ScenarioScriptRuntime::data() const {
    return data_;
}

const script::ScenarioCaptionEvent&
ScenarioScriptRuntime::caption() const {
    return interpreter_.caption();
}

bool ScenarioScriptRuntime::messageActive() const {
    return message_active_;
}

std::int32_t ScenarioScriptRuntime::actorId() const {
    return actor_id_;
}

void ScenarioScriptRuntime::setActorId(std::int32_t actor_id) {
    actor_id_ = actor_id;
}

const script::MessageEvent&
ScenarioScriptRuntime::message() const {
    return message_;
}

std::int32_t ScenarioScriptRuntime::selectedOption() const {
    return selected_option_;
}

void ScenarioScriptRuntime::selectOption(std::int32_t option) {
    if (message_active_ &&
        message_.selection_required &&
        option >= 0) {
        selected_option_ = option;
    }
}

std::int32_t ScenarioScriptRuntime::readOperand(
    const script::Operand& operand) const {
    std::int32_t value = 0;
    if (hooks_.read_world_operand &&
        hooks_.read_world_operand(operand, value)) {
        return value;
    }
    const auto found = values_.find(valueKey(operand));
    return found == values_.end() ? 0 : found->second;
}

bool ScenarioScriptRuntime::writeOperand(
    const script::Operand& operand,
    std::int32_t value) {
    if (hooks_.write_world_operand &&
        hooks_.write_world_operand(operand, value)) {
        return true;
    }
    values_.insert_or_assign(valueKey(operand), value);
    return true;
}

void ScenarioScriptRuntime::showMessage(
    const script::MessageEvent& message) {
    message_ = message;
    message_active_ = true;
    selected_option_ =
        message.selection_required
            ? message.initial_selection
            : -1;
    actor_id_ = hooks_.resolve_actor_id
        ? hooks_.resolve_actor_id(message.character_number)
        : -1;
}

void ScenarioScriptRuntime::clearMessage() {
    message_ = {};
    message_active_ = false;
    actor_id_ = -1;
    selected_option_ = -1;
}
}  // namespace osf
