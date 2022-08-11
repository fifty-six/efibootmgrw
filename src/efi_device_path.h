#pragma once

#include "efibootmgrw.h"

namespace efibootmgrw {

enum class DevicePathType : uint8_t {
    Hardware = 0x01,
    Acpi = 0x02,
    Messaging = 0x03,
    Media = 0x04,
    BiosBootSpecification = 0x05,
    // my life would be so much easier if this was 0
    End = 0x7f,
};

struct efi_device_path {
    DevicePathType type;
    uint8_t subtype;
    uint16_t length;
};

}