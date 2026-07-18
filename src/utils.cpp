#include "utils.hpp"

#include <fstream>
#include <iconv.h>
#include <iterator>
#include <stdexcept>

#include "constants.hpp"

auto read_file(const std::string &path) -> std::vector<uint8_t>
{
    std::ifstream file(path, std::ios::binary);
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
    return {buffer.begin(), buffer.end()};
}

auto utf8_to_gbk(const std::string &utf8) -> std::vector<uint8_t>
{
    iconv_t converter = iconv_open("GBK", "UTF-8");
    if (converter == iconv_t(-1)) // NOLINT(performance-no-int-to-ptr)
    {
        throw std::runtime_error("iconv_open 从 UTF-8 转换到 GBK 失败");
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
