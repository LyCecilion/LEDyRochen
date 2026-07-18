#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "constants.hpp"
#include "types.hpp"

enum class DisplayMode : uint8_t
{
    left = 0,
    right,
    up,
    down,
    still,
    animation,
    drop,
    curtain,
    laser,
};

struct DisplaySettings
{
    int speed{4};
    DisplayMode mode{DisplayMode::left};
    int brightness{50};
    bool blink{false};
    bool border{false};
};

struct ProtocolTimestamp
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
};

auto current_timestamp() -> ProtocolTimestamp;
auto build_header(std::span<const uint16_t> lengths, const DisplaySettings &settings,
                  const ProtocolTimestamp &timestamp) -> std::array<uint8_t, REPORT_SIZE>;
auto build_payload(std::span<const GlyphBitmap> images, const DisplaySettings &settings,
                   const ProtocolTimestamp &timestamp = current_timestamp())
    -> std::vector<uint8_t>;
auto preview(const GlyphBitmap &image) -> std::string;
