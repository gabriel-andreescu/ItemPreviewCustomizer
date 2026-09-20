#include "PCH.h" // IWYU pragma: keep

#include "ModelMatcher.h"

#include <CLIBUtil/string.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace {
void CollapseSlashes(std::string& a_path) {
    std::size_t length = 0;
    bool previousSlash = false;
    for (const auto character : a_path) {
        if (character == '\\') {
            if (previousSlash) {
                continue;
            }
            previousSlash = true;
        } else {
            previousSlash = false;
        }

        a_path[length++] = character;
    }

    a_path.resize(length);
}

[[nodiscard]] bool WildcardMatches(std::string_view a_pattern, std::string_view a_value) noexcept {
    constexpr auto kNotFound = std::string_view::npos;

    std::size_t patternIndex = 0;
    std::size_t valueIndex = 0;
    std::size_t starIndex = kNotFound;
    std::size_t retryIndex = 0;

    while (valueIndex < a_value.size()) {
        if (patternIndex < a_pattern.size() && a_pattern[patternIndex] == '*') {
            starIndex = patternIndex++;
            retryIndex = valueIndex;
        } else if (patternIndex < a_pattern.size() && a_pattern[patternIndex] == a_value[valueIndex]) {
            ++patternIndex;
            ++valueIndex;
        } else if (starIndex != kNotFound) {
            patternIndex = starIndex + 1;
            valueIndex = ++retryIndex;
        } else {
            return false;
        }
    }

    while (patternIndex < a_pattern.size() && a_pattern[patternIndex] == '*') {
        ++patternIndex;
    }

    return patternIndex == a_pattern.size();
}
}

std::string NormalizeModelPath(std::string_view a_path) {
    auto normalized = clib_util::string::tolower(a_path);
    clib_util::string::trim(normalized);
    clib_util::string::replace_all(normalized, "/", "\\");
    CollapseSlashes(normalized);

    while (!normalized.empty() && normalized.front() == '\\') {
        normalized.erase(normalized.begin());
    }

    if (normalized.starts_with(R"(.\)")) {
        normalized.erase(0, 2);
    }

    constexpr std::string_view kMeshesPrefix = R"(meshes\)";
    constexpr std::string_view kMeshesMarker = R"(\meshes\)";

    if (!normalized.starts_with(kMeshesPrefix)) {
        if (const auto pos = normalized.find(kMeshesMarker); pos != std::string::npos) {
            normalized.erase(0, pos + 1);
        }
    }

    if (!normalized.empty() && !normalized.starts_with(kMeshesPrefix) && !normalized.contains(':')) {
        normalized.insert(0, kMeshesPrefix);
    }

    return normalized;
}

ModelPattern::ModelPattern(std::string a_pattern)
    : pattern_(std::move(a_pattern))
    , hasWildcard_(pattern_.contains('*')) {}

bool ModelPattern::HasWildcard() const noexcept {
    return hasWildcard_;
}

bool ModelPattern::Matches(std::string_view a_modelPath) const noexcept {
    return hasWildcard_ ? WildcardMatches(pattern_, a_modelPath) : pattern_ == a_modelPath;
}
