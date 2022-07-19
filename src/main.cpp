#include <utility>
#include <charconv>
#include <system_error>
#include <ranges>

#include "fmt/ranges.h"
#include "fmt/xchar.h"

#include "efibootmgrw.h"
#include "native_methods.h"
#include "lak/array.hpp"
#include "efi_load_option.h"
#include "cmdline.h"

namespace efibootmgrw {

auto res_main(int argc, const char **argv_ptr) -> lak::result<lak::monostate> {
    lak::span<const char *> argv { argv_ptr, static_cast<size_t>(argc) };

    Context ctx;

    parse_args(ctx, argv);

    // alignas(8) lak::array<byte_t, 4096> buf;
    // alignas(4) byte_t buf[4096];
    caching_efi_load_option e;

    winapi::open_current_process_token(winapi::token_adjust_privileges | winapi::token_query)
            .and_then(winapi::authenticate)
            .and_then([&](auto) {
                return winapi::get_firmware_env_var(
                        L"Boot0002",
                        winapi::efi_global_variable,
                        e.bytes()
                );
            })
            .if_ok([&](auto) {
                fmt::print(L"Boot0002: {}\n", e.desc(ctx).data());
                fmt::print("Writing to EFI Boot0001\n");
            })
            .and_then([&](lak::span<void> buf) {
                return winapi::set_firmware_env_var(
                        L"Boot0001",
                        winapi::efi_global_variable,
                        buf
                );
            })
                    // re-read it
            .and_then([&](auto) {
                e.clear();
                return winapi::get_firmware_env_var(
                        L"Boot0001",
                        winapi::efi_global_variable,
                        e.bytes()
                );
            })
            .if_ok([&](auto) {
                fmt::print(L"Boot0001: {}\n", e.desc(ctx).data());
            })
            .map_err(winapi::win_err::to_wstring)
            .if_err([&](const lak::wstring& w) {
                Fatal(ctx, L"{}", w);
            });

    return lak::ok_t { };
}
}

int main(int argc, const char **argv) {
    return efibootmgrw::res_main(argc, argv).is_ok() ? EXIT_SUCCESS : EXIT_FAILURE;
}

