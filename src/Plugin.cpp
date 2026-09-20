#include "ConfigManager.h"
#include "ConsoleCommands.h"
#include "Hooks.h"
#include "Settings.h"

#include <SKSE/SKSE.h>

namespace {
void MessageHandler(SKSE::MessagingInterface::Message* a_message) { // NOLINT(misc-const-correctness)
    if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
        ConfigManager::GetSingleton()->Load();
        Hooks::Install();
        ConsoleCommands::Install();
        SKSE::log::info("Item Preview Customizer initialized");
    }
}
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_extender) {
    SKSE::Init(
        a_extender,
        {
            .logPattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v",
            .trampoline = true,
            .trampolineSize = 14UZ * 2,
        }
    );
    Settings::Reload();

    SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);
    return true;
}
