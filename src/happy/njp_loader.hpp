/*
 * njp_loader.hpp - NJP (NJudgeUniPat) sprite file loader
 * 
 * NJP files contain sprite patterns used by ShadowFlare.
 * Each file has multiple patterns, each pattern has pixel data.
 * Data is RCLIB-L (LZSS) compressed.
 * 
 * NJP files also contain embedded palettes (BGRA format) at the end of the file,
 * after the extended header and metadata sections.
 */

#ifndef NJP_LOADER_HPP
#define NJP_LOADER_HPP

#include "h2d.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace h2d {

/*==============================================================================
 * Pattern - A single sprite frame/tile from an NJP file
 *============================================================================*/

struct Pattern {
    int width = 0;
    int height = 0;
    int bpp = 0;           // Bits per pixel (1, 4, 8, 16, 24)
    int flags = 0;
    Bitmap bitmap;         // RGBA bitmap (converted from indexed if needed)
    
    // For indexed images, we keep the original indexed data too
    std::vector<uint8_t> indexedData;
};

/*==============================================================================
 * SpriteSheet - Collection of patterns from an NJP file (like RKC_UPDIB_UPD)
 *============================================================================*/

class SpriteSheet {
public:
    SpriteSheet();
    ~SpriteSheet();
    
    // Load from file
    bool loadFromFile(const std::string& path);
    
    // Load from memory (already read file contents)
    bool loadFromMemory(const uint8_t* data, size_t size);
    
    // Apply a palette to all indexed patterns
    void applyPalette(const Palette& palette);
    
    // Check if this NJP has an embedded palette
    bool hasEmbeddedPalette() const { return !m_embeddedPalettes.empty(); }
    
    // Get the embedded palette count
    int embeddedPaletteCount() const { return static_cast<int>(m_embeddedPalettes.size()); }
    
    // Get an embedded palette (0 = first/default)
    const Palette* getEmbeddedPalette(int index = 0) const;
    
    // Apply the embedded palette to all indexed patterns (convenience)
    // Returns true if an embedded palette was found and applied
    bool applyEmbeddedPalette(int paletteIndex = 0);
    
    // Access patterns
    int patternCount() const { return static_cast<int>(m_patterns.size()); }
    Pattern* getPattern(int index);
    const Pattern* getPattern(int index) const;
    
    // Get filename (for debugging)
    const std::string& filename() const { return m_filename; }
    
    // Check if loaded successfully
    bool valid() const { return !m_patterns.empty(); }
    
    // Get the BPP of the first pattern (for palette size determination)
    int primaryBpp() const { return m_patterns.empty() ? 8 : m_patterns[0].bpp; }
    
private:
    std::string m_filename;
    std::vector<Pattern> m_patterns;
    std::vector<Palette> m_embeddedPalettes;  // Embedded palettes from NJP
    
    // Internal: decompress RCLIB-L data
    static bool decompressRCLIB(const uint8_t* src, size_t srcSize, 
                                std::vector<uint8_t>& dest);
    
    // Internal: parse decompressed NJP data
    bool parseNJP(const uint8_t* data, size_t size);
    
    // Internal: extract embedded palettes from NJP data
    bool extractPalettes(const uint8_t* data, size_t size, int bpp, int patternCount);
    
    // Internal: convert indexed pixels to RGBA bitmap
    static void convertIndexedToRGBA(const uint8_t* indexed, int width, int height, 
                                      int bpp, Bitmap& out);
};

/*==============================================================================
 * TextureAtlas - GPU-side sprite sheet with sub-texture regions
 *============================================================================*/

class TextureAtlas {
public:
    TextureAtlas();
    ~TextureAtlas();
    
    // Create from a sprite sheet (uploads all patterns to one texture)
    bool createFromSpriteSheet(const SpriteSheet& sheet);
    
    // Get the texture
    const Texture& texture() const { return m_texture; }
    
    // Get source rect for a pattern
    Rect getPatternRect(int index) const;
    
    // Draw a pattern
    void drawPattern(Renderer& renderer, int patternIndex, int x, int y) const;
    
    int patternCount() const { return static_cast<int>(m_rects.size()); }
    
private:
    Texture m_texture;
    std::vector<Rect> m_rects;  // Source rects for each pattern in the atlas
};

} // namespace h2d

#endif // NJP_LOADER_HPP
