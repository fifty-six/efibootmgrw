#pragma once

#include "lak/type_pack.hpp"
#include "lak/type_traits.hpp"
#include "lak/span.hpp"
#include "lak/visit.hpp"

#include <type_traits>

template<typename TEnum, typename... Ts, auto S>
constexpr auto make_visitor(std::array<TEnum, S> tags) {
    return [=]<typename... Us> (TEnum tag, lak::span<byte_t> bytes, lak::overloaded<Us...> op) {
        static_assert(tags.size()   == sizeof...(Us));
        static_assert(sizeof...(Ts) == sizeof...(Us));

        auto f = [&]<typename U, auto I>() {
            if (tag == tags[I]) {
                op(*std::launder(reinterpret_cast<U*>(bytes.data())));
            }
        };

        [&f]<auto... Is>(lak::index_sequence<Is...>) {
            (f.template operator()<Ts, Is>(), ...);
        } (lak::index_sequence_for<Us...> {});
    };
}