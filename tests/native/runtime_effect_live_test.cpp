#include "core/retail_random.hpp"
#include "gapi/gapi.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "render/gameplay_renderer.hpp"
#include "world/enemy_effect_impact.hpp"
#include "world/miss_effect_actor.hpp"
#include "world/retail_save_file.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

class RecordingBackend final : public osf::gapi::Backend {
public:
    struct PatternCall {
        const osf::gapi::NjpImage* image = nullptr;
        std::size_t pattern = 0;
        osf::gapi::PatternDraw draw;
    };

    void beginFrame(osf::gapi::Color) override {}

    bool drawPattern(
        const osf::gapi::NjpImage& image,
        std::size_t pattern,
        const osf::gapi::PatternDraw& draw) override {
        patterns.push_back({&image, pattern, draw});
        return true;
    }

    bool drawBitmap(
        const osf::gapi::BitmapImage&,
        const osf::gapi::BitmapDraw&) override {
        return true;
    }

    bool drawText(
        const osf::gapi::NjpImage&,
        std::string_view,
        const osf::gapi::TextDraw&) override {
        return true;
    }

    bool drawRectangle(
        const osf::gapi::RectangleDraw&) override {
        return true;
    }

    void endFrame() override {}

    std::vector<PatternCall> patterns;
};

const osf::EnemyActor* findEnemy(
    const osf::WorldScene& world,
    std::int32_t id) {
    const auto found = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [id](const osf::EnemyActor& enemy) {
            return enemy.id() == id;
        });
    return found == world.enemies().end()
        ? nullptr
        : &*found;
}

osf::CombatEffectSpawnRequest shippedEffectRequest(
    const osf::WorldScene& world,
    const osf::EnemyActor& enemy,
    const osf::TableDatabase& tables) {
    const osf::EnemyPresentationProfile& profile =
        enemy.presentationProfile();
    const osf::WorldPosition source = enemy.position();
    const osf::WorldPosition target{
        world.playerWorldX(),
        world.playerWorldY(),
    };
    const double direction = std::atan2(
        static_cast<double>(source.y - target.y),
        static_cast<double>(target.x - source.x));
    osf::RetailRandom constructor_random(1);
    return osf::resolveEnemyEffectImpact(
        {
            enemy.characterNumber(),
            source,
            enemy.judgement(),
            direction,
            profile.effect_type[0],
            profile.effect_subtype[0],
            profile.effect_parameter[0],
            profile.effect_additive[0],
            profile.packet_word_31,
            {},
        },
        tables,
        constructor_random);
}

bool containsSample(
    const std::vector<std::int32_t>& samples,
    std::int32_t sample) {
    return std::find(
               samples.begin(),
               samples.end(),
               sample) != samples.end();
}

bool sameItem(
    const osf::InventoryItem& first,
    const osf::InventoryItem& second) {
    return first.category == second.category &&
           first.definition_id == second.definition_id &&
           first.quantity == second.quantity &&
           first.grid_x == second.grid_x &&
           first.grid_y == second.grid_y &&
           first.width == second.width &&
           first.height == second.height &&
           first.durability == second.durability &&
           first.identified == second.identified;
}

bool sameItems(
    const std::vector<osf::InventoryItem>& first,
    const std::vector<osf::InventoryItem>& second) {
    return first.size() == second.size() &&
           std::equal(
               first.begin(),
               first.end(),
               second.begin(),
               sameItem);
}

bool testMissBounceAndFade(
    const std::filesystem::path& data_root) {
    osf::gapi::NjpImage patterns;
    std::string error;
    if (!check(
            patterns.load(
                data_root / "Character" / "OPTION" /
                    "11000011" / "Pattern.Njp",
                &error),
            "The retail MISS pattern could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::MissEffectActor miss;
    if (!check(
            miss.initialize(
                {100, 200},
                {-20, -30, 19, 29},
                patterns) &&
                miss.height() == 400 &&
                miss.opacity() == 1000 &&
                miss.bouncePhase() == 0,
            "The retail MISS actor did not start at its authored "
            "height and strength.")) {
        return false;
    }
    miss.update();
    if (!check(
            miss.height() == 450 &&
                miss.opacity() == 1000 &&
                miss.bouncePhase() == 0,
            "The first MISS vertical-velocity update differed.")) {
        return false;
    }

    bool entered_fade = false;
    bool faded_once = false;
    std::int32_t updates = 1;
    for (; updates < 100 && !miss.expired(); ++updates) {
        miss.update();
        if (miss.bouncePhase() == 3 &&
            miss.opacity() == 1000) {
            entered_fade = true;
        }
        if (miss.bouncePhase() == 3 &&
            miss.opacity() == 900) {
            faded_once = true;
        }
    }
    return check(
        entered_fade &&
            faded_once &&
            miss.expired() &&
            miss.opacity() == 100 &&
            updates == 31,
        "The three retail MISS bounces or ten-step opacity fade "
        "differed.");
}

bool testShippedLiveProjectile(
    const std::filesystem::path& data_root) {
    osf::TableDatabase tables;
    std::string error;
    if (!check(
            tables.load(
                data_root / "System" / "Game" / "Parameter" /
                    "Table.Tbd",
                &error),
            "The live-effect parameter tables could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerLoadRequest player;
    player.name = "EffectLive";
    osf::WorldScene world;
    if (!check(
            world.loadInitialScenario(
                data_root,
                player,
                {3000507, 3, 0},
                &error),
            "The shipped type-two effect scenario could not be "
            "loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const osf::EnemyActor* source = findEnemy(world, 316);
    if (!check(
            source &&
                source->presentationProfile()
                        .effect_type[0] == 2 &&
                source->presentationProfile()
                        .effect_subtype[0] == 10,
            "The shipped live-effect fixture no longer points to "
            "enemy 316 and its type-two attack.")) {
        return false;
    }

    const osf::CombatEffectSpawnRequest request =
        shippedEffectRequest(world, *source, tables);
    const std::int32_t life_before =
        world.playerData().currentLife();
    const std::vector<osf::InventoryItem>
        inventory_before =
            world.playerInventory().items();
    const std::vector<osf::InventoryItem> belt_before =
        world.playerBelt().items();
    const osf::InventoryItem* body_before =
        world.playerEquipment().item(
            osf::EquipmentSlot::body);
    const osf::InventoryItem body_copy =
        body_before
            ? *body_before
            : osf::InventoryItem{};
    if (!check(
            request.valid &&
                request.effect_number == 10002 &&
                request.owner_kind == 4 &&
                request.source_character_number ==
                    source->characterNumber() &&
                body_before &&
                world.runtimeEffectControllerCount() == 0,
            "The shipped type-two request or starter ownership "
            "fixture is invalid.")) {
        return false;
    }

    world.queueCombatEffect(request);
    world.update();
    const std::vector<std::int32_t> launch_audio =
        world.takeAudioSamples();
    if (!check(
            world.runtimeEffectControllerCount() == 0 &&
                world.runtimeEffects().size() == 2 &&
                containsSample(launch_audio, 94) &&
                std::any_of(
                    world.runtimeEffects().begin(),
                    world.runtimeEffects().end(),
                    [](const osf::RuntimeEffectActor& actor) {
                        return actor.resourceId() == 11000027;
                    }) &&
                std::any_of(
                    world.runtimeEffects().begin(),
                    world.runtimeEffects().end(),
                    [](const osf::RuntimeEffectActor& actor) {
                        return actor.resourceId() == 10000040;
                    }),
            "The live type-two controller did not launch both "
            "retail actors and sample 94.")) {
        return false;
    }

    bool rendered_runtime_actor = false;
    bool rendered_child_at_retail_height = false;
    bool heard_impact = false;
    for (std::int32_t update = 0;
         update < 80 &&
         !(heard_impact &&
           world.playerData().currentLife() < life_before);
         ++update) {
        world.update();
        const std::vector<std::int32_t> samples =
            world.takeAudioSamples();
        heard_impact =
            heard_impact || containsSample(samples, 20);

        std::unordered_set<const osf::gapi::NjpImage*>
            runtime_patterns;
        for (const osf::RuntimeEffectActor& actor :
             world.runtimeEffects()) {
            if (actor.hasUpdated()) {
                runtime_patterns.insert(&actor.patterns());
            }
        }
        RecordingBackend backend;
        osf::renderWorldGeometry(backend, world);
        rendered_runtime_actor =
            rendered_runtime_actor ||
            std::any_of(
                backend.patterns.begin(),
                backend.patterns.end(),
                [&runtime_patterns](
                    const RecordingBackend::PatternCall& call) {
                    return runtime_patterns.find(call.image) !=
                           runtime_patterns.end();
                });
        for (const osf::RuntimeEffectActor& actor :
             world.runtimeEffects()) {
            if (!actor.hasUpdated() ||
                actor.resourceId() != 10000040) {
                continue;
            }
            const osf::ScreenPosition screen =
                osf::calculateRealPosition(
                    actor.renderPosition(1.0));
            rendered_child_at_retail_height =
                rendered_child_at_retail_height ||
                std::any_of(
                    backend.patterns.begin(),
                    backend.patterns.end(),
                    [&actor, &world, screen](
                        const RecordingBackend::PatternCall&
                            call) {
                        return call.image ==
                                   &actor.patterns() &&
                               call.draw.y ==
                                   screen.y -
                                       world.renderCameraScreenY(
                                           1.0) -
                                       actor.displayHeight() /
                                           10;
                    });
        }
    }
    if (!check(
            rendered_runtime_actor &&
                rendered_child_at_retail_height &&
                heard_impact &&
                world.playerData().currentLife() < life_before &&
                sameItems(
                    world.playerInventory().items(),
                    inventory_before) &&
                sameItems(
                    world.playerBelt().items(),
                    belt_before) &&
                world.playerEquipment().item(
                    osf::EquipmentSlot::body) &&
                sameItem(
                    *world.playerEquipment().item(
                        osf::EquipmentSlot::body),
                    body_copy),
            "The shipped projectile was not rendered, heard, "
            "received as damage, or preserved item ownership.")) {
        return false;
    }

    const std::int32_t damaged_life =
        world.playerData().currentLife();
    const std::filesystem::path save_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_runtime_effect_live_test";
    const std::filesystem::path save_path =
        save_root / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(save_root, cleanup_error);
    if (!check(
            osf::writeRetailSave(
                save_path,
                world.playerData(),
                world.itemDatabase(),
                world.playerInventory(),
                world.playerEquipment(),
                world.playerBelt(),
                world.playerSpecialItems(),
                0x61,
                &error),
            "The live projectile result could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerLoadRequest saved_player;
    saved_player.source =
        osf::PlayerDataSource::retail_save;
    saved_player.save_path = save_path;
    osf::WorldScene restored;
    const bool loaded =
        restored.loadInitialScenario(
            data_root, saved_player, &error);
    std::filesystem::remove_all(save_root, cleanup_error);
    return check(
        loaded &&
            restored.playerData().currentLife() ==
                (damaged_life > 0
                     ? damaged_life
                     : restored.playerData()
                           .baseMaximumLife()) &&
            sameItems(
                restored.playerInventory().items(),
                inventory_before) &&
            sameItems(
                restored.playerBelt().items(),
                belt_before) &&
            restored.playerEquipment().item(
                osf::EquipmentSlot::body) &&
            sameItem(
                *restored.playerEquipment().item(
                    osf::EquipmentSlot::body),
                body_copy),
        "Saving after the live projectile discarded live damage, "
        "dead-state recovery, or owned items.");
}

bool testShippedLiveTypeThree(
    const std::filesystem::path& data_root) {
    osf::TableDatabase tables;
    std::string error;
    if (!check(
            tables.load(
                data_root / "System" / "Game" / "Parameter" /
                    "Table.Tbd",
                &error),
            "The type-three parameter tables could not be "
            "loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerLoadRequest player;
    player.name = "EffectRing";
    osf::WorldScene world;
    if (!check(
            world.loadInitialScenario(
                data_root,
                player,
                {10001, 0, 0},
                &error),
            "The shipped Plasma Bat scenario could not be "
            "loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const auto source = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [](const osf::EnemyActor& enemy) {
            const auto& profile =
                enemy.presentationProfile();
            return profile.effect_type[0] == 3 &&
                   profile.effect_subtype[0] == 20;
        });
    if (!check(
            source != world.enemies().end(),
            "Scenario 00010001 no longer contains its shipped "
            "type-three Plasma Bat.")) {
        return false;
    }

    osf::CombatEffectSpawnRequest request =
        shippedEffectRequest(world, *source, tables);
    const osf::WorldPosition player_position{
        world.playerWorldX(),
        world.playerWorldY(),
    };
    request.direction_radians = 0.0;
    request.has_explicit_origin = true;
    request.origin = {
        player_position.x - 250,
        player_position.y,
    };
    request.packet.write(36, 100000);
    const std::int32_t life_before =
        world.playerData().currentLife();
    const std::vector<osf::InventoryItem>
        inventory_before =
            world.playerInventory().items();
    const std::vector<osf::InventoryItem> belt_before =
        world.playerBelt().items();
    if (!check(
            request.valid &&
                request.effect_number == 10003 &&
                request.constructor_value_17 == 20 &&
                world.runtimeEffectControllerCount() == 0,
            "The shipped Plasma Bat did not resolve its type-three "
            "controller request.")) {
        return false;
    }

    world.queueCombatEffect(request);
    world.update();
    const std::vector<std::int32_t> launch_audio =
        world.takeAudioSamples();
    if (!check(
            world.runtimeEffectControllerCount() == 1 &&
                world.runtimeEffects().size() == 3 &&
                containsSample(launch_audio, 21) &&
                world.runtimeEffects()[0].resourceId() ==
                    10000030 &&
                world.runtimeEffects()[0].animationChart() >= 0 &&
                world.runtimeEffects()[0].animationChart() < 4 &&
                world.runtimeEffects()[0].hasPacket() &&
                world.runtimeEffects()[1].resourceId() ==
                    10000031 &&
                world.runtimeEffects()[1].hasPacket() &&
                world.runtimeEffects()[2].resourceId() ==
                    10000032 &&
                world.runtimeEffects()[2].hasPacket(),
            "The live type-three controller did not create its "
            "three-layer first wave and sample 21.")) {
        return false;
    }

    world.update();
    world.takeAudioSamples();
    RecordingBackend backend;
    osf::renderWorldGeometry(backend, world);
    const bool rendered =
        std::any_of(
            world.runtimeEffects().begin(),
            world.runtimeEffects().end(),
            [&backend](const osf::RuntimeEffectActor& actor) {
                return std::any_of(
                    backend.patterns.begin(),
                    backend.patterns.end(),
                    [&actor](
                        const RecordingBackend::PatternCall&
                            call) {
                        return call.image ==
                               &actor.patterns();
                    });
            });

    if (!check(
            rendered &&
                world.playerData().currentLife() <
                    life_before &&
                sameItems(
                    world.playerInventory().items(),
                    inventory_before) &&
                sameItems(
                    world.playerBelt().items(),
                    belt_before),
            "The live type-three wave was not rendered, received "
            "as damage, or preserved item ownership.")) {
        return false;
    }

    for (std::int32_t update = 2;
         update < 20;
         ++update) {
        world.update();
        world.takeAudioSamples();
    }
    return check(
        world.runtimeEffectControllerCount() == 0,
        "The shipped type-three controller did not expire after "
        "Table 205's twenty updates.");
}

bool testShippedLiveTypeFour(
    const std::filesystem::path& data_root) {
    osf::TableDatabase tables;
    std::string error;
    if (!check(
            tables.load(
                data_root / "System" / "Game" / "Parameter" /
                    "Table.Tbd",
                &error),
            "The type-four parameter tables could not be "
            "loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerLoadRequest player;
    player.name = "EffectQuake";
    osf::WorldScene world;
    if (!check(
            world.loadInitialScenario(
                data_root,
                player,
                {1000004, 0, 0},
                &error),
            "The shipped type-four effect scenario could not be "
            "loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const auto source = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [](const osf::EnemyActor& enemy) {
            return enemy.presentationProfile()
                       .effect_type[0] == 4;
        });
    if (!check(
            source != world.enemies().end(),
            "Scenario 01000004 no longer contains its shipped "
            "type-four enemy family.")) {
        return false;
    }

    osf::CombatEffectSpawnRequest request =
        shippedEffectRequest(world, *source, tables);
    request.owner_kind = 1;
    request.source_character_number = 0;
    request.packet.write(2, source->characterNumber());
    request.packet.write(36, 100000);
    const std::int32_t life_before =
        world.playerData().currentLife();
    const std::vector<osf::InventoryItem>
        inventory_before =
            world.playerInventory().items();
    const std::vector<osf::InventoryItem> belt_before =
        world.playerBelt().items();
    const osf::InventoryItem* body_before =
        world.playerEquipment().item(
            osf::EquipmentSlot::body);
    const osf::InventoryItem body_copy =
        body_before
            ? *body_before
            : osf::InventoryItem{};
    const std::int32_t camera_y =
        world.cameraScreenY();
    if (!check(
            request.valid &&
                request.effect_number == 10004 &&
                request.constructor_value_12 == 10 &&
                body_before,
            "The shipped enemy did not resolve a valid "
            "type-four request or starter ownership fixture.")) {
        return false;
    }

    world.queueCombatEffect(request);
    for (std::int32_t update_number = 0;
         update_number <= 3;
         ++update_number) {
        world.update();
        world.takeAudioSamples();
    }
    const auto warning = std::find_if(
        world.runtimeEffects().begin(),
        world.runtimeEffects().end(),
        [](const osf::RuntimeEffectActor& actor) {
            return actor.resourceId() == 10000002;
        });
    if (!check(
            warning != world.runtimeEffects().end() &&
                warning->additionalDisplayStatus() == 0x80 &&
                warning->animationChart() == 0 &&
                world.runtimeEffectControllerCount() == 1,
            "The shipped type-four controller did not create its "
            "warning actor on update three.")) {
        return false;
    }

    world.update();
    world.takeAudioSamples();
    const osf::RuntimeEffectActor* warning_actor =
        nullptr;
    for (const osf::RuntimeEffectActor& actor :
         world.runtimeEffects()) {
        if (actor.resourceId() == 10000002) {
            warning_actor = &actor;
            break;
        }
    }
    RecordingBackend warning_backend;
    osf::renderWorldGeometry(warning_backend, world);
    if (!check(
            warning_actor &&
                std::any_of(
                    warning_backend.patterns.begin(),
                    warning_backend.patterns.end(),
                    [warning_actor](
                        const RecordingBackend::PatternCall&
                            call) {
                        return call.image ==
                               &warning_actor->patterns();
                    }),
            "The live type-four warning actor did not enter the "
            "world renderer.")) {
        return false;
    }

    for (std::int32_t update_number = 5;
         update_number < 10;
         ++update_number) {
        world.update();
        world.takeAudioSamples();
    }
    world.update();
    const std::vector<std::int32_t> burst_audio =
        world.takeAudioSamples();
    const auto first_burst = std::find_if(
        world.runtimeEffects().begin(),
        world.runtimeEffects().end(),
        [](const osf::RuntimeEffectActor& actor) {
            return actor.resourceId() == 10000000 &&
                   actor.animationChart() == 1;
        });
    const auto second_burst = std::find_if(
        world.runtimeEffects().begin(),
        world.runtimeEffects().end(),
        [](const osf::RuntimeEffectActor& actor) {
            return actor.resourceId() == 10000000 &&
                   actor.animationChart() == 0;
        });
    const auto invisible = std::find_if(
        world.runtimeEffects().begin(),
        world.runtimeEffects().end(),
        [](const osf::RuntimeEffectActor& actor) {
            return actor.resourceId() == -1;
        });
    if (!check(
            world.runtimeEffectControllerCount() == 0 &&
                first_burst != world.runtimeEffects().end() &&
                second_burst != world.runtimeEffects().end() &&
                invisible != world.runtimeEffects().end() &&
                !invisible->visible() &&
                invisible->hasPacket() &&
                containsSample(burst_audio, 29) &&
                containsSample(burst_audio, 23) &&
                world.cameraScreenY() == camera_y,
            "The shipped type-four burst lost one of its actors, "
            "sounds, or zero-offset first shake update.")) {
        return false;
    }

    world.update();
    world.takeAudioSamples();
    if (!check(
            world.cameraScreenY() == camera_y - 6 &&
                world.playerData().currentLife() < life_before &&
                sameItems(
                    world.playerInventory().items(),
                    inventory_before) &&
                sameItems(
                    world.playerBelt().items(),
                    belt_before) &&
                world.playerEquipment().item(
                    osf::EquipmentSlot::body) &&
                sameItem(
                    *world.playerEquipment().item(
                        osf::EquipmentSlot::body),
                    body_copy),
            "The live type-four damage actor, first camera jolt, "
            "or adjacent item ownership differs.")) {
        return false;
    }

    for (std::int32_t shake_counter = 2;
         shake_counter <= 8;
         ++shake_counter) {
        world.update();
        world.takeAudioSamples();
        const std::int32_t expected =
            shake_counter < 8 &&
                    (shake_counter & 1) != 0
                ? camera_y - 6
                : camera_y;
        if (!check(
                world.cameraScreenY() == expected,
                "The live type-four camera jolt did not alternate "
                "for exactly eight retail updates.")) {
            return false;
        }
    }
    return true;
}

bool testLiveMissPresentation(
    const std::filesystem::path& data_root) {
    osf::TableDatabase tables;
    std::string error;
    if (!tables.load(
            data_root / "System" / "Game" / "Parameter" /
                "Table.Tbd",
            &error)) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerLoadRequest player;
    player.name = "EffectMiss";
    osf::WorldScene world;
    if (!world.loadInitialScenario(
            data_root,
            player,
            {3000507, 3, 0},
            &error)) {
        std::cerr << error << '\n';
        return false;
    }

    bool saw_miss = false;
    for (std::int32_t attempt = 0;
         attempt < 8 && !saw_miss;
         ++attempt) {
        const osf::EnemyActor* source =
            findEnemy(world, 316);
        if (!source) {
            return false;
        }
        osf::CombatEffectSpawnRequest request =
            shippedEffectRequest(world, *source, tables);
        request.packet.write(36, -100000);
        request.packet.write(32, 0);
        request.packet.write(40, 0);
        request.packet.write(41, 0);
        request.packet.write(43, 0);
        world.queueCombatEffect(request);

        for (std::int32_t update = 0;
             update < 80 && world.missEffects().empty();
             ++update) {
            world.update();
            world.takeAudioSamples();
        }
        saw_miss = !world.missEffects().empty();
    }
    if (!check(
            saw_miss &&
                world.missEffects().front().height() == 400 &&
                world.missEffects().front().opacity() == 1000,
            "A missed live projectile did not create effect 20012 "
            "at its retail initial height and opacity.")) {
        return false;
    }

    const osf::MissEffectActor& miss =
        world.missEffects().front();
    const osf::gapi::NjpImage* miss_patterns =
        &miss.patterns();
    const osf::ScreenPosition screen =
        osf::calculateRealPosition(miss.position());
    RecordingBackend backend;
    osf::renderWorldGeometry(backend, world);
    const auto draw = std::find_if(
        backend.patterns.begin(),
        backend.patterns.end(),
        [miss_patterns](
            const RecordingBackend::PatternCall& call) {
            return call.image == miss_patterns;
        });
    return check(
        draw != backend.patterns.end() &&
            draw->pattern == 0 &&
            draw->draw.x ==
                screen.x - world.cameraScreenX() &&
            draw->draw.y ==
                screen.y -
                    world.cameraScreenY() -
                    miss.height() / 10 &&
            draw->draw.opacity == 1000,
        "The live MISS pattern was not drawn at the retail "
        "actor height and strength.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(
            OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(data_root)) {
        return 0;
    }
    return testMissBounceAndFade(data_root) &&
                   testShippedLiveProjectile(data_root) &&
                   testShippedLiveTypeThree(data_root) &&
                   testShippedLiveTypeFour(data_root) &&
                   testLiveMissPresentation(data_root)
               ? 0
               : 1;
#else
    return 0;
#endif
}
