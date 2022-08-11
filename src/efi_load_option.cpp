#include "efibootmgrw.h"
#include "efi_load_option.h"
#include "efi_device_path.h"
#include "reinterpret_visitor.h"

namespace efibootmgrw {

// We have to read into this a bunch anyways, so this is convenient
auto efi_load_option::as_bytes() -> lak::span<byte_t> {
    return lak::span<byte_t> {
            reinterpret_cast<byte_t*>(this),
            sizeof(*this)
    };
}

auto efi_load_option::flexible_data() -> lak::span<byte_t> {
    return { data_ };
}

auto efi_load_option::desc() -> lak::wstring_view {
    return lak::wstring_view::from_c_str(
            reinterpret_cast<const wchar_t*>(data_.data())
    );
}

auto efi_load_option::file_path_list(Context& ctx) -> lak::span<efi_device_path> {
    return file_path_list(ctx, desc().size());
}

auto efi_load_option::file_path_list(Context& ctx, size_t offset) -> lak::span<efi_device_path> {
    // Fairly sure this is UB anyways but it's a flexible length type
    // so I can't even use std::bit_cast, oh well.
    auto* path = reinterpret_cast<efi_device_path*>(std::launder(data_.data() + offset));

    Fatal(ctx, "unimplemented");
}

}