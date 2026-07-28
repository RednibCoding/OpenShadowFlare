#include "lgl_surface_presenter.hpp"

#include "lgl.h"

#include <cstddef>
#include <vector>

namespace {

constexpr const char* kVertexShader = R"(
#version 330 core

out vec2 texture_coordinate;

void main() {
    const vec2 positions[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    const vec2 coordinates[3] = vec2[3](
        vec2(0.0, 0.0),
        vec2(2.0, 0.0),
        vec2(0.0, 2.0)
    );
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    texture_coordinate = coordinates[gl_VertexID];
}
)";

constexpr const char* kFragmentShader = R"(
#version 330 core

in vec2 texture_coordinate;
out vec4 fragment_color;
uniform sampler2D surface_texture;

void main() {
    fragment_color = texture(
        surface_texture,
        vec2(texture_coordinate.x, 1.0 - texture_coordinate.y));
}
)";

void setError(std::string* error, const char* message) {
    if (error) {
        *error = message;
    }
}

}  // namespace

bool LglSurfacePresenter::initialize(std::string* error) {
    shutdown();

    const unsigned int vertex =
        compileShader(LGL_VERTEX_SHADER, kVertexShader, error);
    if (vertex == 0) {
        return false;
    }
    const unsigned int fragment =
        compileShader(LGL_FRAGMENT_SHADER, kFragmentShader, error);
    if (fragment == 0) {
        lglDeleteShader(vertex);
        return false;
    }

    program_ = lglCreateProgram();
    lglAttachShader(program_, vertex);
    lglAttachShader(program_, fragment);
    lglLinkProgram(program_);
    lglDeleteShader(vertex);
    lglDeleteShader(fragment);

    int linked = 0;
    lglGetProgramiv(program_, LGL_LINK_STATUS, &linked);
    if (linked == 0) {
        int length = 0;
        lglGetProgramiv(program_, LGL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(
            static_cast<std::size_t>(length > 1 ? length : 1));
        lglGetProgramInfoLog(
            program_,
            static_cast<int>(log.size()),
            nullptr,
            log.data());
        if (error) {
            *error = log.data();
        }
        shutdown();
        return false;
    }

    lglGenTextures(1, &texture_);
    lglBindTexture(LGL_TEXTURE_2D, texture_);
    lglTexParameteri(
        LGL_TEXTURE_2D, LGL_TEXTURE_MIN_FILTER, LGL_NEAREST);
    lglTexParameteri(
        LGL_TEXTURE_2D, LGL_TEXTURE_MAG_FILTER, LGL_NEAREST);
    lglPixelStorei(LGL_UNPACK_ALIGNMENT, 1);
    lglGenVertexArrays(1, &vertex_array_);

    if (error) {
        error->clear();
    }
    return program_ != 0 && texture_ != 0 && vertex_array_ != 0;
}

void LglSurfacePresenter::shutdown() {
    if (vertex_array_ != 0 && lglDeleteVertexArrays) {
        lglDeleteVertexArrays(1, &vertex_array_);
    }
    if (texture_ != 0 && lglDeleteTextures) {
        lglDeleteTextures(1, &texture_);
    }
    if (program_ != 0 && lglDeleteProgram) {
        lglDeleteProgram(program_);
    }
    vertex_array_ = 0;
    texture_ = 0;
    program_ = 0;
    texture_width_ = 0;
    texture_height_ = 0;
}

void LglSurfacePresenter::present(
    osf::gapi::SurfaceView surface,
    std::int32_t window_width,
    std::int32_t window_height) {
    static_assert(
        sizeof(osf::gapi::Color) == 4,
        "GAPI colors must be tightly packed RGBA bytes.");
    if (program_ == 0 || texture_ == 0 ||
        vertex_array_ == 0 || !surface.pixels ||
        surface.width <= 0 || surface.height <= 0 ||
        window_width <= 0 || window_height <= 0) {
        return;
    }

    lglBindTexture(LGL_TEXTURE_2D, texture_);
    if (texture_width_ != surface.width ||
        texture_height_ != surface.height) {
        texture_width_ = surface.width;
        texture_height_ = surface.height;
        lglTexImage2D(
            LGL_TEXTURE_2D,
            0,
            LGL_RGBA8,
            texture_width_,
            texture_height_,
            0,
            LGL_RGBA,
            LGL_UNSIGNED_BYTE,
            surface.pixels);
    } else {
        lglTexSubImage2D(
            LGL_TEXTURE_2D,
            0,
            0,
            0,
            texture_width_,
            texture_height_,
            LGL_RGBA,
            LGL_UNSIGNED_BYTE,
            surface.pixels);
    }

    lglViewport(0, 0, window_width, window_height);
    lglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    lglClear(LGL_COLOR_BUFFER_BIT);

    const osf::gapi::Viewport viewport =
        osf::gapi::fitViewport(
            surface.width,
            surface.height,
            window_width,
            window_height);
    lglViewport(
        viewport.x,
        window_height - viewport.y - viewport.height,
        viewport.width,
        viewport.height);
    lglUseProgram(program_);
    lglBindVertexArray(vertex_array_);
    lglDrawArrays(LGL_TRIANGLES, 0, 3);
}

unsigned int LglSurfacePresenter::compileShader(
    unsigned int type,
    const char* source,
    std::string* error) {
    const unsigned int shader = lglCreateShader(type);
    if (shader == 0) {
        setError(error, "OpenGL could not create a shader.");
        return 0;
    }
    lglShaderSource(shader, 1, &source, nullptr);
    lglCompileShader(shader);

    int compiled = 0;
    lglGetShaderiv(shader, LGL_COMPILE_STATUS, &compiled);
    if (compiled != 0) {
        return shader;
    }

    int length = 0;
    lglGetShaderiv(shader, LGL_INFO_LOG_LENGTH, &length);
    std::vector<char> log(
        static_cast<std::size_t>(length > 1 ? length : 1));
    lglGetShaderInfoLog(
        shader,
        static_cast<int>(log.size()),
        nullptr,
        log.data());
    if (error) {
        *error = log.data();
    }
    lglDeleteShader(shader);
    return 0;
}
