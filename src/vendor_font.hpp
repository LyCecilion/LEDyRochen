#pragma once

#include <string>

#include "types.hpp"

auto load_vendor_font(const std::string &dir) -> VendorFont;
auto render_vendor(const std::string &text, const VendorFont &vf) -> GlyphBitmap;
