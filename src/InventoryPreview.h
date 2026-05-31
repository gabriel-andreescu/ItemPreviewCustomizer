#pragma once

#include <optional>
#include <string>

namespace RE {
class Inventory3DManager;
class NiAVObject;
class TESBoundObject;
template <class T>
class NiPointer;
}

namespace REL {
template <class T>
class Relocation;
}

namespace InventoryPreview {
struct PreviewRotation {
    float x;
    float y;
    float z;
};

struct CurrentInventoryPreview {
    std::string modelPath;
    std::optional<PreviewRotation> rotation;
};

using ApplyInventoryMarker_t = void(
    RE::Inventory3DManager*,
    RE::TESBoundObject*,
    RE::TESBoundObject*,
    RE::NiPointer<RE::NiAVObject>*
);

void ApplyInventoryMarkerWithOverrides(
    const REL::Relocation<ApplyInventoryMarker_t>& a_original,
    RE::Inventory3DManager* a_manager,
    RE::TESBoundObject* a_item,
    RE::TESBoundObject* a_modelObject,
    RE::NiPointer<RE::NiAVObject>* a_model
);

[[nodiscard]] std::optional<CurrentInventoryPreview> GetCurrentInventoryPreview();
}
