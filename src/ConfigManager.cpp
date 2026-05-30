#include "ConfigManager.h"

#include <SKSE/SKSE.h>

#include <CLIBUtil/string.hpp>
#include <CLIBUtil/timer.hpp>

#include <algorithm>
#include <cmath>
#include <mutex>
#include <shared_mutex>
#include <system_error>

using namespace std::literals;

namespace {
constexpr std::string_view CONFIG_FOLDER = R"(Data\SKSE\Plugins\ItemPreviewCustomizer)";

[[nodiscard]] bool IsJsonFile(const std::filesystem::path& a_path) {
    return clib_util::string::iequals(a_path.extension().string(), ".json"sv);
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
    return Load(std::filesystem::path {CONFIG_FOLDER});
}

bool ConfigManager::Load(const std::filesystem::path& a_configFolder) {
    std::unique_lock lock(configMutex_);

    exactConfigs_.clear();
    wildcardConfigs_.clear();
    foundFileCount_ = 0;
    parsedFileCount_ = 0;
    failedFileCount_ = 0;
    skippedEntryCount_ = 0;

    logger::info("{:*^50}", "CONFIG FILES");

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

    logger::info(
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

std::optional<PreviewConfig> ConfigManager::GetConfig(std::string_view a_modelPath) const {
    const auto modelPath = NormalizeModelPath(a_modelPath);
    if (modelPath.empty()) {
        return std::nullopt;
    }

    std::shared_lock lock(configMutex_);

    if (const auto it = exactConfigs_.find(modelPath); it != exactConfigs_.end()) {
        return it->second.preview;
    }

    for (auto it = wildcardConfigs_.rbegin(); it != wildcardConfigs_.rend(); ++it) {
        if (it->pattern.Matches(modelPath)) {
            return it->preview;
        }
    }

    return std::nullopt;
}

std::size_t ConfigManager::GetConfigCount() const {
    std::shared_lock lock(configMutex_);
    return GetConfigCountUnlocked();
}

std::size_t ConfigManager::GetConfigCountUnlocked() const noexcept {
    return exactConfigs_.size() + wildcardConfigs_.size();
}

std::vector<std::filesystem::path> ConfigManager::CollectConfigFiles(const std::filesystem::path& a_configFolder) {
    std::vector<std::filesystem::path> files;

    std::error_code ec;
    if (!std::filesystem::exists(a_configFolder, ec)) {
        logger::info("Config folder not found | path={}", a_configFolder.string());
        return files;
    }

    if (!std::filesystem::is_directory(a_configFolder, ec)) {
        logger::warn("Config path is not a folder | path={}", a_configFolder.string());
        return files;
    }

    std::filesystem::directory_iterator iter {
        a_configFolder,
        std::filesystem::directory_options::skip_permission_denied,
        ec
    };

    if (ec) {
        logger::error("Failed to scan config folder | path={} | error={}", a_configFolder.string(), ec.message());
        return files;
    }

    const std::filesystem::directory_iterator end;
    while (iter != end) {
        const auto& entry = *iter;

        std::error_code entryError;
        if (entry.is_regular_file(entryError) && IsJsonFile(entry.path())) {
            files.push_back(entry.path());
        } else if (entryError) {
            logger::warn(
                "Failed to inspect config path | path={} | error={}",
                entry.path().string(),
                entryError.message()
            );
        }

        iter.increment(ec);
        if (ec) {
            logger::warn("Failed to advance config scan | path={} | error={}", a_configFolder.string(), ec.message());
            ec.clear();
        }
    }

    std::ranges::sort(files, [](const auto& a_lhs, const auto& a_rhs) {
        return a_lhs.string() < a_rhs.string();
    });

    return files;
}

bool ConfigManager::ReadConfigFile(const std::filesystem::path& a_path) {
    logger::info("Reading config | path={}", a_path.string());

    std::string buffer;
    std::vector<ConfigEntry> entries;

    const auto err = glz::read_file_json<glz::opts {.error_on_unknown_keys = false}>(entries, a_path.string(), buffer);
    if (err) {
        logger::error("Failed to parse config | path={} | error={}", a_path.string(), glz::format_error(err, buffer));
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
        logger::warn(
            "Ignored invalid preview field(s) | path={} | index={} | fields={}",
            a_path.string(),
            a_index,
            skippedFieldCount
        );
    }

    if (!preview.HasValues()) {
        ++skippedEntryCount_;
        logger::warn("Skipping config entry with no preview fields | path={} | index={}", a_path.string(), a_index);
        return;
    }

    if (a_entry.models.empty()) {
        ++skippedEntryCount_;
        logger::warn("Skipping config entry with no models | path={} | index={}", a_path.string(), a_index);
        return;
    }

    for (const auto& rawModel : a_entry.models) {
        auto model = NormalizeModelPath(rawModel);
        if (model.empty()) {
            ++skippedEntryCount_;
            logger::warn("Skipping empty model path | path={} | index={}", a_path.string(), a_index);
            continue;
        }

        ModelPattern pattern {model};
        if (pattern.HasWildcard()) {
            wildcardConfigs_.push_back(
                WildcardConfig {
                    .pattern = std::move(pattern),
                    .preview = preview,
                    .source = a_path,
                }
            );
            continue;
        }

        if (const auto existing = exactConfigs_.find(model); existing != exactConfigs_.end()) {
            logger::info(
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
