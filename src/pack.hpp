#pragma once

#include <cstdint>
#include <vector>

auto pack_image(const std::vector<uint8_t> &pixels, int width) -> std::vector<uint8_t>;
