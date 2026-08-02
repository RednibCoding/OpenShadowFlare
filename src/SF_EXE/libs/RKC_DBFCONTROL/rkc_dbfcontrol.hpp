#ifndef OPENSHADOWFLARE_RKC_DBFCONTROL_HPP
#define OPENSHADOWFLARE_RKC_DBFCONTROL_HPP

#include "gapi/gapi.hpp"

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
    bool drawBitmap(
        const BitmapImage& image,
        const BitmapDraw& draw = {}) override;
    bool drawText(
        const NjpImage& font,
        std::string_view text,
        const TextDraw& draw = {}) override;
    bool drawRectangle(
        const RectangleDraw& draw) override;
    bool drawLine(const LineDraw& draw) override;
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
