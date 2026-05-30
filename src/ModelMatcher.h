#pragma once

#include <string>
#include <string_view>

[[nodiscard]] std::string NormalizeModelPath(std::string_view a_path);

class ModelPattern {
public:
    explicit ModelPattern(std::string a_pattern);

    [[nodiscard]] const std::string& GetPattern() const noexcept;
    [[nodiscard]] bool HasWildcard() const noexcept;
    [[nodiscard]] bool Matches(std::string_view a_modelPath) const noexcept;

private:
    std::string pattern_;
    bool hasWildcard_ {false};
};
