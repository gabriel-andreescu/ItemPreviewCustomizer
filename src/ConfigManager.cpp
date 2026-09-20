#include "PCH.h" // IWYU pragma: keep

#include "ConfigManager.h"

#include "ConfigTypes.h"
#include "ModelMatcher.h"

#include <SKSE/SKSE.h>

#include <CLIBUtil/string.hpp>
#include <CLIBUtil/timer.hpp>
#include <glaze/core/opts.hpp>
#include <glaze/core/reflect.hpp>
#include <glaze/json/read.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <optional>
#include <ranges>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {
constexpr std::string_view kConfigFolder = R"(Data\SKSE\Plugins\ItemPreviewCustomizer)";

[[nodiscard]] bool IsJsonFile(const std::filesystem::path& a_path) {
    return clib_util::string::iequals(a_path.extension().string(), ".json");
}

[[nodiscard]] bool IsValidZoom(const float a_zoom) noexcept {
    return std::isfinite(a_zoom) && a_zoom > 0.0F;
}

[[nodiscard]] bool IsValidRotationDegrees(const float a_rotation) noexcept {
    return std::isfinite(a_rotation);
}

void SanitizeRotation(RotationOverride& a_rotation, std::size_t& a_skippedFieldCount) {
    if (a_rotation.x.has_value() && !IsValidRotationDegrees(*a_rotation.x)) {
        a_rotation.x.reset();
        ++a_skippedFieldCount;
    }
    if (a_rotation.y.has_value() && !IsValidRotationDegrees(*a_rotation.y)) {
        a_rotation.y.reset();
        ++a_skippedFieldCount;
    }
    if (a_rotation.z.has_value() && !IsValidRotationDegrees(*a_rotation.z)) {
        a_rotation.z.reset();
        ++a_skippedFieldCount;
    }
}
}

bool ConfigManager::Load() {
    return Load(std::filesystem::path {kConfigFolder});
}

bool ConfigManager::Load(const std::filesystem::path& a_configFolder) {
    std::unique_lock const lock(configMutex_);

    exactConfigs_.clear();
    wildcardConfigs_.clear();
    foundFileCount_ = 0;
    parsedFileCount_ = 0;
    failedFileCount_ = 0;
    skippedEntryCount_ = 0;

    clib_util::Timer timer;
    timer.start();

    const auto files = CollectConfigFiles(a_configFolder);
    foundFileCount_ = files.size();

    for (const auto& file : files) {
        if (ReadConfigFile(file)) {
            ++parsedFileCount_;
        } else {
            ++failedFileCount_;
        }
    }

    timer.stop();

    SKSE::log::info(
        "Config load complete | files_found={} | files_parsed={} | files_failed={} | configs={} | skipped={} | time={}ms",
        foundFileCount_,
        parsedFileCount_,
        failedFileCount_,
        GetConfigCountUnlocked(),
        skippedEntryCount_,
        timer.duration_ms()
    );

    return GetConfigCountUnlocked() > 0;
}

std::optional<PreviewConfig> ConfigManager::GetConfig(std::string_view a_path) const {
    const auto modelPath = NormalizeModelPath(a_path);
    if (modelPath.empty()) {
        return std::nullopt;
    }

    std::shared_lock const lock(configMutex_);

    if (const auto match = exactConfigs_.find(modelPath); match != exactConfigs_.end()) {
        return match->second.preview;
    }

    for (const auto& wildcardConfig : std::views::reverse(wildcardConfigs_)) {
        if (wildcardConfig.pattern.Matches(modelPath)) {
            return wildcardConfig.preview;
        }
    }

    return std::nullopt;
}

std::size_t ConfigManager::GetConfigCount() const {
    std::shared_lock const lock(configMutex_);
    return GetConfigCountUnlocked();
}

std::size_t ConfigManager::GetConfigCountUnlocked() const noexcept {
    return exactConfigs_.size() + wildcardConfigs_.size();
}

std::vector<std::filesystem::path> ConfigManager::CollectConfigFiles(const std::filesystem::path& a_configFolder) {
    std::vector<std::filesystem::path> files;

    std::error_code error;
    if (!std::filesystem::exists(a_configFolder, error)) {
        SKSE::log::info("Config folder not found | path={}", a_configFolder.string());
        return files;
    }

    if (!std::filesystem::is_directory(a_configFolder, error)) {
        SKSE::log::warn("Config path is not a folder | path={}", a_configFolder.string());
        return files;
    }

    std::filesystem::directory_iterator iter {
        a_configFolder,
        std::filesystem::directory_options::skip_permission_denied,
        error
    };

    if (error) {
        SKSE::log::error("Failed to scan config folder | path={} | error={}", a_configFolder.string(), error.message());
        return files;
    }

    const std::filesystem::directory_iterator end;
    while (iter != end) {
        const auto& entry = *iter;

        std::error_code entryError;
        if (entry.is_regular_file(entryError) && IsJsonFile(entry.path())) {
            files.push_back(entry.path());
        } else if (entryError) {
            SKSE::log::warn(
                "Failed to inspect config path | path={} | error={}",
                entry.path().string(),
                entryError.message()
            );
        }

        iter.increment(error);
        if (error) {
            SKSE::log::warn(
                "Failed to advance config scan | path={} | error={}",
                a_configFolder.string(),
                error.message()
            );
            error.clear();
        }
    }

    std::ranges::sort(files, [](const auto& a_lhs, const auto& a_rhs) { return a_lhs.string() < a_rhs.string(); });

    return files;
}

bool ConfigManager::ReadConfigFile(const std::filesystem::path& a_path) {
    SKSE::log::info("Reading config | path={}", a_path.string());

    std::string buffer;
    std::vector<ConfigEntry> entries;

    const auto err = glz::read_file_json<glz::opts {.error_on_unknown_keys = false}>(entries, a_path.string(), buffer);
    if (err) {
        SKSE::log::error(
            "Failed to parse config | path={} | error={}",
            a_path.string(),
            glz::format_error(err, buffer)
        );
        return false;
    }

    for (std::size_t i = 0; i < entries.size(); ++i) {
        AddEntry(entries[i], a_path, i);
    }

    return true;
}

void ConfigManager::AddEntry(
    const ConfigEntry& a_entry,
    const std::filesystem::path& a_path,
    const std::size_t a_index
) {
    auto preview = a_entry.GetPreviewConfig();
    std::size_t skippedFieldCount = 0;

    if (preview.zoom.has_value() && !IsValidZoom(*preview.zoom)) {
        preview.zoom.reset();
        ++skippedFieldCount;
    }

    SanitizeRotation(preview.rotation, skippedFieldCount);

    if (skippedFieldCount > 0) {
        SKSE::log::warn(
            "Ignored invalid preview field(s) | path={} | index={} | fields={}",
            a_path.string(),
            a_index,
            skippedFieldCount
        );
    }

    if (!preview.HasValues()) {
        ++skippedEntryCount_;
        SKSE::log::warn("Skipping config entry with no preview fields | path={} | index={}", a_path.string(), a_index);
        return;
    }

    if (a_entry.models.empty()) {
        ++skippedEntryCount_;
        SKSE::log::warn("Skipping config entry with no models | path={} | index={}", a_path.string(), a_index);
        return;
    }

    for (const auto& rawModel : a_entry.models) {
        auto model = NormalizeModelPath(rawModel);
        if (model.empty()) {
            ++skippedEntryCount_;
            SKSE::log::warn("Skipping empty model path | path={} | index={}", a_path.string(), a_index);
            continue;
        }

        ModelPattern pattern {model};
        if (pattern.HasWildcard()) {
            wildcardConfigs_.push_back(
                WildcardConfig {
                    .pattern = std::move(pattern),
                    .preview = preview,
                }
            );
            continue;
        }

        if (const auto existing = exactConfigs_.find(model); existing != exactConfigs_.end()) {
            SKSE::log::info(
                "Overriding exact model config | model={} | old={} | new={}",
                model,
                existing->second.source.string(),
                a_path.string()
            );
        }

        exactConfigs_.insert_or_assign(
            std::move(model),
            StoredConfig {
                .preview = preview,
                .source = a_path,
            }
        );
    }
}
