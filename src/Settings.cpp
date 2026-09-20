#include "Settings.h"

#include <SKSE/SKSE.h>

#include <BMK/Settings.h>
#include <CLIBUtil/simpleINI.hpp>
#include <spdlog/spdlog.h>

namespace Settings {
namespace {
    Values current; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    void ReadValues(CSimpleIniA& a_ini, Values& a_values) {
        clib_util::ini::get_value(
            a_ini,
            a_values.debugLogging,
            "General",
            "bDebugLogging",
            "; Toggle debug logging",
            clib_util::ini::bool_format::kNumeric
        );
    }

}

const Values& Get() {
    return current;
}

void Reload() {
    const auto initialValues = Values {};
    auto loaded = BMK::Settings::Load(
        {
            .defaults = L"Data/MCM/Config/ItemPreviewCustomizer/settings.ini",
            .user = L"Data/MCM/Settings/ItemPreviewCustomizer.ini",
        },
        initialValues,
        [](CSimpleIniA& a_defaults, CSimpleIniA& a_user, Values& a_candidate) {
            ReadValues(a_defaults, a_candidate);
            ReadValues(a_user, a_candidate);
        }
    );
    if (!loaded) {
        SKSE::log::warn("Cannot load settings: {}", loaded.error().message);
        return;
    }
    if (loaded->saveFailure) {
        SKSE::log::warn("Cannot save settings: {}", loaded->saveFailure->message);
    }

    current = loaded->values;
    BMK::Settings::ApplyLogLevel(current.debugLogging, SKSE::InitInfo {}.logLevel);
}
}
