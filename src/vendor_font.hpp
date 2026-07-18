#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>

#include "types.hpp"

class UnsupportedVendorGlyph : public std::runtime_error
{
  public:
    using std::runtime_error::runtime_error;
};

auto load_vendor_font(const std::filesystem::path &directory) -> VendorFont;
auto find_vendor_font_dir(const std::filesystem::path &explicit_directory = {})
    -> std::filesystem::path;
auto render_vendor(const std::string &text, const VendorFont &font) -> GlyphBitmap;
