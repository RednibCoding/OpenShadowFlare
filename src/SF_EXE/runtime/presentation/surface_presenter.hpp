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
    // Platforms with an auxiliary display can draw it before the primary
    // surface is presented. The default keeps single-screen presenters
    // unchanged.
    virtual void presentAuxiliary(gapi::SurfaceView) {}

protected:
    SurfacePresenter() = default;
};

std::unique_ptr<SurfacePresenter> createSurfacePresenter();

}  // namespace osf::runtime
