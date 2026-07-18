#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

class EncodingError : public std::runtime_error
{
  public:
    using std::runtime_error::runtime_error;
};

auto read_file(const std::string &path) -> std::vector<uint8_t>;
auto utf8_to_gbk(std::string_view utf8) -> std::vector<uint8_t>;
auto utf8_codepoints(std::string_view utf8) -> std::vector<char32_t>;
