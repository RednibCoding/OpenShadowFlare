#include "gapi/gapi.hpp"
#include "core/retail_random.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "render/gameplay_help_renderer.hpp"
#include "resources/character_visual_resource.hpp"
#include "world/companion_actor.hpp"
#include "world/companion_attack_action.hpp"
#include "world/companion_attack_impact.hpp"
#include "world/companion_profile.hpp"
#include "world/companion_respawn.hpp"
#include "world/companion_target_selector.hpp"
#include "world/world_scene.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>
#include <string>

namespace {

class CompanionPreviewBackend final
    : public osf::gapi::Backend {
public:
    explicit CompanionPreviewBackend(
        const osf::gapi::NjpImage& companion_patterns)
        : companion_patterns_(&companion_patterns) {}

    void beginFrame(osf::gapi::Color) override {}

    bool drawPattern(
        const osf::gapi::NjpImage& image,
        std::size_t,
        const osf::gapi::PatternDraw&) override {
        if (&image == companion_patterns_) {
            companion_drawn = true;
        }
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

    bool companion_drawn = false;

private:
    const osf::gapi::NjpImage* companion_patterns_ =
        nullptr;
};

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

}  // namespace

int main() {
    osf::PlayerInventory ordinary_inventory;
    osf::PlayerInventory quick_inventory;
    if (!check(
            quick_inventory.add(4, 98000002) &&
                osf::retailCompanionRespawnUpdates(
                ordinary_inventory) == 900 &&
                osf::retailCompanionRespawnUpdates(
                    quick_inventory) == 600,
            "The companion respawn delay did not honor the retail "
            "backpack item check.")) {
        return 1;
    }

    const std::filesystem::path data_root =
        std::filesystem::path(
            OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    osf::TableDatabase tables;
    std::string error;
    if (!check(
            tables.load(
                data_root / "System" / "Game" /
                    "Parameter" / "Table.Tbd",
                &error),
            "The retail parameter tables could not be loaded.")) {
        std::cerr << error << '\n';
        return 1;
    }

    osf::CompanionProfile kerberos;
    if (!check(
            osf::decodeCompanionProfile(
                tables, 0, 1, kerberos, &error),
            "Kerberos' level-one profile could not be decoded.")) {
        std::cerr << error << '\n';
        return 1;
    }
    if (!check(
            kerberos.name == "Kerberos" &&
                kerberos.resource_id == 0 &&
                kerberos.red_strength == 1000 &&
                kerberos.green_strength == 1000 &&
                kerberos.blue_strength == 1000 &&
                kerberos.native_element == 0 &&
                kerberos.walking_speed == 25 &&
                kerberos.running_speed == 45 &&
                kerberos.maximum_life == 400 &&
                kerberos.physical_attack == 30 &&
                kerberos.hit_rate == 250 &&
                kerberos.physical_defense == 50 &&
                kerberos.physical_evasion == 20 &&
                kerberos.magical_attack == 100 &&
                kerberos.magical_hit_rate == 50 &&
                kerberos.magical_defense == 250 &&
                kerberos.magical_evasion == 20 &&
                kerberos.attack_speed_rating == 128 &&
                kerberos.parameter_17 == 900 &&
                kerberos.experience_threshold == 100,
            "Kerberos' profile does not match tables 60 and 800.")) {
        return 1;
    }

    osf::CompanionProfile gravity;
    osf::CompanionProfile dune;
    osf::CompanionProfile fang;
    osf::CompanionProfile harley;
    osf::CompanionProfile hawk;
    osf::CompanionProfile unavailable;
    if (!check(
            osf::decodeCompanionProfile(
                tables, 1, 1, gravity, &error) &&
                gravity.name == "Gravity" &&
                gravity.resource_id == 0 &&
                gravity.red_strength == 400 &&
                gravity.native_element == 3 &&
                gravity.physical_attack == 35 &&
                gravity.attack_speed_rating == 160 &&
                gravity.parameter_17 == 600 &&
                osf::decodeCompanionProfile(
                    tables, 2, 1, dune, &error) &&
                dune.name == "Dune" &&
                dune.resource_id == 0 &&
                dune.red_strength == 900 &&
                dune.green_strength == 800 &&
                dune.blue_strength == 700 &&
                dune.native_element == 4 &&
                osf::decodeCompanionProfile(
                    tables, 3, 1, fang, &error) &&
                fang.name == "Fang" &&
                fang.resource_id == 0 &&
                osf::decodeCompanionProfile(
                    tables, 4, 1, harley, &error) &&
                harley.name == "Harley" &&
                harley.resource_id == 1 &&
                osf::decodeCompanionProfile(
                    tables, 5, 1, hawk, &error) &&
                hawk.name == "Hawk" &&
                hawk.resource_id == 2 &&
                !osf::decodeCompanionProfile(
                    tables, 6, 1, unavailable, &error),
            "The shipped companion catalog/profile boundary changed.")) {
        return 1;
    }

    osf::CharacterVisualResources visuals{"PARTNER"};
    const osf::CharacterVisualResource* visual =
        visuals.load(data_root, kerberos.resource_id, &error);
    if (!check(
            visual != nullptr,
            "The PARTNER visual could not be loaded.")) {
        std::cerr << error << '\n';
        return 1;
    }

    osf::WorldScene world;
    if (!check(
            world.loadInitialScenario(
                data_root,
                {
                    osf::PlayerDataSource::new_character,
                    "Companion",
                    osf::playerGenderValue(
                        osf::PlayerGender::female),
                    {},
                },
                &error),
            "Remote Town could not be loaded.")) {
        std::cerr << error << '\n';
        return 1;
    }
    if (!check(
            world.hasCompanion() &&
                world.companion().characterNumber() ==
                    16000000 &&
                world.companion().profile().name ==
                    "Kerberos" &&
                world.companion().position().x ==
                    world.playerWorldX() &&
                world.companion().position().y ==
                    world.playerWorldY() &&
                world.companion().animationChart() == 0,
            "The local player's owned companion was not created at "
            "the scenario entry.")) {
        return 1;
    }
    CompanionPreviewBackend preview_backend{
        world.companion().patterns()};
    osf::gapi::NjpImage empty_patterns;
    osf::renderGameplayHelp(
        preview_backend,
        empty_patterns,
        empty_patterns,
        world,
        0,
        false,
        0);
    if (!check(
            preview_backend.companion_drawn,
            "The retail Help preview did not draw the owned "
            "PARTNER actor.")) {
        return 1;
    }

    osf::CompanionActor actor;
    const osf::WorldPosition origin{
        world.playerWorldX(),
        world.playerWorldY(),
    };
    if (!check(
            actor.initialize(
                kerberos, *visual, 0, origin, 3),
            "The passive companion actor could not be initialized.")) {
        return 1;
    }

    osf::CompanionAttackAnimationTiming attack_timing;
    if (!check(
            osf::buildCompanionAttackAnimationTiming(
                visual->animation(), 3, attack_timing) &&
                attack_timing.frame_count > 1 &&
                osf::retailCompanionAttackSpeedTier(
                    kerberos.attack_speed_rating) == 4,
            "The PARTNER chart-five timing or row-zero speed tier "
            "could not be reconstructed.")) {
        return 1;
    }
    osf::CompanionAttackActionController attack_action;
    if (!check(
            attack_action.start(
                kerberos.attack_speed_rating,
                attack_timing),
            "The companion attack action did not start.")) {
        return 1;
    }
    bool impact_seen = false;
    bool swing_seen = false;
    bool completed = false;
    for (int update = 0;
         update < 200 && attack_action.active();
         ++update) {
        const osf::CompanionAttackActionEvent event =
            attack_action.update();
        impact_seen = impact_seen || event.impact_due;
        swing_seen = swing_seen || event.swing_sound_due;
        completed = completed || event.completed;
    }
    if (!check(
            impact_seen && swing_seen && completed &&
                attack_action.animationFrame() ==
                    attack_timing.frame_count - 1,
            "The companion attack did not scan chart-five impact "
            "and sample markers through its retail completion.")) {
        return 1;
    }

    const std::vector<osf::CompanionEnemyTargetState>
        target_fixture{
            {20000002, {origin.x + 500, origin.y},
             {-80, -80, 79, 79}, 100, 20, true},
            {20000001, {origin.x + 300, origin.y},
             {-80, -80, 79, 79}, 100, 30, true},
            {20000003, {origin.x, origin.y + 200},
             {-80, -80, 79, 79}, 0, 10, true},
        };
    const osf::CompanionEnemyTarget nearest =
        osf::findCompanionEnemyTarget(
            origin,
            actor.judgement(),
            target_fixture,
            1200);
    const osf::CompanionEnemyTarget forward =
        osf::findCompanionForwardEnemyTarget(
            origin,
            actor.judgement(),
            1,
            target_fixture,
            150);
    if (!check(
            nearest.found &&
                nearest.character_number == 20000001 &&
                nearest.physical_evasion == 30 &&
                forward.found &&
                forward.character_number == 20000001,
            "Companion enemy acquisition lost nearest-target, life, "
            "range, or exact-facing behavior.")) {
        return 1;
    }

    osf::RetailRandom attack_random(1);
    const osf::CompanionAttackImpactResult impact =
        osf::resolveCompanionAttackImpact(
            {
                16000000,
                kerberos.level,
                kerberos.physical_attack,
                kerberos.hit_rate,
                kerberos.native_element,
                20000001,
                30,
            },
            attack_random);
    if (!check(
            impact.valid &&
                impact.hit_chance == 98 &&
                impact.hit_roll == 41 &&
                impact.apply_damage &&
                !impact.show_miss &&
                impact.post_hit_audio_sample == 44 &&
                impact.packet[0] == 1 &&
                impact.packet[1] == 0 &&
                impact.packet[2] == 16000000 &&
                impact.packet[3] == 0 &&
                impact.packet[4] == 30 &&
                impact.packet[31] == 1 &&
                impact.packet[32] == 0 &&
                impact.packet[34] >= 21000 &&
                impact.packet[34] <= 21003 &&
                impact.packet[37] == 0 &&
                impact.packet[38] == 1 &&
                impact.packet[41] == -1 &&
                impact.packet[43] == -1 &&
                impact.packet[72] == 1 &&
                impact.packet[74] == -1,
            "The ordinary companion hit check, packet, effect, or "
            "post-hit sample differs from retail.")) {
        return 1;
    }
    actor.updateFollow(
        origin,
        world.playerJudgement(),
        world.ground(),
        world.objectMap());
    if (!check(
            actor.motion() == osf::CompanionMotion::idle &&
                actor.position().x == origin.x &&
                actor.position().y == origin.y,
            "A close companion did not remain idle.")) {
        return 1;
    }

    const osf::WorldPosition walking_owner{
        origin.x + 400,
        origin.y,
    };
    for (int update = 0; update < 5; ++update) {
        actor.updateFollow(
            walking_owner,
            world.playerJudgement(),
            world.ground(),
            world.objectMap());
    }
    if (!check(
            actor.motion() == osf::CompanionMotion::idle,
            "The retail five-update close-follow linger was lost.")) {
        return 1;
    }
    actor.updateFollow(
        walking_owner,
        world.playerJudgement(),
        world.ground(),
        world.objectMap());
    if (!check(
            actor.motion() == osf::CompanionMotion::walking &&
                actor.animationChart() == 1,
            "The companion did not enter its retail walking chart.")) {
        return 1;
    }

    actor.relocate(origin, 3);
    actor.updateFollow(
        {origin.x + 1000, origin.y},
        world.playerJudgement(),
        world.ground(),
        world.objectMap());
    if (!check(
            actor.motion() == osf::CompanionMotion::running &&
                actor.animationChart() == 2,
            "A distant companion did not enter its retail run chart.")) {
        return 1;
    }

    const osf::WorldPosition distant_owner{
        origin.x + 5000,
        origin.y + 1000,
    };
    actor.updateFollow(
        distant_owner,
        world.playerJudgement(),
        world.ground(),
        world.objectMap());
    if (!check(
            actor.position().x == distant_owner.x + 200 &&
                actor.position().y == distant_owner.y + 200,
            "The retail out-of-range companion catch-up was lost.")) {
        return 1;
    }

    osf::CompanionDamageReceiverState hit =
        actor.damageReceiverState();
    hit.current_life = hit.maximum_life - 1;
    hit.presentation_action = 5;
    hit.presentation_counter = 0;
    hit.action_lock = 1;
    hit.reaction_duration = 4;
    hit.reaction_motion = true;
    actor.applyDamageReceiverState(hit);
    bool hit_chart_seen = false;
    for (int update = 0; update < 4; ++update) {
        const osf::CompanionPresentationUpdate presentation =
            actor.updateDamagePresentation(
                world.ground(), world.objectMap());
        hit_chart_seen =
            hit_chart_seen ||
            (presentation.handled &&
             actor.animationChart() == 3);
    }
    if (!check(
            hit_chart_seen &&
                actor.presentationAction() == 2 &&
                actor.currentLife() ==
                    actor.maximumLife() - 1,
            "The companion chart-three hit reaction did not scale "
            "and unlock at its receiver duration.")) {
        return 1;
    }

    osf::CompanionDamageReceiverState defeated =
        actor.damageReceiverState();
    defeated.current_life = 0;
    defeated.presentation_action = 6;
    defeated.presentation_counter = 0;
    defeated.action_lock = 1;
    actor.applyDamageReceiverState(defeated);
    const osf::CompanionPresentationUpdate death_start =
        actor.updateDamagePresentation(
            world.ground(), world.objectMap());
    if (!check(
            death_start.handled &&
                death_start.death_started &&
                actor.animationChart() == 4 &&
                actor.animationDirection() == 8 &&
                !actor.judgementEnabled(),
            "The companion did not enter its retail locked chart-four "
            "death presentation.")) {
        return 1;
    }
    for (int update = 0;
         update < 200 && actor.visible();
         ++update) {
        actor.updateDamagePresentation(
            world.ground(), world.objectMap());
    }
    if (!check(
            !actor.visible() &&
                actor.drawOpacity() == 0,
            "The defeated companion did not hold and fade over its "
            "retail chart-four lifetime.")) {
        return 1;
    }

    actor.beginRevive(origin);
    bool revive_completed = false;
    for (int update = 0;
         update < 200 && !revive_completed;
         ++update) {
        revive_completed =
            actor.updateDamagePresentation(
                world.ground(), world.objectMap())
                .revive_completed;
    }
    if (!check(
            revive_completed &&
                actor.presentationAction() == 2 &&
                actor.currentLife() == actor.maximumLife() &&
                actor.position().x == origin.x &&
                actor.position().y == origin.y &&
                actor.visible(),
            "The companion did not revive at its owner through "
            "PARTNER chart seven with full life.")) {
        return 1;
    }

    osf::CompanionProfile kerberos_level_two;
    if (!check(
            osf::decodeCompanionProfile(
                tables, 0, 2, kerberos_level_two, &error),
            "Kerberos' level-two profile could not be decoded.")) {
        return 1;
    }
    actor.applyLevelProfile(kerberos_level_two);
    if (!check(
            actor.profile().level == 2 &&
                actor.maximumLife() >
                    kerberos.maximum_life &&
                actor.currentLife() == actor.maximumLife(),
            "A companion level-up did not rebuild its table profile "
            "and fully heal the live actor.")) {
        return 1;
    }

    if (!check(
            world.transitionScenario({1, 0, 0}, &error) ==
                osf::ScenarioTravelResult::loaded &&
                world.companion().position().x ==
                    world.playerWorldX() &&
                world.companion().position().y ==
                    world.playerWorldY(),
            "A scenario transition did not carry the owned companion "
            "to the player's entry.")) {
        std::cerr << error << '\n';
        return 1;
    }

    bool attack_chart_seen = false;
    bool swing_sample_seen = false;
    bool hit_sample_seen = false;
    bool enemy_damaged = false;
    for (int update = 0;
         update < 1200 && !enemy_damaged;
         ++update) {
        world.update();
        attack_chart_seen =
            attack_chart_seen ||
            world.companion().animationChart() == 5;
        for (std::int32_t sample :
             world.takeAudioSamples()) {
            swing_sample_seen =
                swing_sample_seen || sample == 95;
            hit_sample_seen =
                hit_sample_seen || sample == 44;
        }
        for (const osf::EnemyActor& enemy :
             world.enemies()) {
            enemy_damaged =
                enemy_damaged ||
                (enemy.currentLife() > 0 &&
                 enemy.currentLife() <
                     enemy.maximumLife());
        }
    }
    if (!check(
            attack_chart_seen &&
                swing_sample_seen &&
                hit_sample_seen &&
                enemy_damaged,
            "The live owned companion did not acquire, approach, "
            "animate, and damage an outdoor enemy with retail audio.")) {
        return 1;
    }
    return 0;
}
