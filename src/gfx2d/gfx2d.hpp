/*
 * gfx2d.hpp - Simple 2D graphics layer using OpenGL 1.2 fixed-function
 * 
 * Designed to replace DirectDraw-based RKC_DIB/RKC_UPDIB/RKC_DBFCONTROL
 * with a cross-platform implementation.
 * 
 * Concepts:
 *   - Bitmap: CPU-side pixel buffer (like RKC_DIB)
 *   - Texture: GPU-side texture created from Bitmap
 *   - Renderer: Handles drawing to screen (like RKC_DBFCONTROL)
 */

#ifndef GFX2D_HPP
#define GFX2D_HPP

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

namespace gfx2d {

/*==============================================================================
 * Color types
 *============================================================================*/

// RGBA color, 8 bits per channel (matches RKC_DIB's RGBQUAD layout)
struct Color {
    uint8_t r, g, b, a;
    
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
    
    // Convert to 32-bit packed RGBA
    uint32_t toRGBA() const { return (r) | (g << 8) | (b << 16) | (a << 24); }
    
    static Color fromRGBA(uint32_t rgba) {
        return Color(rgba & 0xFF, (rgba >> 8) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 24) & 0xFF);
    }
};

/*==============================================================================
 * Rect
 *============================================================================*/

struct Rect {
    int x, y, w, h;
    
    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}
    
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
    
    bool intersects(const Rect& other) const {
        return !(x + w <= other.x || other.x + other.w <= x ||
                 y + h <= other.y || other.y + other.h <= y);
    }
};

/*==============================================================================
 * Bitmap - CPU-side pixel buffer (similar to RKC_DIB)
 *============================================================================*/

class Bitmap {
public:
    Bitmap();
    Bitmap(int width, int height);
    ~Bitmap();
    
    // Non-copyable, movable
    Bitmap(const Bitmap&) = delete;
    Bitmap& operator=(const Bitmap&) = delete;
    Bitmap(Bitmap&& other) noexcept;
    Bitmap& operator=(Bitmap&& other) noexcept;
    
    // Create/release
    bool create(int width, int height);
    void release();
    
    // Properties
    int width() const { return m_width; }
    int height() const { return m_height; }
    int stride() const { return m_width * 4; }  // RGBA = 4 bytes per pixel
    bool valid() const { return m_pixels != nullptr; }
    
    // Pixel access
    uint8_t* pixels() { return m_pixels; }
    const uint8_t* pixels() const { return m_pixels; }
    
    Color getPixel(int x, int y) const;
    void setPixel(int x, int y, Color c);
    
    // Fill operations
    void clear(Color c = Color(0, 0, 0, 255));
    void fillRect(const Rect& rect, Color c);
    
    // Blit operations (like RKC_DIB::TransferToDIB)
    void blit(const Bitmap& src, int destX, int destY);
    void blit(const Bitmap& src, int destX, int destY, const Rect& srcRect);
    void blitKeyed(const Bitmap& src, int destX, int destY, Color colorKey);
    void blitAlpha(const Bitmap& src, int destX, int destY);
    
private:
    int m_width = 0;
    int m_height = 0;
    uint8_t* m_pixels = nullptr;
};

/*==============================================================================
 * Texture - GPU-side texture for rendering
 *============================================================================*/

class Texture {
public:
    Texture();
    ~Texture();
    
    // Non-copyable, movable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;
    
    // Create from bitmap (uploads to GPU)
    bool createFromBitmap(const Bitmap& bitmap);
    
    // Update from bitmap (re-uploads, must be same size)
    bool updateFromBitmap(const Bitmap& bitmap);
    
    void release();
    
    // Properties
    int width() const { return m_width; }
    int height() const { return m_height; }
    bool valid() const { return m_texId != 0; }
    unsigned int glId() const { return m_texId; }
    
private:
    unsigned int m_texId = 0;
    int m_width = 0;
    int m_height = 0;
};

/*==============================================================================
 * Renderer - Draws to screen using OpenGL 1.2 (like RKC_DBFCONTROL)
 *============================================================================*/

class Renderer {
public:
    Renderer();
    ~Renderer();
    
    // Initialize for a given screen size
    bool init(int screenWidth, int screenHeight);
    void shutdown();
    
    // Frame begin/end
    void beginFrame();
    void endFrame();
    
    // Set virtual screen size (for scaling)
    void setVirtualSize(int width, int height);
    
    // Clear screen
    void clear(Color c = Color(0, 0, 0, 255));
    
    // Draw texture (or portion of it)
    void drawTexture(const Texture& tex, int x, int y);
    void drawTexture(const Texture& tex, int x, int y, const Rect& srcRect);
    void drawTextureScaled(const Texture& tex, const Rect& dest);
    void drawTextureScaled(const Texture& tex, const Rect& dest, const Rect& src);
    
    // Draw colored rectangle (for debug/UI)
    void drawRect(const Rect& rect, Color c);
    void drawRectOutline(const Rect& rect, Color c);
    
    // Screen properties
    int screenWidth() const { return m_screenWidth; }
    int screenHeight() const { return m_screenHeight; }
    
private:
    int m_screenWidth = 0;
    int m_screenHeight = 0;
    int m_virtualWidth = 0;
    int m_virtualHeight = 0;
    bool m_initialized = false;
    
    void setupOrtho();
};

/*==============================================================================
 * Palette support (for 8-bit indexed images like original ShadowFlare)
 *============================================================================*/

class Palette {
public:
    Palette();
    
    // Set/get colors (256 entries)
    void setColor(int index, Color c);
    Color getColor(int index) const;
    
    // Load from RGBQUAD array (like original game palettes)
    void loadFromRGBQUAD(const uint8_t* data, int count = 256);
    
    // Convert indexed bitmap to RGBA
    void applyTo(const uint8_t* indexed, int width, int height, Bitmap& out) const;
    
    // Create a default palette suitable for ShadowFlare-style sprites
    static Palette createDefault();
    
    // Load palette from a binary file (1024 bytes = 256 BGRA colors)
    static Palette loadFromFile(const char* filename);
    
private:
    Color m_colors[256];
};

} // namespace gfx2d

#endif // GFX2D_HPP
