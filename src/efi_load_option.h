#pragma once

#include "lak/array.hpp"
#include "efi_device_path.h"
#include "native_methods.h"

namespace efibootmgrw {

struct efi_load_option {
    u32 attributes = 0;
    u16 file_path_list_length = 0;

    // wchar_t[]                  description;
    // efi_device_path_protocol[] file_path_list;
    // byte_t[]                   optional_data;

    // deal with flexible members
    lak::array<byte_t, 4096> data_;

    [[nodiscard]] auto as_bytes() -> lak::span<byte_t>;

    [[nodiscard]] auto flexible_data() -> lak::span<byte_t>;

    [[nodiscard]] auto desc() -> lak::wstring_view;

    [[nodiscard]] auto file_path_list(Context&) -> lak::span<efi_device_path>;

    // Take the length of the string so we can save the effort
    [[nodiscard]] auto file_path_list(Context&, size_t) -> lak::span<efi_device_path>;

    // Needed for safe usage of this class as we assume
    // wchar_t == u16 in our strings
    static_assert(sizeof(wchar_t) == sizeof(char16_t));
};

struct caching_efi_load_option {
    lak::optional<u16> len;
    efi_load_option e;

    [[nodiscard]]
    auto bytes() -> lak::span<byte_t> {
        return e.as_bytes();
    }

    void clear() {
        len = lak::nullopt;
    }

    [[nodiscard]]
    auto desc(Context& ctx) {
        if (len) {
            fmt::print("len = {}\n", len);
            return lak::wstring_view {
                    lak::span<wchar_t>(e.flexible_data()).subspan(0, len)
            };
        } else {
            auto str = e.desc();

            if (str.size() > static_cast<size_t>(std::numeric_limits<i32>::max())) {
                Fatal(ctx, "Description size exceeds capacity!");
            }

            // Safe to narrow now
            len = static_cast<i32>(str.size());

            return str;
        }
    }

    auto read_into(lak::wstring_view var, lak::wstring_view guid) -> winapi::wresult<lak::span<void>> {
        return winapi::get_firmware_env_var(var, guid, bytes());
    }

    [[nodiscard]]
    auto file_path_list(Context& ctx) -> lak::span<efi_device_path> {
        return len
               ? e.file_path_list(ctx, static_cast<size_t>(len))
               : e.file_path_list(ctx, desc(ctx).size());
    }
};

}