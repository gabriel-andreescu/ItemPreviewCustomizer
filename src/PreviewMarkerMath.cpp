#include "PreviewMarkerMath.h"

#include <RE/N/NiMatrix3.h>

#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>

namespace {
constexpr float kDegreesPerTurn = 360.0F;
constexpr float kRadiansPerDegree = std::numbers::pi_v<float> / 180.0F;
constexpr float kMarkerUnitsPerRadian = 1000.0F;
}

float NormalizeDegrees(const float a_degrees) noexcept {
    float normalized = std::fmod(a_degrees, kDegreesPerTurn);
    if (normalized < 0.0F) {
        normalized += kDegreesPerTurn;
    }

    return normalized;
}

std::uint16_t RotationDegreesToMarkerValue(const float a_degrees) noexcept {
    const float normalized = NormalizeDegrees(a_degrees);
    const auto markerUnits = static_cast<std::uint32_t>(
        std::round(normalized * kRadiansPerDegree * kMarkerUnitsPerRadian)
    );

    return static_cast<std::uint16_t>(markerUnits);
}

std::optional<PreviewRotation> RotationMatrixToDegrees(const RE::NiMatrix3& a_matrix) noexcept {
    const auto sineY = -a_matrix.entry[0][2];
    // At gimbal lock, X and Z cannot be recovered independently.
    if (std::abs(sineY) >= 1.0F) {
        return std::nullopt;
    }

    // CommonLib's NiFastATan2 has quadrant errors. Use the standard XYZ inverse.
    return PreviewRotation {
        .x = NormalizeDegrees(std::atan2(a_matrix.entry[1][2], a_matrix.entry[2][2]) / kRadiansPerDegree),
        .y = NormalizeDegrees(std::asin(sineY) / kRadiansPerDegree),
        .z = NormalizeDegrees(std::atan2(a_matrix.entry[0][1], a_matrix.entry[0][0]) / kRadiansPerDegree),
    };
}
