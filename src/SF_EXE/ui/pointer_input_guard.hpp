#ifndef OPENSHADOWFLARE_POINTER_INPUT_GUARD_HPP
#define OPENSHADOWFLARE_POINTER_INPUT_GUARD_HPP

namespace osf {

class PointerInputGuard {
public:
    void reset();
    void consumeUntilRelease(bool pointer_down);
    bool update(bool pointer_down);

private:
    bool consumed_until_release_ = false;
};

}  // namespace osf

#endif
