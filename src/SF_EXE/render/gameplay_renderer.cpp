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

struct WorldDrawEntry {
    const MapObject* object = nullptr;
    const NpcActor* npc = nullptr;
    const GroundItem* item = nullptr;
    bool player = false;
    bool semi_transparent = false;
    DisplayOrderEntry order;
    const ScenarioObjectActor* scenario_object = nullptr;
    const EnemyActor* enemy = nullptr;
    const CombatEffectActor* effect = nullptr;
    const RuntimeEffectActor* runtime_effect = nullptr;
    const MissEffectActor* miss_effect = nullptr;
    const CompanionActor* companion = nullptr;
    bool companion_moon_aura = false;
    bool player_transport = false;
    const PlayerLandMineVisual* land_mine = nullptr;
};

ScreenPosition toScreen(
    std::int32_t world_x,
    std::int32_t world_y) {
    return calculateRealPosition({world_x, world_y});
}

void renderPlayerPowerupPass(
    gapi::Backend& renderer,
    const WorldScene& world,
    const EffectVisualResource* visual,
    bool active,
    std::int32_t frame,
    CharacterColorStrength color,
    std::int32_t camera_x,
    std::int32_t camera_y,
    double interpolation) {
    if (!active || !visual ||
        visual->animation().charts().empty()) {
        return;
    }
    const gapi::CafDirection& direction =
        visual->animation().charts().front().directions[8];
    if (direction.frame_count < 1) {
        return;
    }
    renderCharacterAnimationPass(
        renderer,
        visual->animation(),
        visual->patterns(),
        visual->patterns(),
        world.playerRenderPosition(interpolation),
        0,
        8,
        frame % direction.frame_count,
        [visual](std::size_t part) {
            return part < visual->animation().maxPartCount();
        },
        [color](std::size_t) {
            return color;
        },
        camera_x,
        camera_y,
        false,
        0);
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
        [&world](std::size_t part) {
            return CharacterColorStrength{
                world.playerPartRedStrength(part),
                world.playerPartGreenStrength(part),
                world.playerPartBlueStrength(part),
            };
        },
        camera_x,
        camera_y,
        shadow,
        shadow_opacity);
    if (!shadow) {
        renderPlayerPowerupPass(
            renderer,
            world,
            world.playerMagicShieldVisual(),
            world.playerMagicShieldActive(),
            world.playerMagicShieldFrame(),
            {},
            camera_x,
            camera_y,
            interpolation);
        renderPlayerPowerupPass(
            renderer,
            world,
            world.playerCounterBurstVisual(),
            world.playerCounterBurstActive(),
            world.playerCounterBurstFrame(),
            {},
            camera_x,
            camera_y,
            interpolation);
        renderPlayerPowerupPass(
            renderer,
            world,
            world.playerIncreasedPowerVisual(),
            world.playerIncreasedPowerActive(),
            world.playerIncreasedPowerFrame(),
            {},
            camera_x,
            camera_y,
            interpolation);
        renderPlayerPowerupPass(
            renderer,
            world,
            world.playerBerserkerVisual(),
            world.playerBerserkerActive(),
            world.playerBerserkerFrame(),
            {1000, 200, 200},
            camera_x,
            camera_y,
            interpolation);
        renderPlayerPowerupPass(
            renderer,
            world,
            world.playerEnergyShieldVisual(),
            world.playerEnergyShieldActive(),
            world.playerEnergyShieldFrame(),
            {1000, 1000, 300},
            camera_x,
            camera_y,
            interpolation);
    }
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

void renderCompanionPass(
    gapi::Backend& renderer,
    const CompanionActor& companion,
    std::int32_t camera_x,
    std::int32_t camera_y,
    bool shadow,
    std::int32_t shadow_opacity,
    double interpolation) {
    renderCharacterAnimationPass(
        renderer,
        companion.animation(),
        companion.patterns(),
        companion.shadowPatterns(),
        companion.renderPosition(interpolation),
        companion.animationChart(),
        companion.animationDirection(),
        companion.animationFrame(),
        [&companion](std::size_t part) {
            return companion.partEnabled(part);
        },
        [&companion](std::size_t part) {
            return CharacterColorStrength{
                companion.partRedStrength(part),
                companion.partGreenStrength(part),
                companion.partBlueStrength(part),
            };
        },
        camera_x,
        camera_y,
        shadow,
        shadow_opacity,
        0,
        companion.drawOpacity());
}

void renderCompanionMoonAura(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t camera_x,
    std::int32_t camera_y,
    double interpolation) {
    const EffectVisualResource* visual =
        world.companionMoonAuraVisual();
    if (!visual || visual->animation().charts().empty()) {
        return;
    }
    const gapi::CafDirection& direction =
        visual->animation().charts().front().directions[8];
    if (direction.frame_count < 1) {
        return;
    }
    const std::int32_t frame =
        world.companionMoonAuraFrame() %
        direction.frame_count;
    renderCharacterAnimationPass(
        renderer,
        visual->animation(),
        visual->patterns(),
        visual->patterns(),
        world.companion().renderPosition(interpolation),
        0,
        8,
        frame,
        [visual](std::size_t part) {
            return part < visual->animation().maxPartCount();
        },
        [](std::size_t) {
            return CharacterColorStrength{};
        },
        camera_x,
        camera_y,
        false,
        0);
}

void renderEnemyPass(
    gapi::Backend& renderer,
    const EnemyActor& enemy,
    std::int32_t camera_x,
    std::int32_t camera_y,
    bool shadow,
    std::int32_t shadow_opacity,
    bool hovered,
    double interpolation) {
    renderCharacterAnimationPass(
        renderer,
        enemy.animation(),
        enemy.patterns(),
        enemy.shadowPatterns(),
        enemy.renderPosition(interpolation),
        enemy.animationChart(),
        enemy.direction(),
        enemy.animationFrame(),
        [&enemy](std::size_t part) {
            return enemy.partEnabled(part);
        },
        [&enemy, hovered](std::size_t part) {
            const std::int32_t hover_strength =
                hovered ? 300 : 0;
            return CharacterColorStrength{
                enemy.partRedStrength(part) +
                    hover_strength,
                enemy.partGreenStrength(part) +
                    hover_strength,
                enemy.partBlueStrength(part) +
                    hover_strength,
            };
        },
        camera_x,
        camera_y,
        shadow,
        shadow_opacity * enemy.drawStrength() / 1000,
        0,
        enemy.drawStrength());
}

void renderCombatEffect(
    gapi::Backend& renderer,
    const CombatEffectActor& effect,
    std::int32_t camera_x,
    std::int32_t camera_y) {
    renderCharacterAnimationPass(
        renderer,
        effect.animation(),
        effect.patterns(),
        effect.patterns(),
        effect.position(),
        effect.animationChart(),
        effect.direction(),
        effect.animationFrame(),
        [&effect](std::size_t part) {
            return effect.partEnabled(part);
        },
        [](std::size_t) {
            return CharacterColorStrength{};
        },
        camera_x,
        camera_y,
        false,
        0,
        effect.displayHeight() / 10,
        effect.drawStrength());
}

void renderRuntimeEffect(
    gapi::Backend& renderer,
    const RuntimeEffectActor& effect,
    std::int32_t camera_x,
    std::int32_t camera_y,
    double interpolation) {
    renderCharacterAnimationPass(
        renderer,
        effect.animation(),
        effect.patterns(),
        effect.patterns(),
        effect.renderPosition(interpolation),
        effect.animationChart(),
        effect.animationDirection(),
        effect.animationFrame(),
        [&effect](std::size_t part) {
            return effect.partEnabled(part);
        },
        [&effect](std::size_t) {
            return CharacterColorStrength{
                effect.redStrength(),
                effect.greenStrength(),
                effect.blueStrength(),
            };
        },
        camera_x,
        camera_y,
        false,
        0,
        effect.displayHeight() / 10,
        1000,
        effect.additionalDisplayStatus());
}

void renderPlayerLandMine(
    gapi::Backend& renderer,
    const WorldScene& world,
    const PlayerLandMineVisual& mine,
    std::int32_t camera_x,
    std::int32_t camera_y,
    double interpolation) {
    const WorldPosition world_position =
        mine.renderPosition(interpolation);
    const std::int32_t display_height =
        mine.renderDisplayHeight(interpolation) / 10;
    if (mine.static_pattern) {
        const gapi::NjpImage* patterns =
            world.playerLandMinePatterns();
        if (!patterns || patterns->patterns().empty()) {
            return;
        }
        const ScreenPosition position =
            calculateRealPosition(world_position);
        renderer.drawPattern(
            *patterns,
            0,
            {
                position.x - camera_x,
                position.y - camera_y - display_height,
            });
        return;
    }
    const EffectVisualResource* visual =
        world.playerLandMineVisualResource(
            mine.resource_id);
    if (!visual || visual->animation().charts().empty()) {
        return;
    }
    const gapi::CafDirection& direction =
        visual->animation().charts().front().directions[8];
    if (direction.frame_count < 1) {
        return;
    }
    renderCharacterAnimationPass(
        renderer,
        visual->animation(),
        visual->patterns(),
        visual->patterns(),
        world_position,
        0,
        8,
        mine.animation_frame % direction.frame_count,
        [visual](std::size_t part) {
            return part < visual->animation().maxPartCount();
        },
        [](std::size_t) {
            return CharacterColorStrength{};
        },
        camera_x,
        camera_y,
        false,
        0,
        display_height);
}

void renderPlayerTransport(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t camera_x,
    std::int32_t camera_y) {
    const PlayerTransportSpell& transport =
        world.playerTransportSpell();
    const PlayerTransportEndpoint* endpoint =
        transport.endpoint(world.scenarioId());
    const gapi::NjpImage* patterns =
        world.playerTransportPatterns();
    const EffectVisualResource* visual =
        world.playerTransportVisual();
    if (!endpoint || !patterns || !visual) {
        return;
    }

    const ScreenPosition position =
        calculateRealPosition(endpoint->position);
    for (std::size_t index = 0;
         index < transport.beams().size() &&
         index < patterns->patterns().size();
         ++index) {
        const PlayerTransportBeam& beam =
            transport.beams()[index];
        renderer.drawPattern(
            *patterns,
            index,
            {
                position.x - camera_x,
                position.y - camera_y - beam.height / 10,
                1000,
                1000,
                1000,
                beam.strength,
                1000,
                1000,
                1000,
                -1,
                {},
                gapi::PatternBlendMode::additive,
            });
    }
    if (!transport.centerVisible() ||
        visual->animation().charts().empty()) {
        return;
    }
    renderCharacterAnimationPass(
        renderer,
        visual->animation(),
        visual->patterns(),
        visual->patterns(),
        endpoint->position,
        0,
        8,
        transport.animationFrame(world.scenarioId()),
        [visual](std::size_t part) {
            return part < visual->animation().maxPartCount();
        },
        [](std::size_t) {
            return CharacterColorStrength{};
        },
        camera_x,
        camera_y,
        false,
        0);
}

void renderMissEffect(
    gapi::Backend& renderer,
    const MissEffectActor& effect,
    std::int32_t camera_x,
    std::int32_t camera_y) {
    const ScreenPosition position =
        calculateRealPosition(effect.position());
    renderer.drawPattern(
        effect.patterns(),
        0,
        {
            position.x - camera_x,
            position.y - camera_y -
                effect.height() / 10,
            1000,
            1000,
            1000,
            effect.opacity(),
        });
}

void renderScenarioObjectPass(
    gapi::Backend& renderer,
    const ScenarioObjectActor& object,
    std::int32_t camera_x,
    std::int32_t camera_y,
    bool shadow,
    std::int32_t shadow_opacity,
    bool hovered) {
    if (!object.drawEnabled()) {
        return;
    }
    if (object.hasStaticVisual()) {
        if (shadow && !object.hasStaticShadow()) {
            return;
        }
        const gapi::NjpImage& image =
            shadow
                ? object.staticShadows()
                : object.staticPatterns();
        const ScreenPosition position =
            calculateRealPosition(object.position());
        renderer.drawPattern(
            image,
            static_cast<std::size_t>(object.staticPattern()),
            {
                position.x - camera_x,
                position.y - camera_y -
                    (shadow ? 0 : object.displayHeight()),
                1000,
                1000,
                1000,
                shadow
                    ? std::clamp(shadow_opacity, std::int32_t{0}, std::int32_t{1000})
                    : std::clamp(
                          object.drawStrength(), std::int32_t{0}, std::int32_t{1000}),
                shadow
                    ? 1000
                    : object.redDrawStrength() +
                          (hovered ? 300 : 0),
                shadow
                    ? 1000
                    : object.greenDrawStrength() +
                          (hovered ? 300 : 0),
                shadow
                    ? 1000
                    : object.blueDrawStrength() +
                          (hovered ? 300 : 0),
                -1,
                {},
                !shadow &&
                    (object.displayStatus() & 0x10) != 0
                    ? gapi::PatternBlendMode::additive
                    : gapi::PatternBlendMode::normal,
            });
        return;
    }
    if (!object.hasAnimatedVisual()) {
        return;
    }
    renderCharacterAnimationPass(
        renderer,
        object.animation(),
        object.animationPatterns(),
        object.animationPatterns(),
        object.position(),
        object.animationChart(),
        object.direction(),
        object.animationFrame(),
        [&object](std::size_t part) {
            return object.partEnabled(part);
        },
        [&object, hovered](std::size_t part) {
            const std::int32_t hover_strength =
                hovered ? 300 : 0;
            return CharacterColorStrength{
                object.partRedStrength(part) +
                    hover_strength,
                object.partGreenStrength(part) +
                    hover_strength,
                object.partBlueStrength(part) +
                    hover_strength,
            };
        },
        camera_x,
        camera_y,
        shadow,
        shadow_opacity,
        object.displayHeight(),
        object.drawStrength(),
        object.displayStatus());
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
    std::int32_t shadow_opacity,
    bool semi_transparent) {
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
             ? std::clamp(shadow_opacity, std::int32_t{0}, std::int32_t{1000})
             : std::clamp<std::int32_t>(
                   semi_transparent
                       ? std::min<std::int32_t>(
                             object.opacity, 500)
                       : object.opacity,
                   0,
                   1000),
         shadow ? 1000 : object.red_strength,
         shadow ? 1000 : object.green_strength,
         shadow ? 1000 : object.blue_strength,
         -1,
         {},
         !shadow && (object.status & 0x10) != 0
             ? gapi::PatternBlendMode::additive
             : gapi::PatternBlendMode::normal});
}

std::vector<WorldDrawEntry> collectWorldEntries(
    const WorldScene& world,
    bool shadow,
    std::int32_t camera_x,
    std::int32_t camera_y,
    double interpolation) {
    std::vector<WorldDrawEntry> entries;
    for (const MapObject& object :
         world.objectMap().objects()) {
        if (shadow && (object.status & 8) == 0) {
            continue;
        }
        const gapi::NjpImage* image =
            objectImage(world, object, shadow);
        if (!image ||
            !objectVisible(
                *image,
                object,
                camera_x,
                camera_y,
                shadow)) {
            continue;
        }
        entries.push_back({
            &object,
            nullptr,
            nullptr,
            false,
            false,
            {
                entries.size(),
                {object.world_x, object.world_y},
                object.judgement,
                object.status,
            },
            nullptr,
        });
    }

    for (const ScenarioObjectActor& object :
         world.scenarioObjects()) {
        if (!object.drawEnabled() ||
            (shadow &&
             !object.hasStaticShadow() &&
             !object.hasAnimatedVisual())) {
            continue;
        }
        entries.push_back({
            nullptr,
            nullptr,
            nullptr,
            false,
            false,
            {
                entries.size(),
                object.position(),
                object.judgement(),
                static_cast<std::int16_t>(
                    object.displayStatus()),
            },
            &object,
        });
    }

    if (world.hasPlayer()) {
        entries.push_back({
            nullptr,
            nullptr,
            nullptr,
            true,
            false,
            {
                entries.size(),
                world.playerRenderPosition(interpolation),
                world.playerJudgement(),
                0,
            },
            nullptr,
        });
    }
    if (world.hasCompanion() &&
        world.companion().visible()) {
        WorldDrawEntry entry;
        entry.companion = &world.companion();
        entry.order = {
            entries.size(),
            world.companion().renderPosition(interpolation),
            world.companion().judgement(),
            0,
        };
        entries.push_back(entry);
    }
    if (!shadow && world.companionMoonAuraVisible() &&
        world.companionMoonAuraVisual()) {
        WorldDrawEntry entry;
        entry.companion_moon_aura = true;
        entry.order = {
            entries.size(),
            world.companion().renderPosition(interpolation),
            {1, 1, 1, 1},
            0,
        };
        entries.push_back(entry);
    }
    for (const NpcActor& npc : world.npcs()) {
        if (!npc.visible()) {
            continue;
        }
        entries.push_back({
            nullptr,
            &npc,
            nullptr,
            false,
            false,
            {
                entries.size(),
                npc.renderPosition(interpolation),
                npc.judgement(),
                0,
            },
            nullptr,
        });
    }
    for (const EnemyActor& enemy : world.enemies()) {
        if (!enemy.visible() || !enemy.hasVisual() ||
            enemy.expired()) {
            continue;
        }
        entries.push_back({
            nullptr,
            nullptr,
            nullptr,
            false,
            false,
            {
                entries.size(),
                enemy.renderPosition(interpolation),
                enemy.judgement(),
                0,
            },
            nullptr,
            &enemy,
        });
    }
    for (const GroundItem& item : world.groundItems()) {
        if (!item.visible()) {
            continue;
        }
        entries.push_back({
            nullptr,
            nullptr,
            &item,
            false,
            false,
            {
                entries.size(),
                item.position,
                item.judgement,
                0,
            },
            nullptr,
        });
    }
    if (!shadow) {
        for (const CombatEffectActor& effect :
             world.combatEffects()) {
            if (effect.expired()) {
                continue;
            }
            entries.push_back({
                nullptr,
                nullptr,
                nullptr,
                false,
                false,
                {
                    entries.size(),
                    effect.position(),
                    effect.judgement(),
                    0,
                },
                nullptr,
                nullptr,
                &effect,
            });
        }
        for (const RuntimeEffectActor& effect :
             world.runtimeEffects()) {
            if (!effect.visible() ||
                !effect.hasUpdated()) {
                continue;
            }
            WorldDrawEntry entry;
            entry.runtime_effect = &effect;
            entry.order = {
                entries.size(),
                effect.renderPosition(interpolation),
                effect.judgement(),
                static_cast<std::int16_t>(
                    effect.additionalDisplayStatus()),
            };
            entries.push_back(entry);
        }
        for (const PlayerLandMineVisual& mine :
             world.playerLandMineVisuals()) {
            WorldDrawEntry entry;
            entry.land_mine = &mine;
            entry.order = {
                entries.size(),
                mine.renderPosition(interpolation),
                mine.judgement,
                0,
            };
            entries.push_back(entry);
        }
        for (const MissEffectActor& effect :
             world.missEffects()) {
            if (effect.expired()) {
                continue;
            }
            WorldDrawEntry entry;
            entry.miss_effect = &effect;
            entry.order = {
                entries.size(),
                effect.position(),
                effect.judgement(),
                0,
            };
            entries.push_back(entry);
        }
        const PlayerTransportSpell& transport =
            world.playerTransportSpell();
        const PlayerTransportEndpoint* endpoint =
            transport.endpoint(world.scenarioId());
        if (endpoint && world.playerTransportPatterns() &&
            world.playerTransportVisual()) {
            WorldDrawEntry entry;
            entry.player_transport = true;
            entry.order = {
                entries.size(),
                endpoint->position,
                {-50, -50, 50, 50},
                0,
            };
            entries.push_back(entry);
        }
    }

    std::vector<DisplayOrderEntry> order;
    order.reserve(entries.size());
    for (std::size_t index = 0;
         index < entries.size();
         ++index) {
        entries[index].order.source_index = index;
        order.push_back(entries[index].order);
    }
    sortDisplayObjects(order);

    std::vector<WorldDrawEntry> result;
    result.reserve(entries.size());
    for (const DisplayOrderEntry& ordered : order) {
        result.push_back(
            std::move(entries[ordered.source_index]));
    }
    return result;
}

void drawGroundItem(
    gapi::Backend& renderer,
    const WorldScene& world,
    const GroundItem& item,
    std::int32_t camera_x,
    std::int32_t camera_y,
    bool shadow,
    std::int32_t shadow_opacity,
    bool hovered) {
    const ItemWorldResource* resource =
        world.itemWorldResource(item.resource_id);
    if (!resource) {
        return;
    }
    if (!resource->animated()) {
        const gapi::NjpImage& image =
            shadow
                ? resource->shadowPatterns()
                : resource->patterns();
        if (item.animation_chart < 0 ||
            static_cast<std::size_t>(
                item.animation_chart) >=
                image.patterns().size()) {
            return;
        }
        const ScreenPosition screen =
            calculateRealPosition(item.position);
        const std::int32_t hover_strength =
            hovered ? 300 : 0;
        renderer.drawPattern(
            image,
            static_cast<std::size_t>(
                item.animation_chart),
            {
                screen.x - camera_x,
                screen.y - camera_y -
                    (shadow
                         ? 0
                         : item.height *
                               kRetailHeightScale /
                               100),
                1000,
                1000,
                1000,
                shadow
                    ? std::clamp(
                          shadow_opacity, std::int32_t{0}, std::int32_t{1000})
                    : 1000,
                shadow
                    ? 1000
                    : item.red_strength +
                          hover_strength,
                shadow
                    ? 1000
                    : item.green_strength +
                          hover_strength,
                shadow
                    ? 1000
                    : item.blue_strength +
                          hover_strength,
                -1,
            });
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
        [&item, hovered](std::size_t part) {
            const std::int32_t hover_strength =
                hovered ? 300 : 0;
            return part == 0
                       ? CharacterColorStrength{
                             item.red_strength +
                                 hover_strength,
                             item.green_strength +
                                 hover_strength,
                             item.blue_strength +
                                 hover_strength,
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
    if (entry.object) {
        drawMapObject(
            renderer,
            world,
            *entry.object,
            camera_x,
            camera_y,
            shadow,
            shadow_opacity,
            entry.semi_transparent);
    } else if (entry.scenario_object) {
        renderScenarioObjectPass(
            renderer,
            *entry.scenario_object,
            camera_x,
            camera_y,
            shadow,
            shadow_opacity,
            world.hoveredScenarioObjectId() ==
                entry.scenario_object->id());
    } else if (entry.player) {
        renderPlayerPass(
            renderer,
            world,
            camera_x,
            camera_y,
            shadow,
            shadow_opacity,
            interpolation);
    } else if (entry.companion) {
        renderCompanionPass(
            renderer,
            *entry.companion,
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
    } else if (entry.enemy) {
        renderEnemyPass(
            renderer,
            *entry.enemy,
            camera_x,
            camera_y,
            shadow,
            shadow_opacity,
            world.hoveredEnemyId() == entry.enemy->id(),
            interpolation);
    } else if (entry.item) {
        drawGroundItem(
            renderer,
            world,
            *entry.item,
            camera_x,
            camera_y,
            shadow,
            shadow_opacity,
            world.hoveredGroundItemId() ==
                entry.item->id);
    } else if (entry.effect) {
        renderCombatEffect(
            renderer,
            *entry.effect,
            camera_x,
            camera_y);
    } else if (entry.runtime_effect) {
        renderRuntimeEffect(
            renderer,
            *entry.runtime_effect,
            camera_x,
            camera_y,
            interpolation);
    } else if (entry.land_mine) {
        renderPlayerLandMine(
            renderer,
            world,
            *entry.land_mine,
            camera_x,
            camera_y,
            interpolation);
    } else if (entry.miss_effect) {
        renderMissEffect(
            renderer,
            *entry.miss_effect,
            camera_x,
            camera_y);
    } else if (entry.companion_moon_aura) {
        renderCompanionMoonAura(
            renderer,
            world,
            camera_x,
            camera_y,
            interpolation);
    } else if (entry.player_transport) {
        renderPlayerTransport(
            renderer,
            world,
            camera_x,
            camera_y);
    }
}

void drawWorldEntries(
    gapi::Backend& renderer,
    const WorldScene& world,
    const std::vector<WorldDrawEntry>& entries,
    std::int32_t camera_x,
    std::int32_t camera_y,
    bool shadow,
    bool default_class,
    std::int32_t shadow_opacity,
    double interpolation) {
    for (const WorldDrawEntry& entry : entries) {
        if ((displayClassForStatus(entry.order.status) == 0) !=
            default_class) {
            continue;
        }
        drawWorldEntry(
            renderer,
            world,
            entry,
            camera_x,
            camera_y,
            shadow,
            shadow_opacity,
            interpolation);
    }
}

void markSemiTransparentObjects(
    const WorldScene& world,
    std::vector<WorldDrawEntry>& entries,
    std::int32_t camera_x,
    std::int32_t camera_y,
    double interpolation) {
    if (!world.hasPlayer()) {
        return;
    }
    const auto player = std::find_if(
        entries.begin(),
        entries.end(),
        [](const WorldDrawEntry& entry) {
            return entry.player;
        });
    if (player == entries.end()) {
        return;
    }
    const ScreenPosition player_screen =
        calculateRealPosition(
            world.playerRenderPosition(interpolation));
    const DisplayHitRectangle player_rectangle{
        player_screen.x - camera_x - 25,
        player_screen.y - camera_y - 60,
        player_screen.x - camera_x + 25,
        player_screen.y - camera_y,
    };
    for (auto entry = player;
         entry != entries.end();
         ++entry) {
        if (!entry->object ||
            (entry->object->status & 0x2000) != 0) {
            continue;
        }
        const gapi::NjpImage* image =
            objectImage(world, *entry->object, false);
        if (!image) {
            continue;
        }
        const ScreenPosition anchor =
            calculateRealPosition({
                entry->object->world_x,
                entry->object->world_y,
            });
        entry->semi_transparent =
            displayPatternIntersectsRectangle(
                *image,
                static_cast<std::size_t>(
                    entry->object->pattern),
                {
                    anchor.x - camera_x,
                    anchor.y - camera_y,
                },
                player_rectangle,
                entry->object->height *
                    kRetailHeightScale / 100);
    }
}


}  // namespace

void renderWorldGeometry(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t shadow_opacity,
    double interpolation,
    bool semi_transparent_objects) {
    const GroundMap& ground = world.ground();
    if (ground.width() <= 0 || ground.height() <= 0) {
        return;
    }

    const std::int32_t camera_x =
        world.renderCameraScreenX(interpolation);
    const std::int32_t camera_y =
        world.renderCameraScreenY(interpolation);
    const std::int32_t start_x =
        std::max(camera_x / ground.chipWidth(), std::int32_t{0});
    const std::int32_t start_y =
        std::max(camera_y / ground.chipHeight(), std::int32_t{0});
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

    const std::vector<WorldDrawEntry> shadow_entries =
        collectWorldEntries(
            world,
            true,
            camera_x,
            camera_y,
            interpolation);
    std::vector<WorldDrawEntry> visible_entries =
        collectWorldEntries(
            world,
            false,
            camera_x,
            camera_y,
            interpolation);
    if (semi_transparent_objects) {
        markSemiTransparentObjects(
            world,
            visible_entries,
            camera_x,
            camera_y,
            interpolation);
    }

    // FUN_004030f0 emits the non-default status classes first, then the
    // default class, with a shadow and visible pass for each group.
    drawWorldEntries(
        renderer,
        world,
        shadow_entries,
        camera_x,
        camera_y,
        true,
        false,
        shadow_opacity,
        interpolation);
    drawWorldEntries(
        renderer,
        world,
        visible_entries,
        camera_x,
        camera_y,
        false,
        false,
        shadow_opacity,
        interpolation);
    drawWorldEntries(
        renderer,
        world,
        shadow_entries,
        camera_x,
        camera_y,
        true,
        true,
        shadow_opacity,
        interpolation);
    drawWorldEntries(
        renderer,
        world,
        visible_entries,
        camera_x,
        camera_y,
        false,
        true,
        shadow_opacity,
        interpolation);
}

void renderWorld(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t shadow_opacity,
    const gapi::NjpImage* font,
    double interpolation,
    bool semi_transparent_objects) {
    renderWorldGeometry(
        renderer,
        world,
        shadow_opacity,
        interpolation,
        semi_transparent_objects);
    const std::int32_t camera_x =
        world.renderCameraScreenX(interpolation);
    const std::int32_t camera_y =
        world.renderCameraScreenY(interpolation);
    renderGameplayOverlay(
        renderer,
        world,
        font,
        nullptr,
        camera_x,
        camera_y,
        interpolation);
}

}  // namespace osf
