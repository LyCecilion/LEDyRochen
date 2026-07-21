#include "pack.hpp"
#include "constants.hpp"

#include <stdexcept>

auto pack_image(std::span<const uint8_t> pixels, int width) -> std::vector<uint8_t>
{
    if (width <= 0)
    {
        throw std::invalid_argument("bitmap width must be greater than 0");
    }
    const auto unsigned_width = static_cast<std::size_t>(width);
    if (pixels.size() != unsigned_width * static_cast<std::size_t>(ROWS))
    {
        throw std::invalid_argument("bitmap data length does not match dimensions");
    }

    const int columns = (width + COLUMN_WIDTH - 1) / COLUMN_WIDTH;
    std::vector<uint8_t> packed;
    packed.reserve(static_cast<std::size_t>(columns) * static_cast<std::size_t>(ROWS));

    for (int col = 0; col < columns; col++)
    {
        for (int row = 0; row < ROWS; row++)
        {
            uint8_t value = 0;
            for (int bit = 0; bit < COLUMN_WIDTH; bit++)
            {
                int x = (col * COLUMN_WIDTH) + bit;
                if (x >= width)
                {
                    break;
                }
                const auto pixel_index =
                    (static_cast<std::size_t>(row) * unsigned_width) + static_cast<std::size_t>(x);
                if (pixels[pixel_index] > BRIGHTNESS_DIV)
                {
                    value |= static_cast<uint8_t>(1U << (COLUMN_WIDTH - 1 - bit));
                }
            }
            packed.push_back(value);
        }
    }
    return packed;
}
