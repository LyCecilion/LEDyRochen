#include <iostream>
#include <string>

#include "constants.hpp"
#include "pack.hpp"
#include "types.hpp"
#include "vendor_font.hpp"

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
            std::cout << (glyph.pixels[(row * glyph.width) + col] > BRIGHTNESS_DIV
                              ? "\033[47m \033[0m"
                              : " ");
        }
        std::cout << '\n';
    }
}
