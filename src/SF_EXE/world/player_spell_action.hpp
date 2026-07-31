#ifndef OPENSHADOWFLARE_PLAYER_SPELL_ACTION_HPP
#define OPENSHADOWFLARE_PLAYER_SPELL_ACTION_HPP

#include <cstdint>
#include <vector>

namespace osf {

class TableData;

namespace gapi {
class CafAnimation;
}

enum class PlayerSpellAction : std::int32_t {
    fire_ball = 23,
    ice_bolt = 24,
    plasma = 25,
    hell_fire = 26,
    ice_blast = 27,
    heal = 28,
    moon = 29,
    berserker = 30,
    energy_shield = 31,
    earth_spear = 32,
    flame_strike = 33,
    dread_deathscythe = 34,
    lightning_storm = 35,
    medusa = 36,
    sonic_blade = 37,
    mud_javelin = 38,
    identify = 39,
};

enum class PlayerSpellAnimationVariant {
    standard,
    sonic_blade_subtype_0,
    sonic_blade_subtype_3,
    sonic_blade_subtype_1,
};

struct PlayerSpellAnimationTiming {
    std::int32_t first_chart = -1;
    std::int32_t recovery_chart = -1;
    std::int32_t first_frame_count = 0;
    std::int32_t recovery_frame_count = 0;
    std::vector<std::int16_t> first_frame_statuses;
};

struct PlayerSpellActionEvent {
    PlayerSpellAction action = PlayerSpellAction::fire_ball;
    std::int32_t spell = -1;
    std::int32_t target_character_number = -1;
    std::int32_t aim_world_x = 0;
    std::int32_t aim_world_y = 0;
    std::int32_t effect_delay = 0;
    std::int32_t entry_visual_effect_number = -1;
    bool cast_due = false;
    bool swing_sound_due = false;
    bool completed = false;
};

bool buildPlayerSpellAnimationTiming(
    const gapi::CafAnimation& animation,
    PlayerSpellAction action,
    std::int32_t direction,
    PlayerSpellAnimationTiming& timing);
bool buildPlayerSpellAnimationTiming(
    const gapi::CafAnimation& animation,
    PlayerSpellAction action,
    PlayerSpellAnimationVariant variant,
    std::int32_t direction,
    PlayerSpellAnimationTiming& timing);

bool playerSpellActionForSpell(
    std::int32_t spell,
    PlayerSpellAction& action);
bool playerSonicBladeAnimationVariant(
    std::int32_t weapon_subtype,
    PlayerSpellAnimationVariant& variant);

double retailPlayerSpellAnimationSpeed(
    std::int32_t spell,
    std::int32_t speed_tier,
    const TableData* speed_table);
double retailPlayerSonicBladeAnimationSpeed(
    std::int32_t speed_tier);

class PlayerSpellActionController {
public:
    bool start(
        PlayerSpellAction action,
        std::int32_t spell,
        std::int32_t target_character_number,
        std::int32_t aim_world_x,
        std::int32_t aim_world_y,
        std::int32_t speed_tier,
        const TableData* speed_table,
        PlayerSpellAnimationTiming timing,
        PlayerSpellActionEvent* event = nullptr);
    PlayerSpellActionEvent update(
        std::int32_t speed_tier = -1,
        const TableData* speed_table = nullptr);
    void cancel();

    bool active() const;
    PlayerSpellAction action() const;
    std::int32_t spell() const;
    std::int32_t targetCharacterNumber() const;
    std::int32_t animationChart() const;
    std::int32_t animationFrame() const;
    std::int32_t actionCounter() const;
    std::int32_t displayedFrame() const;
    std::int32_t effectDelay() const;

private:
    void refreshSpeed(
        std::int32_t speed_tier,
        const TableData* speed_table);
    void selectRenderedFrame();

    PlayerSpellAnimationTiming timing_;
    PlayerSpellAction action_ =
        PlayerSpellAction::fire_ball;
    std::int32_t spell_ = -1;
    std::int32_t target_character_number_ = -1;
    std::int32_t aim_world_x_ = 0;
    std::int32_t aim_world_y_ = 0;
    std::int32_t speed_tier_ = 0;
    double animation_speed_ = 1.0;
    std::int32_t completion_increment_ = 1;
    std::int32_t effect_delay_ = 0;
    std::int32_t last_effect_scan_frame_ = -1;
    bool cast_dispatched_ = false;
    std::int32_t action_counter_ = 0;
    std::int32_t displayed_frame_ = 0;
    std::int32_t animation_chart_ = 0;
    std::int32_t animation_frame_ = 0;
    bool active_ = false;
};

}  // namespace osf

#endif
