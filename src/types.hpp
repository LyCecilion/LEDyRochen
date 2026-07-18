#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "constants.hpp"

struct GlyphBitmap
{
    std::vector<uint8_t> pixels; // row-major, 0 or BRIGHT
    int width;

    [[nodiscard]] auto pixel_count() const -> std::size_t
    {
        if (width <= 0)
        {
            throw std::invalid_argument("点阵宽度必须大于 0");
        }
        return static_cast<std::size_t>(width) * static_cast<std::size_t>(ROWS);
    }

    auto validate() const -> void
    {
        if (pixels.size() != pixel_count())
        {
            throw std::invalid_argument("点阵数据长度与尺寸不匹配");
        }
    }
};

struct VendorFont
{
    std::vector<uint8_t> ascii; // ASC11: 256 × ROWS bytes
    std::vector<uint8_t> gbk;   // HZK11: 126×191 × GBK_GLYPH_SIZE bytes
};
