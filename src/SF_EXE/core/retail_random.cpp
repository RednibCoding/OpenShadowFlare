#include "retail_random.hpp"

namespace osf {

RetailRandom::RetailRandom(std::uint32_t seed)
    : state_(seed) {}

void RetailRandom::seed(std::uint32_t value) {
    state_ = value;
}

std::int32_t RetailRandom::next() {
    state_ = state_ * 0x343fdU + 0x269ec3U;
    return static_cast<std::int32_t>((state_ >> 16U) & 0x7fffU);
}

std::uint32_t RetailRandom::state() const {
    return state_;
}

}  // namespace osf
