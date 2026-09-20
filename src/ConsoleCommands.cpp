#include "PCH.h" // IWYU pragma: keep

#include "ConsoleCommands.h"

#include "ConfigManager.h"
#include "InventoryPreview.h"
#include "Settings.h"

#include <RE/C/CommandTable.h>
#include <RE/C/ConsoleLog.h>
#include <RE/S/Script.h>
#include <RE/T/TESObjectREFR.h>
#include <SKSE/SKSE.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <winbase.h>
#include <winuser.h>

namespace ConsoleCommands {
namespace {
    void PrintConsole(const std::string& a_message) {
        if (auto* console = RE::ConsoleLog::GetSingleton()) {
            // Skyrim exposes console output through a printf-style function.
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            console->Print("%s", a_message.c_str());
        }
    }

    struct ClipboardScope {
        ClipboardScope() = default;

        ClipboardScope(const ClipboardScope&) = delete;
        ClipboardScope(ClipboardScope&&) = delete;
        ClipboardScope& operator=(const ClipboardScope&) = delete;
        ClipboardScope& operator=(ClipboardScope&&) = delete;

        ~ClipboardScope() {
            ::CloseClipboard();
        }
    };

    struct ReloadConfig {
        constexpr static std::string_view kOriginalCommand = "ToggleBoundVisGeom";
        constexpr static std::string_view kLongName = "ReloadIPC";
        constexpr static std::string_view kShortName = "ReloadIPC";
        constexpr static std::string_view kHelp = "Reload Item Preview Customizer configs from disk\n";

        // The native console callback has eight ABI-defined parameters.
        // NOLINTNEXTLINE(readability-function-size)
        static bool Execute(
            const RE::SCRIPT_PARAMETER* /*params*/,
            RE::SCRIPT_FUNCTION::ScriptData* /*scriptData*/,
            RE::TESObjectREFR* /*thisObj*/,
            RE::TESObjectREFR* /*containingObj*/,
            RE::Script* /*scriptObj*/,
            RE::ScriptLocals* /*locals*/,
            double& /*result*/,
            std::uint32_t& /*opcodeOffset*/
        ) {
            Settings::Reload();
            auto* manager = ConfigManager::GetSingleton();
            manager->Load();

            PrintConsole(std::format("Reloaded {} config(s). Reopen inventory.", manager->GetConfigCount()));

            return false;
        }
    };

    bool CopyTextToClipboard(std::string_view a_text) {
        if (a_text.empty() || (::OpenClipboard(nullptr) == 0)) {
            return false;
        }

        ClipboardScope const clipboard;
        static_cast<void>(::EmptyClipboard());

        const auto bufferSize = a_text.size() + 1;
        auto* memory = ::GlobalAlloc(GMEM_MOVEABLE, bufferSize);
        if (memory == nullptr) {
            return false;
        }

        auto* buffer = static_cast<char*>(::GlobalLock(memory));
        if (buffer == nullptr) {
            static_cast<void>(::GlobalFree(memory));
            return false;
        }

        std::memcpy(buffer, a_text.data(), a_text.size());
        std::span {buffer, bufferSize}.back() = '\0';
        static_cast<void>(::GlobalUnlock(memory));

        if (::SetClipboardData(CF_TEXT, memory) == nullptr) {
            static_cast<void>(::GlobalFree(memory));
            return false;
        }

        return true;
    }

    [[nodiscard]] bool ShouldIncludeRotation(RE::SCRIPT_FUNCTION::ScriptData* a_scriptData) {
        if ((a_scriptData == nullptr) || a_scriptData->numParams == 0) {
            return false;
        }

        const auto* chunk = a_scriptData->GetIntegerChunk();
        return (chunk != nullptr) && chunk->GetInteger() != 0;
    }

    [[nodiscard]] std::string EscapeJsonString(std::string_view a_value) {
        std::string escaped;
        escaped.reserve(a_value.size());

        for (const auto character : a_value) {
            switch (character) {
                case '\\': escaped += R"(\\)"; break;
                case '"':  escaped += R"(\")"; break;
                case '\b': escaped += R"(\b)"; break;
                case '\f': escaped += R"(\f)"; break;
                case '\n': escaped += R"(\n)"; break;
                case '\r': escaped += R"(\r)"; break;
                case '\t': escaped += R"(\t)"; break;
                default:
                    if (static_cast<unsigned char>(character) < 0x20) {
                        escaped += std::format(
                            R"(\u{:04x})",
                            static_cast<unsigned int>(static_cast<unsigned char>(character))
                        );
                    } else {
                        escaped.push_back(character);
                    }
                    break;
            }
        }

        return escaped;
    }

    [[nodiscard]] std::string FormatRotationDegrees(float a_degrees) {
        if (std::abs(a_degrees) < 0.0005F || std::abs(a_degrees - 360.0F) < 0.0005F) {
            a_degrees = 0.0F;
        }

        auto text = std::format("{:.3f}", a_degrees);
        while (text.contains('.') && text.ends_with('0')) {
            text.pop_back();
        }
        if (text.ends_with('.')) {
            text.pop_back();
        }

        return text == "-0" ? "0" : text;
    }

    [[nodiscard]] std::string FormatPreviewRule(
        const InventoryPreview::CurrentInventoryPreview& a_preview,
        const bool a_includeRotation
    ) {
        std::string rule = std::format(
            "{{\n"
            "  \"models\": [\"{}\"]",
            EscapeJsonString(a_preview.modelPath)
        );

        if (a_includeRotation && a_preview.rotation.has_value()) {
            rule += std::format(
                ",\n"
                "  \"rotation\": {{\n"
                "    \"x\": {},\n"
                "    \"y\": {},\n"
                "    \"z\": {}\n"
                "  }}",
                FormatRotationDegrees(a_preview.rotation->x),
                FormatRotationDegrees(a_preview.rotation->y),
                FormatRotationDegrees(a_preview.rotation->z)
            );
        }

        rule += "\n}";
        return rule;
    }

    struct CopyPreviewPath {
        constexpr static std::string_view kOriginalCommand = "ToggleHeapTracking";
        constexpr static std::string_view kLongName = "CopyIPCPath";
        constexpr static std::string_view kShortName = "CopyIPCPath";
        constexpr static std::string_view
            kHelp = "Copy the current inventory preview rule to the clipboard\n(1: include rotation)";

        constexpr static RE::SCRIPT_PARAMETER kScriptParams = {
            .paramName = "IncludeRotation",
            .paramType = RE::SCRIPT_PARAM_TYPE::kInt,
            .optional = true,
        };

        // The native console callback has eight ABI-defined parameters.
        // NOLINTNEXTLINE(readability-function-size)
        static bool Execute(
            const RE::SCRIPT_PARAMETER* /*params*/,
            RE::SCRIPT_FUNCTION::ScriptData* a_scriptData,
            RE::TESObjectREFR* /*thisObj*/,
            RE::TESObjectREFR* /*containingObj*/,
            RE::Script* /*scriptObj*/,
            RE::ScriptLocals* /*locals*/,
            double& /*result*/,
            std::uint32_t& /*opcodeOffset*/
        ) {
            const auto includeRotation = ShouldIncludeRotation(a_scriptData);
            const auto preview = InventoryPreview::GetCurrentInventoryPreview();
            if (!preview) {
                constexpr auto kNoPreviewModelPathMessage = "No current inventory preview model path available to copy";
                PrintConsole(kNoPreviewModelPathMessage);
                SKSE::log::info("{}", kNoPreviewModelPathMessage);
                return false;
            }

            if (includeRotation && !preview->rotation.has_value()) {
                constexpr auto kNoPreviewRotationMessage = "No current inventory preview rotation available to copy";
                PrintConsole(kNoPreviewRotationMessage);
                SKSE::log::info("{}", kNoPreviewRotationMessage);
                return false;
            }

            const auto rule = FormatPreviewRule(*preview, includeRotation);
            if (CopyTextToClipboard(rule)) {
                PrintConsole(
                    std::format(
                        "Copied current inventory preview rule{}:\n{}",
                        includeRotation ? " with rotation" : "",
                        rule
                    )
                );
                SKSE::log::info(
                    "Copied current inventory preview rule | path={} | rotation={}",
                    preview->modelPath,
                    includeRotation
                );
                return false;
            }

            PrintConsole(std::format("Clipboard copy failed:\n{}", rule));
            SKSE::log::warn("Failed to copy current inventory preview rule to clipboard | path={}", preview->modelPath);
            return false;
        }
    };

    template <class T>
    void InstallConsoleCommand() {
        auto* function = RE::SCRIPT_FUNCTION::LocateConsoleCommand(T::kOriginalCommand);
        if (!function) {
            SKSE::log::warn("Failed to locate console command slot | command={}", T::kOriginalCommand);
            return;
        }

        function->functionName = T::kLongName.data();
        function->shortName = T::kShortName.data();
        function->helpString = T::kHelp.data();
        function->referenceFunction = false;
        if constexpr (requires { T::kScriptParams; }) {
            // CommonLib exposes SetParameters for native script parameter arrays.
            // NOLINTNEXTLINE(modernize-avoid-c-arrays)
            static RE::SCRIPT_PARAMETER params[] = {T::kScriptParams};
            function->SetParameters(params);
        } else {
            function->SetParameters();
        }
        function->executeFunction = &T::Execute;
        function->conditionFunction = nullptr;

        SKSE::log::info("Installed console command | command={}", T::kLongName);
    }
}

void Install() {
    InstallConsoleCommand<ReloadConfig>();
    InstallConsoleCommand<CopyPreviewPath>();
}
}
