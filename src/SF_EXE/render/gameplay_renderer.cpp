#include "gameplay_renderer.hpp"

#include "character_renderer.hpp"
#include "gapi/gapi.hpp"
#include "gameplay_overlay_renderer.hpp"
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

ScreenPosition toScreen(
    std::int32_t world_x,
    std::int32_t world_y) {
    return calculateRealPosition({world_x, world_y});
}

void renderPlayerPass(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t camera_x,
    std::int32_t camera_y,
    bool shadow,
    std::int32_t shadow_opacity,
    double interpolation) {
    if (!world.hasPlayer()) {
        return;
    }
    renderCharacterAnimationPass(
        renderer,
        world.playerAnimation(),
        world.playerPatterns(),
        world.playerShadowPatterns(),
        world.playerRenderPosition(interpolation),
        world.playerAnimationChart(),
        world.playerDirection(),
        world.playerAnimationFrame(),
        [&world](std::size_t part) {
            return world.playerPartEnabled(part);
        },
        [](std::size_t) {
            return CharacterColorStrength{};
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
    bool hovered,
    double interpolation) {
    renderCharacterAnimationPass(
        renderer,
        npc.animation(),
        npc.patterns(),
        npc.shadowPatterns(),
        npc.renderPosition(interpolation),
        npc.animationChart(),
        npc.direction(),
        npc.animationFrame(),
        [&npc](std::size_t part) {
            return npc.partEnabled(part);
        },
        [&npc, hovered](std::size_t part) {
            const std::int32_t hover_strength =
                hovered ? 300 : 0;
            return CharacterColorStrength{
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
    const WorldScene& world,
    double interpolation) {
    std::vector<WorldDrawEntry> result;
    if (world.hasPlayer()) {
        result.push_back({
            nullptr,
            nullptr,
            true,
            calculateRealPosition(
                world.playerRenderPosition(interpolation)).y,
        });
    }
    for (const NpcActor& npc : world.npcs()) {
        result.push_back({
            &npc,
            nullptr,
            false,
            calculateRealPosition(
                npc.renderPosition(interpolation)).y,
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
    renderCharacterAnimationPass(
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
                       ? CharacterColorStrength{
                             item.red_strength,
                             item.green_strength,
                             item.blue_strength,
                         }
                       : CharacterColorStrength{};
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
    std::int32_t shadow_opacity,
    double interpolation) {
    if (entry.player) {
        renderPlayerPass(
            renderer,
            world,
            camera_x,
            camera_y,
            shadow,
            shadow_opacity,
            interpolation);
    } else if (entry.npc) {
        renderNpcPass(
            renderer,
            *entry.npc,
            camera_x,
            camera_y,
            shadow,
            shadow_opacity,
            world.hoveredNpcId() == entry.npc->id(),
            interpolation);
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


}  // namespace

void renderWorld(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t shadow_opacity,
    const gapi::NjpImage* font,
    double interpolation) {
    const GroundMap& ground = world.ground();
    if (ground.width() <= 0 || ground.height() <= 0) {
        return;
    }

    const std::int32_t camera_x =
        world.renderCameraScreenX(interpolation);
    const std::int32_t camera_y =
        world.renderCameraScreenY(interpolation);
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
        collectWorldEntries(world, interpolation);

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
            shadow_opacity,
            interpolation);
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
                shadow_opacity,
                interpolation);
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
            shadow_opacity,
            interpolation);
        ++world_entry_index;
    }
    renderGameplayOverlay(
        renderer,
        world,
        font,
        camera_x,
        camera_y,
        interpolation);
}

}  // namespace osf
