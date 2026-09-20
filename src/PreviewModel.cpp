#include "PCH.h" // IWYU pragma: keep

#include "PreviewModel.h"

#include <RE/B/BSTArray.h>
#include <RE/I/InterfaceLightSchemes.h>
#include <RE/I/Inventory3DManager.h>
#include <RE/N/NiMatrix3.h>
#include <RE/N/NiSmartPointer.h>
#include <REL/Module.h>
#include <REL/Relocation.h>

#include <cstddef>
#include <optional>

namespace InventoryPreview {
namespace {
    struct LoadedModelVR {
        RE::TESForm* item;
        RE::TESBoundObject* modelObject;
        RE::NiPointer<RE::NiAVObject> model;
        RE::INTERFACE_LIGHT_SCHEME lightScheme;
        float boundRadius;
        RE::NiMatrix3 rotation;
        float scale;
    };
    static_assert(sizeof(LoadedModelVR) == 0x48);
    static_assert(offsetof(LoadedModelVR, model) == 0x10);
    static_assert(offsetof(LoadedModelVR, rotation) == 0x20);
    static_assert(offsetof(LoadedModelVR, scale) == 0x44);
}

std::optional<PreviewModel> GetPreviewModel() {
    auto* manager = RE::Inventory3DManager::GetSingleton();
    if (manager == nullptr) {
        return std::nullopt;
    }

    if (REL::Module::IsVR()) {
        const auto& models = REL::RelocateMember<RE::BSTSmallArray<LoadedModelVR, 7>>(manager, 0x58, 0x58);
        if (models.empty()) {
            return std::nullopt;
        }

        // VR rotates the preview's parent node, independently of the loaded model.
        const auto* rotationNode = REL::RelocateMember<RE::NiAVObject*>(manager, 0x278, 0x278);
        const auto& model = models.back();
        return PreviewModel {.item = model.item, .modelObject = model.modelObject, .rotationNode = rotationNode};
    }

    const auto& models = manager->GetRuntimeData().loadedModels;
    if (models.empty()) {
        return std::nullopt;
    }
    const auto& model = models.back();
    return PreviewModel {.item = model.itemBase, .modelObject = model.modelObj, .rotationNode = model.spModel.get()};
}
}
