#include "PreviewMarkerMath.h"

#include <cmath>
#include <numbers>

namespace {
constexpr float DEGREES_PER_TURN = 360.0F;
constexpr float RADIANS_PER_DEGREE = std::numbers::pi_v<float> / 180.0F;
constexpr float MARKER_UNITS_PER_RADIAN = 1000.0F;
}

float NormalizeDegrees(const float a_degrees) noexcept {
    float normalized = std::fmod(a_degrees, DEGREES_PER_TURN);
    if (normalized < 0.0F) {
        normalized += DEGREES_PER_TURN;
    }

    return normalized;
}

std::uint16_t RotationDegreesToMarkerValue(const float a_degrees) noexcept {
    const float normalized = NormalizeDegrees(a_degrees);
    const auto markerUnits = static_cast<std::uint32_t>(
        std::round(normalized * RADIANS_PER_DEGREE * MARKER_UNITS_PER_RADIAN)
    );

    return static_cast<std::uint16_t>(markerUnits);
}
