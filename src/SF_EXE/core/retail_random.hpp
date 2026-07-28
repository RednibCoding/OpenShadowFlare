#ifndef OPENSHADOWFLARE_RETAIL_RANDOM_HPP
#define OPENSHADOWFLARE_RETAIL_RANDOM_HPP

#include <cstdint>

namespace osf {

// The Visual C++ rand() implementation statically linked into ShadowFlare.exe.
class RetailRandom {
public:
    explicit RetailRandom(std::uint32_t seed = 1);

    void seed(std::uint32_t value);
    std::int32_t next();
    std::uint32_t state() const;

private:
    std::uint32_t state_;
};

}  // namespace osf

#endif
