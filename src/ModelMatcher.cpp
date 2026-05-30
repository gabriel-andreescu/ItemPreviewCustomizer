#include "ModelMatcher.h"

#include <CLIBUtil/string.hpp>

#include <cstddef>
#include <utility>

using namespace std::literals;

namespace {
[[nodiscard]] std::string CollapseSlashes(std::string_view a_path) {
    std::string result;
    result.reserve(a_path.size());

    bool previousSlash = false;
    for (const auto ch : a_path) {
        if (ch == '\\') {
            if (previousSlash) {
                continue;
            }
            previousSlash = true;
        } else {
            previousSlash = false;
        }

        result.push_back(ch);
    }

    return result;
}

[[nodiscard]] bool WildcardMatches(std::string_view a_pattern, std::string_view a_value) noexcept {
    constexpr auto npos = std::string_view::npos;

    std::size_t patternIndex = 0;
    std::size_t valueIndex = 0;
    std::size_t starIndex = npos;
    std::size_t retryIndex = 0;

    while (valueIndex < a_value.size()) {
        if (patternIndex < a_pattern.size() && a_pattern[patternIndex] == '*') {
            starIndex = patternIndex++;
            retryIndex = valueIndex;
        } else if (patternIndex < a_pattern.size() && a_pattern[patternIndex] == a_value[valueIndex]) {
            ++patternIndex;
            ++valueIndex;
        } else if (starIndex != npos) {
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
    clib_util::string::replace_all(normalized, "/"sv, "\\"sv);
    normalized = CollapseSlashes(normalized);

    while (!normalized.empty() && normalized.front() == '\\') {
        normalized.erase(normalized.begin());
    }

    if (normalized.starts_with(R"(.\)"sv)) {
        normalized.erase(0, 2);
    }

    constexpr std::string_view meshesPrefix = R"(meshes\)";
    constexpr std::string_view meshesMarker = R"(\meshes\)";

    if (!normalized.starts_with(meshesPrefix)) {
        if (const auto pos = normalized.find(meshesMarker); pos != std::string::npos) {
            normalized.erase(0, pos + 1);
        }
    }

    if (!normalized.empty() && !normalized.starts_with(meshesPrefix) && !normalized.contains(':')) {
        normalized.insert(0, meshesPrefix);
    }

    return normalized;
}

ModelPattern::ModelPattern(std::string a_pattern)
    : pattern_(std::move(a_pattern))
    , hasWildcard_(pattern_.contains('*')) {}

const std::string& ModelPattern::GetPattern() const noexcept {
    return pattern_;
}

bool ModelPattern::HasWildcard() const noexcept {
    return hasWildcard_;
}

bool ModelPattern::Matches(std::string_view a_modelPath) const noexcept {
    return hasWildcard_ ? WildcardMatches(pattern_, a_modelPath) : pattern_ == a_modelPath;
}
