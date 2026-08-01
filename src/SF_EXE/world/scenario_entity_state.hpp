#ifndef OPENSHADOWFLARE_SCENARIO_ENTITY_STATE_HPP
#define OPENSHADOWFLARE_SCENARIO_ENTITY_STATE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {

enum class ScenarioEntityStateChannel : std::size_t {
    visible = 0,
    pointer = 1,
    judgement = 2,
};

class ScenarioEntityState {
public:
    bool initialize(
        const std::vector<std::int32_t>& initial_values);
    void clear();

    std::int32_t value(
        ScenarioEntityStateChannel channel) const;
    void setValue(
        ScenarioEntityStateChannel channel,
        std::int32_t value);
    void setOverride(
        std::int32_t visible,
        std::int32_t pointer,
        std::int32_t judgement);
    bool overrideEnabled() const;
    bool visible() const;
    bool pointerEnabled() const;
    bool judgementEnabled() const;

private:
    std::int32_t effectiveValue(
        ScenarioEntityStateChannel channel) const;

    std::array<std::int32_t, 3> values_{{0, 0, 0}};
    std::array<std::int32_t, 3> override_values_{{0, 0, 0}};
    bool override_enabled_ = false;
};

}  // namespace osf

#endif
