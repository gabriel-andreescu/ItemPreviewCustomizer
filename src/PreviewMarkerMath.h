#pragma once

#include <cstdint>
#include <optional>

namespace RE {
class NiMatrix3;
}

struct PreviewRotation {
    float x;
    float y;
    float z;
};

[[nodiscard]] float NormalizeDegrees(float a_degrees) noexcept;
[[nodiscard]] std::uint16_t RotationDegreesToMarkerValue(float a_degrees) noexcept;
[[nodiscard]] std::optional<PreviewRotation> RotationMatrixToDegrees(const RE::NiMatrix3& a_matrix) noexcept;
