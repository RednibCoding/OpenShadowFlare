/*
 * njp_viewer.cpp - Test loading and displaying NJP files from ShadowFlare
 * 
 * Build (Linux):
 *   g++ -std=c++17 -O2 njp_viewer.cpp njp_loader.cpp gfx2d.cpp -o njp_viewer -lX11 -lGL -ldl
 */

#define HWL_IMPLEMENTATION
#include "hwl.hpp"
#include "gfx2d.hpp"
#include "njp_loader.hpp"

#include <cstdio>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <chrono>
#include <thread>

// Find NJP files in a directory
std::vector<std::string> findNjpFiles(const std::string& dir, int maxFiles = 20) {
    std::vector<std::string> files;
    DIR* d = opendir(dir.c_str());
    if (!d) return files;
    
    struct dirent* entry;
    while ((entry = readdir(d)) != nullptr && files.size() < static_cast<size_t>(maxFiles)) {
        std::string name = entry->d_name;
        // Check for .njp extension (case insensitive)
        if (name.size() > 4) {
            std::string ext = name.substr(name.size() - 4);
            if (ext == ".njp" || ext == ".Njp" || ext == ".NJP") {
                files.push_back(dir + "/" + name);
            }
        }
    }
    closedir(d);
    return files;
}

int main(int argc, char* argv[]) {
    // Default to game's system patterns directory
    std::string searchDir = "../../tmp/ShadowFlare/System/Common/Pattern";
    std::string paletteFile = "";  // Empty = use createDefault()
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-p" || arg == "--palette") {
            if (i + 1 < argc) {
                paletteFile = argv[++i];
            }
        } else {
            searchDir = arg;
        }
    }
    
    printf("NJP Viewer - Loading sprites from: %s\n", searchDir.c_str());
    printf("Controls: LEFT/RIGHT to change file, UP/DOWN to change pattern, ESC to exit\n");
    if (!paletteFile.empty()) {
        printf("Using palette: %s\n", paletteFile.c_str());
    }
    printf("\n");
    
    // Find NJP files
    std::vector<std::string> njpFiles = findNjpFiles(searchDir);
    if (njpFiles.empty()) {
        // Try some other paths
        std::vector<std::string> tryPaths = {
            "../../tmp/ShadowFlare/System/Common/Pattern",
            "../../tmp/ShadowFlare/System/Game/Pattern",
            "../../tmp/ShadowFlare/Player/Male",
            "../../../tmp/ShadowFlare/System/Common/Pattern",
        };
        for (const auto& path : tryPaths) {
            njpFiles = findNjpFiles(path);
            if (!njpFiles.empty()) {
                searchDir = path;
                break;
            }
        }
    }
    
    if (njpFiles.empty()) {
        fprintf(stderr, "No NJP files found. Try: %s <directory>\n", argv[0]);
        return 1;
    }
    
    printf("Found %zu NJP files\n", njpFiles.size());
    
    // Create window
    auto window = hwl::HwlWindow::create("NJP Viewer", 800, 600);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        return 1;
    }
    
    // Initialize renderer
    gfx2d::Renderer renderer;
    renderer.init(window->width(), window->height());
    
    // Create a palette for 8-bit sprites
    gfx2d::Palette defaultPalette;
    if (!paletteFile.empty()) {
        defaultPalette = gfx2d::Palette::loadFromFile(paletteFile.c_str());
    } else {
        defaultPalette = gfx2d::Palette::createDefault();
    }
    
    // Load first sprite sheet
    int currentFile = 0;
    int currentPattern = 0;
    gfx2d::SpriteSheet sheet;
    gfx2d::TextureAtlas atlas;
    
    auto loadFile = [&](int index) {
        if (index < 0 || index >= static_cast<int>(njpFiles.size())) return;
        
        printf("Loading: %s\n", njpFiles[index].c_str());
        if (sheet.loadFromFile(njpFiles[index])) {
            printf("  Loaded %d patterns\n", sheet.patternCount());
            
            // Check for embedded palettes (the loader auto-applies them)
            if (sheet.hasEmbeddedPalette()) {
                printf("  Found %d embedded palette(s) - using first one\n", 
                       sheet.embeddedPaletteCount());
            } else if (!paletteFile.empty()) {
                // Apply the user-specified palette if no embedded palette
                printf("  No embedded palette, using external: %s\n", paletteFile.c_str());
                sheet.applyPalette(defaultPalette);
            } else {
                // Fall back to grayscale default
                printf("  No embedded palette, using grayscale default\n");
                sheet.applyPalette(defaultPalette);
            }
            
            // Print pattern info
            for (int i = 0; i < std::min(5, sheet.patternCount()); i++) {
                const auto* p = sheet.getPattern(i);
                if (p) {
                    printf("  Pattern %d: %dx%d, %d bpp\n", i, p->width, p->height, p->bpp);
                }
            }
            if (sheet.patternCount() > 5) {
                printf("  ... and %d more\n", sheet.patternCount() - 5);
            }
            
            // Create texture atlas
            if (atlas.createFromSpriteSheet(sheet)) {
                printf("  Atlas created successfully\n");;
            }
            
            currentPattern = 0;
        } else {
            printf("  Failed to load!\n");
        }
    };
    
    loadFile(0);
    
    bool needRedraw = true;
    
    while (!window->shouldClose()) {
        // Handle events
        while (auto event = window->pollEvent()) {
            if (event->type == hwl::EventType::KeyDown) {
                if (event->key == hwl::Key::Escape) {
                    window->setShouldClose(true);
                }
                else if (event->key == hwl::Key::Right) {
                    if (currentFile + 1 < static_cast<int>(njpFiles.size())) {
                        currentFile++;
                        loadFile(currentFile);
                        needRedraw = true;
                    }
                }
                else if (event->key == hwl::Key::Left) {
                    if (currentFile > 0) {
                        currentFile--;
                        loadFile(currentFile);
                        needRedraw = true;
                    }
                }
                else if (event->key == hwl::Key::Down) {
                    if (currentPattern + 1 < sheet.patternCount()) {
                        currentPattern++;
                        needRedraw = true;
                    }
                }
                else if (event->key == hwl::Key::Up) {
                    if (currentPattern > 0) {
                        currentPattern--;
                        needRedraw = true;
                    }
                }
            }
            if (event->type == hwl::EventType::Resize) {
                renderer.init(event->width, event->height);
                needRedraw = true;
            }
        }
        
        if (needRedraw) {
            renderer.beginFrame();
            renderer.clear(gfx2d::Color(40, 40, 50, 255));
            
            // Draw current pattern info
            int y = 10;
            
            // Draw the individual pattern (larger, for detail view)
            const gfx2d::Pattern* pattern = sheet.getPattern(currentPattern);
            if (pattern && pattern->bitmap.valid()) {
                // Create a temporary texture for this pattern
                static gfx2d::Texture patternTex;
                patternTex.createFromBitmap(pattern->bitmap);
                
                // Draw at 2x scale
                gfx2d::Rect dest(50, 80, pattern->width * 2, pattern->height * 2);
                renderer.drawTextureScaled(patternTex, dest);
                
                // Draw outline
                renderer.drawRectOutline(gfx2d::Rect(49, 79, dest.w + 2, dest.h + 2), 
                                          gfx2d::Color(255, 255, 0, 255));
            }
            
            // Draw all patterns from atlas (thumbnail strip)
            int thumbX = 50;
            int thumbY = 400;
            int thumbScale = 1;
            
            for (int i = 0; i < std::min(20, sheet.patternCount()); i++) {
                const gfx2d::Pattern* p = sheet.getPattern(i);
                if (p && p->width > 0 && p->height > 0) {
                    gfx2d::Rect src = atlas.getPatternRect(i);
                    if (src.w > 0) {
                        int w = src.w * thumbScale;
                        int h = src.h * thumbScale;
                        
                        // Highlight current pattern
                        if (i == currentPattern) {
                            renderer.drawRect(gfx2d::Rect(thumbX - 2, thumbY - 2, w + 4, h + 4),
                                               gfx2d::Color(255, 255, 0, 255));
                        }
                        
                        renderer.drawTextureScaled(atlas.texture(), 
                                                    gfx2d::Rect(thumbX, thumbY, w, h), src);
                        
                        thumbX += w + 4;
                        if (thumbX > window->width() - 100) {
                            thumbX = 50;
                            thumbY += 80;
                        }
                    }
                }
            }
            
            renderer.endFrame();
            window->swapBuffers();
            needRedraw = false;
        }
        
        // Small sleep to avoid spinning CPU when idle
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    printf("Done!\n");
    return 0;
}
