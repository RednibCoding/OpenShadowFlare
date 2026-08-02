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
    virtual void present(gapi::SurfaceView surface) = 0;
#if OSF_ENABLE_DEBUG_TOOLS
    virtual std::optional<std::uint64_t>
        videoMemoryUsageBytes() const = 0;
#endif

protected:
    SurfacePresenter() = default;
};

std::unique_ptr<SurfacePresenter> createSurfacePresenter();

}  // namespace osf::runtime
