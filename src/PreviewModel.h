#pragma once

#include <optional>

namespace RE {
class NiAVObject;
class TESBoundObject;
class TESForm;
}

namespace InventoryPreview {
struct PreviewModel {
    RE::TESForm* item;
    RE::TESBoundObject* modelObject;
    const RE::NiAVObject* rotationNode;
};

[[nodiscard]] std::optional<PreviewModel> GetPreviewModel();
}
