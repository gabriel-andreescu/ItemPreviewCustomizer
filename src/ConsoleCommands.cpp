#include "ConsoleCommands.h"

#include "ConfigManager.h"
#include "InventoryPreview.h"

#include <Windows.h>

#include <cmath>
#include <cstring>
#include <format>
#include <string>
#include <string_view>

namespace ConsoleCommands {
namespace {
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
        constexpr static auto ORIGINAL_COMMAND = "ToggleBoundVisGeom"sv;
        constexpr static auto LONG_NAME = "ReloadIPC"sv;
        constexpr static auto SHORT_NAME = "ReloadIPC"sv;
        constexpr static auto HELP = "Reload Item Preview Customizer configs from disk\n"sv;

        static bool Execute(
            const RE::SCRIPT_PARAMETER* a_params,
            RE::SCRIPT_FUNCTION::ScriptData* a_scriptData,
            RE::TESObjectREFR* a_thisObj,
            RE::TESObjectREFR* a_containingObj,
            RE::Script* a_scriptObj,
            RE::ScriptLocals* a_locals,
            double& a_result,
            std::uint32_t& a_opcodeOffsetPtr
        ) {
            (void)a_params;
            (void)a_scriptData;
            (void)a_thisObj;
            (void)a_containingObj;
            (void)a_scriptObj;
            (void)a_locals;
            (void)a_result;
            (void)a_opcodeOffsetPtr;

            auto* manager = ConfigManager::GetSingleton();
            manager->Load();

            if (auto* console = RE::ConsoleLog::GetSingleton()) {
                console->Print(
                    "Reloaded %llu config(s). Reopen inventory.",
                    static_cast<unsigned long long>(manager->GetConfigCount())
                );
            }

            return false;
        }
    };

    bool CopyTextToClipboard(std::string_view a_text) {
        if (a_text.empty() || !::OpenClipboard(nullptr)) {
            return false;
        }

        ClipboardScope clipboard;
        static_cast<void>(::EmptyClipboard());

        const auto bufferSize = a_text.size() + 1;
        auto* memory = ::GlobalAlloc(GMEM_MOVEABLE, bufferSize);
        if (!memory) {
            return false;
        }

        auto* buffer = static_cast<char*>(::GlobalLock(memory));
        if (!buffer) {
            static_cast<void>(::GlobalFree(memory));
            return false;
        }

        std::memcpy(buffer, a_text.data(), a_text.size());
        buffer[a_text.size()] = '\0';
        static_cast<void>(::GlobalUnlock(memory));

        if (!::SetClipboardData(CF_TEXT, memory)) {
            static_cast<void>(::GlobalFree(memory));
            return false;
        }

        return true;
    }

    [[nodiscard]] bool ShouldIncludeRotation(RE::SCRIPT_FUNCTION::ScriptData* a_scriptData) {
        if (!a_scriptData || a_scriptData->numParams == 0) {
            return false;
        }

        const auto* chunk = a_scriptData->GetIntegerChunk();
        return chunk && chunk->GetInteger() != 0;
    }

    [[nodiscard]] std::string EscapeJsonString(std::string_view a_value) {
        std::string escaped;
        escaped.reserve(a_value.size());

        for (const auto ch : a_value) {
            switch (ch) {
                case '\\': escaped += R"(\\)"; break;
                case '"':  escaped += R"(\")"; break;
                case '\b': escaped += R"(\b)"; break;
                case '\f': escaped += R"(\f)"; break;
                case '\n': escaped += R"(\n)"; break;
                case '\r': escaped += R"(\r)"; break;
                case '\t': escaped += R"(\t)"; break;
                default:
                    if (static_cast<unsigned char>(ch) < 0x20) {
                        escaped += std::format(
                            R"(\u{:04x})",
                            static_cast<unsigned int>(static_cast<unsigned char>(ch))
                        );
                    } else {
                        escaped.push_back(ch);
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
        constexpr static auto ORIGINAL_COMMAND = "ToggleHeapTracking"sv;
        constexpr static auto LONG_NAME = "CopyIPCPath"sv;
        constexpr static auto SHORT_NAME = "CopyIPCPath"sv;
        constexpr static auto
            HELP = "Copy the current inventory preview rule to the clipboard\n(1: include rotation)"sv;

        constexpr static RE::SCRIPT_PARAMETER SCRIPT_PARAMS = {
            .paramName = "IncludeRotation",
            .paramType = RE::SCRIPT_PARAM_TYPE::kInt,
            .optional = true
        };

        static bool Execute(
            const RE::SCRIPT_PARAMETER* a_params,
            RE::SCRIPT_FUNCTION::ScriptData* a_scriptData,
            RE::TESObjectREFR* a_thisObj,
            RE::TESObjectREFR* a_containingObj,
            RE::Script* a_scriptObj,
            RE::ScriptLocals* a_locals,
            double& a_result,
            std::uint32_t& a_opcodeOffsetPtr
        ) {
            (void)a_params;
            (void)a_thisObj;
            (void)a_containingObj;
            (void)a_scriptObj;
            (void)a_locals;
            (void)a_result;
            (void)a_opcodeOffsetPtr;

            auto* console = RE::ConsoleLog::GetSingleton();
            const auto includeRotation = ShouldIncludeRotation(a_scriptData);
            const auto preview = InventoryPreview::GetCurrentInventoryPreview();
            if (!preview) {
                constexpr auto kNoPreviewModelPathMessage = "No current inventory preview model path available to copy";
                if (console) {
                    console->Print(kNoPreviewModelPathMessage);
                }
                logger::info("{}", kNoPreviewModelPathMessage);
                return false;
            }

            if (includeRotation && !preview->rotation.has_value()) {
                constexpr auto kNoPreviewRotationMessage = "No current inventory preview rotation available to copy";
                if (console) {
                    console->Print(kNoPreviewRotationMessage);
                }
                logger::info("{}", kNoPreviewRotationMessage);
                return false;
            }

            const auto rule = FormatPreviewRule(*preview, includeRotation);
            if (CopyTextToClipboard(rule)) {
                if (console) {
                    console->Print(
                        includeRotation ? "Copied current inventory preview rule with rotation:\n%s"
                                        : "Copied current inventory preview rule:\n%s",
                        rule.c_str()
                    );
                }
                logger::info(
                    "Copied current inventory preview rule | path={} | rotation={}",
                    preview->modelPath,
                    includeRotation
                );
                return false;
            }

            if (console) {
                console->Print("Clipboard copy failed:\n%s", rule.c_str());
            }
            logger::warn("Failed to copy current inventory preview rule to clipboard | path={}", preview->modelPath);
            return false;
        }
    };

    template <class T>
    void InstallConsoleCommand() {
        auto* function = RE::SCRIPT_FUNCTION::LocateConsoleCommand(T::ORIGINAL_COMMAND);
        if (!function) {
            logger::warn("Failed to locate console command slot | command={}", T::ORIGINAL_COMMAND);
            return;
        }

        function->functionName = T::LONG_NAME.data();
        function->shortName = T::SHORT_NAME.data();
        function->helpString = T::HELP.data();
        function->referenceFunction = false;
        if constexpr (requires { T::SCRIPT_PARAMS; }) {
            // CommonLib exposes SetParameters for native script parameter arrays.
            // NOLINTNEXTLINE(modernize-avoid-c-arrays)
            static RE::SCRIPT_PARAMETER params[] = {T::SCRIPT_PARAMS};
            function->SetParameters(params);
        } else {
            function->SetParameters();
        }
        function->executeFunction = &T::Execute;
        function->conditionFunction = nullptr;

        logger::info("Installed console command | command={}", T::LONG_NAME);
    }
}

void Install() {
    InstallConsoleCommand<ReloadConfig>();
    InstallConsoleCommand<CopyPreviewPath>();
}
}
