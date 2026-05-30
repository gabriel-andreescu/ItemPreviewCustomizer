#include "ConsoleCommands.h"

#include "ConfigManager.h"
#include "InventoryPreview.h"

#include <Windows.h>

#include <cstring>
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

    struct CopyPreviewPath {
        constexpr static auto ORIGINAL_COMMAND = "ToggleHeapTracking"sv;
        constexpr static auto LONG_NAME = "CopyIPCPath"sv;
        constexpr static auto SHORT_NAME = "CopyIPCPath"sv;
        constexpr static auto HELP = "Copy the selected inventory item model path to the clipboard\n"sv;

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

            auto* console = RE::ConsoleLog::GetSingleton();
            auto modelPath = InventoryPreview::GetSelectedInventoryModelPath();
            if (!modelPath) {
                constexpr auto kNoSelectedModelPathMessage = "No selected inventory item model path available to copy";
                if (console) {
                    console->Print(kNoSelectedModelPathMessage);
                }
                logger::info("{}", kNoSelectedModelPathMessage);
                return false;
            }

            if (CopyTextToClipboard(*modelPath)) {
                if (console) {
                    console->Print("Copied selected inventory item model path:\n%s", modelPath->c_str());
                }
                logger::info("Copied selected inventory item model path | path={}", *modelPath);
                return false;
            }

            if (console) {
                console->Print("Clipboard copy failed:\n%s", modelPath->c_str());
            }
            logger::warn("Failed to copy selected inventory item model path to clipboard | path={}", *modelPath);
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
        function->SetParameters();
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
