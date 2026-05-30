#pragma once

#include <cstdint>

[[nodiscard]] float NormalizeDegrees(float a_degrees) noexcept;
[[nodiscard]] std::uint16_t RotationDegreesToMarkerValue(float a_degrees) noexcept;
