#pragma once

#include "fmt/format.h"
#include "fmt/xchar.h"

#include "lak/optional.hpp"
#include "lak/result.hpp"

#include "lak/span.hpp"

#include "lak/stdint.hpp"

#include "lak/string.hpp"
#include "lak/string_view_forward.hpp"
#include "lak/string_literals.hpp"

#include <vector>
#include <cstdint>

#include <cassert>

template<typename T>
using vec = std::vector<T>;

using u8    [[maybe_unused]] = uint8_t;
using u16   [[maybe_unused]] = uint16_t;
using u32   [[maybe_unused]] = uint32_t;
using u64   [[maybe_unused]] = uint64_t;
using usize [[maybe_unused]] = uintptr_t;

using i8    [[maybe_unused]] = int8_t;
using i16   [[maybe_unused]] = int16_t;
using i32   [[maybe_unused]] = int32_t;
using i64   [[maybe_unused]] = int64_t;
using isize [[maybe_unused]] = intptr_t;

#ifdef NDEBUG
# ifndef LAK_COMPILER_MSVC
#  define unreachable() __builtin_unreachable()
# else
#  define unreachable() assert(0 && "unreachable")
# endif
#else
#  define unreachable() assert(0 && "unreachable")
#endif

namespace efibootmgrw {

template<typename C>
static std::string add_color(C& ctx, const std::string& msg) {
    if (ctx.args.color_diagnostics)
        return "efibootmgrw: \033[0;1;31m" + msg + ":\033[0m ";
    return "efibootmgrw: " + msg + ": ";
}

struct Context {
    Context() = default;

    vec<lak::astring_view> cmdline_args;

    struct {
        bool delete_boot_num = false;
        bool create = false;
        bool reconnect = false;
        bool no_reconnect = false;
        bool force_gpt = false;
        bool delete_boot_next = false;
        bool quiet = false;
        bool delete_boot_order;
        bool delete_timeout = false;
        bool unicode = false;
        bool verbose = true;
        bool version = false;
        bool write_signature = false;
        bool append_binary_args = false;
        bool color_diagnostics = true;

        lak::optional<i8> edd;

        lak::optional<i64> active;
        lak::optional<i64> inactive;
        lak::optional<i64> boot_num;
        lak::optional<i64> boot_next;
        lak::optional<i64> timeout;

        lak::optional<lak::astring_view> disk;
        lak::optional<lak::astring_view> iface;

        lak::astring_view loader = R"(\elilo.efi)";
        lak::astring_view label = "Linux";

        lak::optional<vec<i64>> boot_order;

        i64 device = 0x80;
        i64 part = 1;
    } args;
};

/*
 * You may be saying, "what the fuck?", but this is to make
 * clang-tidy shut up about "bugprone-unused-raii" for Fatal in particular,
 * as it ignores code from macros and this macro is effectively a no-op wrapper.
 */
// #define Fatal(...) Fatal(__VA_ARGS__)

template<typename C>
struct Fatal {
public:
    explicit Fatal(C& ctx) {
        fmt::print(stderr, "{}", add_color(ctx, "fatal"));
    }

    template<typename... Ts>
    Fatal(C& ctx, const fmt::format_string<Ts...> fmt, Ts&& ... args) {
        fmt::print(stderr, "{}", add_color(ctx, "fatal"));
        fmt::print(stderr, std::forward<const fmt::format_string<Ts...>>(fmt), std::forward<Ts&&>(args)...);
    }

    template<typename... Ts>
    Fatal(C& ctx, const fmt::wformat_string<Ts...> fmt, Ts&& ... args) {
        fmt::print(stderr, "{}", add_color(ctx, "fatal"));
        fmt::print(stderr, std::forward<const fmt::wformat_string<Ts...>>(fmt), std::forward<Ts&&>(args)...);
    }

    [[noreturn]] ~Fatal() {
        exit(1);
    }

    template<class T>
    Fatal& operator<<(T&& val) {
        if constexpr (lak::is_same_v<lak::remove_cvref_t<T>, std::basic_string<wchar_t>>) {
            fmt::print(stderr, L"{}", std::forward<T&&>(val));
        } else {
            fmt::print(stderr, "{}", std::forward<T&&>(val));
        }
        return *this;
    }
};

}