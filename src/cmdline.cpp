#include "cmdline.h"

#include "fmt/xchar.h"

#include <ranges>
#include <system_error>
#include <charconv>
#include <utility>

namespace efibootmgrw {

std::string_view help = R"(
-a | --active             Set bootnum active.
-A | --inactive           Set bootnum inactive.
-b | --bootnum XXXX       Modify BootXXXX (hex).
-B | --delete-bootnum     Delete bootnum.
-c | --create             Create new variable bootnum and add to bootorder.
-d | --disk disk          (Defaults to /dev/sda) containing loader.
-e | --edd [1|3|-1]       Force EDD 1.0 or 3.0 creation variables, or guess.
-E | --device num         EDD 1.0 device number (defaults to 0x80).
-f | --reconnect          Re-connect devices after driver is loaded.
-F | --no-reconnect       Do not re-connect devices after driver is loaded.
-g | --gpt                Force disk w/ invalid PMBR to be treated as GPT.
-i | --iface name         Create a netboot entry for the named interface.
-l | --loader name        (Defaults to \elilo.efi).
-L | --label label        Boot manager display label (defaults to "Linux").
-n | --bootnext XXXX      Set BootNext to XXXX (hex).
-N | --delete-bootnext    Delete BootNext.
-o | --bootorder XXXX,YYYY,ZZZZ,...     Explicitly set BootOrder (hex).
-O | --delete-bootorder   Delete BootOrder.
-p | --part part          (Defaults to 1) containing loader.
-q | --quiet              Be quiet.
-t | --timeout seconds    Boot manager timeout.
-T | --delete-timeout     Delete Timeout value.
-u | --unicode | --UCS-2  Pass extra args as UCS-2 (default is ASCII).
-v | --verbose            Print additional information.
-V | --version            Return version and exit.
-w | --write-signature    Write unique sig to MBR if needed.
-@ | --append-binary-args Append extra variable args from
file (use - to read from stdin).)";

void parse_args(Context& ctx, lak::span<const char *> argv) {
    vec<lak::astring_view> args_v;

    for (const char *str_p : argv) {
        args_v.push_back(lak::astring_view::from_c_str(str_p));
    }

    ctx.cmdline_args = args_v;

    lak::span<lak::astring_view> args = lak::span { args_v }.subspan(1);
    lak::astring_view arg;

    auto read_arg = [&](lak::astring_view short_name, lak::astring_view long_name) {
        for (auto opt : { short_name, long_name }) {
            if (args[0] != opt) {
                return false;
            }

            if (args.size() == 1) {
                Fatal(ctx, "option {}: argument missing!\n", opt);
            } else {
                arg = args[1];
                args = args.subspan(2);
            }

            return true;
        }

        return false;
    };

    auto read_flag = [&](lak::astring_view short_name, lak::astring_view long_name) {
        for (auto opt : { short_name, long_name }) {
            if (args[0] != opt) {
                return false;
            }

            args = args.subspan(1);
            return true;
        }

        return false;
    };

    auto parse_int = [&](lak::astring_view v, i32 base = 10) -> lak::result<i64, std::error_code> {
        i64 res;

        auto from_chars = std::from_chars(v.begin(), v.end(), res, base);

        if (from_chars.ec == std::errc())
            return lak::ok_t { res };
        else
            return lak::err_t { std::make_error_code(from_chars.ec) };
    };

    auto parse_int_fatal = [&](lak::astring_view flag, lak::astring_view v, i32 base = 10) -> i64 {
        lak::result<i64, std::error_code> res = parse_int(v, base);

        return res.if_err([&](std::error_code ec) {
                    auto msg = ec.message();
                    Fatal(ctx, "failed to parse {} for flag {}: {}\n", v, flag, msg);
                    unreachable();
                })
                .if_ok([&](i64 i) {
                    return i;
                })
                .unsafe_unwrap();
    };

    auto read_int_fatal = [&](lak::astring_view flag, i32 base = 10) -> i64 {
        i64 res = parse_int_fatal(flag, arg, base);
        arg = args[1];
        args = args.subspan(1);
        return res;
    };

    auto read_hex_fatal = [&](lak::astring_view flag) -> i64 {
        return read_int_fatal(flag, 16);
    };

    while (!args.empty()) {
        if (read_flag("-h", "-help")) {
            fmt::print("Usage: {} [options] {}\n", ctx.cmdline_args[0], help);
            exit(0);
        }

        if (read_flag("-a", "--active")) {
            ctx.args.active = true;
        } else if (read_flag("-A", "--inactive")) {
            ctx.args.inactive = true;
        } else if (read_arg("-b", "--bootnum")) {
            ctx.args.boot_num = read_hex_fatal("boot_num");
        } else if (read_arg("-B", "--delete-bootnum")) {
            ctx.args.delete_boot_num = true;
        } else if (read_flag("-c", "--create")) {
            ctx.args.create = true;
        } else if (read_arg("-d", "--disk")) {
            ctx.args.disk = arg;
        } else if (read_arg("-e", "--edd")) {
            if (arg == "-1")
                ctx.args.edd = -1;
            else if (arg == "1")
                ctx.args.edd = 1;
            else if (arg == "3")
                ctx.args.edd = 3;
            else
                Fatal(ctx, "Invalid option for edd: {}. Options are {{ -1, 1, 3 }}.\n.", arg);

            args = args.subspan(1);
        } else if (read_arg("-E", "--device")) {
            ctx.args.device = read_hex_fatal("device");
        } else if (read_flag("-f", "--reconnect")) {
            ctx.args.reconnect = true;
        } else if (read_flag("-F", "--no-reconnect")) {
            ctx.args.no_reconnect = true;
        } else if (read_flag("-g", "--gpt")) {
            ctx.args.force_gpt = true;
        } else if (read_arg("-i", "--iface")) {
            ctx.args.iface = arg;
        } else if (read_arg("-l", "--loader")) {
            ctx.args.loader = arg;
        } else if (read_arg("-L", "--label")) {
            ctx.args.label = arg;
        } else if (read_arg("-n", "--bootnext")) {
            ctx.args.device = read_hex_fatal("bootnext");
        } else if (read_flag("-N", "--delete-bootnext")) {
            ctx.args.delete_boot_next = true;
        } else if (read_arg("-o", "--bootorder")) {
            vec<i64> boot_order;

            for (auto subrange : std::ranges::views::split(arg, ",")) {
                lak::astring_view str { subrange.begin(), subrange.end() };

                boot_order.push_back(parse_int_fatal("boot_order", str, 16));
            }

            ctx.args.boot_order = boot_order;
        } else if (read_flag("-O", "--delete-bootorder")) {
            ctx.args.delete_boot_order = true;
        } else if (read_arg("-p", "--part")) {
            ctx.args.device = read_int_fatal("part");
        } else if (read_flag("-q", "--quiet")) {
            ctx.args.quiet = true;
        } else if (read_arg("-t", "--timeout")) {
            ctx.args.device = read_int_fatal("timeout");
        } else if (read_flag("-T", "--delete-timeout")) {
            ctx.args.delete_timeout = true;
        } else if (read_flag("-u", "--unicode") || read_flag("-u", "--UCS-2")) {
            ctx.args.unicode = true;
        } else if (read_flag("-v", "--verbose")) {
            ctx.args.verbose = true;
        } else if (read_flag("-V", "--version")) {
            ctx.args.version = true;
        } else if (read_flag("-w", "--write_signature")) {
            ctx.args.write_signature = true;
        } else if (read_flag("-@", "--append-binary-args")) {
            ctx.args.append_binary_args = true;
        } else {
            Fatal(ctx, "Unrecognized flag {}\n", args[0]);
        }
    }

    if (ctx.args.version) {
        fmt::print("efibootmgrw: version 0.01a\n");
        exit(0);
    }

    if (ctx.args.delete_boot_num && !ctx.args.boot_num) {
        Fatal(ctx, "Cannot delete unspecified boot number!\n");
    }

    if (ctx.args.create && !ctx.args.boot_num) {
        Fatal(ctx, "Cannot create unspecified boot number!\n");
    }

    if ((ctx.args.active || ctx.args.inactive) && !ctx.args.boot_num) {
        Fatal(ctx, "Cannot change activity of unspecified boot number!\n");
    }
}

}