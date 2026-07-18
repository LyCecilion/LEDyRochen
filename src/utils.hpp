#pragma once

#include <cstdint>
#include <string>
#include <vector>

auto read_file(const std::string &path) -> std::vector<uint8_t>;
auto utf8_to_gbk(const std::string &utf8) -> std::vector<uint8_t>;
auto next_codepoint(const char *&ptr) -> int;
