#pragma once

#include <string>

#include "constants.hpp"
#include "types.hpp"

auto render_text(const std::string &text, const std::string &font_path, int font_size = ROWS,
                 int threshold = STB_THRESHOLD) -> GlyphBitmap;
