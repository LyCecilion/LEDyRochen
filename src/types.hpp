#pragma once

#include <cstdint>
#include <vector>

struct GlyphBitmap
{
    std::vector<uint8_t> pixels; // row-major, 0 or BRIGHT
    int width;
};

struct VendorFont
{
    std::vector<uint8_t> ascii; // ASC11: 256 × ROWS bytes
    std::vector<uint8_t> gbk;   // HZK11: 126×191 × GBK_GLYPH_SIZE bytes
};
