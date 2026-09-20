#include "PreviewMarkerMath.h"

#include <RE/N/NiMatrix3.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <numbers>
#include <optional>

TEST_CASE("NormalizeDegrees wraps angles into one positive turn", "[preview-marker-math]") {
    CHECK(NormalizeDegrees(0.0F) == Catch::Approx(0.0F));
    CHECK(NormalizeDegrees(360.0F) == Catch::Approx(0.0F));
    CHECK(NormalizeDegrees(450.0F) == Catch::Approx(90.0F));
    CHECK(NormalizeDegrees(-90.0F) == Catch::Approx(270.0F));
}

TEST_CASE("RotationDegreesToMarkerValue follows inventory preview runtime conversion", "[preview-marker-math]") {
    CHECK(RotationDegreesToMarkerValue(0.0F) == 0);
    CHECK(RotationDegreesToMarkerValue(45.0F) == 785);
    CHECK(RotationDegreesToMarkerValue(90.0F) == 1571);
    CHECK(RotationDegreesToMarkerValue(180.0F) == 3142);
    CHECK(RotationDegreesToMarkerValue(-90.0F) == 4712);
    CHECK(RotationDegreesToMarkerValue(360.0F) == 0);
}

TEST_CASE("Copied rotations reconstruct preview matrices across quadrants", "[preview-marker-math]") {
    constexpr auto kRadiansPerDegree = std::numbers::pi_v<float> / 180.0F;
    constexpr std::array kRotations {
        PreviewRotation {.x = 45.0F, .y = 30.0F, .z = 15.0F},
        PreviewRotation {.x = 45.0F, .y = 30.0F, .z = 75.0F},
        PreviewRotation {.x = 90.0F, .y = 0.0F, .z = 0.0F},
        PreviewRotation {.x = 270.0F, .y = 0.0F, .z = 0.0F},
        PreviewRotation {.x = -135.0F, .y = -30.0F, .z = 120.0F},
        PreviewRotation {.x = 10.0F, .y = 150.0F, .z = 225.0F},
        PreviewRotation {.x = 180.0F, .y = 0.0F, .z = 180.0F},
        PreviewRotation {.x = 315.0F, .y = 45.0F, .z = 300.0F},
    };

    for (const auto& rotation : kRotations) {
        CAPTURE(rotation.x, rotation.y, rotation.z);
        RE::NiMatrix3 original;
        original.SetEulerAnglesXYZ(
            rotation.x * kRadiansPerDegree,
            rotation.y * kRadiansPerDegree,
            rotation.z * kRadiansPerDegree
        );
        const auto copied = RotationMatrixToDegrees(original);
        REQUIRE(copied.has_value());
        RE::NiMatrix3 restored;
        restored.SetEulerAnglesXYZ(
            copied->x * kRadiansPerDegree,
            copied->y * kRadiansPerDegree,
            copied->z * kRadiansPerDegree
        );
        for (std::size_t row = 0; row < 3; ++row) {
            for (std::size_t column = 0; column < 3; ++column) {
                CHECK(restored.entry[row][column] == Catch::Approx(original.entry[row][column]).margin(0.00001F));
            }
        }
    }
}

TEST_CASE("Rotation extraction reports gimbal lock", "[preview-marker-math]") {
    RE::NiMatrix3 matrix;
    matrix.SetEulerAnglesXYZ(0.0F, std::numbers::pi_v<float> / 2.0F, 0.0F);
    CHECK_FALSE(RotationMatrixToDegrees(matrix).has_value());
}
