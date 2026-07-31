#ifndef OPENSHADOWFLARE_PLAYER_SUSTAINED_SPELL_HPP
#define OPENSHADOWFLARE_PLAYER_SUSTAINED_SPELL_HPP

#include <cstdint>

namespace osf {

class TableDatabase;
class PlayerMagic;

class PlayerSustainedSpell {
public:
    bool toggle(
        std::int32_t table_number,
        std::int32_t effective_level,
        const TableDatabase& tables);
    bool deactivate();
    void updateAura(bool displayed);
    void clear();

    bool active() const;
    std::int32_t effectiveLevel() const;
    std::int32_t manaChangeRate() const;
    std::int32_t auraFrame() const;

private:
    std::int32_t effective_level_ = 0;
    std::int32_t mana_change_rate_ = 0;
    std::int32_t aura_frame_ = 0;
};

struct PlayerSustainedSpellTraining {
    bool moon_trained = false;
    bool berserker_trained = false;
};

struct PlayerSustainedSpellShutdown {
    bool moon_deactivated = false;
    bool berserker_deactivated = false;
};

PlayerSustainedSpellShutdown
deactivateSustainedSpellsAtZeroMana(
    std::int32_t current_mana,
    PlayerSustainedSpell& moon,
    PlayerSustainedSpell& berserker);

PlayerSustainedSpellTraining
trainActiveSustainedSpellsOnOwnedKill(
    PlayerMagic& magic,
    const PlayerSustainedSpell& moon,
    const PlayerSustainedSpell& berserker,
    std::int32_t defeat_source_character_number,
    std::int32_t local_player_slot,
    const TableDatabase& tables);

}  // namespace osf

#endif
