#ifndef OPENSHADOWFLARE_PLAYER_MAGIC_SHIELD_HPP
#define OPENSHADOWFLARE_PLAYER_MAGIC_SHIELD_HPP

#include <cstdint>

namespace osf {

class PlayerMagicShield {
public:
    bool toggle();
    bool deactivate();
    void restoreActive(bool active);
    void updateAura(bool displayed);
    void clear();

    bool active() const;
    std::int32_t auraFrame() const;

private:
    bool active_ = false;
    std::int32_t aura_frame_ = 0;
};

}  // namespace osf

#endif
