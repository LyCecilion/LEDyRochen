#include <array>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <getopt.h>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "device.hpp"
#include "protocol.hpp"
#include "truetype.hpp"
#include "vendor_font.hpp"

namespace
{
struct Options
{
    std::vector<std::string> messages;
    bool list_devices{false};
    bool show_help{false};
    std::optional<std::string> device;
    bool dry_run{false};
    bool show_preview{false};
    std::filesystem::path font;
    std::filesystem::path vendor_font_dir;
    bool no_vendor_font{false};
    int font_size{ROWS};
    int threshold{STB_THRESHOLD};
    DisplaySettings display;
};

auto print_help(std::ostream &output) -> void
{
    output << R"(用法：ledyrochen [选项] MESSAGE [MESSAGE ...]

向 0416:5020 CH546 11×44 LED 显示屏写入 1..8 条循环消息。

选项：
  --list-devices          只列出兼容设备
  --device ID             指定 BUS:DEV:OUT:IN
  --dry-run               只编码，不写入显示屏
  --preview               在终端预览 11 像素点阵
  --font PATH             使用指定 TTF/TTC 轮廓字体
  --vendor-font-dir PATH  指定 ASC11/HZK11 所在目录
  --no-vendor-font        禁用原版点阵字库
  --font-size N           初始字号，默认 11
  --threshold N           二值化阈值 0..255，默认 96
  --speed N[,N...]        每条消息的速度 1..8，逗号分隔，默认 4
  --mode MODE[,MODE...]   每条消息的显示方式，逗号分隔，默认 left
  --brightness N          25、50、75 或 100，默认 50
  --blink SLOT[,SLOT...]  对指定编号的消息启用闪烁 (1..8 或 all)
  --border SLOT[,SLOT...] 对指定编号的消息启用边框 (1..8 或 all)
  -h, --help              显示帮助
)";
}

auto parse_integer(std::string_view text, std::string_view name) -> int
{
    int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size())
    {
        throw std::invalid_argument(std::string(name) + " 必须是整数");
    }
    return value;
}

auto parse_mode(std::string_view value) -> DisplayMode
{
    struct NamedMode
    {
        std::string_view name;
        DisplayMode mode;
    };
    constexpr std::array<NamedMode, 9> modes = {
        NamedMode{.name = "left", .mode = DisplayMode::left},
        NamedMode{.name = "right", .mode = DisplayMode::right},
        NamedMode{.name = "up", .mode = DisplayMode::up},
        NamedMode{.name = "down", .mode = DisplayMode::down},
        NamedMode{.name = "still", .mode = DisplayMode::still},
        NamedMode{.name = "animation", .mode = DisplayMode::animation},
        NamedMode{.name = "drop", .mode = DisplayMode::drop},
        NamedMode{.name = "curtain", .mode = DisplayMode::curtain},
        NamedMode{.name = "laser", .mode = DisplayMode::laser},
    };
    for (const auto &named : modes)
    {
        if (value == named.name)
        {
            return named.mode;
        }
    }
    const int numeric = parse_integer(value, "mode");
    if (numeric < 0 || numeric > 8)
    {
        throw std::invalid_argument("mode 必须为名称或 0..8");
    }
    return static_cast<DisplayMode>(numeric);
}

auto split_values(std::string_view text) -> std::vector<std::string_view>
{
    std::vector<std::string_view> parts;
    std::size_t start = 0;
    while (start <= text.size())
    {
        auto end = text.find(',', start);
        if (end == std::string_view::npos) end = text.size();
        auto part = text.substr(start, end - start);
        auto first = part.find_first_not_of(' ');
        auto last = part.find_last_not_of(' ');
        if (first != std::string_view::npos)
            parts.push_back(part.substr(first, last - first + 1));
        start = end + 1;
    }
    return parts;
}

auto parse_slot_indices(std::string_view text) -> std::vector<std::size_t>
{
    if (text == "all")
    {
        std::vector<std::size_t> all(MAX_MESSAGES);
        for (std::size_t i = 0; i < MAX_MESSAGES; ++i) all[i] = i;
        return all;
    }
    auto parts = split_values(text);
    std::vector<std::size_t> indices;
    for (auto part : parts)
    {
        int idx = parse_integer(part, "slot 编号");
        if (idx < 1 || idx > MAX_MESSAGES)
            throw std::invalid_argument("slot 编号必须在 1..8 之间");
        indices.push_back(static_cast<std::size_t>(idx - 1));
    }
    return indices;
}

auto parse_options(int argc, char **argv) -> Options
{
    enum OptionCode : uint16_t
    {
        list_devices = 1000,
        device,
        dry_run,
        preview,
        font,
        vendor_font_dir,
        no_vendor_font,
        font_size,
        threshold,
        speed,
        mode,
        brightness,
        blink,
        border,
    };
    const std::array<option, 16> options = {
        option{
            .name = "list-devices", .has_arg = no_argument, .flag = nullptr, .val = list_devices},
        option{.name = "device", .has_arg = required_argument, .flag = nullptr, .val = device},
        option{.name = "dry-run", .has_arg = no_argument, .flag = nullptr, .val = dry_run},
        option{.name = "preview", .has_arg = no_argument, .flag = nullptr, .val = preview},
        option{.name = "font", .has_arg = required_argument, .flag = nullptr, .val = font},
        option{.name = "vendor-font-dir",
               .has_arg = required_argument,
               .flag = nullptr,
               .val = vendor_font_dir},
        option{.name = "no-vendor-font",
               .has_arg = no_argument,
               .flag = nullptr,
               .val = no_vendor_font},
        option{
            .name = "font-size", .has_arg = required_argument, .flag = nullptr, .val = font_size},
        option{
            .name = "threshold", .has_arg = required_argument, .flag = nullptr, .val = threshold},
        option{.name = "speed", .has_arg = required_argument, .flag = nullptr, .val = speed},
        option{.name = "mode", .has_arg = required_argument, .flag = nullptr, .val = mode},
        option{
            .name = "brightness", .has_arg = required_argument, .flag = nullptr, .val = brightness},
        option{
            .name = "blink", .has_arg = required_argument, .flag = nullptr, .val = blink},
        option{
            .name = "border", .has_arg = required_argument, .flag = nullptr, .val = border},
        option{.name = "help", .has_arg = no_argument, .flag = nullptr, .val = 'h'},
        option{.name = nullptr, .has_arg = 0, .flag = nullptr, .val = 0},
    };

    Options result;
    opterr = 0;
    while (true)
    {
        const int parsed =
            getopt_long(argc, argv, "h", options.data(), nullptr); // NOLINT(concurrency-mt-unsafe)
        if (parsed == -1)
        {
            break;
        }
        switch (parsed)
        {
        case list_devices:
            result.list_devices = true;
            break;
        case device:
            result.device = optarg;
            break;
        case dry_run:
            result.dry_run = true;
            break;
        case preview:
            result.show_preview = true;
            break;
        case font:
            result.font = optarg;
            break;
        case vendor_font_dir:
            result.vendor_font_dir = optarg;
            break;
        case no_vendor_font:
            result.no_vendor_font = true;
            break;
        case font_size:
            result.font_size = parse_integer(optarg, "font-size");
            break;
        case threshold:
            result.threshold = parse_integer(optarg, "threshold");
            break;
        case speed: {
            auto parts = split_values(optarg);
            if (parts.size() == 1)
            {
                int s = parse_integer(parts[0], "speed");
                for (auto &slot : result.display.slots) slot.speed = s;
            }
            else
            {
                for (std::size_t i = 0; i < parts.size() && i < MAX_MESSAGES; ++i)
                    result.display.slots[i].speed = parse_integer(parts[i], "speed");
            }
            break;
        }
        case mode: {
            auto parts = split_values(optarg);
            if (parts.size() == 1)
            {
                DisplayMode m = parse_mode(parts[0]);
                for (auto &slot : result.display.slots) slot.mode = m;
            }
            else
            {
                for (std::size_t i = 0; i < parts.size() && i < MAX_MESSAGES; ++i)
                    result.display.slots[i].mode = parse_mode(parts[i]);
            }
            break;
        }
        case brightness:
            result.display.brightness = parse_integer(optarg, "brightness");
            break;
        case blink:
            for (auto idx : parse_slot_indices(optarg))
                result.display.slots[idx].blink = true;
            break;
        case border:
            for (auto idx : parse_slot_indices(optarg))
                result.display.slots[idx].border = true;
            break;
        case 'h':
            result.show_help = true;
            break;
        default:
            throw std::invalid_argument("未知选项；使用 --help 查看帮助");
        }
    }
    for (int index = optind; index < argc; ++index)
    {
        result.messages.emplace_back(argv[index]);
    }
    return result;
}

auto list_devices() -> int
{
    auto devices = BadgeDevice::find_all();
    if (devices.empty())
    {
        std::cout << "没有找到 0416:5020 设备\n";
        return 1;
    }
    for (const auto &device : devices)
    {
        std::cout << device.device_id() << "  " << device.description() << '\n';
    }
    return 0;
}

auto select_device(std::optional<std::string_view> selector) -> BadgeDevice
{
    auto devices = BadgeDevice::find_all();
    if (devices.empty())
    {
        throw std::runtime_error("找不到 0416:5020 CH546 显示屏；请检查 USB 连接与 udev 规则");
    }
    if (!selector)
    {
        if (devices.size() != 1)
        {
            throw std::runtime_error("连接了多块兼容显示屏，请用 --device 指定");
        }
        return std::move(devices.front());
    }
    for (auto &device : devices)
    {
        if (device.device_id() == *selector)
        {
            return std::move(device);
        }
    }
    throw std::runtime_error("找不到设备 " + std::string(*selector));
}

auto run(const Options &options) -> int
{
    if (options.show_help)
    {
        print_help(std::cout);
        return 0;
    }
    if (options.list_devices)
    {
        return list_devices();
    }
    if (options.messages.empty() || options.messages.size() > MAX_MESSAGES)
    {
        throw std::invalid_argument("请提供 1..8 条消息，或使用 --list-devices");
    }

    std::optional<VendorFont> vendor_font;
    if (options.font.empty() && !options.no_vendor_font)
    {
        const auto directory = find_vendor_font_dir(options.vendor_font_dir);
        if (!directory.empty())
        {
            vendor_font = load_vendor_font(directory);
            std::cout << "使用商家原版点阵字库：" << directory << '\n';
        }
        else
        {
            std::cout << "未找到 vendor/ASC11 与 vendor/HZK11；使用轮廓字体\n";
        }
    }

    std::optional<std::filesystem::path> outline_font;
    std::vector<GlyphBitmap> images;
    images.reserve(options.messages.size());
    for (const auto &message : options.messages)
    {
        if (vendor_font)
        {
            try
            {
                images.push_back(render_vendor(message, *vendor_font));
                continue;
            }
            catch (const UnsupportedVendorGlyph &error)
            {
                std::cout << error.what() << "；本条消息回退到轮廓字体\n";
            }
        }
        if (!outline_font)
        {
            outline_font = find_font(options.font);
        }
        images.push_back(
            render_text(message, outline_font->string(), options.font_size, options.threshold));
    }

    if (options.show_preview)
    {
        for (std::size_t index = 0; index < images.size(); ++index)
        {
            std::cout << '[' << index + 1 << "] \"" << options.messages[index] << "\" ("
                      << images[index].width << "×" << ROWS << ")\n"
                      << preview(images[index]) << '\n';
        }
    }

    const auto payload = build_payload(images, options.display);
    std::cout << "编码完成：" << payload.size() << " 字节，" << payload.size() / REPORT_SIZE
              << " 个 CH546 报告包\n";
    if (options.dry_run)
    {
        return 0;
    }

    const auto selector = options.device ? std::optional<std::string_view>(*options.device)
                                         : std::optional<std::string_view>{};
    auto device = select_device(selector);
    std::cout << "写入：" << device.description() << '\n';
    device.write(payload);
    std::cout << "写入完成。拔掉 USB 或按显示屏按钮即可查看。\n";
    return 0;
}
} // namespace

auto main(int argc, char **argv) -> int
{
    try
    {
        return run(parse_options(argc, argv));
    }
    catch (const std::exception &error)
    {
        std::cerr << "错误：" << error.what() << '\n';
        return 1;
    }
}
