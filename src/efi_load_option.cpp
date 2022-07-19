#include "efibootmgrw.h"
#include "efi_load_option.h"
#include "efi_device_path.h"

namespace efibootmgrw {

// We have to read into this a bunch anyways, so this is convenient
auto efi_load_option::as_bytes() -> lak::span<byte_t> {
    return lak::span<byte_t> {
            reinterpret_cast<byte_t *>(this),
            sizeof(*this)
    };
}

auto efi_load_option::flexible_data() -> lak::span<byte_t> {
    constexpr size_t offset = sizeof(attributes) + sizeof(file_path_list_length);
    return lak::span<byte_t> {
            reinterpret_cast<byte_t *>(this) + offset,
            sizeof(*this) - offset
    };
}

auto efi_load_option::desc() -> lak::wstring_view {
    return lak::wstring_view::from_c_str(
            reinterpret_cast<const wchar_t *>(data_.data())
    );
}

auto efi_load_option::file_path_list(Context& ctx) -> lak::span<efi_device_path> {
    return file_path_list(ctx, desc().size());
}

auto efi_load_option::file_path_list(Context& ctx, size_t len) -> lak::span<efi_device_path> {
    Fatal(ctx, "unimplemented");
}

}