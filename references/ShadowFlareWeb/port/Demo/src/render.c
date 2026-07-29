#include "render.h"
#include <limits.h> 
#include <string.h> 
#include "script_bridge.h" 
#include "inventory.h"     
#include "combat.h"        


void DrawMarker(RKC_DIB *canvas, long cx, long cy, long half, unsigned char b, unsigned char g, unsigned char r)
{
    long x0 = cx - half, x1 = cx + half;
    long y0 = cy - half, y1 = cy + half;
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 > canvas->width)
        x1 = canvas->width;
    if (y1 > canvas->height)
        y1 = canvas->height;
    for (long y = y0; y < y1; y++)
    {
        unsigned char *row = canvas->pixels + (size_t)(canvas->height - 1 - y) * (size_t)canvas->alignWidth;
        for (long x = x0; x < x1; x++)
        {
            unsigned char *px = row + x * 3;
            px[0] = b;
            px[1] = g;
            px[2] = r;
        }
    }
}


static void DrawRect(RKC_DIB *canvas, long x, long y, long w, long h, unsigned char b, unsigned char g,
                     unsigned char r)
{
    long x0 = x, x1 = x + w;
    long y0 = y, y1 = y + h;
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 > canvas->width)
        x1 = canvas->width;
    if (y1 > canvas->height)
        y1 = canvas->height;
    for (long yy = y0; yy < y1; yy++)
    {
        unsigned char *row = canvas->pixels + (size_t)(canvas->height - 1 - yy) * (size_t)canvas->alignWidth;
        for (long xx = x0; xx < x1; xx++)
        {
            unsigned char *px = row + xx * 3;
            px[0] = b;
            px[1] = g;
            px[2] = r;
        }
    }
}


static void DrawRectOutline(RKC_DIB *canvas, long x, long y, long w, long h, long thickness, unsigned char b,
                            unsigned char g, unsigned char r)
{
    DrawRect(canvas, x, y, w, thickness, b, g, r);                 
    DrawRect(canvas, x, y + h - thickness, w, thickness, b, g, r); 
    DrawRect(canvas, x, y, thickness, h, b, g, r);                 
    DrawRect(canvas, x + w - thickness, y, thickness, h, b, g, r); 
}


static void DrawRectAlpha(RKC_DIB *canvas, long x, long y, long w, long h, long opacity, unsigned char b,
                          unsigned char g, unsigned char r)
{
    long x0 = x, x1 = x + w;
    long y0 = y, y1 = y + h;
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 > canvas->width)
        x1 = canvas->width;
    if (y1 > canvas->height)
        y1 = canvas->height;
    for (long yy = y0; yy < y1; yy++)
    {
        unsigned char *row = canvas->pixels + (size_t)(canvas->height - 1 - yy) * (size_t)canvas->alignWidth;
        for (long xx = x0; xx < x1; xx++)
        {
            unsigned char *px = row + xx * 3;
            px[0] = (unsigned char)(px[0] + ((long)b - px[0]) * opacity / 1000);
            px[1] = (unsigned char)(px[1] + ((long)g - px[1]) * opacity / 1000);
            px[2] = (unsigned char)(px[2] + ((long)r - px[2]) * opacity / 1000);
        }
    }
}


static void DrawRectOutlineAlpha(RKC_DIB *canvas, long x, long y, long w, long h, long thickness, long opacity,
                                 unsigned char b, unsigned char g, unsigned char r)
{
    DrawRectAlpha(canvas, x, y, w, thickness, opacity, b, g, r);                 
    DrawRectAlpha(canvas, x, y + h - thickness, w, thickness, opacity, b, g, r); 
    DrawRectAlpha(canvas, x, y, thickness, h, opacity, b, g, r);                 
    DrawRectAlpha(canvas, x + w - thickness, y, thickness, h, opacity, b, g, r); 
}


static int NextSjisGlyph(const unsigned char **cursor, long *outFrame, long *outSrcX, long *outSrcY, long *outWidth)
{
    const unsigned char *s = *cursor;
    unsigned lead = s[0];
    if (lead == 0)
        return 0;

    if ((lead >= 0x81 && lead <= 0x9F) || (lead >= 0xE0 && lead <= 0xFC))
    {
        unsigned trail = s[1];
        if (trail >= 0x40 && trail <= 0xFC && trail != 0x7F)
        {
            *cursor = s + 2;
            *outFrame = lead <= 0x9F ? (long)lead - 0x7F : (long)lead - 0xE0 + 33;
            *outSrcX = (long)(trail % FONT_GLYPHS_PER_ROW) * FONT_CELL_WIDTH_FULL;
            *outSrcY = (long)(trail / FONT_GLYPHS_PER_ROW) * FONT_CELL_HEIGHT;
            *outWidth = FONT_CELL_WIDTH_FULL;
            return 1;
        }
        *cursor = s + 1;
        *outWidth = 0;
        return 1;
    }

    *cursor = s + 1;
    if (lead < 0x20 || lead > 0xDF)
    {
        
        *outWidth = 0;
        return 1;
    }
    *outFrame = FONT_FRAME_SINGLE_BYTE;
    *outSrcX = (long)(lead % FONT_GLYPHS_PER_ROW) * FONT_CELL_WIDTH_HALF;
    *outSrcY = (long)(lead / FONT_GLYPHS_PER_ROW) * FONT_CELL_HEIGHT;
    *outWidth = FONT_CELL_WIDTH_HALF;
    return 1;
}


static void StampGlyphSolid(RKC_DIB *canvas, const RKC_DIB *sheet, long destX, long destY, long srcX, long srcY,
                            long w, long h, unsigned char b, unsigned char g, unsigned char r, long trans)
{
    if (sheet->bpp != 4 && sheet->bpp != 8)
        return;
    for (long yy = 0; yy < h; yy++)
    {
        long sy = srcY + yy, dy = destY + yy;
        if (sy < 0 || sy >= sheet->height || dy < 0 || dy >= canvas->height)
            continue;
        const unsigned char *srow = sheet->pixels + (size_t)(sheet->height - 1 - sy) * (size_t)sheet->alignWidth;
        unsigned char *drow = canvas->pixels + (size_t)(canvas->height - 1 - dy) * (size_t)canvas->alignWidth;
        for (long xx = 0; xx < w; xx++)
        {
            long sx = srcX + xx, dx = destX + xx;
            if (sx < 0 || sx >= sheet->width || dx < 0 || dx >= canvas->width)
                continue;
            unsigned idx = sheet->bpp == 8
                               ? srow[sx]
                               : ((sx & 1) == 0 ? (unsigned)(srow[sx >> 1] >> 4) : (unsigned)(srow[sx >> 1] & 0x0F));
            if (idx == 0)
                continue;
            unsigned char *px = drow + dx * 3;
            px[0] = (unsigned char)((b * trans + px[0] * (1000 - trans)) / 1000);
            px[1] = (unsigned char)((g * trans + px[1] * (1000 - trans)) / 1000);
            px[2] = (unsigned char)((r * trans + px[2] * (1000 - trans)) / 1000);
        }
    }
}


typedef enum
{
    TEXT_STYLE_WHITE,
    TEXT_STYLE_WHITE_SHADOWED,
    TEXT_STYLE_BLACK,
    TEXT_STYLE_YELLOW_SHADOWED,
    TEXT_STYLE_BLUE_SHADOWED,
    TEXT_STYLE_ORANGE_SHADOWED,
    TEXT_STYLE_RED, 
    TEXT_STYLE_GREY, 
    TEXT_STYLE_CYAN, 
} TextStyle;


static long DrawTextSJIS(DemoState *state, long x, long y, const char *text, long maxWidth, TextStyle style,
                         long trans)
{
    if (!state->fontLoaded || !text)
        return 0;
    const unsigned char *cursor = (const unsigned char *)text;
    long cursorX = x;
    long frame, srcX, srcY, w;
    while (NextSjisGlyph(&cursor, &frame, &srcX, &srcY, &w))
    {
        if (w <= 0)
            continue;
        if (maxWidth > 0 && cursorX + w - x > maxWidth)
            break;
        RKC_DIB *sheet = RKC_UPDIB_GetFrame(&state->font, frame);
        if (sheet)
        {
            if (style == TEXT_STYLE_BLACK)
                StampGlyphSolid(&state->canvas, sheet, cursorX, y, srcX, srcY, w, FONT_CELL_HEIGHT, 0, 0, 0, trans);
            else if (style == TEXT_STYLE_RED)
                StampGlyphSolid(&state->canvas, sheet, cursorX, y, srcX, srcY, w, FONT_CELL_HEIGHT, 0, 0, 220, trans);
            else if (style == TEXT_STYLE_GREY)
                StampGlyphSolid(&state->canvas, sheet, cursorX, y, srcX, srcY, w, FONT_CELL_HEIGHT, 104, 124, 149,
                                trans); 
            else if (style == TEXT_STYLE_CYAN)
                StampGlyphSolid(&state->canvas, sheet, cursorX, y, srcX, srcY, w, FONT_CELL_HEIGHT, 230, 230, 120,
                                trans); 
            else if (style == TEXT_STYLE_YELLOW_SHADOWED)
            {
                StampGlyphSolid(&state->canvas, sheet, cursorX + 1, y + 1, srcX, srcY, w, FONT_CELL_HEIGHT, 0, 0, 0,
                                trans);
                StampGlyphSolid(&state->canvas, sheet, cursorX, y, srcX, srcY, w, FONT_CELL_HEIGHT, 175, 192, 205,
                                trans);
            }
            else if (style == TEXT_STYLE_BLUE_SHADOWED)
            {
                StampGlyphSolid(&state->canvas, sheet, cursorX + 1, y + 1, srcX, srcY, w, FONT_CELL_HEIGHT, 0, 0, 0,
                                trans);
                StampGlyphSolid(&state->canvas, sheet, cursorX, y, srcX, srcY, w, FONT_CELL_HEIGHT, 230, 150, 150,
                                trans);
            }
            else if (style == TEXT_STYLE_ORANGE_SHADOWED)
            {
                StampGlyphSolid(&state->canvas, sheet, cursorX + 1, y + 1, srcX, srcY, w, FONT_CELL_HEIGHT, 0, 0, 0,
                                trans);
                StampGlyphSolid(&state->canvas, sheet, cursorX, y, srcX, srcY, w, FONT_CELL_HEIGHT, 70, 140, 230,
                                trans);
            }
            else
            {
                if (style == TEXT_STYLE_WHITE_SHADOWED)
                    StampGlyphSolid(&state->canvas, sheet, cursorX + 1, y + 1, srcX, srcY, w, FONT_CELL_HEIGHT, 0, 0,
                                    0, trans);
                RKC_DIB_TransferToDIBEx(&state->canvas, cursorX, y, w, FONT_CELL_HEIGHT, sheet, srcX, srcY, 0, trans);
            }
        }
        cursorX += w;
    }
    return cursorX - x;
}


static long MeasureTextSJIS(const DemoState *state, const char *text)
{
    if (!state->fontLoaded || !text)
        return 0;
    const unsigned char *cursor = (const unsigned char *)text;
    long width = 0;
    long frame, srcX, srcY, w;
    while (NextSjisGlyph(&cursor, &frame, &srcX, &srcY, &w))
        if (w > 0)
            width += w;
    return width;
}


static long DrawWrappedTextSJIS(DemoState *state, long x, long y, const char *text, long maxWidth, long lineHeight,
                                TextStyle style, int measureOnly)
{
    if (!text)
        return y;
    char line[512];
    line[0] = 0;
    long lineW = 0;
    const char *p = text;
    long spaceW = MeasureTextSJIS(state, " ");
    while (*p)
    {
        if (*p == '\n')
        {
            if (!measureOnly)
                DrawTextSJIS(state, x, y, line, 0, style, 1000);
            y += lineHeight;
            line[0] = 0;
            lineW = 0;
            p++;
            continue;
        }
        if (*p == ' ')
        {
            p++;
            continue;
        }
        const char *ws = p;
        while (*p && *p != ' ' && *p != '\n')
            p++;
        char word[256];
        long wl = p - ws;
        if (wl > (long)sizeof(word) - 1)
            wl = (long)sizeof(word) - 1;
        memcpy(word, ws, (size_t)wl);
        word[wl] = 0;
        long wordW = MeasureTextSJIS(state, word);
        int hasContent = line[0] != 0;
        if (hasContent && lineW + spaceW + wordW > maxWidth)
        {
            if (!measureOnly)
                DrawTextSJIS(state, x, y, line, 0, style, 1000);
            y += lineHeight;
            line[0] = 0;
            lineW = 0;
            hasContent = 0;
        }
        if (hasContent)
        {
            strncat(line, " ", sizeof(line) - strlen(line) - 1);
            lineW += spaceW;
        }
        strncat(line, word, sizeof(line) - strlen(line) - 1);
        lineW += wordW;
    }
    if (line[0])
    {
        if (!measureOnly)
            DrawTextSJIS(state, x, y, line, 0, style, 1000);
        y += lineHeight;
    }
    return y;
}


static void DrawTubeSparkle(DemoState *state, RKC_DIB *sparkle, long tubeX, long tubeY, long fillWidth,
                            long fillHeight)
{
    if (!sparkle || fillWidth <= 0)
        return;
    long minY = HUD_TUBE_SPARKLE_EDGE_MARGIN;
    long maxY = fillHeight - sparkle->height - HUD_TUBE_SPARKLE_EDGE_MARGIN;
    if (maxY < minY)
    {
        
        minY = 0;
        maxY = fillHeight - sparkle->height;
        if (maxY < 0)
            maxY = 0;
    }
    long range = maxY - minY;
    for (int i = 0; i < HUD_TUBE_SPARKLE_COUNT; i++)
    {
        long particleTick = (long)state->tick + i * (HUD_TUBE_SPARKLE_PERIOD_TICKS / HUD_TUBE_SPARKLE_COUNT);
        long lap = particleTick / HUD_TUBE_SPARKLE_PERIOD_TICKS;
        long phase = particleTick % HUD_TUBE_SPARKLE_PERIOD_TICKS;
        long offsetX = phase * fillWidth / HUD_TUBE_SPARKLE_PERIOD_TICKS;

        
        unsigned long seed = (unsigned long)lap * 2654435761UL;
        seed ^= (unsigned long)i * 40503UL + 0x9E3779B9UL;
        seed ^= (unsigned long)tubeY * 2246822519UL;
        seed = seed * 1103515245UL + 12345UL;
        long y = minY + (range > 0 ? (long)((seed >> 8) % (unsigned long)(range + 1)) : 0);

        RKC_DIB_TransferToDIBEx(&state->canvas, tubeX + offsetX, tubeY + y, sparkle->width, sparkle->height, sparkle,
                                0, 0, -1, 1000);
    }
}


static int ComputeMagicSlotBarRect(const DemoState *state, long *outX, long *outWidth)
{
    int leftWindowOpen = state->minimapOpen || state->statusMagicOpen || state->gateWindowOpen; 
    if (!state->magicBarIconLoaded || (leftWindowOpen && state->inventoryOpen))
        return 0;
    *outX = leftWindowOpen         ? MAGIC_SLOT_ROW_X_MINIMAP_OPEN
            : state->inventoryOpen ? MAGIC_SLOT_ROW_X_INVENTORY_OPEN
                                   : MAGIC_SLOT_ROW_X_DEFAULT;
    
    *outWidth = MAGIC_SLOT_GROUP_SIZE * 2 * MAGIC_SLOT_SIZE + MAGIC_SLOT_GROUP_GAP + MAGIC_SLOT_ATTACK_GAP +
                MAGIC_SLOT_SIZE;
    return 1;
}


static void DrawScaledFrame(RKC_DIB *canvas, long dstX, long dstY, long dstW, long dstH, const RKC_DIB *src,
                            long colorKey)
{
    if (!src || !src->pixels || src->width <= 0 || src->height <= 0 || dstW <= 0 || dstH <= 0)
        return;
    RKC_DIB tmp;
    RKC_DIB_Init(&tmp);
    
    if (RKC_DIB_Create(&tmp, dstW, dstH, 8, 1))
    {
        long palBytes = RKC_DIB_GetPaletteCount(src) * 4;
        long tmpPalBytes = RKC_DIB_GetPaletteCount(&tmp) * 4;
        if (palBytes > tmpPalBytes)
            palBytes = tmpPalBytes; 
        if (tmp.palette && src->palette && palBytes > 0)
            memcpy(tmp.palette, src->palette, (size_t)palBytes);
        long stride = RKC_DIB_GetAlignWidth(&tmp);
        for (long dy = 0; dy < dstH; dy++)
        {
            long sy = dy * src->height / dstH;
            unsigned char *drow = tmp.pixels + (size_t)dy * stride;
            for (long dx = 0; dx < dstW; dx++)
            {
                long idx = RKC_DIB_GetPixelIndex(src, dx * src->width / dstW, sy);
                drow[dx] = (unsigned char)(idx < 0 ? 0 : idx);
            }
        }
        RKC_DIB_TransferToDIBEx(canvas, dstX, dstY, dstW, dstH, &tmp, 0, 0, colorKey, 1000);
    }
    RKC_DIB_Release(&tmp);
}

int FindMagicSlotAtScreenPoint(const DemoState *state, long x, long y)
{
    long rowX, rowWidth;
    if (!ComputeMagicSlotBarRect(state, &rowX, &rowWidth))
        return -1;
    if (y < MAGIC_SLOT_ROW_Y || y >= MAGIC_SLOT_ROW_Y + MAGIC_SLOT_SIZE)
        return -1;
    for (int s = 0; s < MAGIC_BAR_SLOT_COUNT; s++)
    {
        long groupGap = s >= MAGIC_SLOT_GROUP_SIZE ? MAGIC_SLOT_GROUP_GAP : 0;
        long slotX = rowX + s * MAGIC_SLOT_SIZE + groupGap;
        if (x >= slotX && x < slotX + MAGIC_SLOT_SIZE)
            return s;
    }
    long attackX = rowX + MAGIC_BAR_SLOT_COUNT * MAGIC_SLOT_SIZE + MAGIC_SLOT_GROUP_GAP + MAGIC_SLOT_ATTACK_GAP;
    if (x >= attackX && x < attackX + MAGIC_SLOT_SIZE)
        return MAGIC_SLOT_ATTACK_INDEX;
    return -1;
}



static long DrawDigits(RKC_DIB *canvas, RKC_UPDIB *hudBar, long x, long y, long value);

void DrawHud(DemoState *state, const Uint8 *keys)
{
    if (!state->hudBarLoaded)
        return;

    RKC_DIB *bg = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_BACKGROUND);
    if (!bg)
        return;
    long bgY = APP_HEIGHT - bg->height;
    
    RKC_DIB_TransferToDIBEx(&state->canvas, 0, bgY, bg->width, bg->height, bg, 0, 0, -1, 1000);

    
    DrawDigits(&state->canvas, &state->hudBar, HUD_LV_DIGITS_X, bgY + HUD_LV_DIGITS_Y, state->playerLevel);

    RKC_DIB *sparkle = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_TUBE_SPARKLE);

    
    RKC_DIB *fill = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_HP_FILL);
    if (fill)
    {
        double frac = state->playerMaxHP > 0 ? (double)state->playerHP / (double)state->playerMaxHP : 0.0;
        if (frac < 0.0)
            frac = 0.0;
        if (frac > 1.0)
            frac = 1.0;
        long fillWidth = (long)(fill->width * frac);
        if (fillWidth > 0)
        {
            
            RKC_DIB_TransferToDIBEx(&state->canvas, HUD_HP_TUBE_OFFSET_X, bgY + HUD_HP_TUBE_OFFSET_Y, fillWidth,
                                    fill->height, fill, 0, 0, -1, 1000);
            DrawTubeSparkle(state, sparkle, HUD_HP_TUBE_OFFSET_X, bgY + HUD_HP_TUBE_OFFSET_Y, fillWidth,
                            fill->height);
        }
    }

    
    RKC_DIB *mpFill = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_MP_FILL);
    if (mpFill)
    {
        double frac = state->playerMaxMP > 0 ? (double)state->playerMP / (double)state->playerMaxMP : 0.0;
        if (frac < 0.0)
            frac = 0.0;
        if (frac > 1.0)
            frac = 1.0;
        long fillWidth = (long)(mpFill->width * frac);
        if (fillWidth > 0)
        {
            RKC_DIB_TransferToDIBEx(&state->canvas, HUD_MP_TUBE_OFFSET_X, bgY + HUD_MP_TUBE_OFFSET_Y, fillWidth,
                                    mpFill->height, mpFill, 0, 0, -1, 1000);
            DrawTubeSparkle(state, sparkle, HUD_MP_TUBE_OFFSET_X, bgY + HUD_MP_TUBE_OFFSET_Y, fillWidth,
                            mpFill->height);
        }
    }

    
    RKC_DIB *partnerGradient = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_PARTNER_HP_GRADIENT);
    if (partnerGradient)
        RKC_DIB_TransferToDIBEx(&state->canvas, HUD_PARTNER_HP_X, HUD_PARTNER_HP_Y, partnerGradient->width,
                                partnerGradient->height, partnerGradient, 0, 0, 0, 1000);
    RKC_DIB *partnerLabel = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_PARTNER_HP_LABEL);
    if (partnerLabel)
        RKC_DIB_TransferToDIBEx(&state->canvas, HUD_PARTNER_HP_X, HUD_PARTNER_HP_Y, partnerLabel->width,
                                partnerLabel->height, partnerLabel, 0, 0, 0, 1000);
    RKC_DIB *partnerTag = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_PARTNER_HP_INACTIVE);
    if (partnerTag)
        RKC_DIB_TransferToDIBEx(&state->canvas, HUD_PARTNER_HP_TAG_X, HUD_PARTNER_HP_TAG_Y, partnerTag->width,
                                partnerTag->height, partnerTag, 0, 0, 0, 1000);

    
    
    long rowX, rowWidth;
    if (ComputeMagicSlotBarRect(state, &rowX, &rowWidth))
    {
        RKC_DIB *emptySlot = RKC_UPDIB_GetFrame(&state->magicBarIcon, HUD_BAR_FRAME_MAGIC_SLOT_EMPTY);
        long selX = 0; 
        const RKC_DIB *selFrame = NULL;
        
        for (int s = 0; s < MAGIC_BAR_SLOT_COUNT; s++)
        {
            long groupGap = s >= MAGIC_SLOT_GROUP_SIZE ? MAGIC_SLOT_GROUP_GAP : 0;
            long slotX = rowX + s * MAGIC_SLOT_SIZE + groupGap;
            RKC_DIB *icon =
                state->magicBarSlot[s] >= 0
                    ? RKC_UPDIB_GetFrame(&state->magicBarIcon, MAGIC_ICON_FRAME_BASE + state->magicBarSlot[s])
                    : emptySlot;
            if (state->selectedMagicSlot == s)
            {
                selX = slotX;
                selFrame = icon;
                continue;
            }
            if (icon)
                RKC_DIB_TransferToDIBEx(&state->canvas, slotX, MAGIC_SLOT_ROW_Y, icon->width, icon->height, icon, 0,
                                        0, -1, 1000);
        }
        
        long attackX =
            rowX + MAGIC_BAR_SLOT_COUNT * MAGIC_SLOT_SIZE + MAGIC_SLOT_GROUP_GAP + MAGIC_SLOT_ATTACK_GAP;
        RKC_DIB *attackSlot = RKC_UPDIB_GetFrame(&state->magicBarIcon, HUD_BAR_FRAME_MAGIC_SLOT_ATTACK);
        if (state->selectedMagicSlot == MAGIC_SLOT_ATTACK_INDEX)
        {
            selX = attackX;
            selFrame = attackSlot;
        }
        else if (attackSlot)
            RKC_DIB_TransferToDIBEx(&state->canvas, attackX, MAGIC_SLOT_ROW_Y, attackSlot->width,
                                    attackSlot->height, attackSlot, 0, 0, -1, 1000);
        
        if (selFrame)
        {
            long grow = MAGIC_SLOT_SELECTED_SIZE - MAGIC_SLOT_SIZE;
            DrawScaledFrame(&state->canvas, selX - grow / 2, MAGIC_SLOT_ROW_Y - grow / 2,
                            MAGIC_SLOT_SELECTED_SIZE, MAGIC_SLOT_SELECTED_SIZE, selFrame, -1);
        }
    }

    
    {
        int running = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL] || state->runToggled;
        long inactiveRowY = running ? HUD_WALK_ROW_Y : HUD_RUN_ROW_Y;
        
        long inactiveRowX = HUD_RUNWALK_ROW_X + (running ? HUD_RUNWALK_WALK_X_OFFSET : 0);
        RKC_DIB *inactiveIcon = RKC_UPDIB_GetFrame(
            &state->hudBar, running ? HUD_BAR_FRAME_WALK_INACTIVE : HUD_BAR_FRAME_RUN_INACTIVE);
        if (inactiveIcon)
            RKC_DIB_TransferToDIBEx(&state->canvas, inactiveRowX, bgY + inactiveRowY, inactiveIcon->width,
                                    inactiveIcon->height, inactiveIcon, 0, 0, -1, 1000);
    }
}


void DrawHudOverlay(DemoState *state)
{
    if (!state->hudBarLoaded)
        return;

    RKC_DIB *bg = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_BACKGROUND);
    if (!bg)
        return;
    long bgY = APP_HEIGHT - bg->height;

    
    RKC_DIB *topTrim = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_TOP_TRIM);
    long topTrimY = bgY;
    if (topTrim)
    {
        topTrimY = bgY - topTrim->height;
        RKC_DIB_TransferToDIBEx(&state->canvas, 0, topTrimY, topTrim->width, topTrim->height, topTrim, 0, 0, 0,
                                1000);
    }
    RKC_DIB *menuButton = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_MENU);
    if (menuButton)
        RKC_DIB_TransferToDIBEx(&state->canvas, HUD_MENU_X, topTrimY - menuButton->height, menuButton->width,
                                menuButton->height, menuButton, 0, 0, 0, 1000);

    
    RKC_DIB *expIndicator = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_EXP);
    if (expIndicator)
    {
        
        RKC_DIB_TransferToDIBEx(&state->canvas, HUD_EXP_X, HUD_EXP_Y, expIndicator->width, expIndicator->height,
                                expIndicator, 0, 0, 0, 1000);
        
        if (state->progressionInitialized && state->tableLoaded)
        {
            const RKC_RPG_TABLEDATA *xp = RKC_RPG_TABLE_GetFromTableNo(&state->table, TABLE_XP_CURVE);
            long threshold = (xp && state->playerLevel >= 1 && state->playerLevel <= PLAYER_LEVEL_CAP)
                                 ? RKC_RPG_TABLEDATA_GetValue(xp, state->playerLevel - 1, 0)
                                 : 0;
            double frac = threshold > 0 ? (double)state->playerExp / (double)threshold : 0.0;
            if (frac < 0.0)
                frac = 0.0;
            if (frac > 1.0)
                frac = 1.0;
            long fillW = (long)((HUD_EXP_TRACK_END - HUD_EXP_TRACK_START) * frac);
            for (long x = HUD_EXP_TRACK_START; x < HUD_EXP_TRACK_START + fillW && x <= HUD_EXP_TRACK_END; x++)
            {
                long top, bot;
                if (x < 66)
                {
                    top = 9;
                    bot = 11;
                }
                else if (x <= 72)
                {
                    top = 8 - (x - 66);
                    bot = 11 - (x - 66);
                }
                else
                {
                    top = 2;
                    bot = 5;
                }
                DrawRectAlpha(&state->canvas, HUD_EXP_X + x, HUD_EXP_Y + top, 1, bot - top + 1, 1000, 200, 110, 50);
            }
        }
    }
}

static const char *PlayerClassName(long cls)
{
    switch (cls)
    {
    case PLAYER_CLASS_HUNTER:
        return "Hunter";
    case PLAYER_CLASS_WARRIOR:
        return "Warrior";
    case PLAYER_CLASS_WITCH:
        return "Witch";
    case PLAYER_CLASS_MERCENARY:
        return "Mercenary";
    default:
        return "";
    }
}


void DrawLevelUpNotice(DemoState *state)
{
    if (!state->levelUpNoticeActive || !state->fontLoaded)
        return;
    long elapsed = state->tick - state->levelUpNoticeArmedTick;
    if (elapsed < 0 || elapsed >= LEVELUP_NOTICE_TOTAL_TICKS)
    {
        state->levelUpNoticeActive = 0;
        return;
    }

    
    static const struct
    {
        int row;
        const char *name;
    } DISPLAY[] = {
        {PLAYER_STAT_MAX_HP, "HP"},
        {PLAYER_STAT_MAX_MP, "MP"},
        {PLAYER_STAT_STRENGTH, "Strength"},
        {PLAYER_STAT_ATTACK, "Attack"},
        {PLAYER_STAT_DEFENCE, "Defense"},
        {PLAYER_STAT_MAGIC_ATTACK, "Magical Attack"},
        {PLAYER_STAT_MAGIC_DEFENCE, "Magical Defense"},
        {PLAYER_STAT_HIT_RATE, "Hit Rate"},
        {PLAYER_STAT_EVASION, "Evasion"},
        {PLAYER_STAT_MAGIC_HIT, "Magical Hit"},
        {PLAYER_STAT_MAGIC_EVASION, "Magical Evasion"},
        {PLAYER_STAT_ATK_SPEED, "Attack Speed"},
        {PLAYER_STAT_WALK_SPEED, "Walk Speed"},
    };
    const int DISPLAY_COUNT = (int)(sizeof(DISPLAY) / sizeof(DISPLAY[0]));
    int nGains = 0;
    for (int i = 0; i < DISPLAY_COUNT; i++)
        if (state->levelUpNoticeGains[DISPLAY[i].row] != 0)
            nGains++;
    int hasSkill = state->levelUpNoticeSkill[0] != '\0';
    int hasClass = state->levelUpNoticeNewClass != 0;

    const long LINE_H = 14, PAD = 10, PANEL_W = 216;
    int extra = (hasSkill || hasClass) ? 1 : 0; 
    int lineCount = 1  + 1  + nGains + extra + (hasSkill ? 1 : 0) + (hasClass ? 1 : 0);
    long panelH = PAD * 2 + lineCount * LINE_H;

    
    long cx = (APP_WIDTH - PANEL_W) / 2, cy = (APP_HEIGHT - panelH) / 2 - 40;
    long tx = APP_WIDTH - PANEL_W - 8, ty = 8;
    long px, py;
    if (elapsed < LEVELUP_NOTICE_CENTER_TICKS)
    {
        px = cx;
        py = cy;
    }
    else if (elapsed < LEVELUP_NOTICE_CENTER_TICKS + LEVELUP_NOTICE_SLIDE_TICKS)
    {
        double t = (double)(elapsed - LEVELUP_NOTICE_CENTER_TICKS) / LEVELUP_NOTICE_SLIDE_TICKS;
        px = cx + (long)((tx - cx) * t);
        py = cy + (long)((ty - cy) * t);
    }
    else
    {
        px = tx;
        py = ty;
    }

    
    long fade = 1000;
    if (elapsed < 12)
        fade = 1000 * elapsed / 12;
    else if (elapsed > LEVELUP_NOTICE_TOTAL_TICKS - 30)
        fade = 1000 * (LEVELUP_NOTICE_TOTAL_TICKS - elapsed) / 30;
    if (fade < 0)
        fade = 0;

    
    DrawRectAlpha(&state->canvas, px, py, PANEL_W, panelH, LEVELUP_NOTICE_PANEL_ALPHA * fade / 1000, 12, 10, 8);
    DrawRectOutlineAlpha(&state->canvas, px, py, PANEL_W, panelH, 1, LEVELUP_NOTICE_BORDER_ALPHA * fade / 1000, 150,
                         150, 150);
    long textTrans = fade;

    long innerX = px + PAD, innerRight = px + PANEL_W - PAD, y = py + PAD;
    char buf[64];
    
    DrawTextSJIS(state, innerX, y, buf, 0, TEXT_STYLE_WHITE_SHADOWED, textTrans);
    y += LINE_H * 2; 
    for (int i = 0; i < DISPLAY_COUNT; i++)
    {
        long g = state->levelUpNoticeGains[DISPLAY[i].row];
        if (g == 0)
            continue;
        DrawTextSJIS(state, innerX, y, DISPLAY[i].name, 0, TEXT_STYLE_WHITE_SHADOWED, textTrans);
        DrawTextSJIS(state, innerRight - 40, y, "+", 0, TEXT_STYLE_WHITE_SHADOWED, textTrans);
        
        DrawTextSJIS(state, innerRight - MeasureTextSJIS(state, buf), y, buf, 0, TEXT_STYLE_WHITE_SHADOWED, textTrans);
        y += LINE_H;
    }
    if (extra)
        y += LINE_H;
    if (hasSkill)
    {
        
        DrawTextSJIS(state, innerX, y, buf, 0, TEXT_STYLE_WHITE_SHADOWED, textTrans);
        y += LINE_H;
    }
    if (hasClass)
    {
        
        DrawTextSJIS(state, innerX, y, buf, 0, TEXT_STYLE_WHITE_SHADOWED, textTrans);
    }
}


void DrawCompass(DemoState *state)
{
    if (!state->compassActive)
        return;

    
    long settledTick = (long)state->compassActivatedTick + 7 * COMPASS_TICKS_PER_ANIM_FRAME;
    long sinceSettled = (long)state->tick - settledTick;
    long fadeAlpha = 1000;
    if (sinceSettled > COMPASS_HOLD_TICKS)
    {
        long fadeElapsed = sinceSettled - COMPASS_HOLD_TICKS;
        fadeAlpha = fadeElapsed >= COMPASS_FADE_TICKS ? 0 : 1000 - (fadeElapsed * 1000) / COMPASS_FADE_TICKS;
    }

    if (state->compassTemplate.kind == LIVE_SPAWN_SPRITE_CAF && fadeAlpha > 0)
    {
        long frame = (long)((state->tick - state->compassActivatedTick) / COMPASS_TICKS_PER_ANIM_FRAME);
        if (frame > 7)
            frame = 7;
        if (frame < 0)
            frame = 0;

        RKC_RPGSCRN_CAF_DrawCmd cmds[8];
        int n = RKC_RPGSCRN_CAF_Resolve(&state->compassTemplate.caf, 0, 8, frame, &state->compassTemplate.animNjp,
                                        &state->compassTemplate.animSdw, NULL, NULL, NULL, NULL, 0, cmds, 8);
        for (int i = 0; i < n; i++)
        {
            const RKC_DIB *icon = cmds[i].icon;
            
            long destX = COMPASS_CENTER_X - icon->width / 2;
            long destY = COMPASS_CENTER_Y - icon->height / 2;
            long scaledTrans = cmds[i].trans * fadeAlpha / 1000;
            
            if (cmds[i].isAdditive)
                RKC_DIB_TransferToDIBAdditive(&state->canvas, destX, destY, icon->width, icon->height, icon, 0, 0, 0,
                                              scaledTrans);
            else
                RKC_DIB_TransferToDIBEx(&state->canvas, destX, destY, icon->width, icon->height, icon, 0, 0, 0,
                                        scaledTrans);
        }
    }

    
    if (state->compassText[0] != '\0' && fadeAlpha > 0)
        DrawTextSJIS(state, COMPASS_TEXT_X, COMPASS_TEXT_Y, state->compassText, 0, TEXT_STYLE_WHITE_SHADOWED,
                     fadeAlpha);
}


void DrawFloatingValues(DemoState *state)
{
    if (!state->fontLoaded)
        return;
    for (int i = 0; i < state->floatingValueCount; i++)
    {
        long wx, wy;
        if (!LookupLiveSpawnPos(state, state->floatingValues[i].characterNo, &wx, &wy))
            continue;
        long sx, sy;
        WorldToScreen(state, wx, wy, &sx, &sy);
        long x = sx - state->cameraX + state->floatingValues[i].offsetX;
        long y = sy - state->cameraY + state->floatingValues[i].offsetY;

        char buf[16];
        
        long len = (long)strlen(buf);
        x -= len * 3; 

        unsigned char r = (unsigned char)(state->floatingValues[i].r & 0xff);
        unsigned char g = (unsigned char)(state->floatingValues[i].g & 0xff);
        unsigned char b = (unsigned char)(state->floatingValues[i].b & 0xff);
        const unsigned char *cursor = (const unsigned char *)buf;
        long cursorX = 0;
        long frame, srcX, srcY, w;
        while (NextSjisGlyph(&cursor, &frame, &srcX, &srcY, &w))
        {
            if (w <= 0)
                continue;
            RKC_DIB *sheet = RKC_UPDIB_GetFrame(&state->font, frame);
            if (sheet)
            {
                StampGlyphSolid(&state->canvas, sheet, x + cursorX + 1, y - 11, srcX, srcY, w, FONT_CELL_HEIGHT, 0, 0,
                                0, 1000);
                StampGlyphSolid(&state->canvas, sheet, x + cursorX, y - 12, srcX, srcY, w, FONT_CELL_HEIGHT, b, g, r,
                                1000);
            }
            cursorX += w;
        }
    }
}


void DrawUnlockSwBubble(DemoState *state)
{
    if (state->unlockSwValue == 0 || state->tick - state->unlockSwSetTick > 1 ||
        state->unlockSwTemplate.kind != LIVE_SPAWN_SPRITE_CAF)
        return;

    long sx, sy;
    WorldToScreen(state, state->playerX, state->playerY, &sx, &sy);
    long destX = sx - state->cameraX;
    long destY = sy - state->cameraY;

    long frame = (long)(state->tick / TICKS_PER_ANIM_FRAME);
    RKC_RPGSCRN_CAF_DrawCmd cmds[8];
    int n = RKC_RPGSCRN_CAF_Resolve(&state->unlockSwTemplate.caf, 0, RKC_RPGSCRN_CAF_NUM_DIRECTIONS - 1, frame,
                                    &state->unlockSwTemplate.animNjp, &state->unlockSwTemplate.animSdw, NULL, NULL,
                                    NULL, NULL, 0, cmds, 8);
    for (int i = 0; i < n; i++)
    {
        const RKC_DIB *icon = cmds[i].icon;
        if (cmds[i].isAdditive)
            RKC_DIB_TransferToDIBAdditive(&state->canvas, destX + cmds[i].offsetX, destY + cmds[i].offsetY, icon->width,
                                          icon->height, icon, 0, 0, 0, cmds[i].trans);
        else
            RKC_DIB_TransferToDIBEx(&state->canvas, destX + cmds[i].offsetX, destY + cmds[i].offsetY, icon->width,
                                    icon->height, icon, 0, 0, 0, cmds[i].trans);
    }
}


void DrawScriptEffects(DemoState *state)
{
    for (int i = 0; i < SCRIPT_EFFECT_MAX; i++)
    {
        ScriptEffect *fx = &state->scriptEffects[i];
        if (!fx->active)
            continue;
        LiveSpawnTemplate *tmpl = &state->scriptEffectTemplates[fx->templateIndex];
        if (tmpl->kind != LIVE_SPAWN_SPRITE_CAF || tmpl->caf.chartCount < 1)
        {
            fx->active = 0;
            continue;
        }
        const RKC_RPGSCRN_CAF_Direction *dir = &tmpl->caf.charts[0].directions[fx->direction];
        long frame = (long)((state->tick - fx->spawnTick) / TICKS_PER_ANIM_FRAME);
        if (dir->maxFrameCount <= 0 || frame >= dir->maxFrameCount)
        {
            fx->active = 0; 
            continue;
        }

        long wx = fx->x, wy = fx->y;
        if (fx->followCharacterNo >= 0)
            LookupLiveSpawnPos(state, fx->followCharacterNo, &wx, &wy); 
        long sx, sy;
        WorldToScreen(state, wx, wy, &sx, &sy);
        long destX = sx - state->cameraX;
        long destY = sy - state->cameraY;

        RKC_RPGSCRN_CAF_DrawCmd cmds[8];
        int n = RKC_RPGSCRN_CAF_Resolve(&tmpl->caf, 0, fx->direction, frame, &tmpl->animNjp, &tmpl->animSdw, NULL,
                                        NULL, NULL, NULL, 0, cmds, 8);
        for (int c = 0; c < n; c++)
        {
            const RKC_DIB *icon = cmds[c].icon;
            if (cmds[c].isAdditive)
                RKC_DIB_TransferToDIBAdditive(&state->canvas, destX + cmds[c].offsetX, destY + cmds[c].offsetY,
                                              icon->width, icon->height, icon, 0, 0, 0, cmds[c].trans);
            else
                RKC_DIB_TransferToDIBEx(&state->canvas, destX + cmds[c].offsetX, destY + cmds[c].offsetY, icon->width,
                                        icon->height, icon, 0, 0, 0, cmds[c].trans);
        }
    }
}


void DrawHitVfx(DemoState *state, long centerX, long centerY, unsigned long vfxTick, int variant)
{
    if (vfxTick == 0 || variant < 0 || variant >= HIT_VFX_VARIANT_COUNT)
        return;
    LiveSpawnTemplate *tmpl = &state->hitVfxTemplates[variant];
    if (tmpl->kind != LIVE_SPAWN_SPRITE_CAF || tmpl->caf.chartCount < 1)
        return;

    
    short frameCount = tmpl->caf.charts[0].directions[8].maxFrameCount;
    if (frameCount <= 0)
        return;

    long elapsedTicks = (long)(state->tick - vfxTick);
    long durationTicks = (long)frameCount * HIT_VFX_TICKS_PER_FRAME;
    if (elapsedTicks < 0 || elapsedTicks >= durationTicks)
        return;

    long frame = elapsedTicks / HIT_VFX_TICKS_PER_FRAME;
    if (frame >= frameCount)
        frame = frameCount - 1;

    RKC_RPGSCRN_CAF_DrawCmd cmds[8];
    int n = RKC_RPGSCRN_CAF_Resolve(&tmpl->caf, 0, 8, frame, &tmpl->animNjp, &tmpl->animSdw, NULL, NULL, NULL, NULL,
                                    0, cmds, 8);
    for (int i = 0; i < n; i++)
    {
        const RKC_DIB *icon = cmds[i].icon;
        long destX = centerX - icon->width / 2;
        long destY = centerY - icon->height / 2;
        RKC_DIB_TransferToDIBEx(&state->canvas, destX, destY, icon->width, icon->height, icon, 0, 0, 0, cmds[i].trans);
    }
}


void SpawnBloodDecal(DemoState *state, long x, long y)
{
    if (!state->bloodDecalSheetLoaded)
        return;
    int frameCount = RKC_UPDIB_GetFrameCount(&state->bloodDecalSheet);
    if (frameCount <= 0)
        return;

    BloodDecal *decal = &state->bloodDecals[state->bloodDecalNextSlot];
    state->bloodDecalNextSlot = (state->bloodDecalNextSlot + 1) % BLOOD_DECAL_MAX;
    decal->x = x;
    decal->y = y;
    decal->variant = rand() % frameCount;
    decal->spawnTick = state->tick;
    decal->active = 1;
}


void DrawBloodDecals(DemoState *state)
{
    if (!state->bloodDecalSheetLoaded)
        return;

    for (int i = 0; i < BLOOD_DECAL_MAX; i++)
    {
        BloodDecal *decal = &state->bloodDecals[i];
        if (!decal->active)
            continue;

        long elapsedTicks = (long)(state->tick - decal->spawnTick);
        long fadeStartTicks = BLOOD_DECAL_FALL_TICKS + BLOOD_DECAL_HOLD_TICKS;
        long totalTicks = fadeStartTicks + BLOOD_DECAL_FADE_TICKS;
        if (elapsedTicks >= totalTicks)
        {
            decal->active = 0;
            continue;
        }

        long trans = 1000;
        if (elapsedTicks > fadeStartTicks)
            trans = 1000 - (elapsedTicks - fadeStartTicks) * 1000 / BLOOD_DECAL_FADE_TICKS;

        
        long fallOffsetY = 0;
        if (elapsedTicks < BLOOD_DECAL_FALL_TICKS)
            fallOffsetY = BLOOD_DECAL_FALL_START_OFFSET_Y * (BLOOD_DECAL_FALL_TICKS - elapsedTicks) /
                          BLOOD_DECAL_FALL_TICKS;

        RKC_DIB *icon = RKC_UPDIB_GetFrame(&state->bloodDecalSheet, decal->variant);
        if (!icon)
            continue;

        long screenX, screenY;
        WorldToScreen(state, decal->x, decal->y, &screenX, &screenY);
        long destX = screenX - state->cameraX - icon->width / 2;
        long destY = screenY - state->cameraY - icon->height / 2 - fallOffsetY;
        
        RKC_DIB_TransferToDIBTint(&state->canvas, destX, destY, icon->width, icon->height, icon, 0, 0, 0, trans,
                                  BLOOD_DECAL_BRIGHTEN_TINT, BLOOD_DECAL_BRIGHTEN_TINT, BLOOD_DECAL_BRIGHTEN_TINT);
    }
}


void DrawLoadingScreen(DemoState *state)
{
    RKC_DIB_FillByte(&state->canvas, 0);
    if (!state->waitingSheetLoaded)
        return;

    RKC_DIB *swords = RKC_UPDIB_GetFrame(&state->waitingSheet, WAITING_SHEET_FRAME_SWORDS);
    if (swords)
        RKC_DIB_TransferToDIBEx(&state->canvas, (APP_WIDTH - swords->width) / 2, (APP_HEIGHT - swords->height) / 2,
                                swords->width, swords->height, swords, 0, 0, 0, 1000);

    RKC_DIB *plate = RKC_UPDIB_GetFrame(&state->waitingSheet, WAITING_SHEET_FRAME_PLATE);
    if (plate)
        RKC_DIB_TransferToDIBEx(&state->canvas, APP_WIDTH - LOADING_PLATE_MARGIN_X - plate->width,
                                APP_HEIGHT - LOADING_PLATE_MARGIN_Y - plate->height, plate->width, plate->height,
                                plate, 0, 0, -1, 1000);
}


static void PlotPixel(RKC_DIB *canvas, long x, long y, unsigned char b, unsigned char g, unsigned char r)
{
    if (x < 0 || x >= canvas->width || y < 0 || y >= canvas->height)
        return;
    unsigned char *row = canvas->pixels + (size_t)(canvas->height - 1 - y) * (size_t)canvas->alignWidth;
    unsigned char *px = row + x * 3;
    px[0] = b;
    px[1] = g;
    px[2] = r;
}


static void SamplePixelAt(const RKC_DIB *icon, long sx, long sy, unsigned char *outB, unsigned char *outG,
                          unsigned char *outR)
{
    const unsigned char *row = icon->pixels + (size_t)(icon->height - 1 - sy) * (size_t)icon->alignWidth;
    if (icon->bpp == 24)
    {
        const unsigned char *px = row + sx * 3;
        *outB = px[0];
        *outG = px[1];
        *outR = px[2];
        return;
    }
    unsigned index = icon->bpp == 8 ? row[sx]
                                    : ((sx & 1) == 0 ? (unsigned)(row[sx >> 1] >> 4) : (unsigned)(row[sx >> 1] & 0x0F));
    const unsigned char *color = icon->palette + (size_t)index * 4;
    *outB = color[0];
    *outG = color[1];
    *outR = color[2];
}


static void SampleIconColor(const RKC_DIB *icon, unsigned char *outB, unsigned char *outG, unsigned char *outR)
{
    SamplePixelAt(icon, icon->width / 2, icon->height / 2, outB, outG, outR);
}


static void DrawIconScaled(RKC_DIB *canvas, const RKC_DIB *icon, long dstX, long dstY, long maxW, long maxH,
                           long colorKey, long opacity, long tintR, long tintG, long tintB)
{
    if (!icon || icon->width <= 0 || icon->height <= 0 || maxW <= 0 || maxH <= 0)
        return;
    double scale =
        icon->width * maxH > icon->height * maxW ? (double)maxW / icon->width : (double)maxH / icon->height;
    if (scale > 1.0)
        scale = 1.0;
    long drawW = (long)(icon->width * scale);
    long drawH = (long)(icon->height * scale);
    if (drawW < 1)
        drawW = 1;
    if (drawH < 1)
        drawH = 1;
    long originX = dstX + (maxW - drawW) / 2;
    long originY = dstY + (maxH - drawH) / 2;
    for (long y = 0; y < drawH; y++)
    {
        long sy = y * icon->height / drawH;
        for (long x = 0; x < drawW; x++)
        {
            long sx = x * icon->width / drawW;
            if (colorKey >= 0 && RKC_DIB_GetPixelIndex(icon, sx, sy) == colorKey)
                continue;
            unsigned char b, g, r;
            SamplePixelAt(icon, sx, sy, &b, &g, &r);
            if (tintB != 1000)
                b = RKC_DIB_TintChannel(b, tintB);
            if (tintG != 1000)
                g = RKC_DIB_TintChannel(g, tintG);
            if (tintR != 1000)
                r = RKC_DIB_TintChannel(r, tintR);
            long px = originX + x, py = originY + y;
            if (px < 0 || px >= canvas->width || py < 0 || py >= canvas->height)
                continue;
            unsigned char *dst = canvas->pixels + (size_t)(canvas->height - 1 - py) * (size_t)canvas->alignWidth +
                                 px * 3;
            dst[0] = (unsigned char)(dst[0] + ((long)b - dst[0]) * opacity / 1000);
            dst[1] = (unsigned char)(dst[1] + ((long)g - dst[1]) * opacity / 1000);
            dst[2] = (unsigned char)(dst[2] + ((long)r - dst[2]) * opacity / 1000);
        }
    }
}


#define GROUND_ICON_SAT_PIXEL_THRESHOLD 60
#define GROUND_ICON_COLORED_PCT 25

static int GroundIconIsAlreadyColored(const RKC_DIB *icon)
{
    if (!icon || icon->width <= 0 || icon->height <= 0)
        return 0;
    long opaque = 0, saturated = 0;
    for (long y = 0; y < icon->height; y++)
        for (long x = 0; x < icon->width; x++)
        {
            
            if (RKC_DIB_GetPixelIndex(icon, x, y) == 0)
                continue;
            unsigned char b, g, r;
            SamplePixelAt(icon, x, y, &b, &g, &r);
            long mx = r > g ? r : g;
            mx = mx > b ? mx : b;
            long mn = r < g ? r : g;
            mn = mn < b ? mn : b;
            opaque++;
            if (mx - mn > GROUND_ICON_SAT_PIXEL_THRESHOLD)
                saturated++;
        }
    return opaque > 0 && (saturated * 100 / opaque) >= GROUND_ICON_COLORED_PCT;
}


static void DrawIconColorized(RKC_DIB *canvas, const RKC_DIB *icon, long dstX, long dstY, long maxW, long maxH,
                              long targetR, long targetG, long targetB)
{
    if (!icon || icon->width <= 0 || icon->height <= 0 || maxW <= 0 || maxH <= 0)
        return;
    double scale = icon->width * maxH > icon->height * maxW ? (double)maxW / icon->width : (double)maxH / icon->height;
    if (scale > 1.0)
        scale = 1.0;
    long drawW = (long)(icon->width * scale);
    long drawH = (long)(icon->height * scale);
    if (drawW < 1)
        drawW = 1;
    if (drawH < 1)
        drawH = 1;
    long originX = dstX + (maxW - drawW) / 2;
    long originY = dstY + (maxH - drawH) / 2;
    for (long y = 0; y < drawH; y++)
    {
        long sy = y * icon->height / drawH;
        for (long x = 0; x < drawW; x++)
        {
            long sx = x * icon->width / drawW;
            if (RKC_DIB_GetPixelIndex(icon, sx, sy) == 0)
                continue;
            unsigned char b, g, r;
            SamplePixelAt(icon, sx, sy, &b, &g, &r);
            long lum = (r * 30 + g * 59 + b * 11) / 100; 
            long oR, oG, oB;
            if (lum < 128)
            {
                oR = targetR * lum / 128;
                oG = targetG * lum / 128;
                oB = targetB * lum / 128;
            }
            else
            {
                long t = lum - 128;
                oR = targetR + (255 - targetR) * t / 127;
                oG = targetG + (255 - targetG) * t / 127;
                oB = targetB + (255 - targetB) * t / 127;
            }
            long px = originX + x, py = originY + y;
            if (px < 0 || px >= canvas->width || py < 0 || py >= canvas->height)
                continue;
            unsigned char *dst =
                canvas->pixels + (size_t)(canvas->height - 1 - py) * (size_t)canvas->alignWidth + px * 3;
            dst[0] = (unsigned char)(oB < 0 ? 0 : oB > 255 ? 255 : oB);
            dst[1] = (unsigned char)(oG < 0 ? 0 : oG > 255 ? 255 : oG);
            dst[2] = (unsigned char)(oR < 0 ? 0 : oR > 255 ? 255 : oR);
        }
    }
}


static long ResolveFallbackItemIconFrame(const DemoState *state, long kind, long templateId)
{
    if (kind == 0 && state->itemDataLoaded)
    {
        const RKC_RPG_ITEMDATA_Record *rec = RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, 0, templateId);
        const RKC_RPG_ITEMDATA_Kind0Tail *tail = rec ? RKC_RPG_ITEMDATA_GetKind0Tail(rec) : NULL;
        if (tail)
        {
            switch (tail->weaponClass)
            {
            case 1:
                return ITEM_ICON_FRAME_ONE_HANDED; 
            case 3:
                return ITEM_ICON_FRAME_TWO_HANDED;
            case 5:
                return ITEM_ICON_FRAME_RANGED;
            case 8:
                return ITEM_ICON_FRAME_STAFF;
            case 9:
                return ITEM_ICON_FRAME_ROD;
            default:
                return ITEM_ICON_FRAME_ONE_HANDED; 
            }
        }
        return ITEM_ICON_FRAME_ONE_HANDED;
    }
    switch (kind)
    {
    case 1:
    {
        
        const RKC_RPG_ITEMDATA_Record *rec =
            state->itemDataLoaded ? RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, 1, templateId) : NULL;
        if (rec && rec->name)
        {
            if (ArmorNameFitsSlot(rec->name, EQUIPMENT_SHIELD_SLOT_INDEX))
                return ITEM_ICON_FRAME_SHIELD;
            if (ArmorNameFitsSlot(rec->name, EQUIPMENT_HELMET_SLOT_INDEX))
                return ITEM_ICON_FRAME_HELMET;
            if (ArmorNameFitsSlot(rec->name, EQUIPMENT_BOOTS_SLOT_INDEX))
                return ITEM_ICON_FRAME_BOOTS;
        }
        return ITEM_ICON_FRAME_ARMOR;
    }
    case 2:
        return ITEM_ICON_FRAME_ACCESSORY;
    case 3:
        return ITEM_ICON_FRAME_TABLET;
    default:
        return ITEM_ICON_FRAME_MISC;
    }
}


static void ResolveItemIcon(const DemoState *state, long kind, long templateId, long *outSheet, long *outFrame,
                            long *outTintR, long *outTintG, long *outTintB)
{
    long iconSheet = -1, iconFrame = -1;
    long weaponGripTintR = 1000, weaponGripTintG = 1000, weaponGripTintB = 1000;
    *outTintR = *outTintG = *outTintB = 1000;
    if (state->itemDataLoaded && kind >= 0)
    {
        const RKC_RPG_ITEMDATA_Record *rec = RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, (int)kind, templateId);
        if (rec)
        {
            
            long sheet = -1, pattern = -1;
            if (RKC_RPG_ITEMDATA_GetBagIcon(rec, &sheet, &pattern) && sheet >= 0 && sheet < ITEM_ICON_SHEET_COUNT &&
                state->itemIconSheetLoaded[sheet] && pattern >= 0 && pattern < state->itemIconSheets[sheet].patternCount)
            {
                iconSheet = sheet;
                iconFrame = state->itemIconSheets[sheet].patternFrame[pattern];
            }
            if (kind == 0)
            {
                const RKC_RPG_ITEMDATA_Kind0Tail *tail = RKC_RPG_ITEMDATA_GetKind0Tail(rec);
                if (tail)
                {
                    weaponGripTintR = (long)tail->weaponGripTintR;
                    weaponGripTintG = (long)tail->weaponGripTintG;
                    weaponGripTintB = (long)tail->weaponGripTintB;
                }
            }
        }
    }
    if (iconSheet >= 0 && iconSheet < ITEM_ICON_SHEET_COUNT && state->itemIconSheetLoaded[iconSheet] &&
        iconFrame >= 0 && iconFrame < RKC_UPDIB_GetFrameCount(&state->itemIconSheets[iconSheet]))
    {
        *outSheet = iconSheet;
        *outFrame = iconFrame;
        *outTintR = *outTintG = *outTintB = 1000;
        return;
    }
    *outSheet = 0;
    *outFrame = ResolveFallbackItemIconFrame(state, kind, templateId);
    if (kind == 0)
    {
        *outTintR = weaponGripTintR;
        *outTintG = weaponGripTintG;
        *outTintB = weaponGripTintB;
    }
}


static void DrawIconByFrame(DemoState *state, long sheetIndex, long frame, long dstX, long dstY, long maxW,
                            long maxH, long opacity, long tintR, long tintG, long tintB)
{
    if (sheetIndex < 0 || sheetIndex >= ITEM_ICON_SHEET_COUNT || !state->itemIconSheetLoaded[sheetIndex])
        return;
    RKC_DIB *icon = RKC_UPDIB_GetFrame(&state->itemIconSheets[sheetIndex], frame);
    if (icon)
        DrawIconScaled(&state->canvas, icon, dstX, dstY, maxW, maxH, 0, opacity, tintR, tintG, tintB);
}


#define GOLD_PILE_ICON_SHEET 0
#define GOLD_PILE_ICON_FRAME 199

static void DrawGoldPileIcon(DemoState *state, long destX, long destY, long maxW, long maxH, int isHovered)
{
    long tintR = 1000, tintG = 1000, tintB = 1000;
    if (isHovered)
        tintR = tintG = tintB = HOVER_HIGHLIGHT_TINT;
    DrawIconByFrame(state, GOLD_PILE_ICON_SHEET, GOLD_PILE_ICON_FRAME, destX, destY, maxW, maxH, 1000, tintR, tintG,
                    tintB);
}


static void DrawItemIcon(DemoState *state, long kind, long templateId, long dstX, long dstY, long maxW, long maxH,
                         long opacity)
{
    if (kind == 4 && templateId == 0)
    {
        DrawGoldPileIcon(state, dstX, dstY, maxW, maxH, 0);
        return;
    }
    long sheetIndex, frame, tintR, tintG, tintB;
    ResolveItemIcon(state, kind, templateId, &sheetIndex, &frame, &tintR, &tintG, &tintB);
    DrawIconByFrame(state, sheetIndex, frame, dstX, dstY, maxW, maxH, opacity, tintR, tintG, tintB);
}


static void DrawImagePanned(RKC_DIB *canvas, const RKC_DIB *src, long dstX, long dstY, long clipX, long clipY,
                            long clipW, long clipH)
{
    long x0 = dstX > clipX ? dstX : clipX;
    long y0 = dstY > clipY ? dstY : clipY;
    long x1 = (dstX + src->width) < (clipX + clipW) ? (dstX + src->width) : (clipX + clipW);
    long y1 = (dstY + src->height) < (clipY + clipH) ? (dstY + src->height) : (clipY + clipH);
    for (long y = y0; y < y1; y++)
    {
        long sy = y - dstY;
        for (long x = x0; x < x1; x++)
        {
            long sx = x - dstX;
            unsigned char b, g, r;
            SamplePixelAt(src, sx, sy, &b, &g, &r);
            PlotPixel(canvas, x, y, b, g, r);
        }
    }
}


static void PlotPixelClipped(RKC_DIB *canvas, long x, long y, long clipX, long clipY, long clipW, long clipH,
                             unsigned char b, unsigned char g, unsigned char r)
{
    if (x < clipX || x >= clipX + clipW || y < clipY || y >= clipY + clipH)
        return;
    PlotPixel(canvas, x, y, b, g, r);
}


void DrawMinimap(DemoState *state)
{
    if (!state->minimapOpen)
        return;
    long areaW = state->ground.areaWidth, areaH = state->ground.areaHeight;
    if (areaW <= 0 || areaH <= 0)
        return;

    
    int realChrome = state->statusSheetLoaded;
    long contentX, contentY, contentW, contentH, anchorX, anchorY;
    if (realChrome)
    {
        contentX = MINIMAP_WINDOW_X + MINIMAP_WINDOW_CONTENT_X;
        contentY = MINIMAP_WINDOW_Y + MINIMAP_WINDOW_CONTENT_Y;
        contentW = MINIMAP_WINDOW_CONTENT_W;
        contentH = MINIMAP_WINDOW_CONTENT_H;
        anchorX = MINIMAP_WINDOW_X + MINIMAP_WINDOW_PLAYER_X;
        anchorY = MINIMAP_WINDOW_Y + MINIMAP_WINDOW_PLAYER_Y;
        
        DrawRect(&state->canvas, contentX, contentY, contentW, contentH, 0, 0, 0);
    }
    else
    {
        
        DrawRect(&state->canvas, MINIMAP_PANEL_X - 6, MINIMAP_PANEL_Y - 6, MINIMAP_PANEL_WIDTH + 12,
                 MINIMAP_PANEL_HEIGHT + 12, 40, 55, 70);
        DrawRect(&state->canvas, MINIMAP_PANEL_X, MINIMAP_PANEL_Y, MINIMAP_PANEL_WIDTH, MINIMAP_PANEL_HEIGHT, 0, 0, 0);
        contentX = MINIMAP_PANEL_X;
        contentY = MINIMAP_PANEL_Y + MINIMAP_CONTENT_Y_OFFSET;
        contentW = MINIMAP_PANEL_WIDTH;
        contentH = MINIMAP_PANEL_HEIGHT - MINIMAP_CONTENT_Y_OFFSET - 10;
        anchorX = contentX + contentW / 2;
        anchorY = contentY + contentH / 2;
    }

    
    long playerScreenX, playerScreenY;
    WorldToScreen(state, state->playerX, state->playerY, &playerScreenX, &playerScreenY);
    double playerMiniX = playerScreenX / 10.0, playerMiniY = playerScreenY / 10.0;
    double panX = anchorX - playerMiniX, panY = anchorY - playerMiniY;

    RKC_DIB *bgFrame = state->minimapBgLoaded ? RKC_UPDIB_GetFrame(&state->minimapBg, 0) : NULL;
    if (bgFrame)
    {
        
        long offsetX, offsetY;
        RKC_UPDIB_GetPatternOffset(&state->minimapBg, 0, &offsetX, &offsetY);
        long dstX = (long)panX + offsetX;
        long dstY = (long)panY + offsetY;
        DrawImagePanned(&state->canvas, bgFrame, dstX, dstY, contentX, contentY, contentW, contentH);
    }
    else
    {
        
        for (long y = 0; y < areaH; y++)
        {
            for (long x = 0; x < areaW; x++)
            {
                RKC_DIB *icon = RKC_RPGSCRN_GROUND_GetTileIcon(&state->ground, &state->patternSet, x, y);
                if (!icon)
                    continue;
                long sx, sy;
                RKC_RPGSCRN_GROUND_CellToScreen(&state->ground, x, y, &sx, &sy);
                long px = (long)(panX + sx / 10.0);
                long py = (long)(panY + sy / 10.0);
                unsigned char b, g, r;
                SampleIconColor(icon, &b, &g, &r);
                PlotPixelClipped(&state->canvas, px, py, contentX, contentY, contentW, contentH, b, g, r);
            }
        }

        long objCount = RKC_RPGSCRN_OBJECTBLOCK_GetCount(&state->objects);
        for (long i = 0; i < objCount; i++)
        {
            RKC_DIB *icon = RKC_RPGSCRN_OBJECTBLOCK_GetIcon(&state->objects, &state->patternSet, i);
            if (!icon)
                continue;
            const RKC_RPGSCRN_OBJECT_Entry *entry = RKC_RPGSCRN_OBJECTBLOCK_Get(&state->objects, i);
            long sx, sy;
            WorldToScreen(state, entry->posX, entry->posY, &sx, &sy);
            long px = (long)(panX + sx / 10.0);
            long py = (long)(panY + sy / 10.0);
            unsigned char b, g, r;
            SampleIconColor(icon, &b, &g, &r);
            
            PlotPixelClipped(&state->canvas, px, py, contentX, contentY, contentW, contentH, b, g, r);
            PlotPixelClipped(&state->canvas, px + 1, py, contentX, contentY, contentW, contentH, b, g, r);
        }
    }

    
    if (realChrome)
    {
        RKC_DIB *top = RKC_UPDIB_GetFrame(&state->statusSheet, MINIMAP_BG_FRAME_TOP);
        RKC_DIB *bottom = RKC_UPDIB_GetFrame(&state->statusSheet, MINIMAP_BG_FRAME_BOTTOM);
        RKC_DIB *left = RKC_UPDIB_GetFrame(&state->statusSheet, MINIMAP_BG_FRAME_LEFT);
        RKC_DIB *right = RKC_UPDIB_GetFrame(&state->statusSheet, MINIMAP_BG_FRAME_RIGHT);
        long sideY = MINIMAP_WINDOW_Y + (top ? top->height : MINIMAP_WINDOW_CONTENT_Y);
        if (top)
            RKC_DIB_TransferToDIBEx(&state->canvas, MINIMAP_WINDOW_X, MINIMAP_WINDOW_Y, top->width, top->height, top,
                                    0, 0, -1, 1000);
        if (bottom)
            RKC_DIB_TransferToDIBEx(&state->canvas, MINIMAP_WINDOW_X,
                                    MINIMAP_WINDOW_Y + MINIMAP_WINDOW_HEIGHT - bottom->height, bottom->width,
                                    bottom->height, bottom, 0, 0, -1, 1000);
        if (left)
            RKC_DIB_TransferToDIBEx(&state->canvas, MINIMAP_WINDOW_X, sideY, left->width, left->height, left, 0, 0,
                                    -1, 1000);
        if (right)
            RKC_DIB_TransferToDIBEx(&state->canvas, MINIMAP_WINDOW_X + MINIMAP_WINDOW_WIDTH - right->width, sideY,
                                    right->width, right->height, right, 0, 0, -1, 1000);
    }

    
    DrawMarker(&state->canvas, anchorX, anchorY, 2, 220, 120, 0);

    
    {
        long labelW = MeasureTextSJIS(state, "PLAYER");
        long labelY = anchorY - MINIMAP_PLAYER_LABEL_GAP_Y - FONT_CELL_HEIGHT;
        DrawTextSJIS(state, anchorX - labelW / 2, labelY, "PLAYER", 0, TEXT_STYLE_WHITE, 1000);
    }

    
    
    if (state->compassText[0] != '\0')
    {
        long nameRightEdge = realChrome ? (MINIMAP_WINDOW_X + MINIMAP_WINDOW_WIDTH - MINIMAP_WINDOW_CONTENT_X)
                                        : (MINIMAP_PANEL_X + MINIMAP_PANEL_WIDTH);
        DrawTextSJIS(state, MINIMAP_AREA_NAME_X, MINIMAP_AREA_NAME_Y, state->compassText,
                     nameRightEdge - MINIMAP_AREA_NAME_X - 10, TEXT_STYLE_WHITE, 1000);
    }
}


static long DrawDigits(RKC_DIB *canvas, RKC_UPDIB *hudBar, long x, long y, long value)
{
    char digits[24];
    int n = 0;
    if (value <= 0)
        digits[n++] = '0';
    else
        while (value > 0 && n < (int)sizeof(digits))
        {
            digits[n++] = (char)('0' + value % 10);
            value /= 10;
        }

    long cursorX = x;
    for (int i = n - 1; i >= 0; i--)
    {
        RKC_DIB *glyph = RKC_UPDIB_GetFrame(hudBar, HUD_BAR_FRAME_DIGIT0 + (digits[i] - '0'));
        if (!glyph)
            continue;
        RKC_DIB_TransferToDIBEx(canvas, cursorX, y, glyph->width, glyph->height, glyph, 0, 0, -1, 1000);
        cursorX += glyph->width + 1;
    }
    return cursorX - x;
}


static void DrawInventoryFallback(DemoState *state)
{
    
    DrawRect(&state->canvas, INVENTORY_PANEL_X - 6, INVENTORY_PANEL_Y - 6, INVENTORY_PANEL_WIDTH + 12,
             INVENTORY_PANEL_HEIGHT + 12, 40, 55, 70);
    DrawRect(&state->canvas, INVENTORY_PANEL_X, INVENTORY_PANEL_Y, INVENTORY_PANEL_WIDTH, INVENTORY_PANEL_HEIGHT, 20,
             20, 20);

    {
        
        long labelW =
            DrawTextSJIS(state, INVENTORY_PANEL_X + 10, INVENTORY_PANEL_Y + 8, "GOLD", 0, TEXT_STYLE_WHITE, 1000);
        if (state->hudBarLoaded)
            DrawDigits(&state->canvas, &state->hudBar, INVENTORY_PANEL_X + 10 + labelW + (labelW ? 6 : 0),
                       INVENTORY_PANEL_Y + 12, state->gold);
    }

    
    long equipX = INVENTORY_PANEL_X + INVENTORY_PANEL_WIDTH - 70;
    DrawRect(&state->canvas, equipX, INVENTORY_PANEL_Y + 10, 60, 70, state->hasWeapon ? 40 : 15,
             state->hasWeapon ? 90 : 15, state->hasWeapon ? 40 : 15);
    if (state->hasWeapon)
        DrawItemIcon(state, 0, state->weapon.tail.templateId, equipX + 4, INVENTORY_PANEL_Y + 14, 52, 62, 1000);
    for (int s = 0; s < EQUIPMENT_ARMOR_SLOTS; s++)
    {
        long cx = equipX + (s % 2) * 32;
        long cy = INVENTORY_PANEL_Y + 88 + (s / 2) * 32;
        int broken = state->hasArmor[s] && state->armor[s].durability <= 0;
        int red = broken ? 90 : (state->hasArmor[s] ? 40 : 15);
        int green = broken ? 15 : (state->hasArmor[s] ? 90 : 15);
        int blue = broken ? 15 : (state->hasArmor[s] ? 40 : 15);
        DrawRect(&state->canvas, cx, cy, 28, 28, blue, green, red); 
        if (state->hasArmor[s])
            DrawItemIcon(state, 1, state->armor[s].tail.templateId, cx + 2, cy + 2, 24, 24, 1000);
    }
    for (int s = 0; s < EQUIPMENT_ACCESSORY_SLOTS; s++)
    {
        long cx = equipX + (s % 2) * 32;
        long cy = INVENTORY_PANEL_Y + 156 + (s / 2) * 32;
        DrawRect(&state->canvas, cx, cy, 28, 28, state->hasAccessory[s] ? 40 : 15, state->hasAccessory[s] ? 90 : 15,
                 state->hasAccessory[s] ? 40 : 15);
        if (state->hasAccessory[s])
            DrawItemIcon(state, 2, state->accessory[s].tail.templateId, cx + 2, cy + 2, 24, 24, 1000);
    }

    
    {
        long nameX = INVENTORY_PANEL_X + 10;
        long nameMaxW = equipX - nameX - 6;
        long ty = INVENTORY_PANEL_Y + 34;
        char line[160];
        if (state->hasWeapon)
        {
            
            DrawTextSJIS(state, nameX, ty, line, nameMaxW, TEXT_STYLE_WHITE, 1000);
            ty += DIALOG_LINE_HEIGHT;
        }
        for (int s = 0; s < EQUIPMENT_ARMOR_SLOTS; s++)
            if (state->hasArmor[s])
            {
                
                DrawTextSJIS(state, nameX, ty, line, nameMaxW, TEXT_STYLE_WHITE, 1000);
                ty += DIALOG_LINE_HEIGHT;
            }
        for (int s = 0; s < EQUIPMENT_ACCESSORY_SLOTS; s++)
            if (state->hasAccessory[s])
            {
                
                DrawTextSJIS(state, nameX, ty, line, nameMaxW, TEXT_STYLE_WHITE, 1000);
                ty += DIALOG_LINE_HEIGHT;
            }
    }

    
    long maxShown = INVENTORY_GRID_COLS * INVENTORY_GRID_ROWS;
    for (long i = 0; i < state->inventoryCount && i < maxShown; i++)
    {
        long col = i % INVENTORY_GRID_COLS, row = i / INVENTORY_GRID_COLS;
        long cellX = INVENTORY_GRID_ORIGIN_X + col * (INVENTORY_GRID_CELL + 2);
        long cellY = INVENTORY_GRID_ORIGIN_Y + row * (INVENTORY_GRID_CELL + 2);
        DrawRect(&state->canvas, cellX, cellY, INVENTORY_GRID_CELL, INVENTORY_GRID_CELL, 45, 40, 35);
        
        DrawItemIcon(state, state->inventory[i].kind, state->inventory[i].templateId, cellX + 1, cellY + 1,
                     INVENTORY_GRID_CELL - 2, INVENTORY_GRID_CELL - 2, 1000);
        
        DrawTextSJIS(state, cellX + 3, cellY + 3, state->inventory[i].name, INVENTORY_GRID_CELL - 6,
                     TEXT_STYLE_WHITE_SHADOWED, 1000);
        if (state->hudBarLoaded)
            DrawDigits(&state->canvas, &state->hudBar, cellX + 4, cellY + INVENTORY_GRID_CELL - 12,
                       state->inventory[i].count);
    }
}



static int FindInventorySlotAtScreenPoint(const DemoState *state, long x, long y)
{
    for (long i = 0; i < state->inventoryCount; i++)
    {
        const InventorySlot *inv = &state->inventory[i];
        if (inv->gridCol < 0)
            continue;
        long cellX = INVENTORY_WINDOW_X + INVENTORY_REAL_GRID_ORIGIN_X + inv->gridCol * INVENTORY_REAL_GRID_CELL;
        long cellY = INVENTORY_WINDOW_Y + INVENTORY_REAL_GRID_ORIGIN_Y + inv->gridRow * INVENTORY_REAL_GRID_CELL;
        long footprintW = inv->gridWidth * INVENTORY_REAL_GRID_CELL;
        long footprintH = inv->gridHeight * INVENTORY_REAL_GRID_CELL;
        if (x >= cellX && x < cellX + footprintW && y >= cellY && y < cellY + footprintH)
            return (int)i;
    }
    return -1;
}


static int ScreenPointToGridCell(long x, long y, long *outCol, long *outRow)
{
    long col = (x - (INVENTORY_WINDOW_X + INVENTORY_REAL_GRID_ORIGIN_X)) / INVENTORY_REAL_GRID_CELL;
    long row = (y - (INVENTORY_WINDOW_Y + INVENTORY_REAL_GRID_ORIGIN_Y)) / INVENTORY_REAL_GRID_CELL;
    if (col < 0 || col >= INVENTORY_REAL_GRID_COLS || row < 0 || row >= INVENTORY_REAL_GRID_ROWS)
        return 0;
    *outCol = col;
    *outRow = row;
    return 1;
}


static void ComputeEquipSlotRect(DragSourceKind kind, int index, long *x, long *y, long *w, long *h)
{
    switch (kind)
    {
    case DRAG_EQUIP_WEAPON:
        *x = 480;
        *y = 15;
        *w = 60;
        *h = 125;
        break;
    case DRAG_EQUIP_ARMOR:
        switch (index)
        {
        case EQUIPMENT_SHIELD_SLOT_INDEX:
            *x = 480;
            *y = 160;
            *w = 60;
            *h = 90;
            break;
        case EQUIPMENT_HELMET_SLOT_INDEX:
            *x = 560;
            *y = 15;
            *w = 60;
            *h = 60;
            break;
        case EQUIPMENT_BOOTS_SLOT_INDEX:
            *x = 560;
            *y = 192;
            *w = 60;
            *h = 60;
            break;
        default: 
            *x = 560;
            *y = 90;
            *w = 60;
            *h = 90;
            break;
        }
        break;
    case DRAG_EQUIP_ACCESSORY:
        *x = 400 + (index % 2) * 40;
        *y = 145 + (index / 2) * 40;
        *w = 26;
        *h = 25;
        break;
    default:
        *x = *y = *w = *h = 0;
        break;
    }
}


static int FindEquipmentSlotAtScreenPoint(const DemoState *state, long x, long y, DragSourceKind *outKind,
                                           int *outIndex)
{
    if (!state->inventoryOpen || !state->statusSheetLoaded)
        return 0;

    long rx, ry, rw, rh;
    ComputeEquipSlotRect(DRAG_EQUIP_WEAPON, 0, &rx, &ry, &rw, &rh);
    if (x >= rx && x < rx + rw && y >= ry && y < ry + rh)
    {
        *outKind = DRAG_EQUIP_WEAPON;
        *outIndex = 0;
        return 1;
    }
    for (int s = 0; s < EQUIPMENT_ARMOR_SLOTS; s++)
    {
        ComputeEquipSlotRect(DRAG_EQUIP_ARMOR, s, &rx, &ry, &rw, &rh);
        if (x >= rx && x < rx + rw && y >= ry && y < ry + rh)
        {
            *outKind = DRAG_EQUIP_ARMOR;
            *outIndex = s;
            return 1;
        }
    }
    for (int s = 0; s < EQUIPMENT_ACCESSORY_SLOTS; s++)
    {
        ComputeEquipSlotRect(DRAG_EQUIP_ACCESSORY, s, &rx, &ry, &rw, &rh);
        if (x >= rx && x < rx + rw && y >= ry && y < ry + rh)
        {
            *outKind = DRAG_EQUIP_ACCESSORY;
            *outIndex = s;
            return 1;
        }
    }
    return 0;
}


void BeginInventoryDrag(DemoState *state, long x, long y)
{
    if (!state->inventoryOpen || !state->statusSheetLoaded)
        return;

    int invSlot = FindInventorySlotAtScreenPoint(state, x, y);
    if (invSlot >= 0)
    {
        state->dragActive = 1;
        state->dragSourceKind = DRAG_INVENTORY;
        state->dragSourceIndex = invSlot;
        state->dragCurrentMouseX = x;
        state->dragCurrentMouseY = y;
        return;
    }

    DragSourceKind equipKind;
    int equipIndex;
    if (FindEquipmentSlotAtScreenPoint(state, x, y, &equipKind, &equipIndex))
    {
        long kind = -1, templateId = 0;
        const char *name = NULL;
        if (equipKind == DRAG_EQUIP_WEAPON && state->hasWeapon)
        {
            kind = 0;
            templateId = state->weapon.tail.templateId;
            name = state->weaponName;
        }
        else if (equipKind == DRAG_EQUIP_ARMOR && state->hasArmor[equipIndex])
        {
            kind = 1;
            templateId = state->armor[equipIndex].tail.templateId;
            name = state->armorName[equipIndex];
        }
        else if (equipKind == DRAG_EQUIP_ACCESSORY && state->hasAccessory[equipIndex])
        {
            kind = 2;
            templateId = state->accessory[equipIndex].tail.templateId;
            name = state->accessoryName[equipIndex];
        }
        if (kind < 0)
            return; 

        int heldSlot = CreateHeldInventorySlot(state, kind, templateId, name);
        if (heldSlot < 0)
            return; 

        if (equipKind == DRAG_EQUIP_WEAPON)
            state->hasWeapon = 0;
        else if (equipKind == DRAG_EQUIP_ARMOR)
            state->hasArmor[equipIndex] = 0;
        else
            state->hasAccessory[equipIndex] = 0;
        RecomputePlayerMaxHP(state);

        state->dragActive = 1;
        state->dragSourceKind = DRAG_INVENTORY;
        state->dragSourceIndex = heldSlot;
        state->dragCurrentMouseX = x;
        state->dragCurrentMouseY = y;
    }
}


void ResolveInventoryDrag(DemoState *state, long x, long y)
{
    int changed = 0;
    int resolved = 0;

    const InventorySlot *inv = &state->inventory[state->dragSourceIndex];
    DragSourceKind targetKind;
    int targetIndex;
    long targetCol, targetRow;
    if (FindEquipmentSlotAtScreenPoint(state, x, y, &targetKind, &targetIndex) &&
        ((targetKind == DRAG_EQUIP_WEAPON && inv->kind == 0) ||
         (targetKind == DRAG_EQUIP_ARMOR && inv->kind == 1 && ArmorNameFitsSlot(inv->name, targetIndex)) ||
         (targetKind == DRAG_EQUIP_ACCESSORY && inv->kind == 2)))
    {
        
        long dispKind = -1, dispTemplateId = 0;
        char dispName[128] = "";
        
        if (targetKind == DRAG_EQUIP_WEAPON && state->hasWeapon)
        {
            dispKind = 0;
            dispTemplateId = state->weapon.tail.templateId;
            
            state->hasWeapon = 0;
        }
        else if (targetKind == DRAG_EQUIP_ARMOR && state->hasArmor[targetIndex])
        {
            dispKind = 1;
            dispTemplateId = state->armor[targetIndex].tail.templateId;
            
            state->hasArmor[targetIndex] = 0;
        }
        else if (targetKind == DRAG_EQUIP_ACCESSORY && state->hasAccessory[targetIndex])
        {
            dispKind = 2;
            dispTemplateId = state->accessory[targetIndex].tail.templateId;
            
            state->hasAccessory[targetIndex] = 0;
        }

        EquipItemIntoSlot(state, inv->kind, inv->templateId, inv->name, targetIndex);
        RemoveOneFromInventorySlot(state, state->dragSourceIndex);
        changed = 1;

        if (dispKind < 0)
            resolved = 1; 
        else
        {
            int heldSlot = CreateHeldInventorySlot(state, dispKind, dispTemplateId, dispName);
            if (heldSlot >= 0)
                state->dragSourceIndex = heldSlot; 
            else
            {
                
                AddItemToInventory(state, dispKind, dispTemplateId, dispName);
                resolved = 1;
            }
        }
    }
    
    else if (ScreenPointToGridCell(x - inv->gridWidth * INVENTORY_REAL_GRID_CELL / 2,
                                   y - inv->gridHeight * INVENTORY_REAL_GRID_CELL / 2, &targetCol, &targetRow))
    {
        
        int drop = ClassifyInventoryDropTarget(state, targetCol, targetRow, inv->gridWidth, inv->gridHeight,
                                                state->dragSourceIndex);
        if (drop == INVENTORY_DROP_FREE)
        {
            state->inventory[state->dragSourceIndex].gridCol = targetCol;
            state->inventory[state->dragSourceIndex].gridRow = targetRow;
            changed = 1;
            resolved = 1;
        }
        else if (drop >= 0)
        {
            state->inventory[state->dragSourceIndex].gridCol = targetCol;
            state->inventory[state->dragSourceIndex].gridRow = targetRow;
            state->dragSourceIndex = drop; 
            changed = 1;
            
        }
        
    }
    
    else if (!IsScreenPointOverUI(state, x, y))
    {
        long clickWorldX, clickWorldY;
        ScreenToWorld(state, x + state->cameraX, y + state->cameraY, &clickWorldX, &clickWorldY);
        DropInventoryItemToGround(state, state->dragSourceIndex, clickWorldX, clickWorldY);
        resolved = 1;
    }

    if (changed)
        RecomputePlayerMaxHP(state);
    if (resolved)
        state->dragActive = 0;
}


void RemapEnclosedColorKeyPixels(RKC_DIB *frame)
{
    if (!frame || frame->bpp != 8 || !frame->pixels || !frame->palette)
        return;
    long w = frame->width, h = frame->height, aw = frame->alignWidth;
    if (w <= 0 || h <= 0)
        return;

    
    long substitute = -1, bestDist = 0;
    long palCount = RKC_DIB_GetPaletteCount(frame);
    for (long i = 1; i < palCount; i++)
    {
        long db = (long)frame->palette[i * 4] - frame->palette[0];
        long dg = (long)frame->palette[i * 4 + 1] - frame->palette[1];
        long dr = (long)frame->palette[i * 4 + 2] - frame->palette[2];
        long d = db * db + dg * dg + dr * dr;
        if (substitute < 0 || d < bestDist)
        {
            bestDist = d;
            substitute = i;
        }
    }
    if (substitute < 0)
        return; 

    unsigned char *mark = calloc((size_t)w * (size_t)h, 1); 
    long *stack = malloc(sizeof(long) * (size_t)w * (size_t)h);
    if (!mark || !stack)
    {
        free(mark);
        free(stack);
        return;
    }
    long sp = 0;
    
    for (long y = 0; y < h; y++)
        for (long x = 0; x < w; x++)
        {
            if (y != 0 && y != h - 1 && x != 0 && x != w - 1)
                continue;
            if (frame->pixels[y * aw + x] == 0 && !mark[y * w + x])
            {
                mark[y * w + x] = 1;
                stack[sp++] = y * w + x;
            }
        }
    while (sp > 0)
    {
        long p = stack[--sp], px = p % w, py = p / w;
        static const long dx[4] = {-1, 1, 0, 0}, dy[4] = {0, 0, -1, 1};
        for (int d = 0; d < 4; d++)
        {
            long nx = px + dx[d], ny = py + dy[d];
            if (nx < 0 || nx >= w || ny < 0 || ny >= h || mark[ny * w + nx])
                continue;
            if (frame->pixels[ny * aw + nx] != 0)
                continue;
            mark[ny * w + nx] = 1;
            stack[sp++] = ny * w + nx;
        }
    }

    for (long y = 0; y < h; y++)
        for (long x = 0; x < w; x++)
            if (frame->pixels[y * aw + x] == 0 && !mark[y * w + x])
                frame->pixels[y * aw + x] = (unsigned char)substitute;

    free(mark);
    free(stack);
}


static void DrawInventoryReal(DemoState *state)
{
    RKC_DIB *left = RKC_UPDIB_GetFrame(&state->statusSheet, INVENTORY_BG_FRAME_LEFT);
    if (left)
        RKC_DIB_TransferToDIBEx(&state->canvas, INVENTORY_WINDOW_X, INVENTORY_WINDOW_Y, left->width, left->height,
                                left, 0, 0, -1, 1000);
    RKC_DIB *gridExt = RKC_UPDIB_GetFrame(&state->statusSheet, INVENTORY_BG_FRAME_GRID_EXT);
    if (gridExt)
        RKC_DIB_TransferToDIBEx(&state->canvas, INVENTORY_WINDOW_X, INVENTORY_WINDOW_Y, gridExt->width,
                                gridExt->height, gridExt, 0, 0, 0, 1000);
    long equipColX = INVENTORY_WINDOW_X + 195; 
    RKC_DIB *equipHeader = RKC_UPDIB_GetFrame(&state->statusSheet, INVENTORY_BG_FRAME_EQUIP_HEADER);
    if (equipHeader)
        RKC_DIB_TransferToDIBEx(&state->canvas, equipColX, INVENTORY_WINDOW_Y, equipHeader->width,
                                equipHeader->height, equipHeader, 0, 0, -1, 1000);
    RKC_DIB *equipPanel = RKC_UPDIB_GetFrame(
        &state->statusSheet, state->playerIsFemale ? INVENTORY_BG_FRAME_EQUIP_FEMALE : INVENTORY_BG_FRAME_EQUIP_MALE);
    long equipPanelY = INVENTORY_WINDOW_Y + INVENTORY_EQUIP_HEADER_HEIGHT;
    if (equipPanel)
        RKC_DIB_TransferToDIBEx(&state->canvas, equipColX, equipPanelY, equipPanel->width, equipPanel->height,
                                equipPanel, 0, 0, -1, 1000);
    RKC_DIB *equipBorder = RKC_UPDIB_GetFrame(&state->statusSheet, INVENTORY_BG_FRAME_EQUIP_BORDER);
    if (equipBorder)
        RKC_DIB_TransferToDIBEx(&state->canvas, equipColX + INVENTORY_EQUIP_PANEL_WIDTH, equipPanelY,
                                equipBorder->width, equipBorder->height, equipBorder, 0, 0, -1, 1000);

    
    {
        long labelW = DrawTextSJIS(state, INVENTORY_WINDOW_X + 12, INVENTORY_WINDOW_Y + 12, "Total Gold", 0,
                                   TEXT_STYLE_WHITE, 1000);
        if (state->hudBarLoaded)
            DrawDigits(&state->canvas, &state->hudBar, INVENTORY_WINDOW_X + 12 + labelW + (labelW ? 8 : 0),
                       INVENTORY_WINDOW_Y + 16, state->gold);
    }

    
    
    long wx, wy, ww, wh;
    ComputeEquipSlotRect(DRAG_EQUIP_WEAPON, 0, &wx, &wy, &ww, &wh);
    
    if (state->hasWeapon)
        
        DrawItemIcon(state, 0, state->weapon.tail.templateId, wx + 2, wy + 2, ww - 4, wh - 4, 1000);
    for (int s = 0; s < EQUIPMENT_ARMOR_SLOTS; s++)
    {
        long cx, cy, cw, ch;
        ComputeEquipSlotRect(DRAG_EQUIP_ARMOR, s, &cx, &cy, &cw, &ch);
        if (state->hasArmor[s] && state->armor[s].durability <= 0)
            DrawRectAlpha(&state->canvas, cx, cy, cw, ch, 400, 20, 20, 90); 
        
        if (state->hasArmor[s])
        {
            
            long opacity = (s == EQUIPMENT_SHIELD_SLOT_INDEX && IsShieldIneffective(state)) ? 350 : 1000;
            
            DrawItemIcon(state, 1, state->armor[s].tail.templateId, cx + 2, cy + 2, cw - 4, ch - 4, opacity);
        }
    }
    for (int s = 0; s < EQUIPMENT_ACCESSORY_SLOTS; s++)
    {
        long cx, cy, cw, ch;
        ComputeEquipSlotRect(DRAG_EQUIP_ACCESSORY, s, &cx, &cy, &cw, &ch);
        
        if (state->hasAccessory[s])
            DrawItemIcon(state, 2, state->accessory[s].tail.templateId, cx + 2, cy + 2, cw - 4, ch - 4, 1000);
    }

    
    for (long i = 0; i < state->inventoryCount; i++)
    {
        const InventorySlot *inv = &state->inventory[i];
        if (inv->gridCol < 0)
            continue;
        long cellX = INVENTORY_WINDOW_X + INVENTORY_REAL_GRID_ORIGIN_X + inv->gridCol * INVENTORY_REAL_GRID_CELL;
        long cellY = INVENTORY_WINDOW_Y + INVENTORY_REAL_GRID_ORIGIN_Y + inv->gridRow * INVENTORY_REAL_GRID_CELL;
        long footprintW = inv->gridWidth * INVENTORY_REAL_GRID_CELL;
        long footprintH = inv->gridHeight * INVENTORY_REAL_GRID_CELL;
        
        if (!(state->dragActive && state->dragSourceKind == DRAG_INVENTORY && state->dragSourceIndex == i))
            DrawItemIcon(state, inv->kind, inv->templateId, cellX + 1, cellY + 1, footprintW - 2, footprintH - 2, 1000);
        
    }
}


static void DrawInventoryDragOverlay(DemoState *state)
{
    
    const InventorySlot *inv = &state->inventory[state->dragSourceIndex];
    long ghostW = inv->gridWidth * INVENTORY_REAL_GRID_CELL;
    long ghostH = inv->gridHeight * INVENTORY_REAL_GRID_CELL;

    long hx, hy, hw, hh;
    int haveHighlight = 0;
    DragSourceKind targetKind;
    int targetIndex;
    if (FindEquipmentSlotAtScreenPoint(state, state->dragCurrentMouseX, state->dragCurrentMouseY, &targetKind,
                                        &targetIndex))
    {
        ComputeEquipSlotRect(targetKind, targetIndex, &hx, &hy, &hw, &hh);
        haveHighlight = 1;
    }
    else
    {
        
        long col, row;
        if (ScreenPointToGridCell(state->dragCurrentMouseX - ghostW / 2, state->dragCurrentMouseY - ghostH / 2, &col,
                                  &row))
        {
            hx = INVENTORY_WINDOW_X + INVENTORY_REAL_GRID_ORIGIN_X + col * INVENTORY_REAL_GRID_CELL;
            hy = INVENTORY_WINDOW_Y + INVENTORY_REAL_GRID_ORIGIN_Y + row * INVENTORY_REAL_GRID_CELL;
            hw = hh = INVENTORY_REAL_GRID_CELL;
            haveHighlight = 1;
        }
    }
    if (haveHighlight)
        DrawRectOutlineAlpha(&state->canvas, hx, hy, hw, hh, 3, 700, 255, 255, 255);

    DrawItemIcon(state, inv->kind, inv->templateId, state->dragCurrentMouseX - ghostW / 2,
                 state->dragCurrentMouseY - ghostH / 2, ghostW, ghostH, 1000);
}


static void DrawEquipGridDebug(DemoState *state)
{
    for (long x = 0; x <= INVENTORY_WINDOW_WIDTH; x += 5)
    {
        int major = (x % 25 == 0);
        DrawRect(&state->canvas, INVENTORY_WINDOW_X + x, INVENTORY_WINDOW_Y, 1, INVENTORY_WINDOW_HEIGHT,
                 major ? 0 : 120, major ? 255 : 200, major ? 255 : 0);
        if (major)
        {
            char label[8];
            
            DrawTextSJIS(state, INVENTORY_WINDOW_X + x + 1, INVENTORY_WINDOW_Y, label, 0, TEXT_STYLE_WHITE, 1000);
        }
    }
    for (long y = 0; y <= INVENTORY_WINDOW_HEIGHT; y += 5)
    {
        int major = (y % 25 == 0);
        DrawRect(&state->canvas, INVENTORY_WINDOW_X, INVENTORY_WINDOW_Y + y, INVENTORY_WINDOW_WIDTH, 1,
                 major ? 0 : 120, major ? 255 : 200, major ? 255 : 0);
        if (major)
        {
            char label[8];
            
            DrawTextSJIS(state, INVENTORY_WINDOW_X, INVENTORY_WINDOW_Y + y + 1, label, 0, TEXT_STYLE_WHITE, 1000);
        }
    }
}


void DrawInventory(DemoState *state)
{
    if (!state->inventoryOpen)
        return;
    if (state->statusSheetLoaded)
    {
        DrawInventoryReal(state);
        if (state->dragActive)
            DrawInventoryDragOverlay(state);
        if (state->equipGridDebug)
            DrawEquipGridDebug(state);
    }
    else
        DrawInventoryFallback(state);
}


void DrawStatusMagic(DemoState *state)
{
    if (!state->statusMagicOpen || !state->statusSheetLoaded)
        return;

    long frameIndex =
        state->statusMagicTab == STATUS_MAGIC_TAB_MAGIC ? STATUS_MAGIC_FRAME_MAGIC_TAB : STATUS_MAGIC_FRAME_STATUS_TAB;
    RKC_DIB *bg = RKC_UPDIB_GetFrame(&state->statusSheet, frameIndex);
    if (!bg)
        return;
    
    RKC_DIB_TransferToDIBEx(&state->canvas, STATUS_MAGIC_WINDOW_X, STATUS_MAGIC_WINDOW_Y, bg->width, bg->height, bg, 0,
                            0, -1, 1000);

    if (state->statusMagicTab == STATUS_MAGIC_TAB_STATUS && state->hudBarLoaded)
    {
        long x = STATUS_MAGIC_WINDOW_X + STATUS_HP_VALUE_X;
        long y = STATUS_MAGIC_WINDOW_Y + STATUS_HP_VALUE_Y;
        x += DrawDigits(&state->canvas, &state->hudBar, x, y, state->playerHP);
        x += 4;
        x += DrawTextSJIS(state, x, y, "/", 0, TEXT_STYLE_WHITE, 1000);
        x += 4;
        DrawDigits(&state->canvas, &state->hudBar, x, y, state->playerMaxHP);
    }

    
    if (state->statusMagicTab == STATUS_MAGIC_TAB_STATUS)
    {
        DrawTextSJIS(state, STATUS_MAGIC_WINDOW_X + STATUS_CLASS_NAME_X, STATUS_MAGIC_WINDOW_Y + STATUS_CLASS_NAME_Y,
                     PlayerClassName(state->playerClass), 0, TEXT_STYLE_WHITE_SHADOWED, 1000);
        long changeTo = state->playerChangeToClass;
        if (changeTo != 0 && changeTo != state->playerClass)
        {
            long frame = -1;
            if (changeTo == PLAYER_CLASS_HUNTER)
                frame = STATUS_FRAME_CLASS_HUNTER;
            else if (changeTo == PLAYER_CLASS_WARRIOR)
                frame = STATUS_FRAME_CLASS_WARRIOR;
            else if (changeTo == PLAYER_CLASS_WITCH)
                frame = state->playerIsFemale ? STATUS_FRAME_CLASS_WITCH : STATUS_FRAME_CLASS_WIZARD;
            RKC_DIB *btn = frame >= 0 ? RKC_UPDIB_GetFrame(&state->statusSheet, frame) : NULL;
            if (btn)
                RKC_DIB_TransferToDIBEx(&state->canvas, STATUS_MAGIC_WINDOW_X + STATUS_CLASS_BUTTON_X,
                                        STATUS_MAGIC_WINDOW_Y + STATUS_CLASS_BUTTON_Y, btn->width, btn->height, btn, 0,
                                        0, 0, 1000);
        }
    }
}




void CloseAllWindows(DemoState *state)
{
    state->minimapOpen = 0;
    state->inventoryOpen = 0;
    state->statusMagicOpen = 0;
    state->questWindowOpen = 0;
    state->questTooltipIndex = -1; 
    state->gateWindowOpen = 0;
    state->dragActive = 0;
}


static const char *QUEST_TITLES_EP1[] = {
    "Defeat the Red Goblin.",
    "Take back Malse's gem.",
    "Take back Syria's Spirit Stone.",
    "Sweep the Dusty Ruins.",
    "Take back Rosanna's Memorable Ruby.",
    "Errand for Gedo.",
    "Sweep the monsters in the Cold Ruins.",
    "Scout the Purgatory of Judgments.",
    "Scout the Remains of Reincarnation.",
    "Scout the Continuing Land.",
    "Scout the Immortal Remains.",
};
#define QUEST_TITLES_EP1_COUNT ((long)(sizeof(QUEST_TITLES_EP1) / sizeof(QUEST_TITLES_EP1[0])))

const char *QuestTitleForIndex(long questIndex)
{
    if (questIndex >= 0 && questIndex < QUEST_TITLES_EP1_COUNT)
        return QUEST_TITLES_EP1[questIndex];
    return "(unknown quest)";
}


static const char *QUEST_DESCRIPTIONS_EP1[] = {
    "The Red Goblin living in the northeast of the Remote Town is tormenting the local people.\n\nSubdue the Red "
    "Goblin as soon as you find it.",
    "(Quest description not yet recovered.)",
    "(Quest description not yet recovered.)",
    "(Quest description not yet recovered.)",
    "(Quest description not yet recovered.)",
    "(Quest description not yet recovered.)",
    "(Quest description not yet recovered.)",
    "(Quest description not yet recovered.)",
    "(Quest description not yet recovered.)",
    "(Quest description not yet recovered.)",
    "(Quest description not yet recovered.)",
};

static const char *QuestDescriptionForIndex(long questIndex)
{
    if (questIndex >= 0 && questIndex < (long)(sizeof(QUEST_DESCRIPTIONS_EP1) / sizeof(QUEST_DESCRIPTIONS_EP1[0])))
        return QUEST_DESCRIPTIONS_EP1[questIndex];
    return "";
}


#define QUEST_TOOLTIP_W 296
#define QUEST_TOOLTIP_H 252
#define QUEST_TOOLTIP_PAD 14

#define QUEST_TOOLTIP_BG_B 108
#define QUEST_TOOLTIP_BG_G 52
#define QUEST_TOOLTIP_BG_R 56

void DrawQuestTooltip(DemoState *state)
{
    if (state->questTooltipIndex < 0 || !state->questWindowOpen || !state->statusSheetLoaded)
        return;
    RKC_DIB *topB = RKC_UPDIB_GetFrame(&state->statusSheet, QUEST_TOOLTIP_FRAME_TOP);
    RKC_DIB *botB = RKC_UPDIB_GetFrame(&state->statusSheet, QUEST_TOOLTIP_FRAME_BOTTOM);
    RKC_DIB *leftB = RKC_UPDIB_GetFrame(&state->statusSheet, QUEST_TOOLTIP_FRAME_LEFT);
    RKC_DIB *rightB = RKC_UPDIB_GetFrame(&state->statusSheet, QUEST_TOOLTIP_FRAME_RIGHT);
    if (!topB || !botB || !leftB || !rightB)
        return;

    long x0 = (APP_WIDTH - QUEST_TOOLTIP_W) / 2;
    long y0 = 40 + (336 - QUEST_TOOLTIP_H) / 2; 

    
    DrawRect(&state->canvas, x0, y0, QUEST_TOOLTIP_W, QUEST_TOOLTIP_H, QUEST_TOOLTIP_BG_B, QUEST_TOOLTIP_BG_G,
             QUEST_TOOLTIP_BG_R);
    RKC_DIB_TransferToDIBEx(&state->canvas, x0, y0, topB->width, topB->height, topB, 0, 0, -1, 1000);
    RKC_DIB_TransferToDIBEx(&state->canvas, x0, y0 + QUEST_TOOLTIP_H - botB->height, botB->width, botB->height, botB, 0,
                            0, -1, 1000);
    RKC_DIB_TransferToDIBEx(&state->canvas, x0, y0, leftB->width, leftB->height, leftB, 0, 0, -1, 1000);
    RKC_DIB_TransferToDIBEx(&state->canvas, x0 + QUEST_TOOLTIP_W - rightB->width, y0, rightB->width, rightB->height,
                            rightB, 0, 0, -1, 1000);

    
    long tx = x0 + QUEST_TOOLTIP_PAD;
    long ty = y0 + QUEST_TOOLTIP_PAD;
    long innerW = QUEST_TOOLTIP_W - 2 * QUEST_TOOLTIP_PAD;
    char titled[160];
    
    ty = DrawWrappedTextSJIS(state, tx, ty, titled, innerW, QUEST_LINE_HEIGHT - 6, TEXT_STYLE_YELLOW_SHADOWED, 0);
    ty += 8; 
    DrawWrappedTextSJIS(state, tx, ty, QuestDescriptionForIndex(state->questTooltipIndex), innerW, QUEST_LINE_HEIGHT - 6,
                        TEXT_STYLE_WHITE, 0);
}


static TextStyle ItemTierTextStyle(long affixTier)
{
    return affixTier == 1   ? TEXT_STYLE_BLUE_SHADOWED
           : affixTier == 2 ? TEXT_STYLE_YELLOW_SHADOWED
           : affixTier == 3 ? TEXT_STYLE_ORANGE_SHADOWED
                            : TEXT_STYLE_WHITE_SHADOWED;
}


enum
{
    BONUS_NONE,
    BONUS_PLAIN,
    BONUS_PCT_PLUS,
    BONUS_PCT,
    BONUS_RECOVERY
};
static const struct
{
    const char *label;
    int fmt;
} ITEM_BONUS_MAP[39] = {
     {"Attack", BONUS_PLAIN},
     {"Defense", BONUS_PLAIN},
     {"Hit Rate", BONUS_PLAIN},
     {"Evasion Rate", BONUS_PLAIN},
     {"Magical Attack", BONUS_PLAIN},
     {"Magical Defense", BONUS_PLAIN},
     {"Magical Hit Rate", BONUS_PLAIN},
     {"Magical Evasion Rate", BONUS_PLAIN},
     {"Speed of Attack", BONUS_PLAIN},
     {"Walking Speed", BONUS_PLAIN},
     {"Maximum HP", BONUS_PLAIN},
     {"Maximum MP", BONUS_PLAIN},
     {"Strength", BONUS_PLAIN},
     {"Magic Level", BONUS_PLAIN},
     {"Probability of Stiffness", BONUS_PCT_PLUS},
     {"Duration of Stiffness", BONUS_PLAIN},
     {NULL, BONUS_NONE},
     {"Life Recovery", BONUS_RECOVERY},
     {"Mental Recovery", BONUS_RECOVERY},
     {"Mental Consumption", BONUS_PLAIN},
     {"Generation of Reflection", BONUS_PCT},
     {"Reflection Rate", BONUS_PCT},
     {"Effect of Stamina Medicine", BONUS_PCT_PLUS},
     {"Effect of Mental Medicine", BONUS_PCT_PLUS},
     {"Incidents of Absorption", BONUS_PCT},
     {"Absorption Rate", BONUS_PCT},
     {"Amount of Gold", BONUS_PCT_PLUS},
     {"Effect of Mine", BONUS_PLAIN},
     {"Number of Mines", BONUS_PLAIN},
     {"Speed of Chant", BONUS_PLAIN},
     {"Companion HP", BONUS_PLAIN},
     {"Companion Attack", BONUS_PLAIN},
     {"Companion Hit Rate", BONUS_PLAIN},
     {"Companion Defense", BONUS_PLAIN},
     {"Companion Evasion Rate", BONUS_PLAIN},
     {"Companion Magical Attack", BONUS_PLAIN},
     {"Companion Magical Hit Rate", BONUS_PLAIN},
     {"Companion Magical Defense", BONUS_PLAIN},
     {"Companion Magical Evasion Rate", BONUS_PLAIN},
};
static const char *const ITEM_RESIST_LABEL[8] = {"Fire", "Water", "Earth", "Thunder", "Holy", "Dark", "Gel", "Metal"};


void DrawItemTooltip(DemoState *state, long cursorX, long cursorY)
{
    
    const void *tail = NULL;
    long tailSize = 0, kind = -1, key = -1;
    const char *name = NULL;
    if (state->inventoryOpen && state->statusSheetLoaded)
    {
        DragSourceKind eqKind;
        int eqIndex;
        if (FindEquipmentSlotAtScreenPoint(state, cursorX, cursorY, &eqKind, &eqIndex))
        {
            if (eqKind == DRAG_EQUIP_WEAPON && state->hasWeapon)
                tail = &state->weapon.tail, tailSize = 804, kind = 0, name = state->weaponName, key = 300000000L;
            else if (eqKind == DRAG_EQUIP_ARMOR && state->hasArmor[eqIndex])
                tail = &state->armor[eqIndex].tail, tailSize = 764, kind = 1, name = state->armorName[eqIndex],
                key = 310000000L + eqIndex;
            else if (eqKind == DRAG_EQUIP_ACCESSORY && state->hasAccessory[eqIndex])
                tail = &state->accessory[eqIndex].tail, tailSize = 672, kind = 2, name = state->accessoryName[eqIndex],
                key = 320000000L + eqIndex;
        }
        if (!tail)
        {
            int slot = FindInventorySlotAtScreenPoint(state, cursorX, cursorY);
            if (slot >= 0 && state->inventory[slot].kind != 4)
            {
                const RKC_RPG_ITEMDATA_Record *rec = RKC_RPG_ITEMDATA_GetFromTemplateId(
                    &state->itemData, (int)state->inventory[slot].kind, state->inventory[slot].templateId);
                if (rec)
                    tail = rec->tail, tailSize = rec->tailSize, kind = state->inventory[slot].kind,
                    name = state->inventory[slot].name,
                    key = kind * 100000000L + state->inventory[slot].templateId;
            }
        }
    }
    else if (!state->inventoryOpen && state->hoveredWorldItemIndex >= 0 &&
             state->hoveredWorldItemIndex < state->worldItemCount)
    {
        const WorldItem *it = &state->worldItems[state->hoveredWorldItemIndex];
        if (!it->isGold && it->resolved)
        {
            const RKC_RPG_ITEMDATA_Record *rec =
                RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, (int)it->kind, it->templateId);
            if (rec)
                tail = rec->tail, tailSize = rec->tailSize, kind = it->kind, name = it->name,
                key = 200000000L + it->kind * 1000000L + it->templateId;
        }
    }

    
    if (key != state->itemTooltipHoverKey)
    {
        state->itemTooltipHoverKey = key;
        state->itemTooltipHoverStartTick = state->tick;
    }
    if (key < 0 || !tail || !state->itemDataLoaded)
        return;
    if (state->tick - state->itemTooltipHoverStartTick < ITEM_TOOLTIP_DELAY_TICKS)
        return;

    RKC_RPG_ITEMDATA_TooltipStats st;
    RKC_RPG_ITEMDATA_GetTooltipStatsFromTail(tail, tailSize, &st);
    long affixTier = tailSize >= 0x0C ? *(const long *)((const unsigned char *)tail + 0x08) : 0;
    long resist[8];
    int haveResist = RKC_RPG_ITEMDATA_GetResistancesFromTail(tail, tailSize, resist);

    
    struct
    {
        const char *label;
        long value;
    } lines[16];
    int n = 0;
    
    if (st.attack != 0)
        lines[n].label = "Attack", lines[n].value = st.attack, n++;
    if (st.hitRate != 0)
        lines[n].label = "Hit Rate", lines[n].value = st.hitRate, n++;
    if (st.defense != 0)
        lines[n].label = "Defense", lines[n].value = st.defense, n++;
    if (st.evasionRate != 0)
        lines[n].label = "Evasion Rate", lines[n].value = st.evasionRate, n++;
    if (st.magicalAttack != 0)
        lines[n].label = "Magical Attack", lines[n].value = st.magicalAttack, n++;
    if (st.magicalHitRate != 0)
        lines[n].label = "Magical Hit Rate", lines[n].value = st.magicalHitRate, n++;
    if (st.magicalDefense != 0)
        lines[n].label = "Magical Defense", lines[n].value = st.magicalDefense, n++;
    if (st.magicalEvasionRate != 0)
        lines[n].label = "Magical Evasion Rate", lines[n].value = st.magicalEvasionRate, n++;
    if (st.speedOfAttack != 0)
        lines[n].label = "Speed of Attack", lines[n].value = st.speedOfAttack, n++;
    if (st.durabilityMax > 0)
        lines[n].label = "Durability", lines[n].value = st.durabilityMax, n++;
    if (st.weight > 0)
        lines[n].label = "Weight", lines[n].value = st.weight, n++;
    lines[n].label = "Required Level", lines[n].value = st.requiredLevel, n++;

    
    struct
    {
        const char *label;
        char value[24];
    } bonus[39];
    int nb = 0;
    for (int b = 0; b < 39 && nb < 39; b++)
    {
        if (ITEM_BONUS_MAP[b].fmt == BONUS_NONE)
            continue;
        long v = RKC_RPG_ITEMDATA_GetBonusFromTail(tail, tailSize, b);
        if (v == 0)
            continue;
        bonus[nb].label = ITEM_BONUS_MAP[b].label;
        switch (ITEM_BONUS_MAP[b].fmt)
        {
        case BONUS_RECOVERY:
            
            break;
        case BONUS_PCT_PLUS:
            
            break;
        case BONUS_PCT:
            
            break;
        default:
            
            break;
        }
        nb++;
    }

    TextStyle tierStyle = ItemTierTextStyle(affixTier);
    char titled[160];
    

    
    const long PAD = 8, LINE_H = 16, GAP = 20;
    long innerW = MeasureTextSJIS(state, titled);
    char numbuf[24];
    for (int i = 0; i < n; i++)
    {
        
        long lw = MeasureTextSJIS(state, lines[i].label) + GAP + MeasureTextSJIS(state, numbuf);
        if (lw > innerW)
            innerW = lw;
    }
    for (int i = 0; i < nb; i++)
    {
        long lw = MeasureTextSJIS(state, bonus[i].label) + GAP + MeasureTextSJIS(state, bonus[i].value);
        if (lw > innerW)
            innerW = lw;
    }
    
    long resistCellW = 0;
    if (haveResist)
    {
        for (int i = 0; i < 8; i++)
        {
            
            long w = MeasureTextSJIS(state, numbuf);
            if (w > resistCellW)
                resistCellW = w;
        }
        resistCellW += 14;
        if (resistCellW * 4 > innerW)
            innerW = resistCellW * 4;
    }

    int bonusRows = nb;
    int resistRows = haveResist ? 2 : 0;
    int gaps = (nb > 0 ? 1 : 0) + (haveResist ? 1 : 0); 
    long panelW = innerW + 2 * PAD;
    long panelH = (long)(n + 1 + bonusRows + resistRows + gaps) * LINE_H + 2 * PAD + 4;

    long px = cursorX + 16, py = cursorY + 16;
    if (px + panelW > APP_WIDTH)
        px = cursorX - panelW - 8;
    if (px < 0)
        px = 0;
    if (py + panelH > APP_HEIGHT)
        py = APP_HEIGHT - panelH;
    if (py < 0)
        py = 0;

    DrawRectAlpha(&state->canvas, px, py, panelW, panelH, ITEM_TOOLTIP_BG_OPACITY, 0, 0, 0);
    long tx = px + PAD, ty = py + PAD;
    DrawTextSJIS(state, tx, ty, titled, 0, tierStyle, 1000);
    ty += LINE_H + 4;
    for (int i = 0; i < n; i++)
    {
        DrawTextSJIS(state, tx, ty, lines[i].label, 0, tierStyle, 1000);
        
        DrawTextSJIS(state, tx + innerW - MeasureTextSJIS(state, numbuf), ty, numbuf, 0,
                     lines[i].value < 0 ? TEXT_STYLE_RED : tierStyle, 1000);
        ty += LINE_H;
    }
    if (nb > 0)
    {
        ty += LINE_H; 
        for (int i = 0; i < nb; i++)
        {
            DrawTextSJIS(state, tx, ty, bonus[i].label, 0, TEXT_STYLE_CYAN, 1000);
            DrawTextSJIS(state, tx + innerW - MeasureTextSJIS(state, bonus[i].value), ty, bonus[i].value, 0,
                         TEXT_STYLE_CYAN, 1000);
            ty += LINE_H;
        }
    }
    if (haveResist)
    {
        ty += LINE_H; 
        for (int i = 0; i < 8; i++)
        {
            long col = i % 4, row = i / 4;
            
            DrawTextSJIS(state, tx + col * resistCellW, ty + row * LINE_H, numbuf, 0,
                         resist[i] < 0 ? TEXT_STYLE_RED : tierStyle, 1000);
        }
    }
}


void DrawQuestWindow(DemoState *state)
{
    if (!state->questWindowOpen || !state->statusSheetLoaded)
        return;
    RKC_DIB *body = RKC_UPDIB_GetFrame(&state->statusSheet, QUEST_WINDOW_FRAME_BODY);
    RKC_DIB *left = RKC_UPDIB_GetFrame(&state->statusSheet, QUEST_WINDOW_FRAME_LEFT);
    RKC_DIB *top = RKC_UPDIB_GetFrame(&state->statusSheet, QUEST_WINDOW_FRAME_TOP);
    RKC_DIB *right = RKC_UPDIB_GetFrame(&state->statusSheet, QUEST_WINDOW_FRAME_RIGHT);
    RKC_DIB *bottom = RKC_UPDIB_GetFrame(&state->statusSheet, QUEST_WINDOW_FRAME_BOTTOM);
    if (!body || !left || !top || !right || !bottom)
        return;

    long winW = top->width;      
    long interiorY = top->height; 
    long interiorH = left->height; 
    long interiorX = left->width;  
    long interiorW = winW - left->width - right->width; 

    
    for (long ty = 0; ty < interiorH; ty += body->height)
        for (long tx = 0; tx < interiorW; tx += body->width)
        {
            long w = body->width < interiorW - tx ? body->width : interiorW - tx;
            long h = body->height < interiorH - ty ? body->height : interiorH - ty;
            RKC_DIB_TransferToDIBEx(&state->canvas, interiorX + tx, interiorY + ty, w, h, body, 0, 0, -1, 1000);
        }

    
    RKC_DIB_TransferToDIBEx(&state->canvas, 0, 0, top->width, top->height, top, 0, 0, -1, 1000);
    RKC_DIB_TransferToDIBEx(&state->canvas, 0, interiorY, left->width, left->height, left, 0, 0, -1, 1000);
    RKC_DIB_TransferToDIBEx(&state->canvas, winW - right->width, interiorY, right->width, right->height, right, 0, 0,
                            -1, 1000);
    RKC_DIB_TransferToDIBEx(&state->canvas, 0, interiorY + interiorH, bottom->width, bottom->height, bottom, 0, 0, -1,
                            1000);

    
    for (int t = 0; t < QUEST_TAB_COUNT; t++)
    {
        long frameIndex =
            (t == state->questWindowTab ? QUEST_TAB_FRAME_SELECTED_BASE : QUEST_TAB_FRAME_UNSELECTED_BASE) + t;
        RKC_DIB *chip = RKC_UPDIB_GetFrame(&state->statusSheet, frameIndex);
        if (chip)
            RKC_DIB_TransferToDIBEx(&state->canvas, QUEST_TAB_X + t * (chip->width + QUEST_TAB_GAP), QUEST_TAB_Y,
                                    chip->width, chip->height, chip, 0, 0, -1, 1000);
    }

    
    if (state->questWindowTab != 0)
        return;
    long y = QUEST_LINE_START_Y;
    for (long i = 0; i < QUEST_TITLES_EP1_COUNT; i++)
    {
        long questState = i < RKC_RPG_SCRIPT_EXEC_QUEST_CAP ? state->execState.questArray[i] : 0;
        if (questState != 1 && questState != 2)
            continue;
        if (state->statusIconSheetLoaded)
        {
            RKC_DIB *icon = RKC_UPDIB_GetFrame(
                &state->statusIconSheet, questState == 2 ? QUEST_ICON_FRAME_COMPLETED : QUEST_ICON_FRAME_ACTIVE);
            if (icon)
                RKC_DIB_TransferToDIBEx(&state->canvas, QUEST_LINE_ICON_X, y - 3, icon->width, icon->height, icon, 0,
                                        0, 0, 1000);
        }
        
        DrawTextSJIS(state, QUEST_LINE_TEXT_X, y, QuestTitleForIndex(i),
                     winW - right->width - QUEST_LINE_TEXT_X - 10,
                     questState == 2 ? TEXT_STYLE_GREY : TEXT_STYLE_WHITE, 1000);
        y += QUEST_LINE_HEIGHT;
    }
}


typedef struct
{
    const char *name;
    long scenarioId;
    long x, y;
} GateWaypoint;
static const GateWaypoint GATE_WAYPOINTS[] = {
    {"Remote Town", 0, 95259, -3241},
    {"Wasteland of Pillars", 6, 35097, -6529},
    {NULL, 10002, 101909, 5141},
    {"Dusty Ruins, B5F", 10003, 67122, -23603},
    {"Cold Svalt Town", 1000000, 84619, -4601},
    {"Vaporous Forest", 1000002, 108691, 20007},
    {"Hanged Men's Forest", 1000003, 74207, 18871},
    {"Forest Divided Like a Cross", 1000004, 85132, -17784},
    {"The Ruins of Fire, Sea of Trees", 1000004, 70278, -30340},
    {"Cold Ruins, B3F", 1020001, 80387, 11501},
    {"Purgatory of Judgments, B1F", 1030000, 74432, 9266},
    {"Immortal Remains, B2F", 1050001, 86023, 15506},
    {"Tower of Ordeal, 1F", 99000001, 30533, -8153},
    {"Kanfore, Mining Town", 2100000, 15079, -801},
    {"Forest of Claws", 2100002, 27277, 2731},
    {"Cross Agora", 2100004, 22374, -11086},
    {NULL, 2100005, 48941, 6907},
    {NULL, 2100006, 21171, -5063},
    {NULL, 2200000, 21704, 944},
    {NULL, 2200003, 28748, -10260},
    {NULL, 2200003, 27633, 5226},
    {NULL, 2210002, 42571, 937},
    {NULL, 2200005, 44788, 1060},
    {NULL, 99000007, 17677, -877},
    {NULL, 99000013, 17938, -1428},
    {NULL, 3900000, 14521, 2608},
    {NULL, 3900001, 13596, 5663},
    {NULL, 3000505, 66859, -3560},
    {NULL, 3000306, 59031, 5258},
    {NULL, 3000307, 67435, -5026},
    {NULL, 3900002, 11559, 6159},
    {NULL, 3900003, 23263, -5615},
    {NULL, 3000402, 32658, 19691},
    {NULL, 3900004, 20805, 5554},
    {NULL, 3000302, 40008, 6020},
    {NULL, 3000303, 21926, -1868},
    {NULL, 3000201, 40036, -537},
    {NULL, 3900005, 20293, -3256},
    {NULL, 3900002, 11559, 6159},
    {NULL, 99000018, 17938, -1428},
    {NULL, 4900001, 8587, -2667},
    {NULL, 4900002, 25791, 10478},
    {NULL, 4000001, 33218, 17252},
    {NULL, 4000002, 43524, -28294},
    {NULL, 4000003, 41190, 3150},
    {NULL, 4000005, 43778, -17608},
    {NULL, 4000007, 33116, 16824},
    {NULL, 4000009, 40936, 32644},
    {NULL, 4060000, 24326, 14733},
    {NULL, 4130000, 30233, 9748},
    {NULL, 99000023, 17938, -1428},
};
#define GATE_WAYPOINT_COUNT ((long)(sizeof(GATE_WAYPOINTS) / sizeof(GATE_WAYPOINTS[0])))


typedef struct
{
    long scenarioId;
    long circleX, circleY; 
    int isRegionAnchor;    
} TransportDest;


static const TransportDest TRANSPORT_DESTS[] = {
    {0, 92323, 2416, 1},         
    {1000000, 87676, 1123, 1},   
    {2100000, 12213, 1646, 1},   
    {2200000, 12988, 8941, 1},   
    {2999999, 5985, 505, 0},     
    {3900000, 10756, 2743, 1},   
    {3900001, 10766, 3933, 1},   
    {3900002, 13408, 2181, 1},   
    {3900003, 24973, 1766, 1},   
    {3900004, 12330, -2631, 1},  
    {3900005, 21650, 3150, 1},   
    {4900000, 8960, 160, 0},     
    {4900001, 11350, 3190, 1},   
    {4900002, 23995, 5115, 1},   
    {99000001, 32000, -8161, 0}, 
    {99000007, 14000, -781, 0},  
    {99000013, 15861, 3348, 0},  
    {99000018, 15861, 3348, 0},  
    {99000023, 0, 0, 0},         
};
#define TRANSPORT_DEST_COUNT ((long)(sizeof(TRANSPORT_DESTS) / sizeof(TRANSPORT_DESTS[0])))

int IsTownScenario(long scenarioId)
{
    
    for (long i = 0; i < TRANSPORT_DEST_COUNT; i++)
        if (TRANSPORT_DESTS[i].scenarioId == scenarioId)
            return 1;
    return 0;
}

void TransportDestTown(long fromScenarioId, long *outScenarioId, long *outX, long *outY)
{
    
    long pick = 0; 
    for (long i = 0; i < TRANSPORT_DEST_COUNT; i++)
        if (TRANSPORT_DESTS[i].isRegionAnchor && TRANSPORT_DESTS[i].scenarioId <= fromScenarioId)
            pick = i;
    *outScenarioId = TRANSPORT_DESTS[pick].scenarioId;
    *outX = TRANSPORT_DESTS[pick].circleX;
    *outY = TRANSPORT_DESTS[pick].circleY;
}


int ActiveTransportCircleHere(const DemoState *state, long *outX, long *outY, int *outIsTownEnd)
{
    const TransportCircle *tc = &state->transportCircle;
    if (!tc->active)
        return 0;
    if (state->currentScenarioId == tc->fieldScenarioId)
    {
        *outX = tc->fieldX;
        *outY = tc->fieldY;
        if (outIsTownEnd)
            *outIsTownEnd = 0;
        return 1;
    }
    if (state->currentScenarioId == tc->townScenarioId)
    {
        *outX = tc->townX;
        *outY = tc->townY;
        if (outIsTownEnd)
            *outIsTownEnd = 1;
        return 1;
    }
    return 0;
}


void DrawGateWindow(DemoState *state)
{
    if (!state->gateWindowOpen || !state->statusSheetLoaded)
        return;

    
    if (state->gateTeleportPendingRow >= 0 && state->tick >= state->gateTeleportAtTick)
    {
        long i = state->gateTeleportPendingRow;
        state->gateTeleportPendingRow = -1;
        
        state->transitionPending = 1;
        state->pendingScenarioId = GATE_WAYPOINTS[i].scenarioId;
        state->pendingEntryPoint = -1;
        state->pendingWarpValid = 1;
        state->pendingWarpX = GATE_WAYPOINTS[i].x;
        state->pendingWarpY = GATE_WAYPOINTS[i].y;
        state->gateWindowOpen = 0;
        return;
    }

    RKC_DIB *body = RKC_UPDIB_GetFrame(&state->statusSheet, GATE_WINDOW_FRAME_BODY);
    if (!body)
        return;
    RKC_DIB_TransferToDIBEx(&state->canvas, 0, 0, body->width, body->height, body, 0, 0, -1, 1000);

    
    long total = 0;
    for (long i = 0; i < GATE_WAYPOINT_COUNT && i < RKC_RPG_SCRIPT_EXEC_ARRAY_A_CAP; i++)
        if (state->execState.arrayA[i] == 1)
            total++;
    long pages = (total + GATE_ROWS_PER_PAGE - 1) / GATE_ROWS_PER_PAGE;
    if (pages < 1)
        pages = 1;
    if (state->gateWindowPage >= pages)
        state->gateWindowPage = (int)pages - 1;
    if (state->gateWindowPage < 0)
        state->gateWindowPage = 0;

    long y = GATE_LINE_START_Y;
    long rows = 0;
    long skip = (long)state->gateWindowPage * GATE_ROWS_PER_PAGE;
    for (long i = 0; i < GATE_WAYPOINT_COUNT && i < RKC_RPG_SCRIPT_EXEC_ARRAY_A_CAP && rows < GATE_ROWS_PER_PAGE; i++)
    {
        if (state->execState.arrayA[i] != 1)
            continue;
        if (skip > 0)
        {
            skip--;
            continue;
        }
        char generic[32];
        const char *name = GATE_WAYPOINTS[i].name;
        if (!name)
        {
            
            name = generic;
        }
        
        int pressed = state->gateTeleportPendingRow == i;
        int hovered = state->gateHoveredRow == i;
        long iconFrame = pressed ? GATE_ICON_FRAME_PRESSED : hovered ? GATE_ICON_FRAME_HOVER : GATE_ICON_FRAME_NORMAL;
        RKC_DIB *rowIcon = RKC_UPDIB_GetFrame(&state->statusSheet, iconFrame);
        if (rowIcon)
            RKC_DIB_TransferToDIBEx(&state->canvas, GATE_LINE_ICON_X, y - 4, rowIcon->width, rowIcon->height, rowIcon,
                                    0, 0, 0, 1000);
        DrawTextSJIS(state, GATE_LINE_TEXT_X, y, name, body->width - GATE_LINE_TEXT_X - 24,
                     (hovered || pressed) ? TEXT_STYLE_WHITE : TEXT_STYLE_GREY, 1000);
        y += GATE_LINE_HEIGHT;
        rows++;
    }

    
    {
        char pageText[24];
        
        long w = MeasureTextSJIS(state, pageText);
        DrawTextSJIS(state, (body->width - w) / 2, GATE_PAGE_TEXT_Y, pageText, 0, TEXT_STYLE_WHITE, 1000);
    }
}


void DrawQuestBanner(DemoState *state)
{
    if (state->questBannerIndex < 0 || state->tick >= state->questBannerUntilTick || state->questWindowOpen)
        return;
    const char *title = QuestTitleForIndex(state->questBannerIndex);
    long w = MeasureTextSJIS(state, title);
    DrawTextSJIS(state, (APP_WIDTH - w) / 2, QUEST_BANNER_Y, title, 0, TEXT_STYLE_YELLOW_SHADOWED, 1000);
}

int IsScreenPointOverUI(DemoState *state, long x, long y)
{
    if (x < 0 || y < 0 || x >= APP_WIDTH || y >= APP_HEIGHT)
        return 0;

    if (state->hudBarLoaded)
    {
        RKC_DIB *bg = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_BACKGROUND);
        if (bg)
        {
            
            long barTop = APP_HEIGHT - bg->height;
            RKC_DIB *trim = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_TOP_TRIM);
            RKC_DIB *menu = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_MENU);
            if (trim)
                barTop -= trim->height;
            if (menu)
                barTop -= menu->height;
            if (y >= barTop)
                return 1;
        }

        RKC_DIB *partnerTag = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_PARTNER_HP_INACTIVE);
        if (partnerTag && x >= HUD_PARTNER_HP_TAG_X && x < HUD_PARTNER_HP_TAG_X + partnerTag->width &&
            y >= HUD_PARTNER_HP_TAG_Y && y < HUD_PARTNER_HP_TAG_Y + partnerTag->height)
            return 1;
        RKC_DIB *partnerGradient = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_PARTNER_HP_GRADIENT);
        if (partnerGradient && x >= HUD_PARTNER_HP_X && x < HUD_PARTNER_HP_X + partnerGradient->width &&
            y >= HUD_PARTNER_HP_Y && y < HUD_PARTNER_HP_Y + partnerGradient->height)
            return 1;
        RKC_DIB *exp = RKC_UPDIB_GetFrame(&state->hudBar, HUD_BAR_FRAME_EXP);
        if (exp && x >= HUD_EXP_X && x < HUD_EXP_X + exp->width && y >= HUD_EXP_Y && y < HUD_EXP_Y + exp->height)
            return 1;

        long rowX, rowWidth;
        if (ComputeMagicSlotBarRect(state, &rowX, &rowWidth) && x >= rowX && x < rowX + rowWidth &&
            y >= MAGIC_SLOT_ROW_Y && y < MAGIC_SLOT_ROW_Y + MAGIC_SLOT_SIZE)
            return 1;
    }

    if (state->minimapOpen)
    {
        long x0, y0, x1, y1;
        if (state->statusSheetLoaded)
        {
            
            x0 = MINIMAP_WINDOW_X;
            y0 = MINIMAP_WINDOW_Y;
            x1 = x0 + MINIMAP_WINDOW_WIDTH;
            y1 = y0 + MINIMAP_WINDOW_HEIGHT;
        }
        else
        {
            x0 = MINIMAP_PANEL_X - 6;
            y0 = MINIMAP_PANEL_Y - 6;
            x1 = x0 + MINIMAP_PANEL_WIDTH + 12;
            y1 = y0 + MINIMAP_PANEL_HEIGHT + 12;
        }
        if (x >= x0 && x < x1 && y >= y0 && y < y1)
            return 1;
    }

    if (state->inventoryOpen)
    {
        long x0, y0, x1, y1;
        if (state->statusSheetLoaded)
        {
            x0 = INVENTORY_WINDOW_X;
            y0 = INVENTORY_WINDOW_Y;
            x1 = x0 + INVENTORY_WINDOW_WIDTH;
            y1 = y0 + INVENTORY_WINDOW_HEIGHT;
        }
        else
        {
            x0 = INVENTORY_PANEL_X - 6;
            y0 = INVENTORY_PANEL_Y - 6;
            x1 = x0 + INVENTORY_PANEL_WIDTH + 12;
            y1 = y0 + INVENTORY_PANEL_HEIGHT + 12;
        }
        if (x >= x0 && x < x1 && y >= y0 && y < y1)
            return 1;
    }

    if (state->statusMagicOpen)
    {
        long x0 = STATUS_MAGIC_WINDOW_X, y0 = STATUS_MAGIC_WINDOW_Y;
        long x1 = x0 + STATUS_MAGIC_WINDOW_WIDTH, y1 = y0 + STATUS_MAGIC_WINDOW_HEIGHT;
        if (x >= x0 && x < x1 && y >= y0 && y < y1)
        {
            
            if (state->mouseLeftDown && y - y0 < STATUS_MAGIC_TAB_ROW_HEIGHT)
                state->statusMagicTab =
                    (x - x0 < STATUS_MAGIC_TAB_SPLIT_X) ? STATUS_MAGIC_TAB_STATUS : STATUS_MAGIC_TAB_MAGIC;

            
            if (state->statusMagicTab == STATUS_MAGIC_TAB_STATUS && state->playerChangeToClass != 0 &&
                state->playerChangeToClass != state->playerClass)
            {
                long bx = x0 + STATUS_CLASS_BUTTON_X, by = y0 + STATUS_CLASS_BUTTON_Y;
                if (state->mouseLeftDown && !state->classChangeClickLatch && x >= bx &&
                    x < bx + STATUS_CLASS_BUTTON_W && y >= by && y < by + STATUS_CLASS_BUTTON_H)
                {
                    PlayerChangeClass(state);
                    state->classChangeClickLatch = 1;
                }
            }
            if (!state->mouseLeftDown)
                state->classChangeClickLatch = 0;
            return 1;
        }
    }

    if (state->gateWindowOpen && state->statusSheetLoaded)
    {
        
        RKC_DIB *body = RKC_UPDIB_GetFrame(&state->statusSheet, GATE_WINDOW_FRAME_BODY);
        if (body && x >= 0 && x < body->width && y >= 0 && y < body->height)
        {
            
            long total = 0;
            for (long i = 0; i < GATE_WAYPOINT_COUNT && i < RKC_RPG_SCRIPT_EXEC_ARRAY_A_CAP; i++)
                if (state->execState.arrayA[i] == 1)
                    total++;
            long pages = (total + GATE_ROWS_PER_PAGE - 1) / GATE_ROWS_PER_PAGE;
            if (pages < 1)
                pages = 1;
            if (!state->mouseLeftDown)
            {
                state->gatePageClickLatch = 0;
                state->gateOpenClickLatch = 0; 
            }
            else if (!state->gateOpenClickLatch && !state->gatePageClickLatch && y >= GATE_PAGE_ARROW_Y0 &&
                     y < GATE_PAGE_ARROW_Y1)
            {
                if (x >= GATE_PAGE_PREV_X0 && x < GATE_PAGE_PREV_X1 && state->gateWindowPage > 0)
                {
                    state->gateWindowPage--;
                    state->gatePageClickLatch = 1;
                }
                else if (x >= GATE_PAGE_NEXT_X0 && x < GATE_PAGE_NEXT_X1 && state->gateWindowPage + 1 < pages)
                {
                    state->gateWindowPage++;
                    state->gatePageClickLatch = 1;
                }
            }

            
            state->gateHoveredRow = -1;
            long rowY = GATE_LINE_START_Y;
            long rows = 0;
            long skip = (long)state->gateWindowPage * GATE_ROWS_PER_PAGE;
            for (long i = 0; i < GATE_WAYPOINT_COUNT && i < RKC_RPG_SCRIPT_EXEC_ARRAY_A_CAP &&
                 rows < GATE_ROWS_PER_PAGE;
                 i++)
            {
                if (state->execState.arrayA[i] != 1)
                    continue;
                if (skip > 0)
                {
                    skip--;
                    continue;
                }
                if (y >= rowY - 6 && y < rowY + GATE_LINE_HEIGHT - 6)
                {
                    state->gateHoveredRow = i;
                    
                    if (state->mouseLeftDown && !state->gateOpenClickLatch && state->gateTeleportPendingRow < 0)
                    {
                        state->gateTeleportPendingRow = i;
                        state->gateTeleportAtTick = state->tick + GATE_TELEPORT_PRESS_TICKS;
                    }
                    break;
                }
                rowY += GATE_LINE_HEIGHT;
                rows++;
            }
            return 1;
        }
    }

    if (state->questWindowOpen && state->statusSheetLoaded)
    {
        
        RKC_DIB *top = RKC_UPDIB_GetFrame(&state->statusSheet, QUEST_WINDOW_FRAME_TOP);
        RKC_DIB *sideBar = RKC_UPDIB_GetFrame(&state->statusSheet, QUEST_WINDOW_FRAME_LEFT);
        RKC_DIB *bottomBar = RKC_UPDIB_GetFrame(&state->statusSheet, QUEST_WINDOW_FRAME_BOTTOM);
        if (top && sideBar && bottomBar && x >= 0 && x < top->width && y >= 0 &&
            y < top->height + sideBar->height + bottomBar->height)
        {
            RKC_DIB *chip = RKC_UPDIB_GetFrame(&state->statusSheet, QUEST_TAB_FRAME_UNSELECTED_BASE);
            long chipW = chip ? chip->width : 20, chipH = chip ? chip->height : 23;
            
            for (int t = 0; state->mouseLeftDown && t < QUEST_TAB_COUNT; t++)
            {
                long chipX = QUEST_TAB_X + t * (chipW + QUEST_TAB_GAP);
                if (y >= QUEST_TAB_Y && y < QUEST_TAB_Y + chipH && x >= chipX && x < chipX + chipW)
                {
                    state->questWindowTab = t;
                    state->questTooltipIndex = -1; 
                }
            }
            
            if (state->mouseLeftDown && state->questWindowTab == 0)
            {
                long lineY = QUEST_LINE_START_Y;
                long clicked = -1;
                for (long i = 0; i < QUEST_TITLES_EP1_COUNT; i++)
                {
                    long qs = i < RKC_RPG_SCRIPT_EXEC_QUEST_CAP ? state->execState.questArray[i] : 0;
                    if (qs != 1 && qs != 2)
                        continue;
                    if (y >= lineY - 4 && y < lineY + QUEST_LINE_HEIGHT - 4)
                    {
                        clicked = i;
                        break;
                    }
                    lineY += QUEST_LINE_HEIGHT;
                }
                if (!state->questTooltipClickLatch)
                {
                    state->questTooltipIndex = clicked; 
                    state->questTooltipClickLatch = 1;
                }
            }
            if (!state->mouseLeftDown)
                state->questTooltipClickLatch = 0;
            return 1;
        }
    }

    return 0;
}


typedef struct
{
    const char *start;
    int bytes;
    int isOption;
    int isQuit;
} DialogRow;


static int ParseDialogMessage(const char *text, DialogRow *rows, int maxRows)
{
    int rowCount = 0;
    const char *lineStart = text;
    for (const char *p = text;; p++)
    {
        if (*p == '\n' || *p == '\0')
        {
            int lineLen = (int)(p - lineStart);
            if (lineLen > 0 && lineStart[lineLen - 1] == '\r')
                lineLen--;
            if (lineLen >= 2 && lineStart[0] == '~' && lineStart[lineLen - 1] == '~' && rowCount < maxRows)
            {
                rows[rowCount].start = lineStart + 1;
                rows[rowCount].bytes = lineLen - 2;
                rows[rowCount].isOption = 1;
                rows[rowCount].isQuit = (lineLen - 2 == 4 && memcmp(lineStart + 1, "QUIT", 4) == 0);
                rowCount++;
            }
            else if (lineLen > 0)
            {
                const unsigned char *cursor = (const unsigned char *)lineStart;
                const unsigned char *lineEnd = (const unsigned char *)lineStart + lineLen;
                const unsigned char *rowStart = cursor;
                long width = 0;
                while (rowCount < maxRows && cursor < lineEnd)
                {
                    const unsigned char *before = cursor;
                    long frame, srcX, srcY, w;
                    if (!NextSjisGlyph(&cursor, &frame, &srcX, &srcY, &w))
                        break;
                    if (w > 0 && width + w > DIALOG_LINE_SAFETY_MAX_WIDTH)
                    {
                        rows[rowCount].start = (const char *)rowStart;
                        rows[rowCount].bytes = (int)(before - rowStart);
                        rows[rowCount].isOption = 0;
                        rows[rowCount].isQuit = 0;
                        rowCount++;
                        rowStart = before;
                        width = 0;
                        continue;
                    }
                    width += w;
                }
                if (rowCount < maxRows && cursor > rowStart)
                {
                    rows[rowCount].start = (const char *)rowStart;
                    rows[rowCount].bytes = (int)((const char *)cursor - (const char *)rowStart);
                    rows[rowCount].isOption = 0;
                    rows[rowCount].isQuit = 0;
                    rowCount++;
                }
            }
            if (*p == '\0')
                break;
            lineStart = p + 1;
        }
    }
    return rowCount;
}


static int ComputeDialogLayout(DemoState *state, DialogRow *rows, long *relY, long *outBubbleX, long *outBubbleY,
                               long *outBubbleWidth, long *outBubbleHeight)
{
    if (state->dialogQueueCount <= 0 || !state->fontLoaded)
        return 0;
    int rowCount = ParseDialogMessage(state->dialogQueue[0].text, rows, DIALOG_MAX_ROWS);
    if (rowCount == 0)
        return 0;

    long y = DIALOG_BUBBLE_PADDING;
    
    long maxRowWidth = 0;
    char measureBuf[DIALOG_LINE_MAX];
    for (int i = 0; i < rowCount; i++)
    {
        if (rows[i].isQuit)
            y += DIALOG_OPTION_QUIT_GAP;
        relY[i] = y;
        y += DIALOG_LINE_HEIGHT;

        int bytes = rows[i].bytes;
        if (bytes >= (int)sizeof(measureBuf))
            bytes = (int)sizeof(measureBuf) - 1;
        memcpy(measureBuf, rows[i].start, (size_t)bytes);
        measureBuf[bytes] = '\0';
        long rowWidth = MeasureTextSJIS(state, measureBuf);
        if (rowWidth > maxRowWidth)
            maxRowWidth = rowWidth;
    }
    
    if (maxRowWidth > DIALOG_LINE_SAFETY_MAX_WIDTH)
        maxRowWidth = DIALOG_LINE_SAFETY_MAX_WIDTH;
    if (maxRowWidth < 2 * DIALOG_BUBBLE_CORNER)
        maxRowWidth = 2 * DIALOG_BUBBLE_CORNER;
    long bubbleWidth = maxRowWidth + 2 * DIALOG_BUBBLE_PADDING;
    long bubbleHeight = y + DIALOG_BUBBLE_PADDING;

    
    long anchorWorldX, anchorWorldY;
    if (state->dialogQueue[0].characterNo < 0 ||
        !LookupLiveSpawnPos(state, state->dialogQueue[0].characterNo, &anchorWorldX, &anchorWorldY))
    {
        anchorWorldX = state->playerX;
        anchorWorldY = state->playerY;
    }
    long anchorScreenX, anchorScreenY;
    WorldToScreen(state, anchorWorldX, anchorWorldY, &anchorScreenX, &anchorScreenY);
    long anchorX = anchorScreenX - state->cameraX;
    long anchorY = anchorScreenY - state->cameraY;

    long bubbleX = anchorX - bubbleWidth / 2;
    long bubbleY = anchorY - DIALOG_BUBBLE_ANCHOR_GAP_Y - bubbleHeight;
    
    if (bubbleX < 4)
        bubbleX = 4;
    if (bubbleX + bubbleWidth > APP_WIDTH - 4)
        bubbleX = APP_WIDTH - 4 - bubbleWidth;
    if (bubbleY < 4)
        bubbleY = 4;
    if (bubbleY + bubbleHeight > APP_HEIGHT - 4)
        bubbleY = APP_HEIGHT - 4 - bubbleHeight;

    *outBubbleX = bubbleX;
    *outBubbleY = bubbleY;
    *outBubbleWidth = bubbleWidth;
    *outBubbleHeight = bubbleHeight;
    return rowCount;
}


int FindDialogOptionAtScreenPoint(DemoState *state, long x, long y)
{
    DialogRow rows[DIALOG_MAX_ROWS];
    long relY[DIALOG_MAX_ROWS];
    long bubbleX, bubbleY, bubbleWidth, bubbleHeight;
    int rowCount = ComputeDialogLayout(state, rows, relY, &bubbleX, &bubbleY, &bubbleWidth, &bubbleHeight);
    for (int i = 0; i < rowCount; i++)
    {
        if (!rows[i].isOption)
            continue;
        long rowTop = bubbleY + relY[i];
        if (x >= bubbleX && x < bubbleX + bubbleWidth && y >= rowTop && y < rowTop + DIALOG_LINE_HEIGHT)
            return i;
    }
    return -1;
}


int DialogHasOptions(DemoState *state)
{
    DialogRow rows[DIALOG_MAX_ROWS];
    long relY[DIALOG_MAX_ROWS];
    long bubbleX, bubbleY, bubbleWidth, bubbleHeight;
    int rowCount = ComputeDialogLayout(state, rows, relY, &bubbleX, &bubbleY, &bubbleWidth, &bubbleHeight);
    for (int i = 0; i < rowCount; i++)
        if (rows[i].isOption)
            return 1;
    return 0;
}


static int ResolveDialogTailFrame(const DemoState *state, long characterNo)
{
    for (long i = 0; i < state->liveSpawnCount; i++)
        if (state->liveSpawns[i].characterNo == characterNo)
            return (state->liveSpawns[i].block == 2 &&
                    state->liveSpawns[i].field8 >= LIVE_SPAWN_BLOCK2_PARTNER_OFFSET)
                       ? HUKIDASI_FRAME_TAIL_PARTNER
                       : HUKIDASI_FRAME_TAIL_NPC;
    return HUKIDASI_FRAME_TAIL_NPC;
}


void DrawDialog(DemoState *state)
{
    if (state->dialogQueueCount <= 0)
        return;

    DialogRow rows[DIALOG_MAX_ROWS];
    long relY[DIALOG_MAX_ROWS];
    long bubbleX, bubbleY, bubbleWidth, bubbleHeight;
    int rowCount = ComputeDialogLayout(state, rows, relY, &bubbleX, &bubbleY, &bubbleWidth, &bubbleHeight);
    if (rowCount == 0)
        return;

    long corner = DIALOG_BUBBLE_CORNER, border = DIALOG_BUBBLE_BORDER;
    if (state->hukidasiLoaded)
    {
        
        DrawRect(&state->canvas, bubbleX + corner, bubbleY + corner, bubbleWidth - 2 * corner,
                 bubbleHeight - 2 * corner, DIALOG_BUBBLE_FILL);
        DrawRect(&state->canvas, bubbleX + corner, bubbleY + border, bubbleWidth - 2 * corner, corner - border,
                 DIALOG_BUBBLE_FILL);
        DrawRect(&state->canvas, bubbleX + corner, bubbleY + bubbleHeight - corner, bubbleWidth - 2 * corner,
                 corner - border, DIALOG_BUBBLE_FILL);
        DrawRect(&state->canvas, bubbleX + border, bubbleY + corner, corner - border, bubbleHeight - 2 * corner,
                 DIALOG_BUBBLE_FILL);
        DrawRect(&state->canvas, bubbleX + bubbleWidth - corner, bubbleY + corner, corner - border,
                 bubbleHeight - 2 * corner, DIALOG_BUBBLE_FILL);

        
        DrawRect(&state->canvas, bubbleX + corner, bubbleY, bubbleWidth - 2 * corner, border,
                 DIALOG_BUBBLE_BORDER_COLOR);
        DrawRect(&state->canvas, bubbleX + corner, bubbleY + bubbleHeight - border, bubbleWidth - 2 * corner, border,
                 DIALOG_BUBBLE_BORDER_COLOR);
        DrawRect(&state->canvas, bubbleX, bubbleY + corner, border, bubbleHeight - 2 * corner,
                 DIALOG_BUBBLE_BORDER_COLOR);
        DrawRect(&state->canvas, bubbleX + bubbleWidth - border, bubbleY + corner, border, bubbleHeight - 2 * corner,
                 DIALOG_BUBBLE_BORDER_COLOR);

        
        static const int cornerFrame[4] = {0, 2, 1, 3};
        long cx[4] = {bubbleX, bubbleX + bubbleWidth - corner, bubbleX, bubbleX + bubbleWidth - corner};
        long cy[4] = {bubbleY, bubbleY, bubbleY + bubbleHeight - corner, bubbleY + bubbleHeight - corner};
        for (int i = 0; i < 4; i++)
        {
            RKC_DIB *sprite = RKC_UPDIB_GetFrame(&state->hukidasi, cornerFrame[i]);
            if (sprite)
                RKC_DIB_TransferToDIBEx(&state->canvas, cx[i], cy[i], sprite->width, sprite->height, sprite, 0, 0, 0,
                                        1000);
        }

        RKC_DIB *tail = RKC_UPDIB_GetFrame(&state->hukidasi,
                                           ResolveDialogTailFrame(state, state->dialogQueue[0].characterNo));
        if (tail)
            RKC_DIB_TransferToDIBEx(&state->canvas,
                                    bubbleX + bubbleWidth / 2 - tail->width / 2 + DIALOG_TAIL_OFFSET_X,
                                    bubbleY + bubbleHeight - DIALOG_TAIL_OFFSET_Y, tail->width, tail->height, tail, 0,
                                    0, 0, 1000);
    }
    else
    {
        
        DrawRect(&state->canvas, bubbleX, bubbleY, bubbleWidth, bubbleHeight, DIALOG_BUBBLE_BORDER_COLOR);
        DrawRect(&state->canvas, bubbleX + border, bubbleY + border, bubbleWidth - 2 * border,
                 bubbleHeight - 2 * border, DIALOG_BUBBLE_FILL);
    }

    char rowBuf[DIALOG_LINE_MAX];
    for (int i = 0; i < rowCount; i++)
    {
        int bytes = rows[i].bytes;
        if (bytes >= (int)sizeof(rowBuf))
            bytes = (int)sizeof(rowBuf) - 1;
        memcpy(rowBuf, rows[i].start, (size_t)bytes);
        rowBuf[bytes] = '\0';
        
        TextStyle style = (rows[i].isOption && state->dialogHoveredOption == i) ? TEXT_STYLE_RED : TEXT_STYLE_BLACK;
        DrawTextSJIS(state, bubbleX + DIALOG_BUBBLE_PADDING, bubbleY + relY[i], rowBuf, 0, style, 1000);
    }
}


void DrawGateLabels(DemoState *state)
{
    if (!state->fontLoaded)
        return;
    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        const LiveSpawn *spawn = &state->liveSpawns[i];
        if (spawn->block != 1 || spawn->field8 != GATE_RUNE_CIRCLE_FIELD8 || spawn->gateHighlightFadeTicks <= 0 ||
            spawn->gateDestinationName[0] == '\0')
            continue;

        long trans = 1000 * spawn->gateHighlightFadeTicks / GATE_HIGHLIGHT_FADE_TICKS;
        long screenX, screenY;
        WorldToScreen(state, spawn->x, spawn->y, &screenX, &screenY);
        long anchorX = screenX - state->cameraX;
        long anchorY = screenY - state->cameraY;
        long labelW = MeasureTextSJIS(state, spawn->gateDestinationName);
        DrawTextSJIS(state, anchorX - labelW / 2, anchorY - GATE_LABEL_GAP_Y, spawn->gateDestinationName, 0,
                     TEXT_STYLE_WHITE_SHADOWED, trans);
    }
}


void DrawTransportCircleLabel(DemoState *state)
{
    if (!state->fontLoaded || !state->transportCircleHovered)
        return;
    long cx, cy;
    if (!ActiveTransportCircleHere(state, &cx, &cy, NULL))
        return;
    long screenX, screenY;
    WorldToScreen(state, cx, cy, &screenX, &screenY);
    long anchorX = screenX - state->cameraX;
    long anchorY = screenY - state->cameraY;
    long labelW = MeasureTextSJIS(state, "PLAYER");
    DrawTextSJIS(state, anchorX - labelW / 2, anchorY - TRANSPORT_CIRCLE_LABEL_GAP_Y, "PLAYER", 0,
                 TEXT_STYLE_WHITE_SHADOWED, 1000);
}


void DrawGateRings(DemoState *state)
{
    if (state->gateRingTemplate.kind != LIVE_SPAWN_SPRITE_CAF)
        return;
    const RKC_RPGSCRN_CAF_Direction *ringDir = &state->gateRingTemplate.caf.charts[0].directions[8];
    if (ringDir->maxFrameCount <= 0)
        return;

    for (long i = 0; i < state->liveSpawnCount; i++)
    {
        const LiveSpawn *spawn = &state->liveSpawns[i];
        if (spawn->block != 1 || spawn->field8 != GATE_RUNE_CIRCLE_FIELD8 || spawn->gateHighlightFadeTicks <= 0)
            continue;

        long gateTrans = 1000 * spawn->gateHighlightFadeTicks / GATE_HIGHLIGHT_FADE_TICKS;
        long screenX, screenY;
        WorldToScreen(state, spawn->x, spawn->y, &screenX, &screenY);
        long destX = screenX - state->cameraX;
        long destY = screenY - state->cameraY;

        long ringFrame = (long)(state->tick / GATE_RING_TICKS_PER_FRAME) % ringDir->maxFrameCount;
        RKC_RPGSCRN_CAF_DrawCmd ringCmds[8];
        int ringN = RKC_RPGSCRN_CAF_Resolve(&state->gateRingTemplate.caf, 0, 8, ringFrame,
                                            &state->gateRingTemplate.animNjp, &state->gateRingTemplate.animSdw, NULL,
                                            NULL, NULL, NULL, 0, ringCmds, 8);
        for (int c = 0; c < ringN; c++)
        {
            const RKC_DIB *ringIcon = ringCmds[c].icon;
            
            long ringDestX = destX + ringCmds[c].offsetX;
            long ringDestY = destY - GATE_RING_GAP_Y + ringCmds[c].offsetY;
            long ringTrans = ringCmds[c].trans * gateTrans / 1000;
            
            if (ringCmds[c].isAdditive)
                RKC_DIB_TransferToDIBAdditive(&state->canvas, ringDestX, ringDestY, ringIcon->width, ringIcon->height,
                                              ringIcon, 0, 0, 0, ringTrans);
            else
                RKC_DIB_TransferToDIBEx(&state->canvas, ringDestX, ringDestY, ringIcon->width, ringIcon->height,
                                        ringIcon, 0, 0, 0, ringTrans);
        }
    }
}


void DrawTransportCircle(DemoState *state)
{
    long tcx, tcy;
    if (!ActiveTransportCircleHere(state, &tcx, &tcy, NULL))
        return; 
    LiveSpawnTemplate *tmpl = &state->transportCircleTemplate;

    long screenX, screenY;
    WorldToScreen(state, tcx, tcy, &screenX, &screenY);
    long destX = screenX - state->cameraX;
    long destY = screenY - state->cameraY;

    long elapsed = (long)(state->tick - state->transportCircle.spawnTick);

    
    long layerCount = RKC_UPDIB_GetPatternCount(&tmpl->staticNjp);
    for (long layer = 0; layer < layerCount; layer++)
    {
        long layerStart = layer * TRANSPORT_CIRCLE_LAYER_INTERVAL_TICKS;
        if (elapsed < layerStart)
            break; 
        RKC_DIB *icon = RKC_UPDIB_GetPatternIcon(&tmpl->staticNjp, layer);
        if (!icon)
            continue;
        long offX, offY;
        RKC_UPDIB_GetPatternOffset(&tmpl->staticNjp, layer, &offX, &offY);

        long fallElapsed = elapsed - layerStart;
        long fallOffsetY = 0;
        long trans = 1000;
        if (fallElapsed < TRANSPORT_CIRCLE_FALL_DURATION_TICKS)
        {
            long remaining = TRANSPORT_CIRCLE_FALL_DURATION_TICKS - fallElapsed;
            fallOffsetY = -(TRANSPORT_CIRCLE_FALL_HEIGHT_PX * remaining / TRANSPORT_CIRCLE_FALL_DURATION_TICKS);
            trans = 1000 * fallElapsed / TRANSPORT_CIRCLE_FALL_DURATION_TICKS;
        }
        RKC_DIB_TransferToDIBAdditive(&state->canvas, destX + offX, destY + offY + fallOffsetY, icon->width,
                                      icon->height, icon, 0, 0, 0, trans);
    }

    
    long buildupTicks = layerCount * TRANSPORT_CIRCLE_LAYER_INTERVAL_TICKS;
    if (elapsed >= buildupTicks && tmpl->caf.chartCount > 0)
    {
        int burstDir = RKC_RPGSCRN_CAF_NUM_DIRECTIONS - 1;
        const RKC_RPGSCRN_CAF_Direction *bdir = &tmpl->caf.charts[0].directions[burstDir];
        if (bdir->maxFrameCount > 0)
        {
            long burstElapsed = (elapsed - buildupTicks) % TRANSPORT_CIRCLE_BURST_PERIOD_TICKS;
            long burstFrame = burstElapsed / TRANSPORT_CIRCLE_BURST_TICKS_PER_FRAME;
            if (burstFrame < bdir->maxFrameCount)
            {
                RKC_RPGSCRN_CAF_DrawCmd cmds[8];
                int n = RKC_RPGSCRN_CAF_Resolve(&tmpl->caf, 0, burstDir, burstFrame, &tmpl->animNjp, &tmpl->animSdw,
                                                NULL, NULL, NULL, NULL, 0, cmds, 8);
                for (int c = 0; c < n; c++)
                {
                    const RKC_DIB *icon = cmds[c].icon;
                    if (cmds[c].isAdditive)
                        RKC_DIB_TransferToDIBAdditive(&state->canvas, destX + cmds[c].offsetX, destY + cmds[c].offsetY,
                                                      icon->width, icon->height, icon, 0, 0, 0, cmds[c].trans);
                    else
                        RKC_DIB_TransferToDIBEx(&state->canvas, destX + cmds[c].offsetX, destY + cmds[c].offsetY,
                                                icon->width, icon->height, icon, 0, 0, 0, cmds[c].trans);
                }
            }
        }
    }
}


void WorldToScreen(const DemoState *state, long worldX, long worldY, long *outScreenX, long *outScreenY)
{
    (void)state;
    *outScreenX = (worldX - worldY) * RKC_RPGSCRN_SCENE_SCALE_X / 100;
    *outScreenY = (worldX + worldY) * RKC_RPGSCRN_SCENE_SCALE_Y / 100;
}


void UpdateCameraFromPlayer(DemoState *state)
{
    int leftWindowOpen = state->minimapOpen || state->statusMagicOpen || state->gateWindowOpen; 
    long targetCenterX = APP_WIDTH / 2;
    if (leftWindowOpen && !state->inventoryOpen)
        targetCenterX = (STATUS_MAGIC_WINDOW_WIDTH + APP_WIDTH) / 2; 
    else if (state->inventoryOpen && !leftWindowOpen)
        targetCenterX = INVENTORY_WINDOW_X / 2; 

    WorldToScreen(state, state->playerX, state->playerY, &state->cameraX, &state->cameraY);
    state->cameraX -= targetCenterX;
    state->cameraY -= APP_HEIGHT / 2;
}


void ScreenToWorld(const DemoState *state, long screenX, long screenY, long *outWorldX, long *outWorldY)
{
    (void)state;
    long px = screenX;
    long py = screenY;
    long sX = RKC_RPGSCRN_SCENE_SCALE_X, sY = RKC_RPGSCRN_SCENE_SCALE_Y;
    long denom = sX * sY * 2;
    *outWorldX = ((sX * py + sY * px) * 100) / denom;
    *outWorldY = ((sX * py - sY * px) * 100) / denom;
    if (px * sY + py * sX < 0)
        *outWorldX -= 1;
    if (py * sX - px * sY < 0)
        *outWorldY -= 1;
}


static void DrawWorldItemIcon(DemoState *state, const WorldItem *item, long destX, long destY, long maxW, long maxH,
                              int isHovered)
{
    
    if (state->itemDataLoaded)
    {
        const RKC_RPG_ITEMDATA_Record *rec =
            RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, (int)item->kind, item->templateId);
        long sheetIdx = -1, frame = -1;
        if (rec && RKC_RPG_ITEMDATA_GetGroundSprite(rec, &sheetIdx, &frame) && sheetIdx >= 0 &&
            sheetIdx < WEAPON_WORLD_SHEET_COUNT && state->weaponWorldSheetLoaded[sheetIdx] && frame >= 0 &&
            frame < RKC_UPDIB_GetFrameCount(&state->weaponWorldSheets[sheetIdx]))
        {
            RKC_DIB *icon = RKC_UPDIB_GetFrame(&state->weaponWorldSheets[sheetIdx], frame);
            if (icon)
            {
                
                long tintR = 1000, tintG = 1000, tintB = 1000;
                int haveColorSource = 0;
                if (item->kind == 0)
                {
                    const RKC_RPG_ITEMDATA_Kind0Tail *tail = RKC_RPG_ITEMDATA_GetKind0Tail(rec);
                    if (tail)
                    {
                        tintR = (long)tail->cellBlockTintR;
                        tintG = (long)tail->cellBlockTintG;
                        tintB = (long)tail->cellBlockTintB;
                        haveColorSource = 1;
                    }
                }
                else if (item->kind == 1 && (ArmorNameFitsSlot(item->name, EQUIPMENT_BODY_SLOT_INDEX) ||
                                             ArmorNameFitsSlot(item->name, EQUIPMENT_SHIELD_SLOT_INDEX)))
                {
                    const RKC_RPG_ITEMDATA_Kind1Tail *tail = RKC_RPG_ITEMDATA_GetKind1Tail(rec);
                    if (tail)
                    {
                        tintR = (long)tail->cellBlockTintR;
                        tintG = (long)tail->cellBlockTintG;
                        tintB = (long)tail->cellBlockTintB;
                        haveColorSource = 1;
                    }
                }

                if (isHovered)
                    DrawIconScaled(&state->canvas, icon, destX, destY, maxW, maxH, 0, 1000, HOVER_HIGHLIGHT_TINT,
                                   HOVER_HIGHLIGHT_TINT, HOVER_HIGHLIGHT_TINT);
                else if (haveColorSource && !GroundIconIsAlreadyColored(icon))
                    DrawIconColorized(&state->canvas, icon, destX, destY, maxW, maxH, tintR * 255 / 1000,
                                      tintG * 255 / 1000, tintB * 255 / 1000);
                else
                    DrawIconScaled(&state->canvas, icon, destX, destY, maxW, maxH, 0, 1000, 1000, 1000, 1000);
                return;
            }
        }
    }

    
    if (item->kind == 4 && item->templateId == 0)
    {
        DrawGoldPileIcon(state, destX, destY, maxW, maxH, isHovered);
        return;
    }

    int kind1UsesCellBlock =
        item->kind == 1 && (ArmorNameFitsSlot(item->name, EQUIPMENT_BODY_SLOT_INDEX) ||
                            ArmorNameFitsSlot(item->name, EQUIPMENT_SHIELD_SLOT_INDEX));
    if ((item->kind == 0 || kind1UsesCellBlock) && state->itemDataLoaded &&
        state->playerTemplate.kind == LIVE_SPAWN_SPRITE_CAF)
    {
        const RKC_RPG_ITEMDATA_Record *rec =
            RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, (int)item->kind, item->templateId);
        long cellBlockIndex = -1, tintR = 1000, tintG = 1000, tintB = 1000;
        if (rec && item->kind == 0)
        {
            const RKC_RPG_ITEMDATA_Kind0Tail *tail = RKC_RPG_ITEMDATA_GetKind0Tail(rec);
            if (tail)
            {
                cellBlockIndex = tail->cellBlockIndex;
                tintR = (long)tail->cellBlockTintR;
                tintG = (long)tail->cellBlockTintG;
                tintB = (long)tail->cellBlockTintB;
            }
        }
        else if (rec)
        {
            const RKC_RPG_ITEMDATA_Kind1Tail *tail = RKC_RPG_ITEMDATA_GetKind1Tail(rec);
            if (tail)
            {
                cellBlockIndex = tail->cellBlockIndex;
                tintR = (long)tail->cellBlockTintR;
                tintG = (long)tail->cellBlockTintG;
                tintB = (long)tail->cellBlockTintB;
            }
        }

        
        long cellBlockCount = 0;
        if (cellBlockIndex >= 0 && 0 < state->playerTemplate.caf.chartCount &&
            state->playerTemplate.caf.charts[0].directions[1].maxFrameCount > 0)
            cellBlockCount = state->playerTemplate.caf.charts[0].directions[1].cellBlockCount;
        if (cellBlockIndex >= 0 && cellBlockIndex < cellBlockCount && cellBlockCount <= PLAYER_MAX_CELL_BLOCKS)
        {
            if (isHovered)
                tintR = tintG = tintB = HOVER_HIGHLIGHT_TINT;
            unsigned int mask[PLAYER_MAX_CELL_BLOCKS] = {0};
            unsigned short mtR[PLAYER_MAX_CELL_BLOCKS], mtG[PLAYER_MAX_CELL_BLOCKS], mtB[PLAYER_MAX_CELL_BLOCKS];
            for (long b = 0; b < cellBlockCount; b++)
                mtR[b] = mtG[b] = mtB[b] = 1000;
            mask[cellBlockIndex] = 1;
            mtR[cellBlockIndex] = (unsigned short)tintR;
            mtG[cellBlockIndex] = (unsigned short)tintG;
            mtB[cellBlockIndex] = (unsigned short)tintB;
            RKC_RPGSCRN_CAF_DrawCmd cmds[2];
            int n = RKC_RPGSCRN_CAF_Resolve(&state->playerTemplate.caf, 0, 1, 0, &state->playerTemplate.animNjp,
                                            &state->playerTemplate.animSdw, mask, mtR, mtG, mtB, cellBlockCount, cmds,
                                            2);
            for (int i = 0; i < n; i++)
                if (!cmds[i].isShadow)
                    DrawIconScaled(&state->canvas, cmds[i].icon, destX, destY, maxW, maxH, 0, 1000, cmds[i].tintR,
                                  cmds[i].tintG, cmds[i].tintB);
            if (n > 0)
                return;
        }
    }

    long sheetIndex, frame, tintR, tintG, tintB;
    ResolveItemIcon(state, item->kind, item->templateId, &sheetIndex, &frame, &tintR, &tintG, &tintB);
    if (isHovered)
        tintR = tintG = tintB = HOVER_HIGHLIGHT_TINT;
    DrawIconByFrame(state, sheetIndex, frame, destX, destY, maxW, maxH, 1000, tintR, tintG, tintB);
}


static void DrawWorldItemHoverLabel(DemoState *state, const WorldItem *item, long destX, long iconTopY)
{
    char label[160];
    if (item->isGold)
        
    else
    {
        if (item->name[0] == '\0')
            return;
        
    }

    long textW = MeasureTextSJIS(state, label);
    long boxW = textW + 2 * HOVER_LABEL_PADDING_X;
    long boxX = destX - boxW / 2;
    long boxY = iconTopY - HOVER_LABEL_GAP_Y - HOVER_LABEL_HEIGHT;
    DrawRect(&state->canvas, boxX, boxY, boxW, HOVER_LABEL_HEIGHT, HOVER_NPC_LABEL_FILL);
    DrawRectOutline(&state->canvas, boxX, boxY, boxW, HOVER_LABEL_HEIGHT, 1, HOVER_LABEL_BORDER_COLOR);

    TextStyle style = TEXT_STYLE_WHITE_SHADOWED;
    if (!item->isGold)
    {
        long affixTier = 0;
        if (state->itemDataLoaded)
        {
            const RKC_RPG_ITEMDATA_Record *rec =
                RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, (int)item->kind, item->templateId);
            affixTier = RKC_RPG_ITEMDATA_GetAffixTier(rec);
        }
        style = affixTier == 1   ? TEXT_STYLE_BLUE_SHADOWED
                : affixTier == 2 ? TEXT_STYLE_YELLOW_SHADOWED
                : affixTier == 3 ? TEXT_STYLE_ORANGE_SHADOWED
                                 : TEXT_STYLE_WHITE_SHADOWED;
    }
    DrawTextSJIS(state, boxX + HOVER_LABEL_PADDING_X, boxY + (HOVER_LABEL_HEIGHT - FONT_CELL_HEIGHT) / 2, label, 0,
                style, 1000);
}


typedef struct MaskRect
{
    long x0, y0, x1, y1;
} MaskRect;


static void BlitLightSource(DemoState *state, long worldX, long worldY, long frameIndex, MaskRect *touched)
{
    RKC_DIB *frame = RKC_UPDIB_GetFrame(&state->darknessTemplate, frameIndex);
    if (!frame)
        return;
    long screenX, screenY;
    WorldToScreen(state, worldX, worldY, &screenX, &screenY);
    long destX = screenX - state->cameraX - frame->width / 2;
    long destY = screenY - state->cameraY - frame->height / 2;
    RKC_DIB_TransferToDIBAdditive(&state->darknessMask, destX, destY, frame->width, frame->height, frame, 0, 0, -1,
                                  1000);
    if (touched)
    {
        long x0 = destX < 0 ? 0 : destX;
        long y0 = destY < 0 ? 0 : destY;
        long x1 = destX + frame->width > state->darknessMask.width ? state->darknessMask.width : destX + frame->width;
        long y1 = destY + frame->height > state->darknessMask.height ? state->darknessMask.height
                                                                     : destY + frame->height;
        if (x0 < x1 && y0 < y1)
        {
            if (x0 < touched->x0)
                touched->x0 = x0;
            if (y0 < touched->y0)
                touched->y0 = y0;
            if (x1 > touched->x1)
                touched->x1 = x1;
            if (y1 > touched->y1)
                touched->y1 = y1;
        }
    }
}


static unsigned char g_darknessLut[256][256]; 
static long g_darknessLutStrength = -1;       

static void ApplyDarknessMask(RKC_DIB *canvas, const RKC_DIB *mask, long strength)
{
    if (g_darknessLutStrength != strength)
    {
        for (long level = 0; level < 256; level++)
            for (long v = 0; v < 256; v++)
            {
                long darkened = v * level / 255;
                g_darknessLut[level][v] = (unsigned char)(v + (darkened - v) * strength / 1000);
            }
        g_darknessLutStrength = strength;
    }

    for (long y = 0; y < canvas->height; y++)
    {
        unsigned char *canvasRow = canvas->pixels + (size_t)(canvas->height - 1 - y) * (size_t)canvas->alignWidth;
        const unsigned char *maskRow = mask->pixels + (size_t)(mask->height - 1 - y) * (size_t)mask->alignWidth;
        for (long x = 0; x < canvas->width; x++)
        {
            unsigned char *cpx = canvasRow + x * 3;
            const unsigned char *lut = g_darknessLut[maskRow[x * 3]];
            cpx[0] = lut[cpx[0]];
            cpx[1] = lut[cpx[1]];
            cpx[2] = lut[cpx[2]];
        }
    }
}


static void ApplyLightGlow(RKC_DIB *canvas, const RKC_DIB *mask, long strength, const MaskRect *touched)
{
    long y0 = touched->y0 < 0 ? 0 : touched->y0;
    long y1 = touched->y1 > canvas->height ? canvas->height : touched->y1;
    long x0 = touched->x0 < 0 ? 0 : touched->x0;
    long x1 = touched->x1 > canvas->width ? canvas->width : touched->x1;
    for (long y = y0; y < y1; y++)
    {
        unsigned char *canvasRow = canvas->pixels + (size_t)(canvas->height - 1 - y) * (size_t)canvas->alignWidth;
        const unsigned char *maskRow = mask->pixels + (size_t)(mask->height - 1 - y) * (size_t)mask->alignWidth;
        for (long x = x0; x < x1; x++)
        {
            long level = maskRow[x * 3];
            if (level == 0)
                continue;
            unsigned char *cpx = canvasRow + x * 3;
            long add = level * strength / 1000;
            for (int c = 0; c < 3; c++)
            {
                long v = cpx[c] + add;
                cpx[c] = (unsigned char)(v > 255 ? 255 : v);
            }
        }
    }
}


void DrawDisplayDarkness(DemoState *state)
{
    if (!state->displayDarknessEnabled || !state->darknessTemplateLoaded || !state->mctLoaded)
        return;
    long strength = DARKNESS_INTENSITY_OFF - state->mct.darknessIntensity;
    if (strength <= 0)
        return;
    if (strength > 1000)
        strength = 1000;

    RKC_DIB_FillByte(&state->darknessMask, DARKNESS_AMBIENT_FLOOR);

    BlitLightSource(state, state->playerX, state->playerY, DARKNESS_PLAYER_LIGHT_FRAME, NULL);

    for (long i = 0; i < state->liveSpawnCount; i++)
        if (state->liveSpawns[i].block == 2)
            BlitLightSource(state, state->liveSpawns[i].x, state->liveSpawns[i].y, DARKNESS_NPC_LIGHT_FRAME, NULL);

    for (long i = 0; i < state->worldItemCount; i++)
    {
        const WorldItem *item = &state->worldItems[i];
        if (item->pickedUp || item->isGold || !state->itemDataLoaded)
            continue;
        const RKC_RPG_ITEMDATA_Record *rec = RKC_RPG_ITEMDATA_GetFromTemplateId(&state->itemData, (int)item->kind,
                                                                                item->templateId);
        long affixTier = RKC_RPG_ITEMDATA_GetAffixTier(rec);
        if (affixTier == 2 || affixTier == 3)
            BlitLightSource(state, item->x, item->y, DARKNESS_ITEM_LIGHT_FRAME, NULL);
    }

    ApplyDarknessMask(&state->canvas, &state->darknessMask, strength);

    
    RKC_DIB_FillByte(&state->darknessMask, 0);
    MaskRect glowRect = {LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN};
    BlitLightSource(state, state->playerX, state->playerY, DARKNESS_GLOW_FRAME, &glowRect);
    for (long i = 0; i < state->liveSpawnCount; i++)
        if (state->liveSpawns[i].block == 2)
            BlitLightSource(state, state->liveSpawns[i].x, state->liveSpawns[i].y, DARKNESS_GLOW_FRAME, &glowRect);
    if (glowRect.x0 < glowRect.x1 && glowRect.y0 < glowRect.y1)
        ApplyLightGlow(&state->canvas, &state->darknessMask, DARKNESS_GLOW_STRENGTH, &glowRect);
}


void DrawWorldItem(DemoState *state, WorldItem *item)
{
    if (item->pickedUp)
        return;
    long screenX, screenY;
    WorldToScreen(state, item->x, item->y, &screenX, &screenY);
    long destX = screenX - state->cameraX, destY = screenY - state->cameraY;

    if (!item->resolved)
    {
        DrawMarker(&state->canvas, destX, destY, 3, 0, 220, 220); 
        return;
    }

    
    long fallOffsetY = 0;
    long jumpElapsed = item->jumpEffectTick >= 0 ? (long)state->tick - item->jumpEffectTick : -1;
    if (jumpElapsed >= 0 && jumpElapsed < ITEM_JUMP_EFFECT_TICKS)
    {
        
        double t = (double)jumpElapsed / (double)ITEM_JUMP_EFFECT_TICKS;
        fallOffsetY = (long)(ITEM_JUMP_EFFECT_HEIGHT_Y * 4.0 * t * (1.0 - t));
    }
    else
    {
        long dropElapsed = (long)(state->tick - item->dropTick);
        if (item->dropTick != 0 && dropElapsed >= 0 && dropElapsed < ITEM_DROP_FALL_TICKS)
            fallOffsetY =
                ITEM_DROP_FALL_START_OFFSET_Y * (ITEM_DROP_FALL_TICKS - dropElapsed) / ITEM_DROP_FALL_TICKS;
    }

    
    int isHovered = state->hoveredWorldItemIndex >= 0 && item == &state->worldItems[state->hoveredWorldItemIndex];

    long iconX = destX - ITEM_WORLD_ICON_SIZE / 2, iconY = destY - ITEM_WORLD_ICON_SIZE / 2 - fallOffsetY;
    DrawWorldItemIcon(state, item, iconX, iconY, ITEM_WORLD_ICON_SIZE, ITEM_WORLD_ICON_SIZE, isHovered);

    
    if (isHovered)
        DrawWorldItemHoverLabel(state, item, destX, iconY);

    item->hitTestHalfWidth = item->hitTestHalfHeight = ITEM_WORLD_ICON_SIZE / 2;
}


long CafSpriteYOffset(int n, const RKC_RPGSCRN_CAF_DrawCmd *cmds)
{
    if (n <= 0)
        return 0;
    return cmds[n - 1].icon->height / 10;
}


long CafSpriteYOffsetCached(long *cacheValue, int *cacheChart, int *cacheDirection, int *cacheValid, long chart,
                            long direction, int n, const RKC_RPGSCRN_CAF_DrawCmd *cmds)
{
    if (!*cacheValid || *cacheChart != (int)chart || *cacheDirection != (int)direction)
    {
        *cacheValue = CafSpriteYOffset(n, cmds);
        *cacheChart = (int)chart;
        *cacheDirection = (int)direction;
        *cacheValid = 1;
    }
    return *cacheValue;
}


long TallestCafIconHeight(int n, const RKC_RPGSCRN_CAF_DrawCmd *cmds)
{
    long tallest = 0;
    for (int i = 0; i < n; i++)
        if (cmds[i].icon->height > tallest)
            tallest = cmds[i].icon->height;
    return tallest;
}


long TallestCafIconWidth(int n, const RKC_RPGSCRN_CAF_DrawCmd *cmds)
{
    long tallest = 0, width = 0;
    for (int i = 0; i < n; i++)
        if (cmds[i].icon->height > tallest)
        {
            tallest = cmds[i].icon->height;
            width = cmds[i].icon->width;
        }
    return width;
}


void DrawJudgeOverlay(DemoState *state)
{
    const int step = 4; 
    for (int sy = 0; sy < APP_HEIGHT; sy += step)
    {
        for (int sx = 0; sx < APP_WIDTH; sx += step)
        {
            long worldX, worldY;
            ScreenToWorld(state, sx + state->cameraX, sy + state->cameraY, &worldX, &worldY);
            if (RKC_RPGSCRN_GROUND_IsBlocked(&state->ground, worldX, worldY))
                DrawMarker(&state->canvas, sx, sy, 1, 0, 0, 220); 
        }
    }
}


static void TrimName(const char *name, char *out, size_t outSize)
{
    out[0] = '\0';
    if (!name)
        return;
    const char *start = name;
    while (*start == ' ')
        start++;
    const char *end = start + strlen(start);
    while (end > start && end[-1] == ' ')
        end--;
    size_t len = (size_t)(end - start);
    if (len >= outSize)
        len = outSize - 1;
    memcpy(out, start, len);
    out[len] = '\0';
}


int IsHoverableLiveSpawn(const LiveSpawn *spawn)
{
    if (spawn->block != 1)
        return 1;
    if (spawn->field8 == GATE_BODY_BASE_FIELD8)
        return spawn->patternIndex == GATE_BODY_PATTERN_INDEX;
    
    if (spawn->hasCheckTrigger)
        return 1;
    
    if (spawn->isCCheckTarget)
        return 1;
    char trimmed[256];
    TrimName(spawn->name, trimmed, sizeof(trimmed));
    return strcmp(trimmed, "Warehouse") == 0;
}


static void DrawSpawnHoverLabel(DemoState *state, const LiveSpawn *spawn, long destX, long spriteTopY)
{
    char trimmed[256];
    TrimName(spawn->name, trimmed, sizeof(trimmed));

    if (spawn->block == 3)
    {
        if (trimmed[0] == '\0')
            return;
        long textW = MeasureTextSJIS(state, trimmed);
        long boxW = textW + 2 * HOVER_LABEL_PADDING_X;
        long boxX = destX - boxW / 2;
        long boxY = spriteTopY - HOVER_LABEL_GAP_Y - HOVER_LABEL_HEIGHT;
        long maxHP = spawn->aiState.maxHP;
        long ratio1000 = maxHP > 0 ? spawn->aiState.currentHP * 1000 / maxHP : 1000;
        if (ratio1000 < 0)
            ratio1000 = 0;
        if (ratio1000 > 1000)
            ratio1000 = 1000;
        
        DrawRect(&state->canvas, boxX, boxY, boxW * ratio1000 / 1000, HOVER_LABEL_HEIGHT, HOVER_ENEMY_BAR_FILL);
        DrawRectOutline(&state->canvas, boxX, boxY, boxW, HOVER_LABEL_HEIGHT, 1, HOVER_LABEL_BORDER_COLOR);
        DrawTextSJIS(state, boxX + HOVER_LABEL_PADDING_X, boxY + (HOVER_LABEL_HEIGHT - FONT_CELL_HEIGHT) / 2, trimmed,
                     0, TEXT_STYLE_WHITE_SHADOWED, 1000);
        return;
    }

    if (spawn->block == 1 && strcmp(trimmed, "Warehouse") != 0)
        return; 
    if (trimmed[0] == '\0')
        return;

    long textW = MeasureTextSJIS(state, trimmed);
    long boxW = textW + 2 * HOVER_LABEL_PADDING_X;
    long boxX = destX - boxW / 2;
    long boxY = spriteTopY - HOVER_LABEL_GAP_Y - HOVER_LABEL_HEIGHT;
    DrawRect(&state->canvas, boxX, boxY, boxW, HOVER_LABEL_HEIGHT, HOVER_NPC_LABEL_FILL);
    DrawRectOutline(&state->canvas, boxX, boxY, boxW, HOVER_LABEL_HEIGHT, 1, HOVER_LABEL_BORDER_COLOR);
    TextStyle style = spawn->block == 1 ? TEXT_STYLE_YELLOW_SHADOWED : TEXT_STYLE_WHITE_SHADOWED;
    DrawTextSJIS(state, boxX + HOVER_LABEL_PADDING_X, boxY + (HOVER_LABEL_HEIGHT - FONT_CELL_HEIGHT) / 2, trimmed, 0,
                 style, 1000);
}


void DrawLiveSpawn(DemoState *state, LiveSpawn *spawn)
{
    
    if (spawn->block == 1 && state->scriptLoaded &&
        !RKC_RPG_SCRIPT_EXEC_IsCharacterActive(&state->execState, spawn->characterNo))
        return;

    
    long deathFrame = -1;
    long deathTrans = 1000;
    if (spawn->block == 3 && spawn->aiState.isDead)
    {
        if (spawn->templateIndex < 0)
            return;
        LiveSpawnTemplate *deadTmpl = &state->templates[spawn->templateIndex];
        if (deadTmpl->kind != LIVE_SPAWN_SPRITE_CAF || deadTmpl->caf.chartCount <= 3)
            return; 
        const RKC_RPGSCRN_CAF_Direction *dir = &deadTmpl->caf.charts[3].directions[spawn->facingDirection];
        if (dir->maxFrameCount <= 0)
            return;
        long elapsedTicks = (long)(state->tick - spawn->deathTick);
        long elapsedFrames = elapsedTicks / TICKS_PER_ANIM_FRAME;
        long lastFrame = dir->maxFrameCount - 1;
        if (elapsedFrames >= lastFrame + DEATH_FADE_DURATION_FRAMES)
            return; 
        deathFrame = elapsedFrames < lastFrame ? elapsedFrames : lastFrame;
        if (elapsedFrames >= lastFrame)
        {
            deathTrans = ((dir->maxFrameCount - elapsedFrames) + (DEATH_FADE_DURATION_FRAMES - 1)) * 1000 /
                         DEATH_FADE_DURATION_FRAMES;
            if (deathTrans < 0)
                deathTrans = 0;
        }
    }

    
    long hitFrame = -1;
    if (spawn->block == 3 && !spawn->aiState.isDead && spawn->hitStunDurationTicks > 0 && spawn->templateIndex >= 0)
    {
        LiveSpawnTemplate *hitTmpl = &state->templates[spawn->templateIndex];
        if (hitTmpl->kind == LIVE_SPAWN_SPRITE_CAF && hitTmpl->caf.chartCount > HIT_REACTION_CHART)
        {
            const RKC_RPGSCRN_CAF_Direction *dir =
                &hitTmpl->caf.charts[HIT_REACTION_CHART].directions[spawn->facingDirection];
            if (dir->maxFrameCount > 0)
            {
                long elapsedTicks = (long)(state->tick - spawn->hitStunTick);
                if (elapsedTicks < spawn->hitStunDurationTicks)
                {
                    long f = elapsedTicks * dir->maxFrameCount / spawn->hitStunDurationTicks;
                    hitFrame = f < dir->maxFrameCount ? f : dir->maxFrameCount - 1;
                }
            }
        }
    }

    
    long attackFrame = -1;
    if (spawn->block == 3 && !spawn->aiState.isDead && spawn->attackCooldownTicks > 0 && spawn->attackChartIndex >= 0 &&
        spawn->templateIndex >= 0)
    {
        LiveSpawnTemplate *atkTmpl = &state->templates[spawn->templateIndex];
        if (atkTmpl->kind == LIVE_SPAWN_SPRITE_CAF && spawn->attackChartIndex < atkTmpl->caf.chartCount)
        {
            const RKC_RPGSCRN_CAF_Direction *dir =
                &atkTmpl->caf.charts[spawn->attackChartIndex].directions[spawn->facingDirection];
            if (dir->maxFrameCount > 0)
            {
                
                long idx = spawn->attackSpeedIndex;
                double speedMultiplier = (idx >= 0 && idx < 10) ? ENEMY_ATTACK_SPEED_TABLE[idx] : 1.0;
                
                long elapsedTicks = (long)(state->tick - spawn->attackAnimTick);
                if ((double)elapsedTicks < (double)dir->maxFrameCount * ATTACK_ANIM_TICKS_PER_FRAME / speedMultiplier)
                    attackFrame = (long)((double)elapsedTicks * speedMultiplier / ATTACK_ANIM_TICKS_PER_FRAME);
            }
        }
    }

    
    long actionFrame = -1;
    if (spawn->actionAnimChart >= 0 && deathFrame < 0 && hitFrame < 0 && attackFrame < 0 && spawn->templateIndex >= 0)
    {
        LiveSpawnTemplate *actTmpl = &state->templates[spawn->templateIndex];
        if (actTmpl->kind == LIVE_SPAWN_SPRITE_CAF && spawn->actionAnimChart < actTmpl->caf.chartCount)
        {
            const RKC_RPGSCRN_CAF_Direction *dir =
                &actTmpl->caf.charts[spawn->actionAnimChart].directions[spawn->facingDirection];
            if (dir->maxFrameCount > 0)
            {
                long elapsedFrames = (long)(state->tick - spawn->actionAnimTick) / TICKS_PER_ANIM_FRAME;
                if (elapsedFrames < dir->maxFrameCount)
                    actionFrame = elapsedFrames;
            }
        }
    }

    
    long talkFrame = -1;
    if (spawn->block == 2 && state->dialogActive && state->dialogQueueCount > 0 &&
        state->dialogQueue[0].characterNo == spawn->characterNo && deathFrame < 0 && hitFrame < 0 &&
        attackFrame < 0 && actionFrame < 0 && spawn->templateIndex >= 0)
    {
        LiveSpawnTemplate *talkTmpl = &state->templates[spawn->templateIndex];
        if (talkTmpl->kind == LIVE_SPAWN_SPRITE_CAF && NPC_TALK_CHART < talkTmpl->caf.chartCount)
        {
            const RKC_RPGSCRN_CAF_Direction *dir =
                &talkTmpl->caf.charts[NPC_TALK_CHART].directions[spawn->facingDirection];
            if (dir->maxFrameCount > 0)
            {
                int isWanderer = spawn->npcWanderL < spawn->npcWanderR;
                if (isWanderer)
                {
                    long elapsedFrames = (long)(state->tick - spawn->talkAnimTick) / TICKS_PER_ANIM_FRAME;
                    talkFrame = elapsedFrames < dir->maxFrameCount ? elapsedFrames : dir->maxFrameCount - 1;
                }
                else
                    talkFrame = (long)(state->tick / TICKS_PER_ANIM_FRAME);
            }
        }
    }

    long screenX, screenY;
    WorldToScreen(state, spawn->x, spawn->y, &screenX, &screenY);
    long destX = screenX - state->cameraX;
    long destY = screenY - state->cameraY;

    if (spawn->templateIndex < 0)
    {
        if (spawn->block == 1)
            DrawMarker(&state->canvas, destX, destY, 3, 0, 220, 0); 
        else if (spawn->block == 2)
            DrawMarker(&state->canvas, destX, destY, 3, 220, 0, 0); 
        else
            DrawMarker(&state->canvas, destX, destY, 3, 0, 0, 220); 
        
        if (state->hoveredSpawnIndex >= 0 && spawn == &state->liveSpawns[state->hoveredSpawnIndex])
            DrawSpawnHoverLabel(state, spawn, destX, destY - 40);
        return;
    }

    if (destX < -256 || destX >= APP_WIDTH + 256 || destY < -256 || destY >= APP_HEIGHT + 256)
        return; 

    
    int isHovered = state->hoveredSpawnIndex >= 0 && spawn == &state->liveSpawns[state->hoveredSpawnIndex];
    
    if (!isHovered && spawn->block == 1 && state->hoveredSpawnIndex >= 0)
    {
        const LiveSpawn *h = &state->liveSpawns[state->hoveredSpawnIndex];
        if (h->block == 3 && h->templateIndex < 0 && h->name != NULL && spawn->x >= h->x + h->rectL &&
            spawn->x <= h->x + h->rectR && spawn->y >= h->y + h->rectT && spawn->y <= h->y + h->rectB)
            isHovered = 1;
    }

    long hitVfxCenterY = destY; 
    long spriteTopY = destY;    
    LiveSpawnTemplate *tmpl = &state->templates[spawn->templateIndex];
    if (tmpl->kind == LIVE_SPAWN_SPRITE_STATIC)
    {
        
        int isGateRuneCircle = spawn->field8 == GATE_RUNE_CIRCLE_FIELD8;
        if (isGateRuneCircle && spawn->gateHighlightFadeTicks <= 0)
            return;
        long gateTrans = isGateRuneCircle ? 1000 * spawn->gateHighlightFadeTicks / GATE_HIGHLIGHT_FADE_TICKS : 1000;

        
        RKC_DIB *icon = RKC_UPDIB_GetPatternIcon(&tmpl->staticNjp, spawn->patternIndex);
        if (icon)
        {
            long offsetX, offsetY;
            RKC_UPDIB_GetPatternOffset(&tmpl->staticNjp, spawn->patternIndex, &offsetX, &offsetY);
            spriteTopY = destY + offsetY;
            
            spawn->hitTestOffsetY = -(offsetY + icon->height / 2);
            spawn->hitTestHalfWidth = icon->width / 2;
            spawn->hitTestHalfHeight = icon->height / 2;
            
            long hoverTint = isHovered ? HOVER_HIGHLIGHT_TINT : 1000;
            
            if (isGateRuneCircle)
                RKC_DIB_TransferToDIBAdditive(&state->canvas, destX + offsetX, destY + offsetY, icon->width,
                                              icon->height, icon, 0, 0, 0, gateTrans);
            else
                RKC_DIB_TransferToDIBTint(&state->canvas, destX + offsetX, destY + offsetY, icon->width, icon->height,
                                          icon, 0, 0, 0, gateTrans, hoverTint, hoverTint, hoverTint);
        }
        
    }
    else if (tmpl->kind == LIVE_SPAWN_SPRITE_CAF)
    {
        
        long chart = deathFrame >= 0    ? 3
                     : hitFrame >= 0    ? HIT_REACTION_CHART
                     : attackFrame >= 0 ? spawn->attackChartIndex
                     : actionFrame >= 0 ? spawn->actionAnimChart
                     : talkFrame >= 0   ? NPC_TALK_CHART
                                        : (spawn->isMoving ? 1 : 0);
        
        long walkFrame = (long)(state->tick * (unsigned long)(spawn->moveSpeedPercent > 0 ? spawn->moveSpeedPercent : 100) /
                                (100UL * TICKS_PER_ANIM_FRAME));
        long frame = deathFrame >= 0    ? deathFrame
                     : hitFrame >= 0    ? hitFrame
                     : attackFrame >= 0 ? attackFrame
                     : actionFrame >= 0 ? actionFrame
                     : talkFrame >= 0   ? talkFrame
                     : spawn->isMoving  ? walkFrame
                                        : (long)(state->tick / TICKS_PER_ANIM_FRAME);
        
        long cafDir = spawn->facingDirection;
        if (chart >= 0 && chart < tmpl->caf.chartCount && cafDir >= 0 && cafDir < RKC_RPGSCRN_CAF_NUM_DIRECTIONS - 1 &&
            tmpl->caf.charts[chart].directions[cafDir].maxFrameCount <= 0 &&
            tmpl->caf.charts[chart].directions[RKC_RPGSCRN_CAF_NUM_DIRECTIONS - 1].maxFrameCount > 0)
            cafDir = RKC_RPGSCRN_CAF_NUM_DIRECTIONS - 1;
        RKC_RPGSCRN_CAF_DrawCmd cmds[8];
        int n = RKC_RPGSCRN_CAF_Resolve(&tmpl->caf, chart, cafDir, frame, &tmpl->animNjp,
                                        &tmpl->animSdw, spawn->cellBlockMask, spawn->cellTintR, spawn->cellTintG,
                                        spawn->cellTintB, spawn->cellBlockMaskCount, cmds, 8);
        
        
        long block1Lift = (spawn->block == 1 && spawn->block1Height > 0) ? spawn->block1Height / 10 : 0;
        long spriteDestY = destY - (block1Lift > 0
                                        ? block1Lift
                                        : CafSpriteYOffsetCached(&spawn->cafHeightCache, &spawn->cafHeightCacheChart,
                                                                 &spawn->cafHeightCacheDirection,
                                                                 &spawn->cafHeightCacheValid, chart,
                                                                 spawn->facingDirection, n, cmds));
        long tallestHeight = TallestCafIconHeight(n, cmds);
        hitVfxCenterY = destY - block1Lift - tallestHeight / 2;
        spriteTopY = destY - block1Lift - tallestHeight;
        
        spawn->hitTestOffsetY = block1Lift + tallestHeight / 2;
        spawn->hitTestHalfWidth = TallestCafIconWidth(n, cmds) / 2;
        spawn->hitTestHalfHeight = tallestHeight / 2;
        for (int i = 0; i < n; i++)
        {
            const RKC_DIB *icon = cmds[i].icon;
            
            long trans = cmds[i].trans * deathTrans / 1000;
            
            int applyHoverTint = isHovered && !cmds[i].isShadow;
            long tintR = applyHoverTint ? HOVER_HIGHLIGHT_TINT : cmds[i].tintR;
            long tintG = applyHoverTint ? HOVER_HIGHLIGHT_TINT : cmds[i].tintG;
            long tintB = applyHoverTint ? HOVER_HIGHLIGHT_TINT : cmds[i].tintB;
            
            if (cmds[i].isAdditive)
                RKC_DIB_TransferToDIBAdditive(&state->canvas, destX + cmds[i].offsetX, spriteDestY + cmds[i].offsetY,
                                              icon->width, icon->height, icon, 0, 0, 0, trans);
            else
                RKC_DIB_TransferToDIBTint(&state->canvas, destX + cmds[i].offsetX, spriteDestY + cmds[i].offsetY,
                                          icon->width, icon->height, icon, 0, 0, 0, trans, tintR, tintG, tintB);
        }
    }

    
    if (spawn->block == 3)
        DrawHitVfx(state, destX, hitVfxCenterY, spawn->hitVfxTick, spawn->hitVfxVariant);

    
    if (isHovered)
        DrawSpawnHoverLabel(state, spawn, destX, spriteTopY);
}


long GetLiveSpawnDepth(DemoState *state, const LiveSpawn *spawn)
{
    long screenX, screenY;
    WorldToScreen(state, spawn->x, spawn->y, &screenX, &screenY);
    (void)screenX;

    if (spawn->templateIndex < 0)
        return screenY;

    LiveSpawnTemplate *tmpl = &state->templates[spawn->templateIndex];
    if (tmpl->kind == LIVE_SPAWN_SPRITE_STATIC)
    {
        
        if (spawn->block == 1 && tmpl->field8 == GATE_RUNE_CIRCLE_FIELD8)
        {
            long minSiblingDepth = 0;
            int foundSibling = 0;
            for (long i = 0; i < state->liveSpawnCount; i++)
            {
                const LiveSpawn *sib = &state->liveSpawns[i];
                if (sib->block != 1 || sib->field8 != GATE_BODY_BASE_FIELD8 || sib->templateIndex < 0)
                    continue;
                if (sib->x != spawn->x || sib->y != spawn->y)
                    continue;
                long sibScreenX, sibScreenY;
                WorldToScreen(state, sib->x, sib->y, &sibScreenX, &sibScreenY);
                (void)sibScreenX;
                LiveSpawnTemplate *sibTmpl = &state->templates[sib->templateIndex];
                
                RKC_DIB *sibIcon = RKC_UPDIB_GetPatternIcon(&sibTmpl->staticNjp, sib->patternIndex);
                long sibOffsetX, sibOffsetY;
                RKC_UPDIB_GetPatternOffset(&sibTmpl->staticNjp, sib->patternIndex, &sibOffsetX, &sibOffsetY);
                (void)sibOffsetX;
                long sibDepth = sibScreenY + sibOffsetY + (sibIcon ? sibIcon->height : 0);
                if (!foundSibling || sibDepth < minSiblingDepth)
                    minSiblingDepth = sibDepth;
                foundSibling = 1;
            }
            return foundSibling ? minSiblingDepth - 1 : GROUND_DECAL_DRAW_DEPTH;
        }

        
        if (spawn->block1Walkable)
            return GROUND_DECAL_DRAW_DEPTH;

        
        RKC_DIB *icon = RKC_UPDIB_GetPatternIcon(&tmpl->staticNjp, spawn->patternIndex);
        long offsetX, offsetY;
        RKC_UPDIB_GetPatternOffset(&tmpl->staticNjp, spawn->patternIndex, &offsetX, &offsetY);
        (void)offsetX;
        return screenY + offsetY + (icon ? icon->height : 0);
    }
    
    if (spawn->block == 1 && spawn->block1Height > 0)
        return screenY + spawn->block1Height / 10;
    return screenY;
}


void DrawCursor(DemoState *state, long mouseX, long mouseY)
{
    if (state->clickRangeSquareVisible)
    {
        long side = (long)(2.0 * state->clickRangeTileHeight * state->ground.chipHeight);
        long opacity = (state->hoveredSpawnIndex >= 0 || state->hoveredWorldItemIndex >= 0)
                          ? CURSOR_SQUARE_HOVER_OPACITY
                          : CURSOR_SQUARE_NON_HOVER_OPACITY;
        DrawRectOutlineAlpha(&state->canvas, mouseX - side / 2, mouseY - side / 2, side, side,
                             CURSOR_SQUARE_OUTLINE_THICKNESS, opacity, CURSOR_SQUARE_COLOR_B, CURSOR_SQUARE_COLOR_G,
                             CURSOR_SQUARE_COLOR_R);
    }

    
    if (state->cursorSheetLoaded)
    {
        RKC_DIB *icon = RKC_UPDIB_GetFrame(&state->cursorSheet, 0);
        
        if (icon)
            RKC_DIB_TransferToDIB(&state->canvas, mouseX, mouseY, icon->width, icon->height, icon, 0, 0, 0);
    }
}
