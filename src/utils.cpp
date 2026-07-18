#include "utils.hpp"

#include <fstream>
#include <iconv.h>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>

#include "constants.hpp"

auto read_file(const std::string &path) -> std::vector<uint8_t>
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("无法打开文件：" + path);
    }
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
    if (file.bad())
    {
        throw std::runtime_error("读取文件失败：" + path);
    }
    return {buffer.begin(), buffer.end()};
}

namespace
{
struct IconvCloser
{
    auto operator()(void *converter) const noexcept -> void
    {
        if (converter != reinterpret_cast<void *>(-1)) // NOLINT(performance-no-int-to-ptr)
        {
            iconv_close(converter);
        }
    }
};

using IconvHandle = std::unique_ptr<void, IconvCloser>;

auto byte_at(std::string_view text, std::size_t index) -> uint8_t
{
    return static_cast<unsigned char>(text[index]);
}
} // namespace

auto utf8_to_gbk(std::string_view utf8) -> std::vector<uint8_t>
{
    IconvHandle converter(iconv_open("GBK", "UTF-8"));
    if (converter.get() == reinterpret_cast<void *>(-1)) // NOLINT(performance-no-int-to-ptr)
    {
        throw std::runtime_error("iconv_open 从 UTF-8 转换到 GBK 失败");
    }

    std::size_t in_bytes = utf8.size();
    std::size_t out_bytes = (utf8.size() * 2) + 1;
    std::vector<char> buffer(out_bytes);

    char *in_ptr = const_cast<char *>(utf8.data()); // NOLINT(misc-const-correctness)
    char *out_ptr = buffer.data();                  // NOLINT(misc-const-correctness)

    const std::size_t result = iconv(converter.get(), &in_ptr, &in_bytes, &out_ptr, &out_bytes);
    if (result == std::numeric_limits<std::size_t>::max())
    {
        throw EncodingError("GBK 编码失败，字符不在 GBK 范围内");
    }

    buffer.resize(buffer.size() - out_bytes);
    return {buffer.begin(), buffer.end()};
}

auto utf8_codepoints(std::string_view utf8) -> std::vector<char32_t>
{
    std::vector<char32_t> codepoints;
    for (std::size_t position = 0; position < utf8.size();)
    {
        const uint8_t leading = byte_at(utf8, position++);
        if (leading < utf8::ASCII_LIMIT)
        {
            codepoints.push_back(leading);
            continue;
        }

        char32_t codepoint = 0;
        std::size_t extra = 0;
        char32_t minimum = 0;
        if ((leading & utf8::TWO_MASK) == utf8::TWO_MATCH)
        {
            codepoint = leading & utf8::TWO_BITS;
            extra = 1;
            minimum = 0x80;
        }
        else if ((leading & utf8::THREE_MASK) == utf8::THREE_MATCH)
        {
            codepoint = leading & utf8::THREE_BITS;
            extra = 2;
            minimum = 0x800;
        }
        else if ((leading & utf8::FOUR_MASK) == utf8::FOUR_MATCH)
        {
            codepoint = leading & utf8::FOUR_BITS;
            extra = 3;
            minimum = 0x10000;
        }
        else
        {
            throw EncodingError("非法 UTF-8 起始字节");
        }

        if (extra > utf8.size() - position)
        {
            throw EncodingError("UTF-8 序列在字符中途结束");
        }
        for (std::size_t index = 0; index < extra; ++index)
        {
            const uint8_t trailing = byte_at(utf8, position++);
            if ((trailing & 0xC0U) != 0x80U)
            {
                throw EncodingError("非法 UTF-8 续字节");
            }
            codepoint = (codepoint << utf8::SHIFT) | (trailing & utf8::TRAIL_BITS);
        }
        if (codepoint < minimum || (codepoint >= 0xD800 && codepoint <= 0xDFFF) ||
            codepoint > 0x10FFFF)
        {
            throw EncodingError("非法 UTF-8 码点");
        }
        codepoints.push_back(codepoint);
    }
    return codepoints;
}
