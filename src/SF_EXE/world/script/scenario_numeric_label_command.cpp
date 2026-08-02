#include "scenario_numeric_label_command.hpp"

#include <cstddef>
#include <string>

namespace osf {
namespace {

constexpr std::size_t kArgumentCount = 7;

}  // namespace

bool makeScenarioNumericLabel(
    const std::vector<std::int32_t>& arguments,
    WorldPosition anchor,
    ScenarioTextLabel& label) {
    if (arguments.size() != kArgumentCount) {
        return false;
    }

    label = {
        anchor,
        arguments[1],
        arguments[2],
        std::to_string(arguments[3]),
        arguments[4],
        arguments[5],
        arguments[6],
        0,
    };
    return true;
}

}  // namespace osf
