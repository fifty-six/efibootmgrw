#include "lak/span.hpp"
#include "lak/alloc.hpp"
#include "lak/utility.hpp"
#include "lak/const_string.hpp"
#include "lak/type_traits.hpp"
#include "lak/optional.hpp"

#include "fmt/format.h"

enum class ParseError {
    SuppliedFlagWithoutArgument,
    ParseFunctionFailed
};

template<typename F, typename V>
concept ParseFunc = requires (F&& f, lak::astring_view view) {
    { f(view) } -> std::convertible_to<lak::optional<V>>;
};

template <typename... Ts, typename F>
auto enumerate_type_fold(F&& f, lak::tuple<Ts...> &tup) -> lak::result<lak::monostate, ParseError> {
    using res_t = lak::result<lak::monostate, ParseError>;

    struct fold_wrapper {
        res_t res;

        fold_wrapper(res_t res) : res{res} {};

        fold_wrapper operator ||(fold_wrapper &&c) {
            if (!res.is_err())
                res = c.res;

            return *this;
        }

        operator res_t() { return res; }
    };

    return [&f, &tup]<auto... Is>(lak::index_sequence<Is...>) -> res_t {
        return (fold_wrapper(f.template operator()<Ts, Is>(tup.template get<Is>())) || ...);
    }(lak::index_sequence_for<Ts...>{});
}

template <lak::aconst_string short_form, lak::aconst_string long_form, typename TValue, typename F>
    requires ParseFunc<F, TValue> || lak::is_same_v<F, void>
struct cli_option {
    lak::optional<TValue> value;

    static_assert(!lak::is_same_v<F, void>, "non-bool specalization must pass function!");
};

template <lak::aconst_string short_form, lak::aconst_string long_form>
struct cli_option<short_form, long_form, bool, void> {
    bool value;
};

template <lak::aconst_string short_form, lak::aconst_string long_form>
struct cli_option<short_form, long_form, lak::astring_view, void> {
    lak::astring_view value;
};

template <lak::aconst_string short_form, lak::aconst_string long_form, typename TValue, typename F>
auto parse_cli_option(cli_option<short_form, long_form, TValue, F>& opt, int i, int argc, const char** argv) -> lak::result<lak::monostate, ParseError>
{
    auto str = lak::astring_view::from_c_str(argv[i]);

    lak::astring_view sf_view (short_form.begin(), short_form.end());
    lak::astring_view lf_view (long_form.begin(), long_form.end());

    if (str != sf_view && str != lf_view) {
        return lak::ok_t {};
    }

    if constexpr (!lak::is_same_v<bool, TValue> || !lak::is_same_v<F, void>) {
        if (i < argc - 1) {
            if constexpr (lak::is_same_v<F, void> && lak::is_same_v<TValue, lak::astring_view>) {
                opt.value = lak::astring_view::from_c_str(argv[i + 1]);
            } else {
                auto parsed = (F{})(lak::astring_view::from_c_str(argv[i + 1]));

                if (!parsed.has_value())
                    return lak::err_t{ParseError::ParseFunctionFailed};

                opt.value = *parsed;
            }
        } else {
            return lak::err_t{ParseError::SuppliedFlagWithoutArgument};
        }
    } else {
        // We don't need something after a boolean arg like -fomit-frame-pointer
        opt.value = true;
    }

    return lak::ok_t {};
}

template <typename... Ts>
struct arg_parser {
    using value_type = lak::tuple<Ts...>;

    int argc;
    const char** argv;

    explicit arg_parser(int argc, const char** argv) : argc {argc}, argv {argv} {};

    auto parse() -> lak::result<value_type, ParseError> {
        value_type res {};

        for (size_t i = 0; i < argc; i++) {
            auto err = enumerate_type_fold<Ts...>([&]<typename T, auto I>(T& t) {
                return parse_cli_option(t, i, argc, argv);
            }, res);

            if (err.is_err())
                return lak::err_t { err.unsafe_unwrap_err() };
        }

        return lak::ok_t { lak::move(res) };
    }
};

auto res_main(int argc, const char** argv) -> lak::result<lak::monostate> {
    arg_parser
    <
        // cli_option<"-a", "-amogus", bool, void>,
        // cli_option<"-i", "-int", int, decltype([](lak::astring_view v) -> lak::optional<int> {
        //     return { std::stoi(v) };
        // })>
        cli_option<"-v", "--verbose", bool, void>,
        cli_option<"-c", "--create", lak::astring_view, void>
    > parser { argc, argv };

    auto res = parser.parse();

    // res.if_ok([](auto&& v) {
    //     auto opt = v.template get<0>();
    //     auto opt2 = v.template get<1>();

    //     fmt::print("opt.val {}\n", opt.val);
    //     fmt::print("opt2.val {}\n", opt2.value.has_value() ? std::to_string(*opt2.value) : "no");
    // });

    return lak::ok_t {};
}

int main(int argc, const char** argv) {
    return res_main(argc, argv).is_ok() ? EXIT_SUCCESS : EXIT_FAILURE;
}

