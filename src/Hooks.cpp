#include "Hooks.h"

#include "InventoryPreview.h"

namespace Hooks {
namespace {
    struct ApplyInventoryMarkerSync {
        static void thunk(
            RE::Inventory3DManager* a_manager,
            RE::TESBoundObject* a_item,
            RE::TESBoundObject* a_modelObject,
            RE::NiPointer<RE::NiAVObject>* a_model
        ) {
            InventoryPreview::ApplyInventoryMarkerWithOverrides(func, a_manager, a_item, a_modelObject, a_model);
        }

        static inline REL::Relocation<InventoryPreview::ApplyInventoryMarker_t> func;
    };

    struct ApplyInventoryMarkerDeferred {
        static void thunk(
            RE::Inventory3DManager* a_manager,
            RE::TESBoundObject* a_item,
            RE::TESBoundObject* a_modelObject,
            RE::NiPointer<RE::NiAVObject>* a_model
        ) {
            InventoryPreview::ApplyInventoryMarkerWithOverrides(func, a_manager, a_item, a_modelObject, a_model);
        }

        static inline REL::Relocation<InventoryPreview::ApplyInventoryMarker_t> func;
    };
}

void Install() {
#ifndef __clang_analyzer__
    SKSE::AllocTrampoline(14 * 2);

    // RE: Inventory3DManager::LoadInventoryItem.
    REL::Relocation<std::uintptr_t> syncTarget {
        REL::VariantID(50885, 51758, 0x8B4ED0),
        REL::VariantOffset(0x281, 0x293, 0x282)
    };
    REL::Relocation<std::uintptr_t> deferredTarget {
        REL::VariantID(50885, 51758, 0x8B4ED0),
        REL::VariantOffset(0xFFA, 0x10F3, 0x1500)
    };

    stl::write_thunk_call<ApplyInventoryMarkerSync>(syncTarget);
    stl::write_thunk_call<ApplyInventoryMarkerDeferred>(deferredTarget);
#endif

    logger::info("Hooks: inventory preview hooks installed");
}
}
