#ifndef OPENSHADOWFLARE_SOFTWARE_BACKEND_HPP
#define OPENSHADOWFLARE_SOFTWARE_BACKEND_HPP

#include "gapi.hpp"

#include <functional>
#include <vector>

namespace osf::gapi {

class SoftwareBackend final : public Backend {
public:
    using PresentCallback = std::function<void(SurfaceView)>;

    SoftwareBackend(
        std::int32_t width,
        std::int32_t height,
        PresentCallback present = {});

    void beginFrame(Color clear_color) override;
    bool drawPattern(
        const NjpImage& image,
        std::size_t pattern_index,
        const PatternDraw& draw = {}) override;
    void endFrame() override;

    SurfaceView surface() const;

private:
    std::int32_t width_ = 0;
    std::int32_t height_ = 0;
    std::vector<Color> pixels_;
    PresentCallback present_;
};

}  // namespace osf::gapi

#endif
