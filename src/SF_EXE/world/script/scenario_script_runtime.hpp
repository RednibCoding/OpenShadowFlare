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
    std::function<void(const script::MessageEvent&)> show_message;
    std::function<bool(
        std::int32_t,
        const std::vector<std::int32_t>&)> native_command;
    std::function<bool(
        script::ValueQuery,
        std::int32_t&)> query_value;
};

class ScenarioScriptRuntime {
public:
    explicit ScenarioScriptRuntime(
        ScenarioScriptRuntimeHooks hooks = {});

    bool load(
        const std::filesystem::path& path,
        std::string* error = nullptr);
    void clear();

    script::StepResult startStatus(
        std::int32_t kind,
        std::int32_t character_number);
    script::StepResult resume(std::int32_t selection = -1);
    const script::ScriptData& data() const;

private:
    std::int32_t readOperand(
        const script::Operand& operand) const;
    bool writeOperand(
        const script::Operand& operand,
        std::int32_t value);

    ScenarioScriptRuntimeHooks hooks_;
    script::ScriptData data_;
    script::Interpreter interpreter_;
    std::unordered_map<std::uint64_t, std::int32_t> values_;
};

}  // namespace osf

#endif
