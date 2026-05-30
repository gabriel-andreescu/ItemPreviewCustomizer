#include "PreviewMarkerMath.h"

#include <cmath>

namespace {
constexpr float DEGREES_PER_TURN = 360.0F;
constexpr float MARKER_UNITS_PER_TURN = 6553.6F;
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
    auto markerUnits = static_cast<std::uint32_t>(
        std::floor((normalized * (MARKER_UNITS_PER_TURN / DEGREES_PER_TURN)) + 0.5F)
    );

    if (static_cast<float>(markerUnits) >= MARKER_UNITS_PER_TURN) {
        markerUnits = 0;
    }

    return static_cast<std::uint16_t>(markerUnits);
}
