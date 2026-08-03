#ifndef OPENSHADOWFLARE_SCENARIO_VISUAL_PRESENTATION_HPP
#define OPENSHADOWFLARE_SCENARIO_VISUAL_PRESENTATION_HPP

#include <cstddef>
#include <cstdint>

namespace osf {

class ScenarioVisualPresentation {
public:
    void clear();
    void begin(
        std::int32_t visual_id,
        std::size_t page_count);
    void requestAdvance();
    void advanceFrame();

    bool active() const;
    std::int32_t visualId() const;
    std::size_t page() const;
    std::int32_t counter() const;
    std::int32_t fadeStrength() const;
    bool continueVisible() const;
    std::int32_t continueOffset() const;

private:
    std::int32_t visual_id_ = -1;
    std::size_t page_ = 0;
    std::size_t page_count_ = 0;
    std::int32_t counter_ = 0;
    bool advance_requested_ = false;
    bool closing_ = false;
};

}  // namespace osf

#endif
