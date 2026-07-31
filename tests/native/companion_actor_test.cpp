#include "gapi/gapi.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "render/gameplay_help_renderer.hpp"
#include "resources/character_visual_resource.hpp"
#include "world/companion_actor.hpp"
#include "world/companion_profile.hpp"
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
                kerberos.attack_speed == 900 &&
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
                gravity.attack_speed == 600 &&
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
    return 0;
}
