add_rules("mode.debug", "mode.release")

set_project("efibootmgrw")

if is_plat("windows") then
    set_languages("cxxlatest")

    -- bruh
    add_cxflags("/Zc:__cplusplus")
    add_cxflags("/Zc:preprocessor")
else
    set_languages("cxx20")
end

add_rules("mode.debug", "mode.release")

add_requires("fmt 8.0.0")

set_warnings("allextra")

if is_mode("debug") then
    add_defines("DEBUG")

    set_symbols("debug")

    set_optimize("fastest")

    add_cxflags("-fsanitize=address,undefined,integer -g -fno-omit-frame-pointer")
    add_ldflags("-fsanitize=address,undefined,integer -g -fno-omit-frame-pointer")
end

if is_mode("release") then
    add_defines("NDEBUG")

    add_cxflags("-fomit-frame-pointer")

    set_optimize("fastest")
end

target("efibootmgrw")
    set_kind("binary")

    add_files("src/*.cpp")
    add_headerfiles("src/*.h")

    add_syslinks("kernel32", "advapi32", "user32")

    add_includedirs("lak/inc")
    add_includedirs("lak/src")

    add_files("lak/src/*.cpp", {
      includedirs = "lak/inc/",
      defines = {
        "UNICODE",
        "WIN32_LEAN_AND_MEAN",
        "NOMINMAX"
      }
    })

    add_packages("fmt")
