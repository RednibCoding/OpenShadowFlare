#pragma once

#include "gapi/gapi.hpp"

#include <memory>
#include <string>
#if OSF_ENABLE_DEBUG_TOOLS
#include <cstdint>
#include <optional>
#endif

struct LwlWindow;

namespace osf::runtime {

class SurfacePresenter {
public:
    virtual ~SurfacePresenter() = default;

    SurfacePresenter(const SurfacePresenter&) = delete;
    SurfacePresenter& operator=(const SurfacePresenter&) = delete;

    virtual bool initialize(
        LwlWindow* window,
        std::string* error = nullptr) = 0;
    // Prepare the finished software surface without waiting for display
    // synchronization. The common runtime profiles this work.
    virtual void prepareFrame(gapi::SurfaceView surface) = 0;
    // Make the prepared frame visible. This may wait for the display.
    virtual void displayFrame() = 0;
#if OSF_ENABLE_DEBUG_TOOLS
    virtual std::optional<std::uint64_t>
        videoMemoryUsageBytes() const = 0;
#endif

protected:
    SurfacePresenter() = default;
};

std::unique_ptr<SurfacePresenter> createSurfacePresenter();

}  // namespace osf::runtime
