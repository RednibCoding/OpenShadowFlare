#include "runtime/platform/memory_usage.hpp"

#include <mach/mach.h>

namespace osf::runtime {

std::optional<std::uint64_t> residentMemoryUsageBytes() {
    mach_task_basic_info_data_t info{};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(
            mach_task_self(),
            MACH_TASK_BASIC_INFO,
            reinterpret_cast<task_info_t>(&info),
            &count) != KERN_SUCCESS) {
        return std::nullopt;
    }
    return static_cast<std::uint64_t>(info.resident_size);
}

}  // namespace osf::runtime
