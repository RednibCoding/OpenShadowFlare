#ifndef OPENSHADOWFLARE_PLAYER_ENERGY_SHIELD_HPP
#define OPENSHADOWFLARE_PLAYER_ENERGY_SHIELD_HPP

#include <cstdint>

namespace osf {

class PlayerMagic;
class TableDatabase;

class PlayerEnergyShield {
public:
    bool toggle(std::int32_t current_mana);
    bool deactivate();
    void updateAura(bool displayed);
    void clear();

    bool active() const;
    std::int32_t auraFrame() const;

private:
    bool active_ = false;
    std::int32_t aura_frame_ = 0;
};

bool trainEnergyShieldOnOwnedKill(
    PlayerMagic& magic,
    const PlayerEnergyShield& energy_shield,
    std::int32_t defeat_source_character_number,
    std::int32_t local_player_slot,
    const TableDatabase& tables);

}  // namespace osf

#endif
