#include "runtime/presentation/surface_presenter.hpp"

#include "lgl.h"
#include "lwl.h"

#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr const char* kVertexShaderBody = R"(
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

constexpr const char* kFragmentShaderBody = R"(
in vec2 texture_coordinate;
out vec4 fragment_color;
uniform sampler2D surface_texture;

const float kSharpness = 1.0;
const float kFlatRange = 0.05;

vec3 sampleAt(vec2 uv) {
    return texture(
        surface_texture,
        clamp(uv, vec2(0.0), vec2(1.0))).rgb;
}

void main() {
    vec2 texel = 1.0 / vec2(textureSize(surface_texture, 0));
    vec2 flipped = vec2(texture_coordinate.x, 1.0 - texture_coordinate.y);

    vec3 a = sampleAt(flipped + vec2(-texel.x, -texel.y));
    vec3 b = sampleAt(flipped + vec2( 0.0,      -texel.y));
    vec3 c = sampleAt(flipped + vec2( texel.x,  -texel.y));
    vec3 d = sampleAt(flipped + vec2(-texel.x,   0.0));
    vec3 e = sampleAt(flipped);
    vec3 f = sampleAt(flipped + vec2( texel.x,   0.0));
    vec3 g = sampleAt(flipped + vec2(-texel.x,   texel.y));
    vec3 h = sampleAt(flipped + vec2( 0.0,       texel.y));
    vec3 i = sampleAt(flipped + vec2( texel.x,   texel.y));

    vec3 luma_weights = vec3(0.299, 0.587, 0.114);
    float la = dot(a, luma_weights);
    float lb = dot(b, luma_weights);
    float lc = dot(c, luma_weights);
    float ld = dot(d, luma_weights);
    float lf = dot(f, luma_weights);
    float lg = dot(g, luma_weights);
    float lh = dot(h, luma_weights);
    float li = dot(i, luma_weights);

    float min_luma = min(min(min(min(la, lb), min(lc, ld)), min(lf, lg)), min(lh, li));
    float max_luma = max(max(max(max(la, lb), max(lc, ld)), max(lf, lg)), max(lh, li));
    float range = max_luma - min_luma;
    float amount = kSharpness * smoothstep(0.0, kFlatRange, range);

    vec3 lowpass = (a + c + g + i) * 0.0625
                 + (b + d + f + h) * 0.125
                 + e * 0.25;
    vec3 sharpened = e + (e - lowpass) * amount;

    fragment_color = vec4(clamp(sharpened, 0.0, 1.0), 1.0);
}
)";

void setError(std::string* error, const char* message) {
    if (error) {
        *error = message;
    }
}

void* loadOpenGlFunction(const char* name, void*) {
    return lwl_gl_get_proc_address(name);
}

class LglSurfacePresenter final
    : public osf::runtime::SurfacePresenter {
public:
    ~LglSurfacePresenter() override {
        shutdown();
    }

    bool initialize(
        LwlWindow* window,
        std::string* error) override;
    void present(osf::gapi::SurfaceView surface) override;

private:
    void shutdown();
    unsigned int compileShader(
        unsigned int type,
        const char* source,
        std::string* error);

    LwlWindow* window_ = nullptr;
    LwlGlContext* context_ = nullptr;
    unsigned int program_ = 0;
    unsigned int texture_ = 0;
    unsigned int vertex_array_ = 0;
    std::int32_t texture_width_ = 0;
    std::int32_t texture_height_ = 0;
};

bool LglSurfacePresenter::initialize(
    LwlWindow* window,
    std::string* error) {
    shutdown();

    const LwlGlConfig config = lwl_gl_config_default();
    context_ = lwl_gl_context_create(window, &config);
    if (!context_ || !lwl_gl_context_make_current(context_)) {
        setError(error, "Could not create the requested graphics context.");
        shutdown();
        return false;
    }

    const LglApi api = config.api == LWL_GL_API_ES
        ? LGL_API_OPENGL_ES
        : LGL_API_DESKTOP_OPENGL;
    if (!lgl_load_for_api(loadOpenGlFunction, nullptr, api)) {
        setError(error, lgl_last_error());
        shutdown();
        return false;
    }

    const char* version = api == LGL_API_OPENGL_ES
        ? "#version 300 es\n"
        : "#version 330 core\n";
    const char* fragmentPrecision = api == LGL_API_OPENGL_ES
        ? "precision highp float;\n"
        : "";
    const std::string vertexSource =
        std::string(version) + kVertexShaderBody;
    const std::string fragmentSource =
        std::string(version) + fragmentPrecision +
        kFragmentShaderBody;

    const unsigned int vertex =
        compileShader(
            LGL_VERTEX_SHADER, vertexSource.c_str(), error);
    if (vertex == 0) {
        return false;
    }
    const unsigned int fragment =
        compileShader(
            LGL_FRAGMENT_SHADER, fragmentSource.c_str(), error);
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
        LGL_TEXTURE_2D, LGL_TEXTURE_MIN_FILTER, LGL_LINEAR);
    lglTexParameteri(
        LGL_TEXTURE_2D, LGL_TEXTURE_MAG_FILTER, LGL_LINEAR);
    lglPixelStorei(LGL_UNPACK_ALIGNMENT, 1);
    lglGenVertexArrays(1, &vertex_array_);

    window_ = window;
    if (!lwl_gl_context_set_swap_interval(context_, 1)) {
        std::fprintf(
            stderr,
            "Warning: display synchronization is unavailable.\n");
    }

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
    lgl_reset();
    lwl_gl_context_destroy(context_);
    context_ = nullptr;
    window_ = nullptr;
}

void LglSurfacePresenter::present(
    osf::gapi::SurfaceView surface) {
    int window_width = 0;
    int window_height = 0;
    lwl_window_get_size(
        window_, &window_width, &window_height);
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
    lwl_gl_context_swap_buffers(context_);
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

}  // namespace

std::unique_ptr<osf::runtime::SurfacePresenter>
osf::runtime::createSurfacePresenter() {
    return std::make_unique<LglSurfacePresenter>();
}
