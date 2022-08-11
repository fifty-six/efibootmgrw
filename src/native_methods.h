#pragma once

#include "efibootmgrw.h"

#include "lak/../../src/win32/wrapper.hpp"

#include "minwindef.h"
#include "WinBase.h"

namespace efibootmgrw::winapi {
using dword = DWORD;
using luid_t = LUID;
using token_elevation_t = TOKEN_ELEVATION;

struct win_err {
    dword err;

    explicit win_err(dword err) : err { err } {}

    [[nodiscard]] auto wstring() const -> lak::wstring {
        return lak::winapi::error_code_to_wstring(err);
    }

    [[nodiscard]] static auto to_wstring(const struct win_err& e) -> lak::wstring {
        return e.wstring();
    }
};

struct handle_t {
    HANDLE handle;

    explicit handle_t(HANDLE handle) : handle { handle } {}

    [[nodiscard]] auto data() -> HANDLE * {
        return &handle;
    }

    [[nodiscard]] operator HANDLE() const { // NOLINT(google-explicit-constructor)
        return handle;
    }
};

struct tok_privs {
    dword count;
    luid_t luid;
    dword attributes;
};

template<typename T>
using wresult = lak::result<T, win_err>;

const lak::wstring efi_global_variable = L"{8BE4DF61-93CA-11D2-AA0D-00E098032B8C}";

const lak::wstring sys_env_priv = SE_SYSTEM_ENVIRONMENT_NAME;
const lak::wstring shutdown_priv = SE_SHUTDOWN_NAME;

constexpr dword token_adjust_privileges = TOKEN_ADJUST_PRIVILEGES;
constexpr dword token_query = TOKEN_QUERY;

[[nodiscard]]
inline auto get_last_error() -> win_err {
    return win_err { ::GetLastError() };
}

[[nodiscard]]
inline auto open_current_process_token(dword privs) -> wresult<handle_t> {
    handle_t token { nullptr };

    auto proc = ::GetCurrentProcess();

    if (!::OpenProcessToken(proc, privs, token.data()))
        return lak::err_t { get_last_error() };

    return lak::ok_t { token };;
}

[[nodiscard]]
inline auto open_current_process_token() -> wresult<handle_t> {
    return lak::ok_t { handle_t { ::GetCurrentProcessToken() }};
}

[[nodiscard]]
inline auto authenticate(Context& ctx, handle_t token) -> wresult<lak::monostate> {
    for (const lak::wstring& priv : { sys_env_priv, shutdown_priv }) {
        tok_privs privs { };

        if (!::LookupPrivilegeValueW(nullptr, priv.data(), &privs.luid))
            return lak::err_t { get_last_error() };

        privs.count += 1;
        privs.attributes = SE_PRIVILEGE_ENABLED;

        if (!::AdjustTokenPrivileges(token, false, reinterpret_cast<TOKEN_PRIVILEGES*>(&privs), 0, nullptr, nullptr))
            return lak::err_t { get_last_error() };
    }

    token_elevation_t elevation;
    dword cb_size = sizeof(token_elevation_t);
    // Even if the token has the privileges, it won't be valid
    // if we aren't in administrator mode, so check that now.
    if (!::GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &cb_size)) {
        return lak::err_t { get_last_error() };
    } else if (!elevation.TokenIsElevated) {
        Fatal(ctx) << "Not running under administrator mode!\n";
    }

    return lak::ok_t { };
}

[[nodiscard]]
inline auto get_firmware_env_var(lak::wstring_view name, lak::wstring_view guid, lak::span<void> buf)
-> wresult<lak::span<void>> {
    dword ret = ::GetFirmwareEnvironmentVariableW(name.data(), guid.data(), buf.data(), buf.size_bytes());

    if (ret == 0) {
        return lak::err_t { get_last_error() };
    }

    return lak::ok_t { lak::span<void> { lak::span<byte_t> { buf }.subspan(0, ret) }};
}

inline auto set_firmware_env_var(lak::wstring_view name, lak::wstring_view guid, lak::span<void> buf)
-> wresult<lak::monostate> {
    if (!::SetFirmwareEnvironmentVariableW(name.data(), guid.data(), buf.data(), buf.size_bytes())) {
        return lak::err_t { get_last_error() };
    }

    return lak::ok_t { };
}

}