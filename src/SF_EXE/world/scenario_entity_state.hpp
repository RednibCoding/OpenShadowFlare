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
    bool visible() const;
    bool pointerEnabled() const;
    bool judgementEnabled() const;

private:
    std::array<std::int32_t, 3> values_{{0, 0, 0}};
};

}  // namespace osf

#endif
