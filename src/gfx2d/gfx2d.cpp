/*
 * gfx2d.cpp - Implementation of 2D graphics layer
 */

#include "gfx2d.hpp"
#include <cstring>
#include <algorithm>

// OpenGL 1.2 - no extensions needed, just the basic fixed-function stuff
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif
#include <GL/gl.h>

namespace gfx2d {

/*==============================================================================
 * Bitmap Implementation
 *============================================================================*/

Bitmap::Bitmap() = default;

Bitmap::Bitmap(int width, int height) {
    create(width, height);
}

Bitmap::~Bitmap() {
    release();
}

Bitmap::Bitmap(Bitmap&& other) noexcept
    : m_width(other.m_width), m_height(other.m_height), m_pixels(other.m_pixels) {
    other.m_width = 0;
    other.m_height = 0;
    other.m_pixels = nullptr;
}

Bitmap& Bitmap::operator=(Bitmap&& other) noexcept {
    if (this != &other) {
        release();
        m_width = other.m_width;
        m_height = other.m_height;
        m_pixels = other.m_pixels;
        other.m_width = 0;
        other.m_height = 0;
        other.m_pixels = nullptr;
    }
    return *this;
}

bool Bitmap::create(int width, int height) {
    release();
    if (width <= 0 || height <= 0) return false;
    
    m_width = width;
    m_height = height;
    m_pixels = new uint8_t[width * height * 4];
    
    // Clear to transparent black
    std::memset(m_pixels, 0, width * height * 4);
    return true;
}

void Bitmap::release() {
    delete[] m_pixels;
    m_pixels = nullptr;
    m_width = 0;
    m_height = 0;
}

Color Bitmap::getPixel(int x, int y) const {
    if (!m_pixels || x < 0 || y < 0 || x >= m_width || y >= m_height) {
        return Color();
    }
    const uint8_t* p = m_pixels + (y * m_width + x) * 4;
    return Color(p[0], p[1], p[2], p[3]);
}

void Bitmap::setPixel(int x, int y, Color c) {
    if (!m_pixels || x < 0 || y < 0 || x >= m_width || y >= m_height) {
        return;
    }
    uint8_t* p = m_pixels + (y * m_width + x) * 4;
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
    p[3] = c.a;
}

void Bitmap::clear(Color c) {
    if (!m_pixels) return;
    
    // Fill first pixel
    uint8_t* p = m_pixels;
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
    p[3] = c.a;
    
    // Copy to rest (fast for uniform color)
    uint32_t pixel = c.toRGBA();
    uint32_t* pixels32 = reinterpret_cast<uint32_t*>(m_pixels);
    int count = m_width * m_height;
    for (int i = 0; i < count; i++) {
        pixels32[i] = pixel;
    }
}

void Bitmap::fillRect(const Rect& rect, Color c) {
    if (!m_pixels) return;
    
    int x0 = std::max(0, rect.x);
    int y0 = std::max(0, rect.y);
    int x1 = std::min(m_width, rect.x + rect.w);
    int y1 = std::min(m_height, rect.y + rect.h);
    
    uint32_t pixel = c.toRGBA();
    uint32_t* pixels32 = reinterpret_cast<uint32_t*>(m_pixels);
    
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            pixels32[y * m_width + x] = pixel;
        }
    }
}

void Bitmap::blit(const Bitmap& src, int destX, int destY) {
    blit(src, destX, destY, Rect(0, 0, src.width(), src.height()));
}

void Bitmap::blit(const Bitmap& src, int destX, int destY, const Rect& srcRect) {
    if (!m_pixels || !src.valid()) return;
    
    // Clip source rect
    int sx0 = std::max(0, srcRect.x);
    int sy0 = std::max(0, srcRect.y);
    int sx1 = std::min(src.width(), srcRect.x + srcRect.w);
    int sy1 = std::min(src.height(), srcRect.y + srcRect.h);
    
    // Adjust dest based on clipping
    destX += sx0 - srcRect.x;
    destY += sy0 - srcRect.y;
    
    for (int sy = sy0; sy < sy1; sy++) {
        int dy = destY + (sy - sy0);
        if (dy < 0 || dy >= m_height) continue;
        
        for (int sx = sx0; sx < sx1; sx++) {
            int dx = destX + (sx - sx0);
            if (dx < 0 || dx >= m_width) continue;
            
            const uint8_t* sp = src.pixels() + (sy * src.width() + sx) * 4;
            uint8_t* dp = m_pixels + (dy * m_width + dx) * 4;
            
            dp[0] = sp[0];
            dp[1] = sp[1];
            dp[2] = sp[2];
            dp[3] = sp[3];
        }
    }
}

void Bitmap::blitKeyed(const Bitmap& src, int destX, int destY, Color colorKey) {
    if (!m_pixels || !src.valid()) return;
    
    uint32_t key = colorKey.toRGBA() & 0x00FFFFFF;  // Ignore alpha in key
    
    for (int sy = 0; sy < src.height(); sy++) {
        int dy = destY + sy;
        if (dy < 0 || dy >= m_height) continue;
        
        for (int sx = 0; sx < src.width(); sx++) {
            int dx = destX + sx;
            if (dx < 0 || dx >= m_width) continue;
            
            const uint8_t* sp = src.pixels() + (sy * src.width() + sx) * 4;
            uint32_t srcPixel = *reinterpret_cast<const uint32_t*>(sp);
            
            // Skip if matches color key (compare RGB only)
            if ((srcPixel & 0x00FFFFFF) == key) continue;
            
            uint8_t* dp = m_pixels + (dy * m_width + dx) * 4;
            dp[0] = sp[0];
            dp[1] = sp[1];
            dp[2] = sp[2];
            dp[3] = sp[3];
        }
    }
}

void Bitmap::blitAlpha(const Bitmap& src, int destX, int destY) {
    if (!m_pixels || !src.valid()) return;
    
    for (int sy = 0; sy < src.height(); sy++) {
        int dy = destY + sy;
        if (dy < 0 || dy >= m_height) continue;
        
        for (int sx = 0; sx < src.width(); sx++) {
            int dx = destX + sx;
            if (dx < 0 || dx >= m_width) continue;
            
            const uint8_t* sp = src.pixels() + (sy * src.width() + sx) * 4;
            uint8_t* dp = m_pixels + (dy * m_width + dx) * 4;
            
            uint8_t sa = sp[3];
            if (sa == 0) continue;        // Fully transparent, skip
            if (sa == 255) {              // Fully opaque, just copy
                dp[0] = sp[0];
                dp[1] = sp[1];
                dp[2] = sp[2];
                dp[3] = 255;
                continue;
            }
            
            // Alpha blend: out = src * alpha + dst * (1 - alpha)
            uint8_t da = 255 - sa;
            dp[0] = (sp[0] * sa + dp[0] * da) / 255;
            dp[1] = (sp[1] * sa + dp[1] * da) / 255;
            dp[2] = (sp[2] * sa + dp[2] * da) / 255;
            dp[3] = 255;
        }
    }
}

/*==============================================================================
 * Texture Implementation
 *============================================================================*/

Texture::Texture() = default;

Texture::~Texture() {
    release();
}

Texture::Texture(Texture&& other) noexcept
    : m_texId(other.m_texId), m_width(other.m_width), m_height(other.m_height) {
    other.m_texId = 0;
    other.m_width = 0;
    other.m_height = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        release();
        m_texId = other.m_texId;
        m_width = other.m_width;
        m_height = other.m_height;
        other.m_texId = 0;
        other.m_width = 0;
        other.m_height = 0;
    }
    return *this;
}

bool Texture::createFromBitmap(const Bitmap& bitmap) {
    if (!bitmap.valid()) return false;
    
    release();
    
    glGenTextures(1, &m_texId);
    glBindTexture(GL_TEXTURE_2D, m_texId);
    
    // Set filtering (nearest for pixel-perfect 2D)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    
    // Upload pixels
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap.width(), bitmap.height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap.pixels());
    
    m_width = bitmap.width();
    m_height = bitmap.height();
    
    return true;
}

bool Texture::updateFromBitmap(const Bitmap& bitmap) {
    if (!valid() || !bitmap.valid()) return false;
    if (bitmap.width() != m_width || bitmap.height() != m_height) return false;
    
    glBindTexture(GL_TEXTURE_2D, m_texId);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height,
                    GL_RGBA, GL_UNSIGNED_BYTE, bitmap.pixels());
    
    return true;
}

void Texture::release() {
    if (m_texId != 0) {
        glDeleteTextures(1, &m_texId);
        m_texId = 0;
    }
    m_width = 0;
    m_height = 0;
}

/*==============================================================================
 * Renderer Implementation
 *============================================================================*/

Renderer::Renderer() = default;

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::init(int screenWidth, int screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    m_virtualWidth = screenWidth;
    m_virtualHeight = screenHeight;
    
    // Basic OpenGL setup
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    setupOrtho();
    
    m_initialized = true;
    return true;
}

void Renderer::shutdown() {
    m_initialized = false;
}

void Renderer::setVirtualSize(int width, int height) {
    m_virtualWidth = width;
    m_virtualHeight = height;
    setupOrtho();
}

void Renderer::setupOrtho() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Top-left origin (like DirectDraw/2D games expect)
    glOrtho(0, m_virtualWidth, m_virtualHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glViewport(0, 0, m_screenWidth, m_screenHeight);
}

void Renderer::beginFrame() {
    // Could add state tracking here if needed
}

void Renderer::endFrame() {
    // Flush any pending draws
    glFlush();
}

void Renderer::clear(Color c) {
    glClearColor(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::drawTexture(const Texture& tex, int x, int y) {
    drawTexture(tex, x, y, Rect(0, 0, tex.width(), tex.height()));
}

void Renderer::drawTexture(const Texture& tex, int x, int y, const Rect& srcRect) {
    if (!tex.valid()) return;
    
    glBindTexture(GL_TEXTURE_2D, tex.glId());
    glColor4f(1, 1, 1, 1);
    
    // Calculate texture coordinates
    float u0 = static_cast<float>(srcRect.x) / tex.width();
    float v0 = static_cast<float>(srcRect.y) / tex.height();
    float u1 = static_cast<float>(srcRect.x + srcRect.w) / tex.width();
    float v1 = static_cast<float>(srcRect.y + srcRect.h) / tex.height();
    
    float x0 = static_cast<float>(x);
    float y0 = static_cast<float>(y);
    float x1 = x0 + srcRect.w;
    float y1 = y0 + srcRect.h;
    
    glBegin(GL_QUADS);
        glTexCoord2f(u0, v0); glVertex2f(x0, y0);
        glTexCoord2f(u1, v0); glVertex2f(x1, y0);
        glTexCoord2f(u1, v1); glVertex2f(x1, y1);
        glTexCoord2f(u0, v1); glVertex2f(x0, y1);
    glEnd();
}

void Renderer::drawTextureScaled(const Texture& tex, const Rect& dest) {
    drawTextureScaled(tex, dest, Rect(0, 0, tex.width(), tex.height()));
}

void Renderer::drawTextureScaled(const Texture& tex, const Rect& dest, const Rect& src) {
    if (!tex.valid()) return;
    
    glBindTexture(GL_TEXTURE_2D, tex.glId());
    glColor4f(1, 1, 1, 1);
    
    float u0 = static_cast<float>(src.x) / tex.width();
    float v0 = static_cast<float>(src.y) / tex.height();
    float u1 = static_cast<float>(src.x + src.w) / tex.width();
    float v1 = static_cast<float>(src.y + src.h) / tex.height();
    
    float x0 = static_cast<float>(dest.x);
    float y0 = static_cast<float>(dest.y);
    float x1 = x0 + dest.w;
    float y1 = y0 + dest.h;
    
    glBegin(GL_QUADS);
        glTexCoord2f(u0, v0); glVertex2f(x0, y0);
        glTexCoord2f(u1, v0); glVertex2f(x1, y0);
        glTexCoord2f(u1, v1); glVertex2f(x1, y1);
        glTexCoord2f(u0, v1); glVertex2f(x0, y1);
    glEnd();
}

void Renderer::drawRect(const Rect& rect, Color c) {
    glDisable(GL_TEXTURE_2D);
    glColor4ub(c.r, c.g, c.b, c.a);
    
    glBegin(GL_QUADS);
        glVertex2f(static_cast<float>(rect.x), static_cast<float>(rect.y));
        glVertex2f(static_cast<float>(rect.x + rect.w), static_cast<float>(rect.y));
        glVertex2f(static_cast<float>(rect.x + rect.w), static_cast<float>(rect.y + rect.h));
        glVertex2f(static_cast<float>(rect.x), static_cast<float>(rect.y + rect.h));
    glEnd();
    
    glEnable(GL_TEXTURE_2D);
}

void Renderer::drawRectOutline(const Rect& rect, Color c) {
    glDisable(GL_TEXTURE_2D);
    glColor4ub(c.r, c.g, c.b, c.a);
    
    float x0 = static_cast<float>(rect.x);
    float y0 = static_cast<float>(rect.y);
    float x1 = x0 + rect.w;
    float y1 = y0 + rect.h;
    
    glBegin(GL_LINE_LOOP);
        glVertex2f(x0, y0);
        glVertex2f(x1, y0);
        glVertex2f(x1, y1);
        glVertex2f(x0, y1);
    glEnd();
    
    glEnable(GL_TEXTURE_2D);
}

/*==============================================================================
 * Palette Implementation
 *============================================================================*/

Palette::Palette() {
    // Default to grayscale
    for (int i = 0; i < 256; i++) {
        m_colors[i] = Color(i, i, i, 255);
    }
}

void Palette::setColor(int index, Color c) {
    if (index >= 0 && index < 256) {
        m_colors[index] = c;
    }
}

Color Palette::getColor(int index) const {
    if (index >= 0 && index < 256) {
        return m_colors[index];
    }
    return Color();
}

void Palette::loadFromRGBQUAD(const uint8_t* data, int count) {
    // RGBQUAD is BGRA order in memory
    for (int i = 0; i < count && i < 256; i++) {
        m_colors[i].b = data[i * 4 + 0];
        m_colors[i].g = data[i * 4 + 1];
        m_colors[i].r = data[i * 4 + 2];
        m_colors[i].a = 255;  // RGBQUAD reserved byte, we use full alpha
    }
}

void Palette::applyTo(const uint8_t* indexed, int width, int height, Bitmap& out) const {
    if (!out.valid() || out.width() != width || out.height() != height) {
        out.create(width, height);
    }
    
    uint8_t* dest = out.pixels();
    for (int i = 0; i < width * height; i++) {
        Color c = m_colors[indexed[i]];
        dest[i * 4 + 0] = c.r;
        dest[i * 4 + 1] = c.g;
        dest[i * 4 + 2] = c.b;
        dest[i * 4 + 3] = c.a;
    }
}

Palette Palette::createDefault() {
    // Try to load from default_palette.bin in the same directory as the executable
    Palette pal = loadFromFile("default_palette.bin");
    if (pal.m_colors[1].r != 0 || pal.m_colors[1].g != 0 || pal.m_colors[1].b != 0) {
        return pal;  // Loaded successfully
    }
    
    // Fallback: simple grayscale palette
    for (int i = 0; i < 256; i++) {
        uint8_t v = (uint8_t)i;
        pal.m_colors[i] = Color(v, v, v, (i == 0) ? 0 : 255);
    }
    return pal;
}

Palette Palette::loadFromFile(const char* filename) {
    Palette pal;
    
    // Initialize to zero (black transparent)
    for (int i = 0; i < 256; i++) {
        pal.m_colors[i] = Color(0, 0, 0, 0);
    }
    
    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("Warning: Could not load palette from '%s'\n", filename);
        return pal;
    }
    
    // Get file size to determine if it's 16 or 256 color palette
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    int numColors = (int)(size / 4);  // 4 bytes per color (BGRA)
    if (numColors > 256) numColors = 256;
    
    // Read BGRA data and convert to RGBA
    uint8_t bgra[4];
    for (int i = 0; i < numColors; i++) {
        if (fread(bgra, 1, 4, f) != 4) break;
        uint8_t b = bgra[0];
        uint8_t g = bgra[1];
        uint8_t r = bgra[2];
        // Alpha: index 0 is transparent, all others opaque
        uint8_t a = (i == 0) ? 0 : 255;
        pal.m_colors[i] = Color(r, g, b, a);
    }
    
    fclose(f);
    printf("Loaded %d-color palette from '%s'\n", numColors, filename);
    return pal;
}

} // namespace gfx2d
