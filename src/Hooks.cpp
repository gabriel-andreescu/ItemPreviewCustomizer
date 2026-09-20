#include "PCH.h" // IWYU pragma: keep

#include "Hooks.h"

#include "InventoryPreview.h"

#include <RE/I/Inventory3DManager.h>
#include <SKSE/SKSE.h>

#include <cstdint>

namespace Hooks {
namespace {
    struct ApplyInventoryMarkerSync {
        static void Thunk(
            RE::Inventory3DManager* a_manager,
            RE::TESBoundObject* a_item,
            RE::TESBoundObject* a_modelObject,
            RE::NiPointer<RE::NiAVObject>* a_model
        ) {
            InventoryPreview::ApplyInventoryMarkerWithOverrides(func, a_manager, a_item, a_modelObject, a_model);
        }

        static inline REL::Relocation<InventoryPreview::ApplyInventoryMarker> func;
    };

    struct ApplyInventoryMarkerDeferred {
        static void Thunk(
            RE::Inventory3DManager* a_manager,
            RE::TESBoundObject* a_item,
            RE::TESBoundObject* a_modelObject,
            RE::NiPointer<RE::NiAVObject>* a_model
        ) {
            InventoryPreview::ApplyInventoryMarkerWithOverrides(func, a_manager, a_item, a_modelObject, a_model);
        }

        static inline REL::Relocation<InventoryPreview::ApplyInventoryMarker> func;
    };
}

void Install() {
    // LoadInventoryItem applies the marker before displaying the model.
    REL::Relocation<std::uintptr_t> syncTarget {
        REL::VariantID(50885, 51758, 0x8B4ED0),
        REL::VariantOffset(0x281, 0x293, 0x282)
    };
    REL::Relocation<std::uintptr_t> deferredTarget {
        REL::VariantID(50900, 51776, 0x8B6220),
        REL::VariantOffset(0x1BA, 0x1C3, 0x1B0)
    };

    ApplyInventoryMarkerSync::func = syncTarget.write_call<5>(ApplyInventoryMarkerSync::Thunk);
    ApplyInventoryMarkerDeferred::func = deferredTarget.write_call<5>(ApplyInventoryMarkerDeferred::Thunk);

    SKSE::log::info("Inventory preview hooks installed");
}
}
