#include <utility>

#include "fmt/ranges.h"
#include "fmt/xchar.h"
#include "fmt/color.h"

#include "efibootmgrw.h"
#include "native_methods.h"
#include "efi_load_option.h"
#include "cmdline.h"
#include "reinterpret_visitor.h"

namespace efibootmgrw {

auto read_entry(u16 id, efi_load_option& opt) {
    auto var = fmt::format(L"Boot{:0>4LX}", id);

    auto v = winapi::get_firmware_env_var(
            var,
            winapi::efi_global_variable,
            opt.as_bytes()
    );

    return v;
}

auto default_print(Context& ctx) -> winapi::wresult<lak::monostate> {
    auto get_u16 = [&](lak::wstring_view var) -> winapi::wresult<u16> {
        u16 out;

        winapi::wresult<lak::span<void>> res = winapi::get_firmware_env_var(
                var,
                winapi::efi_global_variable,
                { &out, sizeof(u16) }
        );

        // The buffer value is insignificant.
        return res.map([&](auto) { return out; });
    };

    winapi::wresult<u16> boot_next    = get_u16(L"BootNext");
    winapi::wresult<u16> timeout      = get_u16(L"Timeout");
    winapi::wresult<u16> boot_current = get_u16(L"BootCurrent");

    lak::array<byte_t, 8192> buf;
    winapi::wresult<lak::span<void>> boot_order = winapi::get_firmware_env_var(
            L"BootOrder",
            winapi::efi_global_variable,
            lak::span<byte_t> { buf }
    );

    boot_next.if_ok([&](u16 next) {
        fmt::print("BootNext: {:0>4LX}\n", next);
    });

    boot_current.if_ok([&](u16 current) {
        fmt::print("BootCurrent: {:0>4LX}\n", current);
    });

    timeout.if_ok([&](u16 timeout) {
        fmt::print("Timeout: {} seconds\n", timeout);
    });

    return boot_order.map([&](lak::span<void> res) {
        lak::span<u16> ids { res };

        for (u16 id : ids) {
            efi_load_option opt;

            read_entry(id, opt).if_ok([&](lak::span<void>) {
                fmt::print(L"Boot{:0>4LX}: {}\n", id, opt.desc());
            }).if_err([&](winapi::win_err err) {
                fmt::print(
                    stderr,
                    fmt::emphasis::bold | fg(fmt::color::crimson),
                    L"Unable to read Boot{:0>4LX}: {}!",
                    id,
                    lak::wstring_view { err.wstring() }
                );
            });
        }

        return lak::monostate { };
    });
}

template<typename F, typename... Args>
auto partial(F&& f, Args&& ... args) {
    return [=]<typename... Rest>(Rest&& ... rest) mutable {
        return f(
                std::forward<Args&&>(args)...,
                std::forward<Rest&&>(rest)...
        );
    };
}

auto res_main(int argc, const char** argv_ptr) -> lak::result<lak::monostate> {
    lak::span argv { argv_ptr, static_cast<size_t>(argc) };

    Context ctx;

    parse_args(ctx, argv);

    auto fatal_w = partial(Fatal<Context>::from_wstr, ctx);

    winapi::open_current_process_token(winapi::token_adjust_privileges | winapi::token_query)
            .and_then(partial(winapi::authenticate, ctx))
            .map_err(winapi::win_err::to_wstring)
            .if_err(fatal_w);

    if (ctx.cmdline_args.size() == 1) {
        default_print(ctx)
            .map_err(winapi::win_err::to_wstring)
            .if_err(fatal_w);
    }

    // Test of copying Boot0002 to Boot0001
    if (ctx.cmdline_args[1] == "aaa") {
        caching_efi_load_option e;

        auto v = winapi::get_firmware_env_var(
                L"Boot0002",
                winapi::efi_global_variable,
                e.bytes()
        );

        v.if_ok([&](auto) {
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
         .if_err(fatal_w);
    }

    return lak::ok_t { };
}
}

int main(int argc, const char** argv) {
    return efibootmgrw::res_main(argc, argv).is_ok() ? EXIT_SUCCESS : EXIT_FAILURE;
}

