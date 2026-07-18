#include "vendor_font.hpp"

#include "constants.hpp"
#include "utils.hpp"

auto load_vendor_font(const std::string &dir) -> VendorFont
{
    return {.ascii = read_file(dir + "/ASC11"), .gbk = read_file(dir + "/HZK11")};
}

auto render_vendor(const std::string &text, const VendorFont &vf) -> GlyphBitmap
{
    auto gbk_bytes = utf8_to_gbk(text);

    // First pass: compute total pixel width
    int total_width = 0;
    size_t i = 0;
    while (i < gbk_bytes.size())
    {
        uint8_t byte0 = gbk_bytes[i++];
        total_width += (byte0 <= ASCII_MAX) ? ASCII_GLYPH_WIDTH : GBK_GLYPH_WIDTH;
        if (byte0 > ASCII_MAX) i++;
    }

    // Second pass: render into pixel buffer
    std::vector<uint8_t> pixels(static_cast<size_t>(total_width * ROWS), 0);
    int cursor = 0;
    i = 0;
    while (i < gbk_bytes.size())
    {
        uint8_t byte0 = gbk_bytes[i++];
        if (byte0 <= ASCII_MAX)
        { // ASCII glyph
            int offset = byte0 * ROWS;
            for (int row = 0; row < ROWS; row++)
            {
                uint8_t row_data = vf.ascii[offset + row];
                for (int x = 0; x < ASCII_GLYPH_WIDTH; x++)
                {
                    pixels[(row * total_width) + cursor + x] =
                        (((row_data >> (ASCII_GLYPH_WIDTH - 1 - x)) & 1) != 0) ? BRIGHT : DARK;
                }
            }
            cursor += ASCII_GLYPH_WIDTH;
        }
        else
        { // GBK: 11 columns
            uint8_t lead = byte0;
            uint8_t trail = gbk_bytes[i++];
            int index = ((lead - GBK_LEAD_MIN) * GBK_TRAIL_COUNT) + (trail - GBK_TRAIL_MIN);
            int offset = index * GBK_GLYPH_SIZE;
            for (int row = 0; row < ROWS; row++)
            {
                auto row_data = static_cast<uint16_t>(
                    (static_cast<uint16_t>(vf.gbk[offset + (row * GBK_BYTES_PER_ROW)])
                     << 8) | // NOLINT(readability-magic-numbers), why warn me here??
                    vf.gbk[offset + (row * GBK_BYTES_PER_ROW) + 1]);
                for (int x = 0; x < GBK_GLYPH_WIDTH; x++)
                {
                    pixels[(row * total_width) + cursor + x] =
                        (((row_data >> (GBK_STORAGE_WIDTH - 1 - x)) & 1) != 0) ? BRIGHT : DARK;
                }
            }
            cursor += GBK_GLYPH_WIDTH;
        }
    }
    return {.pixels = pixels, .width = total_width};
}
