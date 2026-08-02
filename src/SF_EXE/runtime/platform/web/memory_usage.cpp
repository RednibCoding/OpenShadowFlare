#include "runtime/platform/memory_usage.hpp"

#include <emscripten/heap.h>

namespace osf::runtime {

std::optional<std::uint64_t> residentMemoryUsageBytes() {
    return static_cast<std::uint64_t>(emscripten_get_heap_size());
}

}  // namespace osf::runtime
