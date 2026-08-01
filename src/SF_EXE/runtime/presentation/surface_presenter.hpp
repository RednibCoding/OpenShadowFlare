#pragma once

#include "gapi/gapi.hpp"

#include <memory>
#include <string>

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

    // Re-establish platform display state after an external modal takeover of the
    // display (e.g. a system on-screen keyboard). Default: nothing to do.
    virtual void reset() {}

protected:
    SurfacePresenter() = default;
};

std::unique_ptr<SurfacePresenter> createSurfacePresenter();

}  // namespace osf::runtime
