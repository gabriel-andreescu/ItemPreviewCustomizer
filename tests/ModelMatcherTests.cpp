#include "ModelMatcher.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("NormalizeModelPath returns normalized model paths", "[model-matcher]") {
    CHECK(
        NormalizeModelPath(" Meshes/LeftHandRingsSKSE//Rings/DummyRing_1.NIF ")
        == R"(meshes\lefthandringsskse\rings\dummyring_1.nif)"
    );
    CHECK(NormalizeModelPath(R"(\meshes\items\ring_go.nif)") == R"(meshes\items\ring_go.nif)");
    CHECK(NormalizeModelPath(R"(.\meshes\items\ring_go.nif)") == R"(meshes\items\ring_go.nif)");
    CHECK(
        NormalizeModelPath(R"(LeftHandRingsSKSE\Rings\DummyRing_1.nif)")
        == R"(meshes\lefthandringsskse\rings\dummyring_1.nif)"
    );
    CHECK(
        NormalizeModelPath(R"(C:\Games\Skyrim Special Edition\Data\Meshes\Items\Ring_GO.NIF)")
        == R"(meshes\items\ring_go.nif)"
    );
}

TEST_CASE("ModelPattern matches exact normalized paths", "[model-matcher]") {
    ModelPattern pattern {NormalizeModelPath(R"(meshes\items\ring_go.nif)")};

    CHECK_FALSE(pattern.HasWildcard());
    CHECK(pattern.Matches(NormalizeModelPath(R"(meshes/items/ring_go.nif)")));
    CHECK_FALSE(pattern.Matches(NormalizeModelPath(R"(meshes/items/amulet_go.nif)")));
}

TEST_CASE("ModelPattern supports star wildcards", "[model-matcher]") {
    ModelPattern pattern {NormalizeModelPath(R"(meshes/*/rings/dummyring_*.nif)")};

    CHECK(pattern.HasWildcard());
    CHECK(pattern.Matches(NormalizeModelPath(R"(meshes/lefthandringsskse/rings/dummyring_1.nif)")));
    CHECK(pattern.Matches(NormalizeModelPath(R"(lefthandringsskse/rings/dummyring_1.nif)")));
    CHECK_FALSE(pattern.Matches(NormalizeModelPath(R"(meshes/vanilla/rings/gold_go.nif)")));
}

TEST_CASE("ModelPattern treats non-star characters literally", "[model-matcher]") {
    ModelPattern pattern {NormalizeModelPath(R"(meshes/items/ring?.nif)")};

    CHECK_FALSE(pattern.HasWildcard());
    CHECK(pattern.Matches(NormalizeModelPath(R"(meshes/items/ring?.nif)")));
    CHECK_FALSE(pattern.Matches(NormalizeModelPath(R"(meshes/items/ring1.nif)")));
}
