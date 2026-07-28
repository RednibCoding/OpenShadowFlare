#include "gameplay_renderer.hpp"

#include "gapi/gapi.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {
namespace {

constexpr std::int32_t kScreenWidth = 640;
constexpr std::int32_t kScreenHeight = 480;
constexpr std::int32_t kRetailBaseX = 15;
constexpr std::int32_t kRetailBaseY = 10;
constexpr std::int32_t kRetailHeightScale = 20;

struct ObjectDrawEntry {
    const MapObject* object = nullptr;
    std::int32_t depth = 0;
    std::int32_t display_class = 0;
};

std::int32_t toScreenX(
    std::int32_t world_x,
    std::int32_t world_y) {
    return static_cast<std::int32_t>(
        (static_cast<std::int64_t>(world_x - world_y) *
         kRetailBaseX) /
        100);
}

std::int32_t toScreenY(
    std::int32_t world_x,
    std::int32_t world_y) {
    return static_cast<std::int32_t>(
        (static_cast<std::int64_t>(world_x + world_y) *
         kRetailBaseY) /
        100);
}

void renderPlayerPass(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t animation_frame,
    bool shadow,
    std::int32_t shadow_opacity) {
    if (!world.hasPlayer() ||
        world.playerAnimation().charts().empty()) {
        return;
    }
    const gapi::CafChart& chart =
        world.playerAnimation().charts().front();
    const std::int32_t direction_index =
        world.playerDirection();
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

    std::vector<const gapi::CafCell*> ordered(
        direction.parts.size(), nullptr);
    for (std::size_t part_index = 0;
         part_index < direction.parts.size();
         ++part_index) {
        if (!world.playerPartEnabled(part_index)) {
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
                static_cast<std::size_t>(cell.priority)] = &cell;
        }
    }

    for (std::size_t priority = ordered.size();
         priority != 0;
         --priority) {
        const gapi::CafCell* cell = ordered[priority - 1];
        if (!cell || cell->pattern_index < 0 ||
            (((cell->status & 8) != 0) != shadow)) {
            continue;
        }
        renderer.drawPattern(
            shadow
                ? world.playerShadowPatterns()
                : world.playerPatterns(),
            static_cast<std::size_t>(cell->pattern_index),
            {kScreenWidth / 2,
             kScreenHeight / 2,
             1000,
             1000,
             1000,
             shadow
                 ? std::clamp(shadow_opacity, 0, 1000)
                 : std::clamp<std::int32_t>(
                       cell->transparency, 0, 1000)});
    }
}

void renderPlayer(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t animation_frame) {
    renderPlayerPass(
        renderer, world, animation_frame, false, 1000);
}

void renderPlayerShadow(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t animation_frame,
    std::int32_t shadow_opacity) {
    renderPlayerPass(
        renderer,
        world,
        animation_frame,
        true,
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
    const std::int32_t anchorX =
        toScreenX(object.world_x, object.world_y) -
        camera_x;
    const std::int32_t anchorY =
        toScreenY(object.world_x, object.world_y) -
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
    const std::int32_t brightness = std::clamp(
        (static_cast<std::int32_t>(object.red_strength) +
         static_cast<std::int32_t>(object.green_strength) +
         static_cast<std::int32_t>(object.blue_strength)) /
            3,
        0,
        1000);
    renderer.drawPattern(
        *image,
        static_cast<std::size_t>(object.pattern),
        {toScreenX(object.world_x, object.world_y) -
             camera_x,
         toScreenY(object.world_x, object.world_y) -
             camera_y -
             (shadow
                  ? 0
                  : object.height * kRetailHeightScale / 100),
         1000,
         1000,
         shadow ? 1000 : brightness,
         shadow
             ? std::clamp(shadow_opacity, 0, 1000)
             : std::clamp<std::int32_t>(
                   object.opacity, 0, 1000)});
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
            toScreenY(
                object.world_x + object.judgement.left,
                object.world_y + object.judgement.top),
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
    std::int32_t animation_frame,
    std::int32_t shadow_opacity) {
    const GroundMap& ground = world.ground();
    if (ground.width() <= 0 || ground.height() <= 0) {
        return;
    }

    const std::int32_t camera_x =
        toScreenX(
            world.playerWorldX(),
            world.playerWorldY()) -
        kScreenWidth / 2;
    const std::int32_t camera_y =
        toScreenY(
            world.playerWorldX(),
            world.playerWorldY()) -
        kScreenHeight / 2;
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
    renderPlayerShadow(
        renderer,
        world,
        animation_frame,
        shadow_opacity);
    const std::int32_t playerDepth =
        toScreenY(
            world.playerWorldX(),
            world.playerWorldY());
    bool playerDrawn = false;
    for (const ObjectDrawEntry& entry : defaultObjects) {
        if (!playerDrawn && world.hasPlayer() &&
            playerDepth < entry.depth) {
            renderPlayer(renderer, world, animation_frame);
            playerDrawn = true;
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
    if (!playerDrawn) {
        renderPlayer(renderer, world, animation_frame);
    }
}

}  // namespace osf
