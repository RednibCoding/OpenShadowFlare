#ifndef OPENSHADOWFLARE_NPC_SCRIPT_ACTION_HPP
#define OPENSHADOWFLARE_NPC_SCRIPT_ACTION_HPP

#include <cstdint>

namespace osf {

struct NpcScriptActionUpdate {
    bool handled = false;
    bool completed = false;
    std::int32_t action = 1;
    std::int32_t frame = 0;
};

class NpcScriptActionController {
public:
    bool start(
        std::int32_t action,
        std::int32_t repeat,
        std::int32_t restart_frame,
        std::int32_t end_frame);
    void cancel();
    NpcScriptActionUpdate update(std::int32_t frame_count);

    bool active() const;
    std::int32_t action() const;

private:
    std::int32_t action_ = 1;
    std::int32_t frame_ = -1;
    std::int32_t restart_frame_ = -1;
    std::int32_t end_frame_ = -1;
    bool repeat_ = false;
    bool active_ = false;
};

}  // namespace osf

#endif
