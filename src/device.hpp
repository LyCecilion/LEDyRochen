#pragma once

#include <cstdint>
#include <libusb.h>
#include <vector>

#include "constants.hpp"

struct BadgeDevice
{
    libusb_device_handle *handle{nullptr};
    uint8_t endpoint_out{0};
    uint8_t endpoint_in{0};

    static auto find_all() -> std::vector<BadgeDevice>;

    auto write(const std::vector<uint8_t> &payload) -> void
    {
        for (size_t offset = 0; offset < payload.size(); offset += REPORT_SIZE)
        {
            int written = 0;
            libusb_interrupt_transfer(handle, endpoint_out,
                                      const_cast<uint8_t *>(payload.data()) + offset, REPORT_SIZE,
                                      &written, TRANSFER_TIMEOUT);
            uint8_t ack[REPORT_SIZE]; // NOLINT(modernize-avoid-c-arrays)
            int read = 0;
            libusb_interrupt_transfer(handle, endpoint_in, ack, REPORT_SIZE, &read,
                                      TRANSFER_TIMEOUT);
        }
    }

    auto close() -> void
    {
        if (handle != nullptr)
        {
            libusb_close(handle);
            handle = nullptr;
        }
    }
};
