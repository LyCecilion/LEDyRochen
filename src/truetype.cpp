#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "truetype.hpp"
#include "utils.hpp"

#include <stdexcept>

auto render_text(const std::string &text, const std::string &font_path, int font_size,
                 int threshold) -> GlyphBitmap
{
    constexpr int MIN_FONT_SIZE = 5;

    std::vector<uint8_t> font_data = read_file(font_path);

    stbtt_fontinfo font;
    stbtt_InitFont(&font, font_data.data(), 0);

    int ascent, descent, line_gap;
    float scale;
    bool scale_found = false;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);

    for (int size = font_size; size > MIN_FONT_SIZE; size--)
    {
        scale = stbtt_ScaleForPixelHeight(&font, static_cast<float>(size));
        int scaled_height =
            static_cast<int>(static_cast<float>(ascent - descent + line_gap) * scale);
        if (scaled_height > ROWS) continue;
        scale_found = true;
        break;
    }
    if (!scale_found)
    {
        throw std::runtime_error("文字无法缩放到 " + std::to_string(ROWS) + " 像素高");
    }

    int total_width = 0;

    const char *ptr = text.c_str();
    while (ptr < text.c_str() + text.size())
    {
        int cp = next_codepoint(ptr);
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&font, cp, &advance, &lsb);
        total_width += static_cast<int>(static_cast<float>(advance) * scale);
    }

    std::vector<uint8_t> pixels(static_cast<size_t>(total_width * ROWS), 0);
    int x_cursor = 0;
    ptr = text.c_str();
    while (ptr < text.c_str() + text.size())
    {
        int cp = next_codepoint(ptr);
        int w, h, xoff, yoff;

        unsigned char *glyph =
            stbtt_GetCodepointBitmap(&font, scale, scale, cp, &w, &h, &xoff, &yoff);

        int baseline = static_cast<int>(static_cast<float>(ascent) * scale);
        int y_start = baseline + yoff;

        for (int gy = 0; gy < h; gy++)
        {
            int py = y_start + gy;
            if (py < 0 || py >= ROWS) continue;
            for (int gx = 0; gx < w; gx++)
            {
                int px = x_cursor + gx;
                if (px >= total_width) continue;
                uint8_t val = glyph[(gy * w) + gx];
                if (val > threshold)
                {
                    pixels[(py * total_width) + px] = BRIGHT;
                }
            }
        }

        stbtt_FreeBitmap(glyph, nullptr);

        int advance, lsb;
        stbtt_GetCodepointHMetrics(&font, cp, &advance, &lsb);
        x_cursor += static_cast<int>(static_cast<float>(advance) * scale);
    }

    return {.pixels = pixels, .width = total_width};
}
