#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iconv.h>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <libusb.h>

constexpr int ROWS = 11;
constexpr int MAX_PAYLOAD = 8192;
constexpr int REPORT_SIZE = 64;
constexpr int COLUMN_WIDTH = 8;
constexpr int BRIGHTNESS_DIV = 127;
constexpr int ASCII_GLYPH_WIDTH = 8;
constexpr int GBK_GLYPH_WIDTH = 11;
constexpr int GBK_STORAGE_WIDTH = 16; // HZK11 stores 16-pixel rows
constexpr int GBK_LEAD_MIN = 0x81;
constexpr int GBK_TRAIL_MIN = 0x40;
constexpr int GBK_TRAIL_COUNT = 191;
constexpr int GBK_BYTES_PER_ROW = 2; // HZK11 stores 16-bit rows
constexpr int GBK_GLYPH_SIZE = ROWS * GBK_BYTES_PER_ROW;
constexpr uint8_t ASCII_MAX = 0x7F;

constexpr std::array<uint8_t, 64> HEADER_TEMPLATE = {
    0x77, 0x61, 0x6E, 0x67, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// UTF-8 encoding constants (RFC 3629)
namespace utf8
{
constexpr unsigned char ASCII_LIMIT = 0x80;
constexpr unsigned char TWO_MASK = 0xE0;
constexpr unsigned char TWO_MATCH = 0xC0;
constexpr unsigned char TWO_BITS = 0x1F;
constexpr unsigned char THREE_MASK = 0xF0;
constexpr unsigned char THREE_MATCH = 0xE0;
constexpr unsigned char THREE_BITS = 0x0F;
constexpr unsigned char FOUR_MASK = 0xF8;
constexpr unsigned char FOUR_MATCH = 0xF0;
constexpr unsigned char FOUR_BITS = 0x07;
constexpr int SHIFT = 6; // cont​inuation byte: 10xxxxxx → 6 bits
constexpr unsigned char TRAIL_BITS = 0x3F;
} // namespace utf8

constexpr uint16_t VID = 0x0416;
constexpr uint16_t PID = 0x5020;

struct BadgeDevice
{
    libusb_device_handle *handle;
    uint8_t endpoint_out;
    uint8_t endpoint_in;

    static auto find_all() -> std::vector<BadgeDevice>;
    auto write(const std::vector<uint8_t> &payload) -> void
    {
        for (size_t offset = 0; offset < payload.size(); offset += 64)
        {
            int written = 0;
            libusb_interrupt_transfer(handle, endpoint_out, payload.data() + offset, 64, &written,
                                      3000);
            uint8_t ack[64];
            int read = 0;
            libusb_interrupt_transfer(handle, endpoint_in, ack, 64, &read, 3000);
        }
    };
    auto close() -> void {
        if (handle != nullptr) {
            libusb_close(handle);
            handle = nullptr;
        }
    }
};

auto pack_image(const std::vector<uint8_t> &pixels, int width) -> std::vector<uint8_t>
{
    if (pixels.size() / width != ROWS) return {}; // limity'rochen just wanna `return 0;` here...

    int columns = (width + COLUMN_WIDTH - 1) / COLUMN_WIDTH;
    std::vector<uint8_t> packed;

    // truth is you're once fond of OI but why u cant write this kind of easy code now :cry:
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

struct GlyphBitmap
{
    std::vector<uint8_t> pixels; // row-major
    int width;
};

auto read_file(const std::string &path) -> std::vector<uint8_t>;
auto utf8_to_gbk(const std::string &utf8) -> std::vector<uint8_t>;

struct VendorFont
{
    std::vector<uint8_t> ascii; // ASC11: 256*11 bytes
    std::vector<uint8_t> gbk;   // HZK11: 126*191*22 bytes
};

auto load_vendor_font(const std::string &dir) -> VendorFont
{
    return {.ascii = read_file(dir + "/ASC11"), .gbk = read_file(dir + "/HZK11")};
}

// Build pixel buffer from vendor fonts (11px height, returns row-major 0/255 pixels)
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
                        (((row_data >> (ASCII_GLYPH_WIDTH - 1 - x)) & 1) != 0) ? 255 : 0;
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
                    (static_cast<uint16_t>(vf.gbk[offset + (row * GBK_BYTES_PER_ROW)]) << 8) |
                    vf.gbk[offset + (row * GBK_BYTES_PER_ROW) + 1]);
                for (int x = 0; x < GBK_GLYPH_WIDTH; x++)
                {
                    pixels[(row * total_width) + cursor + x] =
                        (((row_data >> (GBK_STORAGE_WIDTH - 1 - x)) & 1) != 0) ? 255 : 0;
                }
            }
            cursor += GBK_GLYPH_WIDTH;
        }
    }
    return {.pixels = pixels, .width = total_width};
}

// better way recommended by Claude
auto read_file(const std::string &path) -> std::vector<uint8_t>
{
    std::ifstream file(path, std::ios::binary);
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
    return {buffer.begin(), buffer.end()};
}

// Convert UTF-8 string to GBK bytes
auto utf8_to_gbk(const std::string &utf8) -> std::vector<uint8_t>
{
    iconv_t converter = iconv_open("GBK", "UTF-8");
    if (converter == iconv_t(-1)) // NOLINT(performance-no-int-to-ptr)
    {
        throw std::runtime_error("iconv_open UTF-8→GBK 失败");
    }

    size_t in_bytes = utf8.size();
    size_t out_bytes = (utf8.size() * 2) + 1;
    std::vector<char> buffer(out_bytes);

    char *in_ptr = const_cast<char *>(utf8.data());
    char *out_ptr = buffer.data();

    size_t result = iconv(converter, &in_ptr, &in_bytes, &out_ptr, &out_bytes);
    iconv_close(converter);

    if (result == static_cast<size_t>(-1))
    {
        throw std::runtime_error("GBK 编码失败，字符不在 GBK 范围内");
    }

    size_t written = buffer.size() - out_bytes;
    return {buffer.begin(), buffer.begin() + static_cast<ptrdiff_t>(written)};
}

auto next_codepoint(const char *&ptr) -> int
{
    // problems will occur when decoding UTF-8, and limity'rochen found this :cool:

    unsigned char c = *ptr++;

    // ASCII
    if (c < utf8::ASCII_LIMIT) return c;

    int cp;
    int extra;
    if ((c & utf8::TWO_MASK) == utf8::TWO_MATCH)
    {
        cp = c & utf8::TWO_BITS;
        extra = 1;
    }
    else if ((c & utf8::THREE_MASK) == utf8::THREE_MATCH)
    {
        cp = c & utf8::THREE_BITS;
        extra = 2; // CJK characters
    }
    else if ((c & utf8::FOUR_MASK) == utf8::FOUR_MATCH)
    {
        cp = c & utf8::FOUR_BITS;
        extra = 3; // emoji
    }
    else
    {
        return -1; // illegal UTF-8
    }

    for (int i = 0; i < extra; i++)
    {
        cp = (cp << utf8::SHIFT) | (*ptr++ & utf8::TRAIL_BITS);
    }
    return cp;
}

auto render_text(const std::string &text, const std::string &font_path, int font_size = 11,
                 int threshold = 96) -> GlyphBitmap
{
    constexpr int MIN_FONT_SIZE = 5;
    // read font file
    std::vector<uint8_t> font_data = read_file(font_path);

    // initialize stb
    stbtt_fontinfo font;
    stbtt_InitFont(&font, font_data.data(), 0);

    // query font for height
    int ascent, descent, line_gap;
    float scale;
    bool scale_found = false;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);

    // get a suitable scale
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

    // calculate total width
    const char *ptr = text.c_str();
    while (ptr < text.c_str() + text.size())
    {
        int cp = next_codepoint(ptr);
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&font, cp, &advance, &lsb);
        total_width += static_cast<int>(static_cast<float>(advance) * scale);
    }

    // draw glyph
    std::vector<uint8_t> pixels(static_cast<size_t>(total_width * ROWS), 0);
    int x_cursor = 0;
    ptr = text.c_str();
    while (ptr < text.c_str() + text.size())
    {
        int cp = next_codepoint(ptr);
        int w, h, xoff, yoff;

        // get bitmap
        unsigned char *glyph =
            stbtt_GetCodepointBitmap(&font, scale, scale, cp, &w, &h, &xoff, &yoff);

        // get y offset
        int baseline = static_cast<int>(static_cast<float>(ascent) * scale);
        int y_start = baseline + yoff;

        // copy
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
                    pixels[(py * total_width) + px] = 255;
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

auto main() -> int
{
    const std::string vendor_dir = "/home/lycecilion/Workspace/active/RocheLCD/vendor";
    auto vf = load_vendor_font(vendor_dir);
    std::cout << "加载原版字库 ASC11=" << vf.ascii.size() << "B  HZK11=" << vf.gbk.size() << "B\n";

    auto glyph = render_vendor("Hello 你好！", vf);
    std::cout << "像素: " << glyph.width << "x" << ROWS << "\n";

    auto packed = pack_image(glyph.pixels, glyph.width);
    std::cout << "打包: " << packed.size() << " 字节\n";

    for (int row = 0; row < ROWS; row++)
    {
        for (int col = 0; col < glyph.width; col++)
        {
            std::cout << (glyph.pixels[(row * glyph.width) + col] > 127 ? "\033[47m \033[0m" : " ");
        }
        std::cout << '\n';
    }
}
