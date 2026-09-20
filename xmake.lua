set_xmakever("3.1.1")
set_project("ItemPreviewCustomizer")
set_license("GPL-3.0")
set_policy("package.requires_lock", true)

local version = "0.2.2"

add_repositories("bmk https://github.com/gabriel-andreescu/BethesdaModKit.git")
add_addons("bmk 0.3.0")
includes("@addon/bmk/project")
includes("@addon/bmk/native")

-- Dependencies
add_requires("commonlibsse-ng 8.0.1", { system = false })
add_requires("clib-util 1.5.0", { system = false })
add_requires("glaze 6.0.3", { system = false })
add_requires("catch2 3.15.2", { system = false })
add_requires("bmk")

-- Build targets

target("Native", function()
    set_default(false)
    set_basename("ItemPreviewCustomizer")
    set_version(version)
    set_pcxxheader("src/PCH.h")
    add_rules("@commonlibsse-ng/plugin", {
        author = "GabonZ",
        description = "Customizes item preview zoom and rotation.",
    })
    add_rules("@addon/bmk/skyrim.plugin")
    add_files("$(projectdir)/src/**.cpp")
    add_includedirs("$(projectdir)/src")
    add_defines("WIN32_LEAN_AND_MEAN", "NOGDI")
    add_packages("commonlibsse-ng", "clib-util", "glaze", "bmk")
    add_syslinks("user32")
end)

target("NativeTests", function()
    set_kind("binary")
    set_default(false)
    add_rules("platform.windows.subsystem")
    set_values("windows.subsystem", "console")
    add_rules("@addon/bmk/native.compiler")
    set_pcxxheader("src/PCH.h")
    add_defines("WIN32_LEAN_AND_MEAN", "NOGDI")
    add_files(
        "src/ConfigManager.cpp",
        "src/ModelMatcher.cpp",
        "src/PreviewMarkerMath.cpp",
        "tests/ConfigManagerTests.cpp",
        "tests/ModelMatcherTests.cpp",
        "tests/PreviewMarkerMathTests.cpp"
    )
    add_includedirs("src")
    add_packages("commonlibsse-ng", "clib-util", "glaze")
    add_packages("catch2", { components = { "main", "lib" } })
    add_tests("native")
end)

-- Packages
target("ItemPreviewCustomizer", function()
    set_version(version)
    add_rules("@addon/bmk/skyrim.package", {
        targets = {
            "Native",
        },
        nexus = {
            mod_id = "7318624453648",
            file_id = "7480700",
            category = "main",
            primary = true,
            display_name = "Item Preview Customizer",
        },
    })
    add_installfiles("$(projectdir)/assets/(**)")
end)
