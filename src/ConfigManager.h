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

    [[nodiscard]] std::optional<PreviewConfig> GetConfig(std::string_view a_path) const;
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
    };

    ConfigManager() = default;

    [[nodiscard]] static std::vector<std::filesystem::path> CollectConfigFiles(
        const std::filesystem::path& a_configFolder
    );

    bool ReadConfigFile(const std::filesystem::path& a_path);
    void AddEntry(const ConfigEntry& a_entry, const std::filesystem::path& a_path, std::size_t a_index);
    [[nodiscard]] std::size_t GetConfigCountUnlocked() const noexcept;

    mutable std::shared_mutex _configMutex;
    std::unordered_map<std::string, StoredConfig> _exactConfigs;
    std::vector<WildcardConfig> _wildcardConfigs;
    std::size_t _foundFileCount {0};
    std::size_t _parsedFileCount {0};
    std::size_t _failedFileCount {0};
    std::size_t _skippedEntryCount {0};
};
