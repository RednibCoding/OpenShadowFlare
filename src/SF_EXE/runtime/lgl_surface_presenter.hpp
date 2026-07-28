#ifndef OPENSHADOWFLARE_LGL_SURFACE_PRESENTER_HPP
#define OPENSHADOWFLARE_LGL_SURFACE_PRESENTER_HPP

#include "gapi/gapi.hpp"

#include <cstdint>
#include <string>

class LglSurfacePresenter {
public:
    bool initialize(std::string* error = nullptr);
    void shutdown();
    void present(
        osf::gapi::SurfaceView surface,
        std::int32_t window_width,
        std::int32_t window_height);

private:
    unsigned int compileShader(
        unsigned int type,
        const char* source,
        std::string* error);

    unsigned int program_ = 0;
    unsigned int texture_ = 0;
    unsigned int vertex_array_ = 0;
    std::int32_t texture_width_ = 0;
    std::int32_t texture_height_ = 0;
};

#endif
