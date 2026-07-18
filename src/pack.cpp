#include "pack.hpp"
#include "constants.hpp"

auto pack_image(const std::vector<uint8_t> &pixels, int width) -> std::vector<uint8_t>
{
    if (pixels.size() / width != ROWS) return {};

    int columns = (width + COLUMN_WIDTH - 1) / COLUMN_WIDTH;
    std::vector<uint8_t> packed;

    for (int col = 0; col < columns; col++)
    {
        for (int row = 0; row < ROWS; row++)
        {
            uint8_t value = 0;
            for (int bit = 0; bit < COLUMN_WIDTH; bit++)
            {
                int x = (col * COLUMN_WIDTH) + bit;
                if (x >= width) break;
                if (pixels[(row * width) + x] > BRIGHTNESS_DIV)
                {
                    value |= 1U << (COLUMN_WIDTH - 1U - bit);
                }
            }
            packed.push_back(value);
        }
    }
    return packed;
}
