#ifndef OPENSHADOWFLARE_SCENARIO_SCRIPT_RUNTIME_HPP
#define OPENSHADOWFLARE_SCENARIO_SCRIPT_RUNTIME_HPP

#include "libs/RKC_RPG_SCRIPT/rkc_rpg_script.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace osf {

struct ScenarioScriptRuntimeHooks {
    std::function<bool(
        const script::Operand&,
        std::int32_t&)> read_world_operand;
    std::function<bool(
        const script::Operand&,
        std::int32_t)> write_world_operand;
    std::function<std::int32_t(std::int32_t)> resolve_actor_id;
    std::function<bool(
        std::int32_t,
        const std::vector<std::int32_t>&)> native_command;
    std::function<bool(
        script::ValueQuery,
        std::int32_t&)> query_value;
    std::function<bool(
        std::int32_t,
        std::int32_t&)> measure_character_distance;
};

class ScenarioScriptRuntime {
public:
    explicit ScenarioScriptRuntime(
        ScenarioScriptRuntimeHooks hooks = {});

    bool load(
        const std::filesystem::path& path,
        std::string* error = nullptr);
    void adopt(script::ScriptData data);
    void clear();

    script::StepResult startStatus(
        std::int32_t kind,
        std::int32_t character_number);
    script::StepResult runStatusKind(std::int32_t kind);
    script::StepResult resume(std::int32_t selection = -1);
    const script::ScriptData& data() const;

    bool messageActive() const;
    std::int32_t actorId() const;
    void setActorId(std::int32_t actor_id);
    const script::MessageEvent& message() const;
    std::int32_t selectedOption() const;
    void selectOption(std::int32_t option);

private:
    std::int32_t readOperand(
        const script::Operand& operand) const;
    bool writeOperand(
        const script::Operand& operand,
        std::int32_t value);
    void showMessage(const script::MessageEvent& message);
    void clearMessage();

    ScenarioScriptRuntimeHooks hooks_;
    script::ScriptData data_;
    script::Interpreter interpreter_;
    std::unordered_map<std::uint64_t, std::int32_t> values_;
    script::MessageEvent message_;
    bool message_active_ = false;
    std::int32_t actor_id_ = -1;
    std::int32_t selected_option_ = -1;
};

}  // namespace osf

#endif
