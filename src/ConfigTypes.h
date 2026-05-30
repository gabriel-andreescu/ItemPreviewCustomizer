#pragma once

#include <glaze/glaze.hpp>

#include <optional>
#include <string>
#include <vector>

struct RotationOverride {
    std::optional<float> x;
    std::optional<float> y;
    std::optional<float> z;

    [[nodiscard]] bool HasValues() const noexcept {
        return x.has_value() || y.has_value() || z.has_value();
    }

    void Overlay(const RotationOverride& a_other) noexcept {
        if (a_other.x.has_value()) {
            x = a_other.x;
        }
        if (a_other.y.has_value()) {
            y = a_other.y;
        }
        if (a_other.z.has_value()) {
            z = a_other.z;
        }
    }
};

struct PreviewConfig {
    std::optional<float> zoom;
    RotationOverride rotation;

    [[nodiscard]] bool HasValues() const noexcept {
        return zoom.has_value() || rotation.HasValues();
    }
};

struct ConfigEntry {
    std::vector<std::string> models;
    std::optional<float> zoom;
    RotationOverride rotation;
    RotationOverride rotationDegrees;

    [[nodiscard]] PreviewConfig GetPreviewConfig() const noexcept {
        PreviewConfig config;
        config.zoom = zoom;
        config.rotation = rotation;
        config.rotation.Overlay(rotationDegrees);
        return config;
    }
};

template <>
struct glz::meta<RotationOverride> {
    using T = RotationOverride;
    static constexpr auto value = object("x", &T::x, "y", &T::y, "z", &T::z);
};

template <>
struct glz::meta<ConfigEntry> {
    using T = ConfigEntry;
    static constexpr auto value = object(
        "models",
        &T::models,
        "zoom",
        &T::zoom,
        "rotation",
        &T::rotation,
        "rotationDegrees",
        &T::rotationDegrees
    );
};
