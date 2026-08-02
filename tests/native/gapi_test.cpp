#include "gapi/gapi.hpp"
#include "libs/RKC_DBFCONTROL/rkc_dbfcontrol.hpp"
#include "libs/RKC_DIB/rkc_dib.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "render/character_renderer.hpp"
#include "render/gameplay_hud_renderer.hpp"
#include "render/gameplay_renderer.hpp"
#include "render/loading_renderer.hpp"
#include "render/system_cursor_renderer.hpp"
#include "render/title_renderer.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace {

void appendI32(
    std::vector<std::uint8_t>& bytes,
    std::int32_t value) {
    const std::uint32_t raw = static_cast<std::uint32_t>(value);
    bytes.push_back(static_cast<std::uint8_t>(raw));
    bytes.push_back(static_cast<std::uint8_t>(raw >> 8u));
    bytes.push_back(static_cast<std::uint8_t>(raw >> 16u));
    bytes.push_back(static_cast<std::uint8_t>(raw >> 24u));
}

void appendI16(
    std::vector<std::uint8_t>& bytes,
    std::int16_t value) {
    const std::uint16_t raw = static_cast<std::uint16_t>(value);
    bytes.push_back(static_cast<std::uint8_t>(raw));
    bytes.push_back(static_cast<std::uint8_t>(raw >> 8u));
}

void appendBytes(
    std::vector<std::uint8_t>& destination,
    const char* source,
    std::size_t size) {
    destination.insert(
        destination.end(), source, source + size);
}

std::vector<std::uint8_t> makeCompressedNjpFixture() {
    std::vector<std::uint8_t> bytes;
    const char header[16] = "NJudgeUniPat003";
    appendBytes(bytes, header, sizeof(header));

    appendI32(bytes, 1);
    appendI32(bytes, 8);
    appendI32(bytes, 8);
    appendI32(bytes, 2);
    appendI32(bytes, 2);
    appendI32(bytes, 1);

    const char compressionMagic[8] = {
        'R', 'C', 'L', 'I', 'B', '-', 'L', '\x1a',
    };
    appendBytes(
        bytes, compressionMagic, sizeof(compressionMagic));
    appendI32(bytes, 8);
    appendI32(bytes, 9);
    bytes.push_back(0);
    const std::uint8_t pixels[8] = {
        3, 4, 0, 0,
        1, 2, 0, 0,
    };
    bytes.insert(bytes.end(), pixels, pixels + sizeof(pixels));

    appendI32(bytes, 1);
    appendI32(bytes, 1);
    appendI32(bytes, 1);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 2);
    appendI32(bytes, 2);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 1000);
    appendI32(bytes, 1000);

    appendI32(bytes, 2);
    for (std::int32_t palette = 0; palette < 2; ++palette) {
        for (std::int32_t index = 0; index < 256; ++index) {
            std::uint8_t red = 0;
            std::uint8_t green = 0;
            std::uint8_t blue = 0;
            if (palette == 1 && index == 1) {
                green = 200;
            } else if (index == 1) {
                red = 100;
            } else if (index == 2) {
                green = 120;
            } else if (index == 3) {
                blue = 140;
            } else if (index == 4) {
                red = 200;
                green = 180;
                blue = 160;
            }
            bytes.push_back(red);
            bytes.push_back(green);
            bytes.push_back(blue);
            bytes.push_back(0);
        }
    }
    return bytes;
}

std::vector<std::uint8_t> makeFontNjpFixture() {
    std::vector<std::uint8_t> bytes;
    const char header[16] = "NJudgeUniPat002";
    appendBytes(bytes, header, sizeof(header));

    appendI32(bytes, 1);
    appendI32(bytes, 8);
    appendI32(bytes, 16);
    appendI32(bytes, 16);
    appendI32(bytes, 0);
    std::vector<std::uint8_t> pixels(16 * 16, 0);
    pixels[11 * 16 + 1] = 1;
    bytes.insert(bytes.end(), pixels.begin(), pixels.end());

    appendI32(bytes, 1);
    appendI32(bytes, 1);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 16);
    appendI32(bytes, 16);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 1000);
    appendI32(bytes, 1000);

    appendI32(bytes, 1);
    for (std::int32_t index = 0; index < 256; ++index) {
        const std::uint8_t value = index == 1 ? 255 : 0;
        bytes.push_back(value);
        bytes.push_back(value);
        bytes.push_back(value);
        bytes.push_back(0);
    }
    return bytes;
}

std::vector<std::uint8_t> makeBitmapFixture() {
    std::vector<std::uint8_t> bytes(62, 0);
    bytes[0] = 'B';
    bytes[1] = 'M';
    bytes[2] = 62;
    bytes[10] = 54;
    bytes[14] = 40;
    bytes[18] = 2;
    bytes[22] = 1;
    bytes[26] = 1;
    bytes[28] = 24;
    bytes[34] = 8;
    bytes[54] = 30;
    bytes[55] = 20;
    bytes[56] = 10;
    bytes[57] = 60;
    bytes[58] = 50;
    bytes[59] = 40;
    return bytes;
}

std::vector<std::uint8_t> makeCafFixture(
    std::int16_t cell_status = 16) {
    std::vector<std::uint8_t> bytes;
    const char header[16] = "CHRAnimation002";
    appendBytes(bytes, header, sizeof(header));
    appendI32(bytes, 1);
    appendI16(bytes, 1);
    for (std::int32_t direction = 0; direction < 8; ++direction) {
        appendI32(bytes, 0);
        appendI16(bytes, 0);
    }
    appendI32(bytes, 1);
    appendI16(bytes, 2);
    appendI32(bytes, 2);
    appendI16(bytes, cell_status);
    appendI16(bytes, 1000);
    appendI32(bytes, 4);
    appendI16(bytes, 0);
    appendI16(bytes, cell_status);
    appendI16(bytes, 750);
    appendI32(bytes, 7);
    appendI16(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 1);
    return bytes;
}

struct PatternCall {
    std::size_t index = 0;
    osf::gapi::PatternDraw draw;
};

class RecordingBackend final : public osf::gapi::Backend {
public:
    void beginFrame(osf::gapi::Color) override {}

    bool drawPattern(
        const osf::gapi::NjpImage&,
        std::size_t pattern_index,
        const osf::gapi::PatternDraw& draw) override {
        patterns.push_back({pattern_index, draw});
        return true;
    }

    bool drawBitmap(
        const osf::gapi::BitmapImage&,
        const osf::gapi::BitmapDraw&) override {
        return true;
    }

    bool drawText(
        const osf::gapi::NjpImage&,
        std::string_view,
        const osf::gapi::TextDraw&) override {
        return true;
    }

    bool drawRectangle(
        const osf::gapi::RectangleDraw& draw) override {
        rectangles.push_back(draw);
        return true;
    }

    void endFrame() override {}

    std::vector<PatternCall> patterns;
    std::vector<osf::gapi::RectangleDraw> rectangles;
};

bool check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
    }
    return condition;
}

bool testViewport() {
    const osf::gapi::Viewport wide =
        osf::gapi::fitViewport(640, 480, 1920, 1080);
    const osf::gapi::Viewport fourK =
        osf::gapi::fitViewport(640, 480, 3840, 2160);
    return check(
        wide.x == 240 && wide.y == 0 &&
            wide.width == 1440 && wide.height == 1080 &&
            fourK.x == 480 && fourK.y == 0 &&
            fourK.width == 2880 && fourK.height == 2160,
        "GAPI aspect-fit viewport calculation differs.");
}

bool testNjpAndSoftwareBackend() {
    osf::gapi::NjpImage image;
    std::string error;
    if (!check(
            image.decode(makeCompressedNjpFixture(), &error),
            error.c_str())) {
        return false;
    }
    if (!check(
            image.version() == 3 &&
                image.parts().size() == 1 &&
                image.parts()[0].stride == 4 &&
                image.patterns().size() == 1 &&
                image.patterns()[0].parts.size() == 1 &&
                image.palettes().size() == 2,
            "The portable NJP decoder produced the wrong structure.")) {
        return false;
    }
    if (!check(
            osf::displayPatternContainsPoint(
                image, 0, {10, 20}, {10, 20}) &&
                osf::displayPatternContainsPoint(
                    image, 0, {10, 20}, {11, 21}) &&
                !osf::displayPatternContainsPoint(
                    image, 0, {10, 20}, {12, 21}) &&
                osf::displayPatternIntersectsRectangle(
                    image,
                    0,
                    {10, 20},
                    {9, 19, 10, 20}) &&
                !osf::displayPatternIntersectsRectangle(
                    image,
                    0,
                    {10, 20},
                    {8, 18, 9, 19}),
            "RKC_RPGSCRN display-object pixel hit testing "
            "differs from the rendered NJP cells.")) {
        return false;
    }

    std::int32_t presented = 0;
    osf::gapi::SoftwareBackend backend(
        2,
        2,
        [&presented](osf::gapi::SurfaceView surface) {
            if (surface.width == 2 && surface.height == 2) {
                ++presented;
            }
        });
    backend.beginFrame({1, 2, 3, 255});
    if (!check(
            backend.drawPattern(
                image, 0, {0, 0, 1000, 1000, 500}),
            "The software backend rejected a valid NJP pattern.")) {
        return false;
    }
    backend.endFrame();

    const osf::gapi::SurfaceView surface = backend.surface();
    if (!check(
        sizeof(osf::gapi::Color) == 4 &&
            presented == 1 &&
            surface.pixels[0].red == 50 &&
            surface.pixels[0].green == 0 &&
            surface.pixels[1].red == 0 &&
            surface.pixels[1].green == 60 &&
            surface.pixels[2].blue == 70 &&
            surface.pixels[3].red == 100 &&
            surface.pixels[3].green == 90 &&
            surface.pixels[3].blue == 80,
        "NJP orientation, palette conversion, or brightness differs.")) {
        return false;
    }

    osf::gapi::SoftwareBackend opacityBackend(1, 1);
    opacityBackend.beginFrame({20, 40, 60, 255});
    opacityBackend.drawPattern(
        image, 0, {0, 0, 1000, 1000, 1000, 500});
    const osf::gapi::Color blended =
        opacityBackend.surface().pixels[0];
    if (!check(
            blended.red == 60 &&
            blended.green == 20 &&
            blended.blue == 30,
            "GAPI pattern opacity did not blend portably.")) {
        return false;
    }

    osf::gapi::SoftwareBackend additiveBackend(1, 1);
    additiveBackend.beginFrame({20, 40, 60, 255});
    osf::gapi::PatternDraw additiveDraw{
        0, 0, 1000, 1000, 1000, 500};
    additiveDraw.blend_mode =
        osf::gapi::PatternBlendMode::additive;
    additiveBackend.drawPattern(image, 0, additiveDraw);
    const osf::gapi::Color added =
        additiveBackend.surface().pixels[0];
    if (!check(
            added.red == 70 &&
                added.green == 40 &&
                added.blue == 60,
            "GAPI additive sprite blending differs from retail.")) {
        return false;
    }

    osf::gapi::SoftwareBackend tintBackend(1, 1);
    tintBackend.beginFrame({20, 40, 60, 255});
    tintBackend.drawPattern(
        image,
        0,
        {0, 0, 1000, 1000, 1000, 1000, 1300, 1300, 1300});
    const osf::gapi::Color tinted =
        tintBackend.surface().pixels[0];
    if (!check(
            tinted.red == 146 &&
                tinted.green == 76 &&
                tinted.blue == 76,
            "GAPI color strength did not reproduce retail pale tinting.")) {
        return false;
    }

    osf::gapi::SoftwareBackend paletteBackend(1, 1);
    paletteBackend.beginFrame({20, 40, 60, 255});
    if (!check(
            paletteBackend.drawPattern(
                image,
                0,
                {0,
                 0,
                 1000,
                 1000,
                 1000,
                 1000,
                 1000,
                 1000,
                 1000,
                 1}) &&
                paletteBackend.surface().pixels[0].red == 0 &&
                paletteBackend.surface().pixels[0].green == 200 &&
                paletteBackend.surface().pixels[0].blue == 0,
            "GAPI ignored an explicit NJP palette selection.")) {
        return false;
    }

    osf::gapi::SoftwareBackend clippedBackend(2, 2);
    clippedBackend.beginFrame({7, 8, 9, 255});
    clippedBackend.drawPattern(
        image,
        0,
        {0,
         0,
         1000,
         1000,
         1000,
         1000,
         1000,
         1000,
         1000,
         -1,
         {1, 0, 1, 2}});
    const osf::gapi::SurfaceView clippedSurface =
        clippedBackend.surface();
    if (!check(
            clippedSurface.pixels[0].red == 7 &&
                clippedSurface.pixels[0].green == 8 &&
                clippedSurface.pixels[2].red == 7 &&
                clippedSurface.pixels[2].green == 8 &&
                clippedSurface.pixels[1].green == 120 &&
                clippedSurface.pixels[3].red == 200,
            "GAPI pattern clipping did not preserve the destination bounds.")) {
        return false;
    }

    osf::gapi::SoftwareBackend farClippedBackend(1, 1);
    farClippedBackend.beginFrame({7, 8, 9, 255});
    farClippedBackend.drawPattern(
        image,
        0,
        {-1999, -1999, 1000000, 1000000});
    const osf::gapi::Color farClipped =
        farClippedBackend.surface().pixels[0];
    if (!check(
            farClipped.red == 200 &&
                farClipped.green == 180 &&
                farClipped.blue == 160,
            "A large clipped pattern sampled the wrong visible pixel.")) {
        return false;
    }

    osf::gapi::SoftwareBackend rectangleBackend(3, 2);
    rectangleBackend.beginFrame({20, 40, 60, 255});
    rectangleBackend.drawRectangle({
        0, 0, 2, 2, {100, 80, 60, 255}, 1000, 500,
    });
    rectangleBackend.drawRectangle({
        2, 0, 1, 2, {9, 8, 7, 255}, 1000, 2000,
    });
    const osf::gapi::SurfaceView rectangleSurface =
        rectangleBackend.surface();
    const osf::gapi::Color rectangle = rectangleSurface.pixels[0];
    return check(
        rectangle.red == 60 &&
            rectangle.green == 60 &&
            rectangle.blue == 60 &&
            rectangleSurface.pixels[4].red == 60 &&
            rectangleSurface.pixels[2].red == 9 &&
            rectangleSurface.pixels[5].blue == 7,
        "GAPI rectangle rows or clamped opacity differ.");
}

bool testGameplayHudPackets() {
    osf::gapi::NjpImage bar;
    RecordingBackend backend;
    osf::renderGameplayHud(
        backend,
        bar,
        {
            123,
            50,
            100,
            200,
            160,
            20,
            25,
            true,
            true,
            false,
            2,
        });

    RecordingBackend activationBackend;
    osf::renderGameplayHud(
        activationBackend,
        bar,
        {
            1,
            1,
            1,
            1,
            1,
            0,
            1,
            false,
            false,
            true,
            0,
        });
    RecordingBackend companionBackend;
    osf::GameplayHudValues companionValues;
    companionValues.companion_present = true;
    companionValues.companion_current_life = 29;
    companionValues.companion_maximum_life = 109;
    companionValues.companion_inactive = true;
    companionValues.animation_counter = 1;
    osf::renderGameplayHud(
        companionBackend,
        bar,
        companionValues);
    RecordingBackend activeCompanionBackend;
    companionValues.companion_inactive = false;
    osf::renderGameplayHud(
        activeCompanionBackend,
        bar,
        companionValues);
    return check(
        osf::gameplayHudBarWidth(0, 100) == 0 &&
            osf::gameplayHudBarWidth(1, 1000) == 1 &&
            osf::gameplayHudBarWidth(50, 100) == 103 &&
            osf::gameplayHudBarWidth(200, 100) == 206 &&
            osf::gameplayHudExperienceBarWidth(
                20, 25) == 87 &&
            backend.rectangles.size() == 1 &&
            backend.rectangles[0].x == 0 &&
            backend.rectangles[0].y == 412 &&
            backend.rectangles[0].width == 640 &&
            backend.rectangles[0].height == 68 &&
            backend.patterns.size() == 11 &&
            backend.patterns[0].index == 7 &&
            backend.patterns[1].index == 8 &&
            backend.patterns[2].index == 10 &&
            backend.patterns[3].index == 22 &&
            backend.patterns[3].draw.x == 69 &&
            backend.patterns[4].index == 21 &&
            backend.patterns[4].draw.x == 60 &&
            backend.patterns[5].index == 20 &&
            backend.patterns[5].draw.x == 51 &&
            backend.patterns[6].index == 0 &&
            backend.patterns[6].draw.clip.x == 81 &&
            backend.patterns[6].draw.clip.y == 425 &&
            backend.patterns[6].draw.clip.width == 103 &&
            backend.patterns[7].index == 3 &&
            backend.patterns[7].draw.clip.x == 106 &&
            backend.patterns[7].draw.clip.y == 452 &&
            backend.patterns[7].draw.clip.width == 206 &&
            backend.patterns[8].index == 12 &&
            backend.patterns[8].draw.red_strength == 800 &&
            backend.patterns[8].draw.green_strength == 800 &&
            backend.patterns[8].draw.blue_strength == 800 &&
            backend.patterns[9].index == 15 &&
            backend.patterns[10].index == 14 &&
            backend.patterns[10].draw.clip.x == 530 &&
            backend.patterns[10].draw.clip.y == 395 &&
            backend.patterns[10].draw.clip.width == 87 &&
            backend.patterns[10].draw.clip.height == 9,
        "The gameplay HUD packets differ from FUN_004039f0.") &&
        check(
            activationBackend.patterns.size() == 8 &&
                activationBackend.patterns[6].index == 13 &&
                activationBackend.patterns[7].index == 15,
            "The Increased Power activation marker differs from "
            "FUN_004039f0.") &&
        check(
            osf::gameplayHudCompanionBarWidth(0, 109) == 0 &&
                osf::gameplayHudCompanionBarWidth(1, 200) == 0 &&
                osf::gameplayHudCompanionBarWidth(29, 109) == 29 &&
                osf::gameplayHudCompanionBarWidth(109, 109) == 109 &&
                companionBackend.patterns.size() == 8 &&
                companionBackend.patterns[0].index == 30 &&
                companionBackend.patterns[1].index == 29 &&
                companionBackend.patterns[1].draw.clip.x == 81 &&
                companionBackend.patterns[1].draw.clip.y == 396 &&
                companionBackend.patterns[1].draw.clip.width == 29 &&
                companionBackend.patterns[1].draw.clip.height == 11 &&
                companionBackend.patterns[1].draw.red_strength == 1500 &&
                companionBackend.patterns[2].index == 32 &&
                companionBackend.patterns[3].index == 7,
            "The companion frame, reverse life fill, low-life pulse, "
            "or inactive marker differs from FUN_004039f0.") &&
        check(
            activeCompanionBackend.patterns.size() == 8 &&
                activeCompanionBackend.patterns[2].index == 31,
            "The active companion HUD marker differs from "
            "FUN_004039f0.");
}

bool testTruncatedNjp() {
    std::vector<std::uint8_t> bytes = makeCompressedNjpFixture();
    bytes.resize(40);
    osf::gapi::NjpImage image;
    std::string error;
    return check(
        !image.decode(bytes, &error) && !error.empty(),
        "The NJP decoder accepted a truncated compressed bitmap.");
}

bool testBitmapAndTextDrawing() {
    osf::gapi::BitmapImage bitmap;
    std::string error;
    if (!check(
            bitmap.decode(makeBitmapFixture(), &error),
            error.c_str())) {
        return false;
    }

    osf::gapi::NjpImage font;
    if (!check(
            font.decode(makeFontNjpFixture(), &error),
            error.c_str())) {
        return false;
    }

    osf::gapi::SoftwareBackend backend(5, 5);
    backend.beginFrame({0, 0, 0, 255});
    if (!check(
            backend.drawRectangle(
                {1, 1, 3, 2, {80, 40, 20, 255}, 500}) &&
                backend.drawBitmap(bitmap, {0, 0}) &&
                backend.drawText(
                    font,
                    "A",
                    {2, 3, {200, 100, 50, 255}, 500}),
            "The software backend rejected bitmap or text drawing.")) {
        return false;
    }
    const osf::gapi::SurfaceView surface = backend.surface();
    return check(
        bitmap.width() == 2 && bitmap.height() == 1 &&
            surface.pixels[0].red == 10 &&
            surface.pixels[0].green == 20 &&
            surface.pixels[0].blue == 30 &&
            surface.pixels[1].red == 40 &&
            surface.pixels[1].green == 50 &&
            surface.pixels[1].blue == 60 &&
            surface.pixels[1 * 5 + 1].red == 40 &&
            surface.pixels[1 * 5 + 1].green == 20 &&
            surface.pixels[1 * 5 + 1].blue == 10 &&
            surface.pixels[3 * 5 + 2].red == 100 &&
            surface.pixels[3 * 5 + 2].green == 50 &&
            surface.pixels[3 * 5 + 2].blue == 25,
        "BMP orientation, BGR conversion, or font tinting differs.");
}

bool testMutableBitmapMask() {
    osf::gapi::BitmapImage bitmap;
    if (!check(
            bitmap.create(4, 4, {0, 0, 0, 0}),
            "A mutable bitmap mask could not be created.")) {
        return false;
    }
    bitmap.fillRectangle(
        1, 1, 2, 2, {255, 0, 0, 128});

    osf::gapi::SoftwareBackend backend(4, 4);
    backend.beginFrame({0, 255, 0, 255});
    backend.drawBitmap(
        bitmap,
        {0, 0, 1000, 1000, 1000, {2, 1, 1, 2}});
    const osf::gapi::SurfaceView surface = backend.surface();
    const osf::gapi::Color untouched = surface.pixels[1 * 4 + 1];
    const osf::gapi::Color blended = surface.pixels[1 * 4 + 2];
    return check(
        untouched.red == 0 &&
            untouched.green == 255 &&
            blended.red == 128 &&
            blended.green == 127 &&
            surface.pixels[2 * 4 + 2].red == 128 &&
            surface.pixels[3 * 4 + 2].green == 255,
        "Bitmap alpha or destination clipping differs.");
}

bool testCafAndTitleAnimation() {
    osf::gapi::CafAnimation animation;
    std::string error;
    if (!check(
            animation.decode(makeCafFixture(), &error),
            error.c_str())) {
        return false;
    }
    if (!check(
            animation.version() == 2 &&
                animation.charts().size() == 1 &&
                animation.maxPartCount() == 1 &&
                animation.charts()[0].status == 1 &&
                animation.charts()[0].directions[8].frame_count == 2 &&
                animation.charts()[0].directions[8].parts.size() == 1 &&
                animation.charts()[0]
                        .directions[8]
                        .parts[0][1]
                        .pattern_index == 7,
            "The portable CAF decoder produced the wrong structure.")) {
        return false;
    }

    osf::gapi::NjpImage title;
    osf::gapi::NjpImage smokeImage;
    std::array<osf::TitleSmokeAsset, 10> smoke{};
    smoke[3] = {&smokeImage, &animation};
    osf::TitleFrameResult frame;
    frame.scene_brightness = 800;
    frame.smoke_frames[3] = 1;

    RecordingBackend backend;
    osf::renderTitle(backend, title, smoke, frame);
    return check(
        backend.patterns.size() == 5 &&
            backend.patterns[0].index == 0 &&
            backend.patterns[3].index == 3 &&
            backend.patterns[4].index == 7 &&
            backend.patterns[4].draw.x == 562 &&
            backend.patterns[4].draw.y == 60 &&
            backend.patterns[4].draw.brightness == 800 &&
            backend.patterns[4].draw.opacity == 750 &&
            backend.patterns[4].draw.blend_mode ==
                osf::gapi::PatternBlendMode::additive,
        "The title steam CAF frame or retail draw packet differs.");
}

bool testCafCharacterDrawModes() {
    osf::gapi::CafAnimation animation;
    std::string error;
    if (!check(
            animation.decode(makeCafFixture(8), &error),
            error.c_str())) {
        return false;
    }

    osf::gapi::NjpImage patterns;
    osf::gapi::NjpImage shadows;
    RecordingBackend backend;
    const auto enabled = [](std::size_t) {
        return true;
    };
    const auto color = [](std::size_t) {
        return osf::CharacterColorStrength{};
    };
    osf::renderCharacterAnimationPass(
        backend,
        animation,
        patterns,
        shadows,
        {0, 0},
        0,
        8,
        1,
        enabled,
        color,
        0,
        0,
        false,
        500);
    osf::renderCharacterAnimationPass(
        backend,
        animation,
        patterns,
        shadows,
        {0, 0},
        0,
        8,
        1,
        enabled,
        color,
        0,
        0,
        true,
        500);
    return check(
        backend.patterns.size() == 2 &&
            backend.patterns[0].index == 7 &&
            backend.patterns[0].draw.opacity == 750 &&
            backend.patterns[0].draw.blend_mode ==
                osf::gapi::PatternBlendMode::normal &&
            backend.patterns[1].index == 7 &&
            backend.patterns[1].draw.opacity == 500,
        "A shadow-enabled CAF cell did not keep its normal pass.");
}

bool testInitialLoadingPackets() {
    osf::gapi::NjpImage waiting;
    RecordingBackend backend;
    osf::renderInitialLoadingScreen(
        backend, waiting, 0, false);
    osf::renderInitialLoadingScreen(
        backend, waiting, 0, true);
    osf::renderInitialLoadingScreen(
        backend, waiting, 7, true);
    osf::renderInitialLoadingScreen(
        backend, waiting, 15, true);

    return check(
        backend.patterns.size() == 8 &&
            backend.patterns[0].index == 0 &&
            backend.patterns[0].draw.x == 0 &&
            backend.patterns[0].draw.y == 0 &&
            backend.patterns[1].index == 3 &&
            backend.patterns[1].draw.x == 572 &&
            backend.patterns[1].draw.y == 443 &&
            backend.patterns[3].index == 2 &&
            backend.patterns[3].draw.x == 592 &&
            backend.patterns[3].draw.y == 450 &&
            backend.patterns[5].draw.x == 599 &&
            backend.patterns[7].draw.x == 607,
        "The initial loading artwork or confirmation arrow differs.");
}

bool testSystemCursorPackets() {
    osf::gapi::NjpImage system_patterns;
    RecordingBackend backend;
    osf::renderSystemCursor(
        backend, system_patterns, 123, 234, false);
    osf::renderSystemCursor(
        backend, system_patterns, 321, 432, true);
    return check(
        backend.patterns.size() == 2 &&
            backend.patterns[0].index == 0 &&
            backend.patterns[0].draw.x == 123 &&
            backend.patterns[0].draw.y == 234 &&
            backend.patterns[1].index == 1 &&
            backend.patterns[1].draw.x == 321 &&
            backend.patterns[1].draw.y == 432,
        "The normal or Identify system cursor draw packet differs.");
}

bool testSoftwareLineDrawing() {
    RecordingBackend fallback;
    if (!check(
            fallback.drawLine({
                0, 0, 2, 1, {255, 255, 255, 255}, 1000, 1000, {},
            }),
            "The generic GAPI line fallback rejected a valid line.")) {
        return false;
    }
    if (!check(
            fallback.rectangles.size() == 3 &&
                fallback.rectangles[0].x == 0 &&
                fallback.rectangles[0].y == 0 &&
                fallback.rectangles[1].x == 1 &&
                fallback.rectangles[1].y == 1 &&
                fallback.rectangles[2].x == 2 &&
                fallback.rectangles[2].y == 1,
            "The generic GAPI line fallback missed a Bresenham point.")) {
        return false;
    }

    osf::gapi::SoftwareBackend backend(4, 4);
    backend.beginFrame({0, 0, 0, 255});
    if (!check(
            backend.drawLine({
                0, 1, 2, 1, {200, 100, 50, 255}, 1000, 500, {}}),
            "The software GAPI rejected a valid line.")) {
        return false;
    }
    const osf::gapi::SurfaceView surface = backend.surface();
    const osf::gapi::Color first = surface.pixels[4];
    const osf::gapi::Color middle = surface.pixels[5];
    const osf::gapi::Color last = surface.pixels[6];
    return check(
        first.red == 100 && first.green == 50 && first.blue == 25 &&
            middle.red == 100 && middle.green == 50 &&
            middle.blue == 25 && last.red == 100 &&
            last.green == 50 && last.blue == 25 &&
            surface.pixels[7].red == 0,
        "The software GAPI line missed an endpoint or blended incorrectly.");
}

}  // namespace

int main() {
    if (!testViewport() ||
        !testNjpAndSoftwareBackend() ||
        !testTruncatedNjp() ||
        !testBitmapAndTextDrawing() ||
        !testMutableBitmapMask() ||
        !testCafAndTitleAnimation() ||
        !testCafCharacterDrawModes() ||
        !testSoftwareLineDrawing() ||
        !testInitialLoadingPackets() ||
        !testSystemCursorPackets() ||
        !testGameplayHudPackets()) {
        return 1;
    }
    return 0;
}
