#include "PreviewMarkerMath.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

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
