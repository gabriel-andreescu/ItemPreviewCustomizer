#pragma once

#include "ConfigTypes.h"
#include "ModelMatcher.h"

#include <REX/REX/Singleton.h>

#include <filesystem>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class ConfigManager : public REX::Singleton<ConfigManager> {
public:
    bool Load();
    bool Load(const std::filesystem::path& a_configFolder);

    [[nodiscard]] std::optional<PreviewConfig> GetConfig(std::string_view a_modelPath) const;
    [[nodiscard]] std::size_t GetConfigCount() const;

private:
    friend class REX::Singleton<ConfigManager>;

    struct StoredConfig {
        PreviewConfig preview;
        std::filesystem::path source;
    };

    struct WildcardConfig {
        ModelPattern pattern;
        PreviewConfig preview;
        std::filesystem::path source;
    };

    ConfigManager() = default;

    [[nodiscard]] static std::vector<std::filesystem::path> CollectConfigFiles(
        const std::filesystem::path& a_configFolder
    );

    bool ReadConfigFile(const std::filesystem::path& a_path);
    void AddEntry(const ConfigEntry& a_entry, const std::filesystem::path& a_path, std::size_t a_index);
    [[nodiscard]] std::size_t GetConfigCountUnlocked() const noexcept;

    mutable std::shared_mutex configMutex_;
    std::unordered_map<std::string, StoredConfig> exactConfigs_;
    std::vector<WildcardConfig> wildcardConfigs_;
    std::size_t foundFileCount_ {0};
    std::size_t parsedFileCount_ {0};
    std::size_t failedFileCount_ {0};
    std::size_t skippedEntryCount_ {0};
};
