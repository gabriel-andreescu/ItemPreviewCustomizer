#include "ConfigManager.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>

namespace {
class TempDir {
public:
    TempDir() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() / ("ipc-tests-" + std::to_string(stamp));
        std::filesystem::create_directories(path_);
    }

    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    [[nodiscard]] const std::filesystem::path& Path() const noexcept {
        return path_;
    }

private:
    std::filesystem::path path_;
};

struct NullLogger {
    NullLogger() {
        auto logger = std::make_shared<spdlog::logger>("ipc-tests", std::make_shared<spdlog::sinks::null_sink_mt>());
        spdlog::set_default_logger(std::move(logger));
        spdlog::set_level(spdlog::level::off);
    }
};

const NullLogger nullLogger;

void WriteFile(const std::filesystem::path& a_path, std::string_view a_contents) {
    std::filesystem::create_directories(a_path.parent_path());

    std::ofstream file {a_path};
    REQUIRE(file.is_open());
    file << a_contents;
    REQUIRE(file.good());
}

float GetZoom(std::string_view a_modelPath) {
    const auto config = ConfigManager::GetSingleton()->GetConfig(a_modelPath);
    REQUIRE(config.has_value());
    REQUIRE(config->zoom.has_value());
    return *config->zoom;
}

RotationOverride GetRotation(std::string_view a_modelPath) {
    const auto config = ConfigManager::GetSingleton()->GetConfig(a_modelPath);
    REQUIRE(config.has_value());
    REQUIRE(config->rotation.HasValues());
    return config->rotation;
}
}

TEST_CASE("ConfigManager handles missing and empty folders", "[config-manager]") {
    TempDir tempDir;
    const auto missingFolder = tempDir.Path() / "missing";

    auto* manager = ConfigManager::GetSingleton();

    CHECK_FALSE(manager->Load(missingFolder));
    CHECK(manager->GetConfigCount() == 0);

    const auto emptyFolder = tempDir.Path() / "empty";
    std::filesystem::create_directories(emptyFolder);

    CHECK_FALSE(manager->Load(emptyFolder));
    CHECK(manager->GetConfigCount() == 0);
}

TEST_CASE("ConfigManager loads exact model configs", "[config-manager]") {
    TempDir tempDir;
    WriteFile(
        tempDir.Path() / "exact.json",
        R"([
          {
            "models": ["meshes/LeftHandRingsSKSE/Rings/DummyRing_1.nif"],
            "zoom": 0.25,
            "unknownFutureField": "ignored"
          }
        ])"
    );

    auto* manager = ConfigManager::GetSingleton();

    REQUIRE(manager->Load(tempDir.Path()));
    CHECK(manager->GetConfigCount() == 1);
    CHECK(GetZoom(R"(meshes\LeftHandRingsSKSE\Rings\DummyRing_1.nif)") == Catch::Approx(0.25F));
}

TEST_CASE("ConfigManager loads wildcard model configs", "[config-manager]") {
    TempDir tempDir;
    WriteFile(
        tempDir.Path() / "wildcard.json",
        R"([
          {
            "models": ["meshes/LeftHandRingsSKSE/Rings/DummyRing_*.nif"],
            "zoom": 0.5
          }
        ])"
    );

    auto* manager = ConfigManager::GetSingleton();

    REQUIRE(manager->Load(tempDir.Path()));
    CHECK(GetZoom(R"(meshes/LeftHandRingsSKSE/Rings/DummyRing_1.nif)") == Catch::Approx(0.5F));
    CHECK(GetZoom(R"(LeftHandRingsSKSE/Rings/DummyRing_1.nif)") == Catch::Approx(0.5F));
    CHECK_FALSE(manager->GetConfig(R"(meshes/LeftHandRingsSKSE/Rings/GoldRing_1.nif)").has_value());
}

TEST_CASE("ConfigManager loads rotation overrides", "[config-manager]") {
    TempDir tempDir;
    WriteFile(
        tempDir.Path() / "rotation.json",
        R"([
          {
            "models": ["meshes/items/ring_go.nif"],
            "rotation": {
              "x": 15.0,
              "z": -90.0
            }
          }
        ])"
    );

    auto* manager = ConfigManager::GetSingleton();

    REQUIRE(manager->Load(tempDir.Path()));
    CHECK(manager->GetConfigCount() == 1);

    const auto rotation = GetRotation(R"(meshes/items/ring_go.nif)");
    REQUIRE(rotation.x.has_value());
    CHECK(*rotation.x == Catch::Approx(15.0F));
    CHECK_FALSE(rotation.y.has_value());
    REQUIRE(rotation.z.has_value());
    CHECK(*rotation.z == Catch::Approx(-90.0F));
}

TEST_CASE("ConfigManager lets rotationDegrees alias overlay rotation", "[config-manager]") {
    TempDir tempDir;
    WriteFile(
        tempDir.Path() / "rotation.json",
        R"([
          {
            "models": ["meshes/items/ring_go.nif"],
            "rotation": {
              "x": 15.0,
              "y": 30.0
            },
            "rotationDegrees": {
              "y": 45.0,
              "z": 90.0
            }
          }
        ])"
    );

    auto* manager = ConfigManager::GetSingleton();

    REQUIRE(manager->Load(tempDir.Path()));

    const auto rotation = GetRotation(R"(meshes/items/ring_go.nif)");
    REQUIRE(rotation.x.has_value());
    CHECK(*rotation.x == Catch::Approx(15.0F));
    REQUIRE(rotation.y.has_value());
    CHECK(*rotation.y == Catch::Approx(45.0F));
    REQUIRE(rotation.z.has_value());
    CHECK(*rotation.z == Catch::Approx(90.0F));
}

TEST_CASE("ConfigManager keeps loading after invalid JSON files", "[config-manager]") {
    TempDir tempDir;
    WriteFile(tempDir.Path() / "a_invalid.json", R"([{)");
    WriteFile(
        tempDir.Path() / "b_valid.json",
        R"([
          {
            "models": ["meshes/items/ring_go.nif"],
            "zoom": 0.75
          }
        ])"
    );

    auto* manager = ConfigManager::GetSingleton();

    REQUIRE(manager->Load(tempDir.Path()));
    CHECK(manager->GetConfigCount() == 1);
    CHECK(GetZoom(R"(meshes/items/ring_go.nif)") == Catch::Approx(0.75F));
}

TEST_CASE("ConfigManager skips no-op config entries", "[config-manager]") {
    TempDir tempDir;
    WriteFile(
        tempDir.Path() / "noop.json",
        R"([
          {
            "models": ["meshes/items/ring_go.nif"]
          }
        ])"
    );

    auto* manager = ConfigManager::GetSingleton();

    CHECK_FALSE(manager->Load(tempDir.Path()));
    CHECK(manager->GetConfigCount() == 0);
    CHECK_FALSE(manager->GetConfig(R"(meshes/items/ring_go.nif)").has_value());
}

TEST_CASE("ConfigManager ignores invalid preview fields and keeps valid fields", "[config-manager]") {
    TempDir tempDir;
    WriteFile(
        tempDir.Path() / "invalid-fields.json",
        R"([
          {
            "models": ["meshes/items/ring_go.nif"],
            "zoom": -1.0,
            "rotation": {
              "x": 35.0
            }
          }
        ])"
    );

    auto* manager = ConfigManager::GetSingleton();

    REQUIRE(manager->Load(tempDir.Path()));
    CHECK(manager->GetConfigCount() == 1);

    const auto config = manager->GetConfig(R"(meshes/items/ring_go.nif)");
    REQUIRE(config.has_value());
    CHECK_FALSE(config->zoom.has_value());
    REQUIRE(config->rotation.x.has_value());
    CHECK(*config->rotation.x == Catch::Approx(35.0F));
}

TEST_CASE("ConfigManager resolves duplicate exact configs by sorted file order", "[config-manager]") {
    TempDir tempDir;
    WriteFile(
        tempDir.Path() / "a_first.json",
        R"([
          {
            "models": ["meshes/items/ring_go.nif"],
            "zoom": 0.25
          }
        ])"
    );
    WriteFile(
        tempDir.Path() / "z_second.json",
        R"([
          {
            "models": ["meshes/items/ring_go.nif"],
            "zoom": 0.9
          }
        ])"
    );

    auto* manager = ConfigManager::GetSingleton();

    REQUIRE(manager->Load(tempDir.Path()));
    CHECK(manager->GetConfigCount() == 1);
    CHECK(GetZoom(R"(meshes/items/ring_go.nif)") == Catch::Approx(0.9F));
}

TEST_CASE("ConfigManager prefers exact configs before wildcard configs", "[config-manager]") {
    TempDir tempDir;
    WriteFile(
        tempDir.Path() / "a_exact.json",
        R"([
          {
            "models": ["meshes/items/ring_go.nif"],
            "zoom": 1.0
          }
        ])"
    );
    WriteFile(
        tempDir.Path() / "z_wildcard.json",
        R"([
          {
            "models": ["meshes/items/*_go.nif"],
            "zoom": 0.25
          }
        ])"
    );

    auto* manager = ConfigManager::GetSingleton();

    REQUIRE(manager->Load(tempDir.Path()));
    CHECK(manager->GetConfigCount() == 2);
    CHECK(GetZoom(R"(meshes/items/ring_go.nif)") == Catch::Approx(1.0F));
}

TEST_CASE("ConfigManager resolves wildcard conflicts by later loaded match", "[config-manager]") {
    TempDir tempDir;
    WriteFile(
        tempDir.Path() / "a_first.json",
        R"([
          {
            "models": ["meshes/items/*_go.nif"],
            "zoom": 0.25
          }
        ])"
    );
    WriteFile(
        tempDir.Path() / "z_second.json",
        R"([
          {
            "models": ["meshes/items/ring*_go.nif"],
            "zoom": 0.6
          }
        ])"
    );

    auto* manager = ConfigManager::GetSingleton();

    REQUIRE(manager->Load(tempDir.Path()));
    CHECK(manager->GetConfigCount() == 2);
    CHECK(GetZoom(R"(meshes/items/ring01_go.nif)") == Catch::Approx(0.6F));
}
