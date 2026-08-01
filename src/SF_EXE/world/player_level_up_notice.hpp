#ifndef OPENSHADOWFLARE_PLAYER_LEVEL_UP_NOTICE_HPP
#define OPENSHADOWFLARE_PLAYER_LEVEL_UP_NOTICE_HPP

#include <cstdint>
#include <string>

namespace osf {

struct PlayerLevelUpNotice {
    std::string text;
    std::int32_t counter = 0;

    bool active() const;
    bool dismissible() const;
    void update();
    void dismiss();
};

}  // namespace osf

#endif
