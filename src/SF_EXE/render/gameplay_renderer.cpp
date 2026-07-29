#include "gameplay_renderer.hpp"

#include "gapi/gapi.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osf {
namespace {

constexpr std::int32_t kScreenWidth = 640;
constexpr std::int32_t kScreenHeight = 480;
constexpr std::int32_t kRetailHeightScale = 20;

struct ObjectDrawEntry {
    const MapObject* object = nullptr;
    std::int32_t depth = 0;
    std::int32_t display_class = 0;
};

struct WorldDrawEntry {
    const NpcActor* npc = nullptr;
    const GroundItem* item = nullptr;
    bool player = false;
    std::int32_t depth = 0;
};

struct ColorStrength {
    std::int32_t red = 1000;
    std::int32_t green = 1000;
    std::int32_t blue = 1000;
};

ScreenPosition toScreen(
    std::int32_t world_x,
    std::int32_t world_y) {
    return calculateRealPosition({world_x, world_y});
}

template <typename PartEnabled, typename PartColorStrength>
void renderCharacterPass(
    gapi::Backend& renderer,
    const gapi::CafAnimation& animation,
    const gapi::NjpImage& patterns,
    const gapi::NjpImage& shadow_patterns,
    WorldPosition position,
    std::int32_t chart_index,
    std::int32_t direction_index,
    std::int32_t animation_frame,
    PartEnabled part_enabled,
    PartColorStrength part_color_strength,
    std::int32_t camera_x,
    std::int32_t camera_y,
    bool shadow,
    std::int32_t shadow_opacity,
    std::int32_t screen_height = 0) {
    if (animation.charts().empty()) {
        return;
    }
    const std::int32_t selected_chart_index =
        std::clamp(
            chart_index,
            0,
            static_cast<std::int32_t>(
                animation.charts().size() - 1));
    const gapi::CafChart& chart =
        animation.charts()[
            static_cast<std::size_t>(selected_chart_index)];
    if (direction_index < 0 ||
        static_cast<std::size_t>(direction_index) >=
            chart.directions.size()) {
        return;
    }
    const gapi::CafDirection& direction =
        chart.directions[
            static_cast<std::size_t>(direction_index)];
    if (direction.frame_count <= 0 ||
        direction.parts.empty()) {
        return;
    }
    if ((chart.status & 1) != 0) {
        animation_frame %= direction.frame_count;
    }
    if (animation_frame < 0 ||
        animation_frame >= direction.frame_count) {
        animation_frame = 0;
    }

    struct OrderedCell {
        const gapi::CafCell* cell = nullptr;
        std::size_t part = 0;
    };
    std::vector<OrderedCell> ordered(direction.parts.size());
    for (std::size_t part_index = 0;
         part_index < direction.parts.size();
         ++part_index) {
        if (!part_enabled(part_index)) {
            continue;
        }
        const std::vector<gapi::CafCell>& part =
            direction.parts[part_index];
        if (static_cast<std::size_t>(animation_frame) >=
            part.size()) {
            continue;
        }
        const gapi::CafCell& cell =
            part[static_cast<std::size_t>(animation_frame)];
        if (cell.priority >= 0 &&
            static_cast<std::size_t>(cell.priority) <
                ordered.size()) {
            ordered[
                static_cast<std::size_t>(cell.priority)] = {
                    &cell,
                    part_index,
                };
        }
    }

    for (std::size_t priority = ordered.size();
         priority != 0;
         --priority) {
        const OrderedCell& ordered_cell =
            ordered[priority - 1];
        const gapi::CafCell* cell = ordered_cell.cell;
        if (!cell || cell->pattern_index < 0 ||
            (((cell->status & 8) != 0) != shadow)) {
            continue;
        }
        const ColorStrength strength =
            part_color_strength(ordered_cell.part);
        const ScreenPosition screen_position =
            toScreen(position.x, position.y);
        const std::int32_t palette =
            shadow || animation.palette_mode() == 0
                ? -1
                : animation.chart_priority_stride() *
                          selected_chart_index +
                      cell->priority;
        renderer.drawPattern(
            shadow
                ? shadow_patterns
                : patterns,
            static_cast<std::size_t>(cell->pattern_index),
            {screen_position.x - camera_x,
             screen_position.y - camera_y - screen_height,
             1000,
             1000,
             1000,
             shadow
                 ? std::clamp(shadow_opacity, 0, 1000)
                 : std::clamp<std::int32_t>(
                       cell->transparency, 0, 1000),
             shadow ? 1000 : strength.red,
             shadow ? 1000 : strength.green,
             shadow ? 1000 : strength.blue,
             palette});
    }
}

void renderPlayerPass(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t camera_x,
    std::int32_t camera_y,
    bool shadow,
    std::int32_t shadow_opacity) {
    if (!world.hasPlayer()) {
        return;
    }
    renderCharacterPass(
        renderer,
        world.playerAnimation(),
        world.playerPatterns(),
        world.playerShadowPatterns(),
        {world.playerWorldX(), world.playerWorldY()},
        world.playerAnimationChart(),
        world.playerDirection(),
        world.playerAnimationFrame(),
        [&world](std::size_t part) {
            return world.playerPartEnabled(part);
        },
        [](std::size_t) {
            return ColorStrength{};
        },
        camera_x,
        camera_y,
        shadow,
        shadow_opacity);
}

void renderNpcPass(
    gapi::Backend& renderer,
    const NpcActor& npc,
    std::int32_t camera_x,
    std::int32_t camera_y,
    bool shadow,
    std::int32_t shadow_opacity,
    bool hovered) {
    renderCharacterPass(
        renderer,
        npc.animation(),
        npc.patterns(),
        npc.shadowPatterns(),
        npc.position(),
        npc.animationChart(),
        npc.direction(),
        npc.animationFrame(),
        [&npc](std::size_t part) {
            return npc.partEnabled(part);
        },
        [&npc, hovered](std::size_t part) {
            const std::int32_t hover_strength =
                hovered ? 300 : 0;
            return ColorStrength{
                npc.partRedStrength(part) + hover_strength,
                npc.partGreenStrength(part) + hover_strength,
                npc.partBlueStrength(part) + hover_strength,
            };
        },
        camera_x,
        camera_y,
        shadow,
        shadow_opacity);
}

std::int32_t displayClass(std::int16_t status) {
    std::int32_t result = (status & 0x100) != 0 ? 1 : 0;
    if ((status & 0x80) != 0) {
        result = 2;
    }
    if ((status & 0x20) != 0) {
        result = 3;
    }
    return result;
}

bool isDefaultDisplayClass(std::int16_t status) {
    return (status & 0x1a0) == 0;
}

const gapi::NjpImage* objectImage(
    const WorldScene& world,
    const MapObject& object,
    bool shadow) {
    const std::int32_t patternSet =
        object.pattern_set + (shadow ? 1 : 0);
    if (patternSet < 0 ||
        static_cast<std::size_t>(patternSet) >=
            world.mapPatterns().size()) {
        return nullptr;
    }
    const auto& image =
        world.mapPatterns()[
            static_cast<std::size_t>(patternSet)];
    if (!image || image->isShadow() != shadow ||
        object.pattern < 0 ||
        static_cast<std::size_t>(object.pattern) >=
            image->patterns().size()) {
        return nullptr;
    }
    return image.get();
}

bool objectVisible(
    const gapi::NjpImage& image,
    const MapObject& object,
    std::int32_t camera_x,
    std::int32_t camera_y,
    bool shadow) {
    const gapi::NjpPattern& pattern =
        image.patterns()[
            static_cast<std::size_t>(object.pattern)];
    const ScreenPosition anchor =
        toScreen(object.world_x, object.world_y);
    const std::int32_t anchorX = anchor.x - camera_x;
    const std::int32_t anchorY =
        anchor.y -
        camera_y -
        (shadow ? 0 : object.height * kRetailHeightScale / 100);
    return anchorX + pattern.x < kScreenWidth &&
           anchorX + pattern.x + pattern.width > 0 &&
           anchorY + pattern.y < kScreenHeight &&
           anchorY + pattern.y + pattern.height > 0;
}

void drawMapObject(
    gapi::Backend& renderer,
    const WorldScene& world,
    const MapObject& object,
    std::int32_t camera_x,
    std::int32_t camera_y,
    bool shadow,
    std::int32_t shadow_opacity) {
    const gapi::NjpImage* image =
        objectImage(world, object, shadow);
    if (!image ||
        !objectVisible(
            *image, object, camera_x, camera_y, shadow)) {
        return;
    }
    const ScreenPosition position =
        toScreen(object.world_x, object.world_y);
    renderer.drawPattern(
        *image,
        static_cast<std::size_t>(object.pattern),
        {position.x - camera_x,
         position.y -
             camera_y -
             (shadow
                  ? 0
                  : object.height * kRetailHeightScale / 100),
         1000,
         1000,
         1000,
         shadow
             ? std::clamp(shadow_opacity, 0, 1000)
             : std::clamp<std::int32_t>(
                   object.opacity, 0, 1000),
         shadow ? 1000 : object.red_strength,
         shadow ? 1000 : object.green_strength,
         shadow ? 1000 : object.blue_strength});
}

std::vector<ObjectDrawEntry> collectObjects(
    const WorldScene& world,
    bool default_class,
    std::int32_t camera_x,
    std::int32_t camera_y) {
    std::vector<ObjectDrawEntry> result;
    for (const MapObject& object :
         world.objectMap().objects()) {
        const gapi::NjpImage* image =
            objectImage(world, object, false);
        const gapi::NjpImage* shadow =
            (object.status & 8) != 0
                ? objectImage(world, object, true)
                : nullptr;
        const bool visible =
            image && objectVisible(
                         *image,
                         object,
                         camera_x,
                         camera_y,
                         false);
        const bool shadowVisible =
            shadow && objectVisible(
                          *shadow,
                          object,
                          camera_x,
                          camera_y,
                          true);
        if ((!visible && !shadowVisible) ||
            isDefaultDisplayClass(object.status) != default_class) {
            continue;
        }
        result.push_back({
            &object,
            toScreen(
                object.world_x + object.judgement.left,
                object.world_y + object.judgement.top).y,
            displayClass(object.status),
        });
    }
    std::stable_sort(
        result.begin(),
        result.end(),
        [](const ObjectDrawEntry& left,
           const ObjectDrawEntry& right) {
            if (left.display_class != right.display_class) {
                return left.display_class < right.display_class;
            }
            return left.depth < right.depth;
        });
    return result;
}

void drawObjectShadows(
    gapi::Backend& renderer,
    const WorldScene& world,
    const std::vector<ObjectDrawEntry>& objects,
    std::int32_t camera_x,
    std::int32_t camera_y,
    std::int32_t shadow_opacity) {
    for (const ObjectDrawEntry& entry : objects) {
        if ((entry.object->status & 8) == 0) {
            continue;
        }
        drawMapObject(
            renderer,
            world,
            *entry.object,
            camera_x,
            camera_y,
            true,
            shadow_opacity);
    }
}

std::vector<WorldDrawEntry> collectWorldEntries(
    const WorldScene& world) {
    std::vector<WorldDrawEntry> result;
    if (world.hasPlayer()) {
        result.push_back({
            nullptr,
            nullptr,
            true,
            toScreen(
                world.playerWorldX(),
                world.playerWorldY()).y,
        });
    }
    for (const NpcActor& npc : world.npcs()) {
        result.push_back({
            &npc,
            nullptr,
            false,
            toScreen(
                npc.position().x,
                npc.position().y).y,
        });
    }
    for (const GroundItem& item : world.groundItems()) {
        result.push_back({
            nullptr,
            &item,
            false,
            toScreen(item.position.x, item.position.y).y,
        });
    }
    std::stable_sort(
        result.begin(),
        result.end(),
        [](const WorldDrawEntry& left,
           const WorldDrawEntry& right) {
            return left.depth < right.depth;
        });
    return result;
}

void drawGroundItem(
    gapi::Backend& renderer,
    const WorldScene& world,
    const GroundItem& item,
    std::int32_t camera_x,
    std::int32_t camera_y,
    bool shadow,
    std::int32_t shadow_opacity) {
    const ItemWorldResource* resource =
        world.itemWorldResource(item.resource_id);
    if (!resource) {
        return;
    }
    renderCharacterPass(
        renderer,
        resource->animation(),
        resource->patterns(),
        resource->shadowPatterns(),
        item.position,
        item.animation_chart,
        8,
        0,
        [](std::size_t) {
            return true;
        },
        [&item](std::size_t part) {
            return part == 0
                       ? ColorStrength{
                             item.red_strength,
                             item.green_strength,
                             item.blue_strength,
                         }
                       : ColorStrength{};
        },
        camera_x,
        camera_y,
        shadow,
        shadow_opacity,
        shadow
            ? 0
            : item.height * kRetailHeightScale / 100);
}

void drawWorldEntry(
    gapi::Backend& renderer,
    const WorldScene& world,
    const WorldDrawEntry& entry,
    std::int32_t camera_x,
    std::int32_t camera_y,
    bool shadow,
    std::int32_t shadow_opacity) {
    if (entry.player) {
        renderPlayerPass(
            renderer,
            world,
            camera_x,
            camera_y,
            shadow,
            shadow_opacity);
    } else if (entry.npc) {
        renderNpcPass(
            renderer,
            *entry.npc,
            camera_x,
            camera_y,
            shadow,
            shadow_opacity,
            world.hoveredNpcId() == entry.npc->id());
    } else if (entry.item) {
        drawGroundItem(
            renderer,
            world,
            *entry.item,
            camera_x,
            camera_y,
            shadow,
            shadow_opacity);
    }
}

std::string conversationDisplayText(
    const std::string& source) {
    std::string result;
    result.reserve(source.size());
    for (std::size_t index = 0; index < source.size(); ++index) {
        const unsigned char byte =
            static_cast<unsigned char>(source[index]);
        if (byte == '\r') {
            continue;
        }
        result.push_back(static_cast<char>(byte));
    }
    return result;
}

bool shiftJisLead(std::uint8_t value) {
    return (value >= 0x80u && value <= 0x9fu) ||
           value >= 0xe0u;
}

std::int32_t textPixelWidth(
    std::string_view text,
    std::int32_t cell_width) {
    std::int32_t width = 0;
    std::int32_t maximum = 0;
    for (std::size_t index = 0; index < text.size(); ++index) {
        const std::uint8_t byte =
            static_cast<std::uint8_t>(text[index]);
        if (byte == '\r') {
            continue;
        }
        if (byte == '\n') {
            maximum = std::max(maximum, width);
            width = 0;
            continue;
        }
        if (shiftJisLead(byte) && index + 1 < text.size()) {
            ++index;
            width += cell_width * 2;
        } else {
            width += cell_width;
        }
    }
    return std::max(maximum, width);
}

std::int32_t textLineCount(std::string_view text) {
    std::int32_t lines = 0;
    bool content_after_break = false;
    for (char character : text) {
        if (character == '\r') {
            continue;
        }
        if (character == '\n') {
            ++lines;
            content_after_break = false;
        } else {
            content_after_break = true;
        }
    }
    if (content_after_break || lines == 0) {
        ++lines;
    }
    return lines;
}

const NpcActor* findNpc(
    const WorldScene& world,
    std::int32_t id) {
    const auto found = std::find_if(
        world.npcs().begin(),
        world.npcs().end(),
        [id](const NpcActor& npc) {
            return npc.id() == id;
        });
    return found == world.npcs().end() ? nullptr : &*found;
}

gapi::Color npcNameColor(const NpcActor& npc) {
    const std::uint32_t color = npc.nameColor();
    return {
        static_cast<std::uint8_t>(color),
        static_cast<std::uint8_t>(color >> 8u),
        static_cast<std::uint8_t>(color >> 16u),
        255,
    };
}

void drawHoveredNpcLabel(
    gapi::Backend& renderer,
    const WorldScene& world,
    const gapi::NjpImage* font,
    std::int32_t camera_x,
    std::int32_t camera_y) {
    if (!font) {
        return;
    }
    const NpcActor* npc =
        findNpc(world, world.hoveredNpcId());
    if (!npc || npc->name().empty()) {
        return;
    }
    const ScreenPosition anchor =
        toScreen(npc->position().x, npc->position().y);
    const std::int32_t center_x = anchor.x - camera_x;
    const std::int32_t label_y =
        anchor.y - camera_y - npc->labelHeight();
    const std::int32_t half_width =
        textPixelWidth(npc->name(), 6) / 2;
    renderer.drawRectangle({
        center_x - half_width - 4,
        label_y - 2,
        half_width * 2 + 5,
        15,
        {0, 0, 0, 255},
        1000,
        500,
    });
    renderer.drawText(
        *font,
        npc->name(),
        {
            center_x - half_width + 1,
            label_y + 1,
            {0, 0, 0, 255},
        });
    renderer.drawText(
        *font,
        npc->name(),
        {
            center_x - half_width,
            label_y,
            npcNameColor(*npc),
        });
}

void drawConversation(
    gapi::Backend& renderer,
    const WorldScene& world,
    const gapi::NjpImage* font,
    std::int32_t camera_x,
    std::int32_t camera_y) {
    if (!world.conversationActive() || !font ||
        font->patterns().empty()) {
        return;
    }
    const NpcActor* actor =
        findNpc(world, world.conversationActorId());
    if (!actor) {
        return;
    }
    const gapi::NjpPattern& font_pattern =
        font->patterns().front();
    const std::int32_t cell_width =
        font_pattern.width / 16;
    const std::int32_t cell_height =
        font_pattern.height / 16;
    if (cell_width <= 0 || cell_height <= 0) {
        return;
    }
    const std::string text =
        conversationDisplayText(world.conversationText());
    const std::int32_t width =
        textPixelWidth(text, cell_width) + 8;
    const std::int32_t height =
        textLineCount(text) * cell_height + 8;
    const ScreenPosition projected =
        toScreen(actor->position().x, actor->position().y);
    const std::int32_t anchor_x =
        projected.x - camera_x;
    const std::int32_t anchor_y =
        projected.y - camera_y - actor->labelHeight();
    const std::int32_t x =
        anchor_x + 12 - width / 2;
    const std::int32_t y =
        anchor_y - 16 - height;
    const std::int32_t frame_x = x - 9;
    const std::int32_t frame_y = y - 9;
    const std::int32_t frame_width = width + 18;
    const std::int32_t frame_height = height + 18;

    // Hukidasi patterns 0-3 are the four 9x9 rounded corners. Their edge
    // pixels continue as black, black, 160, 224, then the 248 interior.
    // Keep the fills out of each transparent outer corner so the world
    // remains visible around the curve.
    renderer.drawRectangle({
        frame_x + 4,
        frame_y + 4,
        frame_width - 8,
        frame_height - 8,
        {255, 255, 255, 255},
    });
    renderer.drawRectangle({
        frame_x + 9,
        frame_y,
        frame_width - 18,
        2,
        {0, 0, 0, 255},
    });
    renderer.drawRectangle({
        frame_x + 9,
        frame_y + 2,
        frame_width - 18,
        1,
        {160, 160, 160, 255},
    });
    renderer.drawRectangle({
        frame_x + 9,
        frame_y + 3,
        frame_width - 18,
        1,
        {224, 224, 224, 255},
    });
    renderer.drawRectangle({
        frame_x + 9,
        frame_y + frame_height - 2,
        frame_width - 18,
        2,
        {0, 0, 0, 255},
    });
    renderer.drawRectangle({
        frame_x + 9,
        frame_y + frame_height - 3,
        frame_width - 18,
        1,
        {160, 160, 160, 255},
    });
    renderer.drawRectangle({
        frame_x + 9,
        frame_y + frame_height - 4,
        frame_width - 18,
        1,
        {224, 224, 224, 255},
    });
    renderer.drawRectangle({
        frame_x,
        frame_y + 9,
        2,
        frame_height - 18,
        {0, 0, 0, 255},
    });
    renderer.drawRectangle({
        frame_x + 2,
        frame_y + 9,
        1,
        frame_height - 18,
        {160, 160, 160, 255},
    });
    renderer.drawRectangle({
        frame_x + 3,
        frame_y + 9,
        1,
        frame_height - 18,
        {224, 224, 224, 255},
    });
    renderer.drawRectangle({
        frame_x + frame_width - 2,
        frame_y + 9,
        2,
        frame_height - 18,
        {0, 0, 0, 255},
    });
    renderer.drawRectangle({
        frame_x + frame_width - 3,
        frame_y + 9,
        1,
        frame_height - 18,
        {160, 160, 160, 255},
    });
    renderer.drawRectangle({
        frame_x + frame_width - 4,
        frame_y + 9,
        1,
        frame_height - 18,
        {224, 224, 224, 255},
    });
    const gapi::NjpImage& frame = world.speechPatterns();
    if (frame.patterns().size() >= 5) {
        renderer.drawPattern(
            frame, 0, {frame_x, frame_y});
        renderer.drawPattern(
            frame,
            2,
            {frame_x + frame_width - 9, frame_y});
        renderer.drawPattern(
            frame,
            1,
            {frame_x, frame_y + frame_height - 9});
        renderer.drawPattern(
            frame,
            3,
            {frame_x + frame_width - 9,
             frame_y + frame_height - 9});
        renderer.drawPattern(
            frame,
            4,
            {x + width / 2 - 5, y + height + 5});
    }
    renderer.drawText(
        *font,
        text,
        {
            x + 4,
            y + 4,
            {0, 0, 0, 255},
        });
}

}  // namespace

void renderInitialLoadingScreen(
    gapi::Backend& renderer,
    const gapi::NjpImage& waiting,
    std::int32_t counter,
    bool ready_to_continue) {
    renderer.drawPattern(
        waiting,
        0,
        {0, 0});
    if (!ready_to_continue) {
        renderer.drawPattern(
            waiting,
            3,
            {572, 443});
        return;
    }

    const std::int32_t arrow_offset =
        std::max(counter, 0) % 16;
    renderer.drawPattern(
        waiting,
        2,
        {592 + arrow_offset, 450});
}

void renderWorld(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t shadow_opacity,
    const gapi::NjpImage* font) {
    const GroundMap& ground = world.ground();
    if (ground.width() <= 0 || ground.height() <= 0) {
        return;
    }

    const std::int32_t camera_x =
        world.cameraScreenX();
    const std::int32_t camera_y =
        world.cameraScreenY();
    const std::int32_t start_x =
        std::max(camera_x / ground.chipWidth(), 0);
    const std::int32_t start_y =
        std::max(camera_y / ground.chipHeight(), 0);
    const std::int32_t end_x = std::min(
        (camera_x + kScreenWidth) / ground.chipWidth(),
        ground.width() - 1);
    const std::int32_t end_y = std::min(
        (camera_y + kScreenHeight) / ground.chipHeight(),
        ground.height() - 1);

    const auto& patterns = world.mapPatterns();
    for (std::int32_t y = start_y; y <= end_y; ++y) {
        for (std::int32_t x = start_x; x <= end_x; ++x) {
            const GroundCell* cell = ground.cell(x, y);
            if (!cell || cell->pattern_set < 0 ||
                static_cast<std::size_t>(cell->pattern_set) >=
                    patterns.size() ||
                !patterns[
                    static_cast<std::size_t>(
                        cell->pattern_set)] ||
                cell->pattern < 0) {
                continue;
            }
            renderer.drawPattern(
                *patterns[
                    static_cast<std::size_t>(
                        cell->pattern_set)],
                static_cast<std::size_t>(cell->pattern),
                {ground.chipWidth() * x - camera_x,
                 ground.chipHeight() * y - camera_y});
        }
    }

    const std::vector<ObjectDrawEntry> specialObjects =
        collectObjects(world, false, camera_x, camera_y);
    const std::vector<ObjectDrawEntry> defaultObjects =
        collectObjects(world, true, camera_x, camera_y);
    const std::vector<WorldDrawEntry> worldEntries =
        collectWorldEntries(world);

    drawObjectShadows(
        renderer,
        world,
        specialObjects,
        camera_x,
        camera_y,
        shadow_opacity);
    for (const ObjectDrawEntry& entry : specialObjects) {
        drawMapObject(
            renderer,
            world,
            *entry.object,
            camera_x,
            camera_y,
            false,
            shadow_opacity);
    }

    drawObjectShadows(
        renderer,
        world,
        defaultObjects,
        camera_x,
        camera_y,
        shadow_opacity);

    for (const WorldDrawEntry& entry : worldEntries) {
        drawWorldEntry(
            renderer,
            world,
            entry,
            camera_x,
            camera_y,
            true,
            shadow_opacity);
    }

    std::size_t world_entry_index = 0;
    for (const ObjectDrawEntry& entry : defaultObjects) {
        while (world_entry_index < worldEntries.size() &&
               worldEntries[world_entry_index].depth < entry.depth) {
            drawWorldEntry(
                renderer,
                world,
                worldEntries[world_entry_index],
                camera_x,
                camera_y,
                false,
                shadow_opacity);
            ++world_entry_index;
        }
        drawMapObject(
            renderer,
            world,
            *entry.object,
            camera_x,
            camera_y,
            false,
            shadow_opacity);
    }
    while (world_entry_index < worldEntries.size()) {
        drawWorldEntry(
            renderer,
            world,
            worldEntries[world_entry_index],
            camera_x,
            camera_y,
            false,
            shadow_opacity);
        ++world_entry_index;
    }
    drawHoveredNpcLabel(
        renderer, world, font, camera_x, camera_y);
    drawConversation(
        renderer, world, font, camera_x, camera_y);
}

}  // namespace osf
