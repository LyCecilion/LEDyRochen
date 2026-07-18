#pragma once

#include <cstdint>
#include <span>
#include <vector>

auto pack_image(std::span<const uint8_t> pixels, int width) -> std::vector<uint8_t>;
