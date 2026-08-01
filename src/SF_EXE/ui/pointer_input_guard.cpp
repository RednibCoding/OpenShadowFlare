#include "pointer_input_guard.hpp"

namespace osf {

void PointerInputGuard::reset() {
    consumed_until_release_ = false;
}

void PointerInputGuard::consumeUntilRelease(
    bool pointer_down) {
    consumed_until_release_ = pointer_down;
}

bool PointerInputGuard::update(bool pointer_down) {
    if (!consumed_until_release_) {
        return false;
    }
    if (!pointer_down) {
        consumed_until_release_ = false;
    }
    return true;
}

}  // namespace osf
