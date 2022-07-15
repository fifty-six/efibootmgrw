#pragma once

#include <fmt/format.h>
#include "efibootmgrw.h"

#include "lak/string.hpp"

template <> struct fmt::formatter<lak::wstring> {
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) {
        auto it = ctx.begin(), end = ctx.end();

        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const lak::wstring& str, FormatContext& ctx) -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "{}", lak::wstring_view { str });
    }
};