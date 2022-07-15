add_rules("mode.debug", "mode.release")

set_project("efibootmgrw")

if is_plat("windows") then
    set_languages("cxxlatest")
    -- bruh
    add_cxflags("/Zc:__cplusplus")
    add_cxflags("/Zc:preprocessor")

    -- add_cxflags("/p:AppxBundlePlatforms=x64", { force = true })
    -- add_cxflags("/p:AppxPackageDir=./AppxPackages", { force = true })
    -- add_cxflags("/p:AppxBundle=Always", { force = true })
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

--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro defination
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

