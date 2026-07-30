#include "gameplay_transport_renderer.hpp"

#include "gapi/gapi.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "states/gameplay_transport.hpp"
#include "ui/conversation_layout.hpp"
#include "world/transport_catalog.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace osf {
namespace {

void drawText(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const std::string& text,
    std::int32_t x,
    std::int32_t y,
    std::uint8_t strength) {
    renderer.drawText(
        font,
        text,
        {x + 1, y + 1, {0, 0, 0, 255}});
    renderer.drawText(
        font,
        text,
        {x, y, {strength, strength, strength, 255}});
}

}  // namespace

void renderGameplayTransport(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const GameplayTransport& transport,
    const TransportCatalog& catalog) {
    if (!transport.active()) {
        return;
    }

    renderer.drawRectangle({
        0,
        0,
        320,
        412,
        {0, 0, 0, 255},
    });
    // FUN_0040c950 uses Status.njp pattern 13 for the authored
    // transport frame, then compacts enabled Table 40 rows into ten
    // slots per page.
    renderer.drawPattern(status_patterns, 13);
    const std::vector<std::int32_t> enabled =
        catalog.enabledRows();
    const std::vector<std::int32_t> visible =
        transport.visibleDestinations(enabled);
    for (std::size_t index = 0;
         index < visible.size();
         ++index) {
        const TransportDestination* destination =
            catalog.find(visible[index]);
        if (!destination) {
            continue;
        }
        const bool hovered =
            transport.hoveredDestination() ==
            destination->row;
        const std::int32_t y =
            59 + static_cast<std::int32_t>(index) * 30;
        renderer.drawPattern(
            status_patterns,
            hovered ? 23 : 22,
            {52, y});
        drawText(
            renderer,
            font,
            destination->name,
            79,
            y + 6,
            hovered ? 224 : 128);
    }

    const std::int32_t pages =
        transport.pageCount(enabled.size());
    if (transport.page() > 0) {
        renderer.drawPattern(status_patterns, 11);
    }
    if (transport.page() + 1 < pages) {
        renderer.drawPattern(status_patterns, 12);
    }
    if (!visible.empty()) {
        const std::string page =
            std::to_string(transport.page() + 1) +
            " / " + std::to_string(pages);
        drawText(
            renderer,
            font,
            page,
            (153 - bitmapTextPixelWidth(page, 6)) / 2 + 84,
            370,
            224);
    }
}

}  // namespace osf
