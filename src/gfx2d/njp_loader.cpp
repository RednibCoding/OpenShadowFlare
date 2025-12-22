/*
 * njp_loader.cpp - NJP sprite file loader implementation
 */

#include "njp_loader.hpp"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cstdio>

namespace gfx2d {

// Helper to read uint32_t little-endian
static inline uint32_t readU32LE(const uint8_t* p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

/*==============================================================================
 * RCLIB-L Decompression (LZSS variant)
 * 
 * Header: "RCLIB-L" (7 bytes) + terminator (1 byte) + decompSize (4 bytes) + reserved (4 bytes)
 * Algorithm: 4KB sliding window, initial position 0xFEE, zero-filled
 * Flags: MSB-first, bit=1 means match reference, bit=0 means literal
 *============================================================================*/

bool SpriteSheet::decompressRCLIB(const uint8_t* src, size_t srcSize, 
                                   std::vector<uint8_t>& dest) {
    // Minimum header size
    if (srcSize < 16) return false;
    
    // Check magic "RCLIB-L" (7 bytes only, byte 8 varies)
    if (memcmp(src, "RCLIB-L", 7) != 0) {
        return false;
    }
    
    // Get decompressed size (little-endian at offset 8)
    uint32_t decompSize = src[8] | (src[9] << 8) | (src[10] << 16) | (src[11] << 24);
    
    dest.resize(decompSize);
    
    // Initialize sliding window
    uint8_t window[4096];
    memset(window, 0, sizeof(window));
    
    size_t srcPos = 16;  // Skip header
    size_t destPos = 0;
    int winPos = 0xFEE;
    
    while (srcPos < srcSize && destPos < decompSize) {
        if (srcPos >= srcSize) break;
        uint8_t flags = src[srcPos++];
        
        // Process 8 bits MSB first
        for (uint8_t mask = 0x80; mask != 0 && destPos < decompSize; mask >>= 1) {
            if (flags & mask) {
                // Match reference
                if (srcPos + 2 > srcSize) break;
                uint8_t b1 = src[srcPos++];
                uint8_t b2 = src[srcPos++];
                
                int offset = b1 | ((b2 & 0xF0) << 4);
                int length = (b2 & 0x0F) + 3;
                
                for (int i = 0; i < length && destPos < decompSize; i++) {
                    uint8_t c = window[(offset + i) & 0xFFF];
                    dest[destPos++] = c;
                    window[winPos] = c;
                    winPos = (winPos + 1) & 0xFFF;
                }
            } else {
                // Literal byte
                if (srcPos >= srcSize) break;
                uint8_t c = src[srcPos++];
                dest[destPos++] = c;
                window[winPos] = c;
                winPos = (winPos + 1) & 0xFFF;
            }
        }
    }
    
    return destPos == decompSize;
}

/*==============================================================================
 * SpriteSheet Implementation
 *============================================================================*/

SpriteSheet::SpriteSheet() = default;
SpriteSheet::~SpriteSheet() = default;

bool SpriteSheet::loadFromFile(const std::string& path) {
    m_filename = path;
    m_patterns.clear();
    
    // Read file
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        fprintf(stderr, "SpriteSheet: Failed to open '%s'\n", path.c_str());
        return false;
    }
    
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0);
    
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    
    return loadFromMemory(data.data(), size);
}

bool SpriteSheet::loadFromMemory(const uint8_t* data, size_t size) {
    m_patterns.clear();
    
    if (!data || size < 16) return false;
    
    // Check if data is compressed (starts with RCLIB-L)
    if (memcmp(data, "RCLIB-L", 7) == 0) {
        std::vector<uint8_t> decompressed;
        if (!decompressRCLIB(data, size, decompressed)) {
            fprintf(stderr, "SpriteSheet: Failed to decompress RCLIB-L data\n");
            return false;
        }
        return parseNJP(decompressed.data(), decompressed.size());
    }
    
    // Not compressed, parse directly
    return parseNJP(data, size);
}

bool SpriteSheet::parseNJP(const uint8_t* data, size_t size) {
    // Header: "NJudgeUniPat003" (16 bytes) + patternCount (4 bytes)
    if (size < 20) return false;
    
    // Check magic
    if (memcmp(data, "NJudgeUniPat", 12) != 0) {
        fprintf(stderr, "SpriteSheet: Invalid NJP magic\n");
        return false;
    }
    
    // Get pattern count (little-endian at offset 16)
    uint32_t patternCount = data[16] | (data[17] << 8) | (data[18] << 16) | (data[19] << 24);
    
    // Parse each pattern
    size_t pos = 20;
    m_patterns.reserve(patternCount);
    
    for (uint32_t i = 0; i < patternCount; i++) {
        // Per-pattern header: 20 bytes
        // 0x00: uint32_t unknown (possibly offset or ID)
        // 0x04: uint32_t bpp
        // 0x08: uint32_t width
        // 0x0C: uint32_t height
        // 0x10: uint32_t flags
        // 0x14: RCLIB-L compressed pixel data (size determined by scanning)
        
        if (pos + 20 > size) {
            fprintf(stderr, "SpriteSheet: Truncated pattern header at pattern %u\n", i);
            break;
        }
        
        // Skip the unknown field (we don't use it)
        // uint32_t unknown = data[pos] | ... 
        
        Pattern pattern;
        pattern.bpp = data[pos + 4] | (data[pos + 5] << 8) | 
                      (data[pos + 6] << 16) | (data[pos + 7] << 24);
        pattern.width = data[pos + 8] | (data[pos + 9] << 8) | 
                        (data[pos + 10] << 16) | (data[pos + 11] << 24);
        pattern.height = data[pos + 12] | (data[pos + 13] << 8) | 
                         (data[pos + 14] << 16) | (data[pos + 15] << 24);
        pattern.flags = data[pos + 16] | (data[pos + 17] << 8) | 
                        (data[pos + 18] << 16) | (data[pos + 19] << 24);
        
        pos += 20;
        
        // Debug output for the first few patterns
        if (i < 3) {
            printf("  Pattern %u: bpp=%u, %ux%u, flags=0x%x\n",
                   i, pattern.bpp, pattern.width, pattern.height, pattern.flags);
        }
        
        // Skip empty patterns
        if (pattern.width == 0 || pattern.height == 0) {
            m_patterns.push_back(std::move(pattern));
            continue;
        }
        
        // The pixel data should be RCLIB-L compressed
        if (pos + 16 > size) {
            fprintf(stderr, "SpriteSheet: No room for RCLIB-L header at pattern %u\n", i);
            break;
        }
        
        if (memcmp(data + pos, "RCLIB-L", 7) != 0) {
            fprintf(stderr, "SpriteSheet: Expected RCLIB-L at pattern %u, offset 0x%zx\n", i, pos);
            break;
        }
        
        // Decompress the RCLIB-L data
        // We pass the remaining buffer size; decompression will stop when done
        std::vector<uint8_t> pixelData;
        if (!decompressRCLIB(data + pos, size - pos, pixelData)) {
            fprintf(stderr, "SpriteSheet: Failed to decompress pattern %u pixels\n", i);
            m_patterns.push_back(std::move(pattern));
            // Try to find next pattern by scanning for next header
            // This is a recovery mechanism
            break;
        }
        
        // Store indexed data
        pattern.indexedData = std::move(pixelData);
        
        // Convert to RGBA bitmap
        convertIndexedToRGBA(pattern.indexedData.data(), 
                             pattern.width, pattern.height,
                             pattern.bpp, pattern.bitmap);
        
        // Find the next pattern by scanning for next RCLIB-L header preceded by
        // what looks like a valid pattern header (bpp 4/8/24, reasonable width/height)
        // Or we can scan for the end of the compressed data
        // For now, scan forward for next pattern header
        size_t nextPos = pos + 16;  // Skip RCLIB-L header minimum
        while (nextPos + 20 < size) {
            // Check if this could be a pattern header followed by RCLIB-L
            if (memcmp(data + nextPos + 20, "RCLIB-L", 7) == 0) {
                // Validate the header looks reasonable
                uint32_t nextBpp = data[nextPos + 4] | (data[nextPos + 5] << 8);
                uint32_t nextW = data[nextPos + 8] | (data[nextPos + 9] << 8);
                uint32_t nextH = data[nextPos + 12] | (data[nextPos + 13] << 8);
                if ((nextBpp == 4 || nextBpp == 8 || nextBpp == 24 || nextBpp == 32) &&
                    nextW > 0 && nextW < 4096 && nextH > 0 && nextH < 4096) {
                    break;
                }
            }
            nextPos++;
        }
        
        // If we're at the last pattern or didn't find next, go to end
        if (i == patternCount - 1 || nextPos + 20 >= size) {
            pos = size;  // End
        } else {
            pos = nextPos;
        }
        
        m_patterns.push_back(std::move(pattern));
    }
    
    // Try to extract embedded palettes
    // We need to find the extended header after all patterns
    // For now, get BPP from first pattern
    int primaryBpp = m_patterns.empty() ? 8 : m_patterns[0].bpp;
    extractPalettes(data, size, primaryBpp, patternCount);
    
    // If we found embedded palettes, apply the first one automatically
    if (!m_embeddedPalettes.empty()) {
        applyEmbeddedPalette(0);
    }
    
    return !m_patterns.empty();
}

/*==============================================================================
 * Palette Extraction
 * 
 * NJP files have an extended header after all pattern data, followed by 
 * metadata and then palette data in BGRA format.
 * 
 * Structure after patterns:
 *   Extended Header (12 bytes): pattern_count, pattern_count, palette_count
 *   Extended Metadata (32 bytes per pattern)
 *   Optional nested section counts
 *   Palette data (at end of file)
 *============================================================================*/

bool SpriteSheet::extractPalettes(const uint8_t* data, size_t size, int bpp, int patternCount) {
    m_embeddedPalettes.clear();
    
    // Only 4-bit and 8-bit indexed images have palettes
    if (bpp != 4 && bpp != 8) {
        return false;
    }
    
    size_t paletteSize = (bpp == 4) ? 64 : 1024;  // 16 or 256 colors * 4 bytes
    
    // Method: Scan backward from end of file to find palette data
    // Palette entries are BGRA (4 bytes each), color 0 is often black/transparent
    // We look for the palette region at the end of the file
    
    // Minimum file size for palette
    if (size < 20 + 16 + paletteSize) {  // header + rclib + palette
        return false;
    }
    
    // Strategy: Look for extended header pattern (three uint32s where first two match patternCount)
    // Then calculate where palettes should be based on metadata
    
    // Scan for extended header: two consecutive values matching pattern count
    // It appears after all RCLIB blocks
    size_t extHeaderPos = 0;
    size_t minHeaderPos = 20 + static_cast<size_t>(patternCount) * 20;
    for (size_t pos = 20; pos + 12 < size; pos++) {
        uint32_t v1 = readU32LE(data + pos);
        uint32_t v2 = readU32LE(data + pos + 4);
        
        // Check if both match pattern count and we're past the RCLIB data
        if (v1 == static_cast<uint32_t>(patternCount) && 
            v2 == static_cast<uint32_t>(patternCount)) {
            // Make sure we're not in the middle of RCLIB data
            // Check that we're at least after all pattern headers
            if (pos > minHeaderPos) {
                extHeaderPos = pos;
                break;
            }
        }
    }
    
    if (extHeaderPos == 0) {
        // Couldn't find extended header, try reading palette from end of file
        // This is a fallback for files where we can't parse the header
        if (size >= paletteSize) {
            size_t palOffset = size - paletteSize;
            
            // Sanity check: first color is often 0 (transparent/black)
            // and palette should have varied colors
            Palette pal;
            for (int i = 0; i < (bpp == 4 ? 16 : 256); i++) {
                const uint8_t* c = data + palOffset + i * 4;
                // BGRA -> RGBA
                pal.setColor(i, Color(c[2], c[1], c[0], 255));
            }
            // Color 0 is transparent
            pal.setColor(0, Color(0, 0, 0, 0));
            
            m_embeddedPalettes.push_back(std::move(pal));
            return true;
        }
        return false;
    }
    
    // Read palette count from extended header
    uint32_t paletteCount = readU32LE(data + extHeaderPos + 8);
    
    // Sanity check - palette count should be reasonable
    if (paletteCount == 0 || paletteCount > 16) {
        // Either no palettes or something's wrong, try the fallback
        paletteCount = 1;
    }
    
    // Calculate expected palette region size
    size_t totalPaletteBytes = paletteCount * paletteSize;
    
    // Palette region is at end of file
    if (size < totalPaletteBytes) {
        return false;
    }
    
    size_t palOffset = size - totalPaletteBytes;
    
    // Parse each palette
    for (uint32_t p = 0; p < paletteCount; p++) {
        Palette pal;
        size_t thisOffset = palOffset + p * paletteSize;
        int colorCount = (bpp == 4) ? 16 : 256;
        
        for (int i = 0; i < colorCount; i++) {
            const uint8_t* c = data + thisOffset + i * 4;
            // BGRA -> RGBA
            pal.setColor(i, Color(c[2], c[1], c[0], 255));
        }
        // Color 0 is typically transparent
        pal.setColor(0, Color(0, 0, 0, 0));
        
        m_embeddedPalettes.push_back(std::move(pal));
    }
    
    return !m_embeddedPalettes.empty();
}

const Palette* SpriteSheet::getEmbeddedPalette(int index) const {
    if (index >= 0 && index < static_cast<int>(m_embeddedPalettes.size())) {
        return &m_embeddedPalettes[index];
    }
    return nullptr;
}

bool SpriteSheet::applyEmbeddedPalette(int paletteIndex) {
    const Palette* pal = getEmbeddedPalette(paletteIndex);
    if (pal) {
        applyPalette(*pal);
        return true;
    }
    return false;
}

void SpriteSheet::convertIndexedToRGBA(const uint8_t* indexed, int width, int height,
                                        int bpp, Bitmap& out) {
    out.create(width, height);
    
    // Calculate stride: bytes per row, aligned to 4 bytes
    // For sub-byte bpp, we need to round up the bits to bytes first
    int bitsPerRow = width * bpp;
    int bytesPerRow = (bitsPerRow + 7) / 8;  // Round up to bytes
    int stride = (bytesPerRow + 3) & ~3;     // Align to 4 bytes
    
    for (int y = 0; y < height; y++) {
        // NJP data is stored bottom-up (like Windows DIB)
        int srcY = height - 1 - y;
        
        for (int x = 0; x < width; x++) {
            Color c;
            
            switch (bpp) {
                case 8: {
                    // 8-bit indexed - use grayscale for now (palette applied later)
                    uint8_t idx = indexed[srcY * stride + x];
                    // Index 0 is typically transparent in ShadowFlare sprites
                    if (idx == 0) {
                        c = Color(0, 0, 0, 0);  // Transparent
                    } else {
                        // Grayscale - real palette would be loaded from game data
                        c = Color(idx, idx, idx, 255);
                    }
                    break;
                }
                case 16: {
                    // 16-bit RGB565
                    int offset = srcY * stride + x * 2;
                    uint16_t pixel = indexed[offset] | (indexed[offset + 1] << 8);
                    // Check for magenta color key (0xF81F in RGB565 = R=31, G=0, B=31)
                    if (pixel == 0xF81F) {
                        c = Color(0, 0, 0, 0);  // Transparent
                    } else {
                        uint8_t r = ((pixel >> 11) & 0x1F) * 255 / 31;
                        uint8_t g = ((pixel >> 5) & 0x3F) * 255 / 63;
                        uint8_t b = (pixel & 0x1F) * 255 / 31;
                        c = Color(r, g, b, 255);
                    }
                    break;
                }
                case 24: {
                    // 24-bit BGR
                    int offset = srcY * stride + x * 3;
                    uint8_t b_val = indexed[offset];
                    uint8_t g_val = indexed[offset + 1];
                    uint8_t r_val = indexed[offset + 2];
                    // Magenta color key
                    if (r_val == 255 && g_val == 0 && b_val == 255) {
                        c = Color(0, 0, 0, 0);
                    } else {
                        c = Color(r_val, g_val, b_val, 255);
                    }
                    break;
                }
                case 32: {
                    // 32-bit BGRA
                    int offset = srcY * stride + x * 4;
                    uint8_t b_val = indexed[offset];
                    uint8_t g_val = indexed[offset + 1];
                    uint8_t r_val = indexed[offset + 2];
                    uint8_t a_val = indexed[offset + 3];
                    c = Color(r_val, g_val, b_val, a_val);
                    break;
                }
                case 4: {
                    // 4-bit indexed (2 pixels per byte)
                    int offset = srcY * stride + x / 2;
                    uint8_t byte = indexed[offset];
                    uint8_t idx = (x & 1) ? (byte & 0x0F) : (byte >> 4);
                    if (idx == 0) {
                        c = Color(0, 0, 0, 0);  // Transparent
                    } else {
                        // Grayscale - real palette would be loaded from game data
                        c = Color(idx * 17, idx * 17, idx * 17, 255);
                    }
                    break;
                }
                case 1: {
                    // 1-bit (8 pixels per byte)
                    int offset = srcY * stride + x / 8;
                    uint8_t byte = indexed[offset];
                    uint8_t bit = (byte >> (7 - (x & 7))) & 1;
                    c = bit ? Color(255, 255, 255, 255) : Color(0, 0, 0, 0);
                    break;
                }
                default:
                    c = Color(255, 0, 255, 255);  // Error: magenta
                    break;
            }
            
            out.setPixel(x, y, c);
        }
    }
}

void SpriteSheet::applyPalette(const Palette& palette) {
    for (auto& pattern : m_patterns) {
        if ((pattern.bpp == 8 || pattern.bpp == 4) && !pattern.indexedData.empty()) {
            // Calculate stride (same as in convertIndexedToRGBA)
            int bitsPerRow = pattern.width * pattern.bpp;
            int bytesPerRow = (bitsPerRow + 7) / 8;
            int stride = (bytesPerRow + 3) & ~3;
            
            pattern.bitmap.create(pattern.width, pattern.height);
            
            for (int y = 0; y < pattern.height; y++) {
                // NJP data is stored bottom-up (like Windows DIB)
                int srcY = pattern.height - 1 - y;
                
                for (int x = 0; x < pattern.width; x++) {
                    uint8_t idx;
                    if (pattern.bpp == 8) {
                        idx = pattern.indexedData[srcY * stride + x];
                    } else {
                        // 4-bit: 2 pixels per byte
                        int offset = srcY * stride + x / 2;
                        uint8_t byte = pattern.indexedData[offset];
                        idx = (x & 1) ? (byte & 0x0F) : (byte >> 4);
                    }
                    
                    Color c;
                    if (idx == 0) {
                        c = Color(0, 0, 0, 0);  // Transparent
                    } else {
                        c = palette.getColor(idx);
                    }
                    pattern.bitmap.setPixel(x, y, c);
                }
            }
        }
    }
}

Pattern* SpriteSheet::getPattern(int index) {
    if (index >= 0 && index < static_cast<int>(m_patterns.size())) {
        return &m_patterns[index];
    }
    return nullptr;
}

const Pattern* SpriteSheet::getPattern(int index) const {
    if (index >= 0 && index < static_cast<int>(m_patterns.size())) {
        return &m_patterns[index];
    }
    return nullptr;
}

/*==============================================================================
 * TextureAtlas Implementation
 *============================================================================*/

TextureAtlas::TextureAtlas() = default;
TextureAtlas::~TextureAtlas() = default;

bool TextureAtlas::createFromSpriteSheet(const SpriteSheet& sheet) {
    m_rects.clear();
    
    int count = sheet.patternCount();
    if (count == 0) return false;
    
    // Calculate atlas dimensions
    // Simple row-based packing for now
    int totalWidth = 0;
    int maxHeight = 0;
    
    for (int i = 0; i < count; i++) {
        const Pattern* p = sheet.getPattern(i);
        if (p && p->bitmap.valid()) {
            totalWidth += p->width;
            maxHeight = std::max(maxHeight, p->height);
        }
    }
    
    // If patterns are too wide, we should use multiple rows
    // For simplicity, use a power-of-2 texture with simple row packing
    int atlasWidth = 1;
    while (atlasWidth < totalWidth && atlasWidth < 4096) {
        atlasWidth *= 2;
    }
    
    // Calculate height needed with row wrapping
    int atlasHeight = maxHeight;
    int currentX = 0;
    int currentY = 0;
    int rowHeight = 0;
    
    std::vector<Rect> tempRects;
    for (int i = 0; i < count; i++) {
        const Pattern* p = sheet.getPattern(i);
        if (!p || !p->bitmap.valid()) {
            tempRects.push_back(Rect(0, 0, 0, 0));
            continue;
        }
        
        // Wrap to next row if needed
        if (currentX + p->width > atlasWidth) {
            currentX = 0;
            currentY += rowHeight;
            rowHeight = 0;
        }
        
        tempRects.push_back(Rect(currentX, currentY, p->width, p->height));
        currentX += p->width;
        rowHeight = std::max(rowHeight, p->height);
    }
    atlasHeight = currentY + rowHeight;
    
    // Round up to power of 2
    int h = 1;
    while (h < atlasHeight && h < 4096) {
        h *= 2;
    }
    atlasHeight = h;
    
    // Create atlas bitmap
    Bitmap atlas(atlasWidth, atlasHeight);
    atlas.clear(Color(0, 0, 0, 0));  // Transparent
    
    // Blit all patterns into atlas
    m_rects = std::move(tempRects);
    for (int i = 0; i < count; i++) {
        const Pattern* p = sheet.getPattern(i);
        if (p && p->bitmap.valid() && m_rects[i].w > 0) {
            atlas.blit(p->bitmap, m_rects[i].x, m_rects[i].y);
        }
    }
    
    // Upload to GPU
    return m_texture.createFromBitmap(atlas);
}

Rect TextureAtlas::getPatternRect(int index) const {
    if (index >= 0 && index < static_cast<int>(m_rects.size())) {
        return m_rects[index];
    }
    return Rect(0, 0, 0, 0);
}

void TextureAtlas::drawPattern(Renderer& renderer, int patternIndex, int x, int y) const {
    if (!m_texture.valid()) return;
    
    Rect src = getPatternRect(patternIndex);
    if (src.w > 0 && src.h > 0) {
        renderer.drawTexture(m_texture, x, y, src);
    }
}

} // namespace gfx2d
