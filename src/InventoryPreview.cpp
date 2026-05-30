#include "InventoryPreview.h"

#include "ConfigManager.h"
#include "ModelMatcher.h"
#include "PreviewMarkerMath.h"

#include "RE/B/BSInvMarker.h"
#include "RE/E/EffectSetting.h"
#include "RE/I/InventoryEntryData.h"
#include "RE/I/InventoryMenu.h"
#include "RE/I/ItemList.h"
#include "RE/S/SpellItem.h"
#include "RE/T/TESBipedModelForm.h"
#include "RE/T/TESBoundObject.h"
#include "RE/T/TESForm.h"
#include "RE/T/TESModel.h"
#include "RE/T/TESObjectARMO.h"
#include "RE/T/TESObjectBOOK.h"
#include "RE/T/TESObjectWEAP.h"
#include "RE/T/TESShout.h"
#include "RE/U/UI.h"

#include <string>

namespace InventoryPreview {
namespace {
    [[nodiscard]] std::string GetModelPath(const RE::TESModel& a_model) {
        const auto* model = a_model.GetModel();
        return model ? std::string {model} : std::string {};
    }

    [[nodiscard]] std::string GetArmorModelPath(const RE::TESObjectARMO& a_armor) {
        for (const auto& worldModel : a_armor.worldModels) {
            auto path = GetModelPath(worldModel);
            if (!path.empty()) {
                return path;
            }
        }

        return {};
    }

    [[nodiscard]] std::string GetModelPath(const RE::TESBoundObject* a_object) {
        if (!a_object) {
            return {};
        }

        if (const auto* armor = a_object->As<RE::TESObjectARMO>()) {
            if (auto path = GetArmorModelPath(*armor); !path.empty()) {
                return path;
            }
        }

        if (const auto* model = a_object->As<RE::TESModel>()) {
            if (auto path = GetModelPath(*model); !path.empty()) {
                return path;
            }
        }

        return {};
    }

    [[nodiscard]] const RE::TESBoundObject* GetMenuDisplayObject(const RE::TESForm& a_item) {
        switch (a_item.GetFormType()) {
            case RE::FormType::MagicEffect:
                if (const auto* effect = a_item.As<RE::EffectSetting>()) {
                    return effect->GetMenuDisplayObject();
                }
                break;
            case RE::FormType::Spell:
                if (const auto* spell = a_item.As<RE::SpellItem>()) {
                    return spell->GetMenuDisplayObject();
                }
                break;
            case RE::FormType::Shout:
                if (const auto* shout = a_item.As<RE::TESShout>()) {
                    return shout->GetMenuDisplayObject();
                }
                break;
            default: break;
        }

        return nullptr;
    }

    [[nodiscard]] const RE::TESBoundObject* GetInventoryPreviewModelObject(const RE::TESForm* a_item) {
        if (!a_item) {
            return nullptr;
        }

        switch (a_item->GetFormType()) {
            case RE::FormType::MagicEffect:
            case RE::FormType::Spell:
            case RE::FormType::Shout:       return GetMenuDisplayObject(*a_item);
            case RE::FormType::Weapon:
                if (const auto* weapon = a_item->As<RE::TESObjectWEAP>()) {
                    return weapon->firstPersonModelObject;
                }
                break;
            case RE::FormType::Book:
                if (const auto* book = a_item->As<RE::TESObjectBOOK>()) {
                    return book->inventoryModel;
                }
                break;
            default: return a_item->As<RE::TESBoundObject>();
        }

        return nullptr;
    }

    [[nodiscard]] std::string ResolveModelPath(
        const RE::TESBoundObject* a_modelObject,
        const RE::TESBoundObject* a_item
    ) {
        if (auto path = GetModelPath(a_modelObject); !path.empty()) {
            return path;
        }

        if (a_modelObject != a_item) {
            return GetModelPath(GetInventoryPreviewModelObject(a_item));
        }

        return {};
    }

    [[nodiscard]] RE::InventoryMenu* GetOpenInventoryMenu() {
        auto* ui = RE::UI::GetSingleton();
        if (!ui) {
            return nullptr;
        }

        auto inventoryMenu = ui->GetMenu<RE::InventoryMenu>();
        return inventoryMenu.get();
    }

    [[nodiscard]] RE::TESBoundObject* GetSelectedInventoryObject() {
        auto* inventoryMenu = GetOpenInventoryMenu();
        auto* itemList = inventoryMenu ? inventoryMenu->GetRuntimeData().itemList : nullptr;
        auto* selectedItem = itemList ? itemList->GetSelectedItem() : nullptr;
        auto* entry = selectedItem ? selectedItem->data.objDesc : nullptr;
        return entry ? entry->object : nullptr;
    }

    [[nodiscard]] RE::BSInvMarker* FindInventoryMarker(const RE::NiAVObject& a_modelRoot) {
        static const RE::BSFixedString invMarkerName {"INV"};
        if (auto* namedMarker = netimmerse_cast<RE::BSInvMarker*>(a_modelRoot.GetExtraData(invMarkerName))) {
            return namedMarker;
        }

        for (std::uint16_t i = 0; i < a_modelRoot.GetExtraDataSize(); ++i) {
            if (auto* marker = netimmerse_cast<RE::BSInvMarker*>(a_modelRoot.GetExtraDataAt(i))) {
                return marker;
            }
        }

        return nullptr;
    }

    class MarkerOverrideScope {
    public:
        explicit MarkerOverrideScope(RE::BSInvMarker& a_marker)
            : marker_(a_marker)
            , original_(
                  MarkerSnapshot {
                      .zoom = a_marker.zoom,
                      .rotationX = a_marker.rotationX,
                      .rotationY = a_marker.rotationY,
                      .rotationZ = a_marker.rotationZ,
                  }
              ) {}

        MarkerOverrideScope(const MarkerOverrideScope&) = delete;
        MarkerOverrideScope(MarkerOverrideScope&&) = delete;
        MarkerOverrideScope& operator=(const MarkerOverrideScope&) = delete;
        MarkerOverrideScope& operator=(MarkerOverrideScope&&) = delete;

        ~MarkerOverrideScope() {
            marker_.zoom = original_.zoom;
            marker_.rotationX = original_.rotationX;
            marker_.rotationY = original_.rotationY;
            marker_.rotationZ = original_.rotationZ;
        }

        void Apply(const PreviewConfig& a_config) const {
            if (a_config.zoom.has_value()) {
                marker_.zoom = *a_config.zoom;
            }

            if (a_config.rotation.x.has_value()) {
                marker_.rotationX = RotationDegreesToMarkerValue(*a_config.rotation.x);
            }
            if (a_config.rotation.y.has_value()) {
                marker_.rotationY = RotationDegreesToMarkerValue(*a_config.rotation.y);
            }
            if (a_config.rotation.z.has_value()) {
                marker_.rotationZ = RotationDegreesToMarkerValue(*a_config.rotation.z);
            }
        }

    private:
        struct MarkerSnapshot {
            float zoom;
            std::uint16_t rotationX;
            std::uint16_t rotationY;
            std::uint16_t rotationZ;
        };

        RE::BSInvMarker& marker_;
        MarkerSnapshot original_;
    };
}

void ApplyInventoryMarkerWithOverrides(
    const REL::Relocation<ApplyInventoryMarker_t>& a_original,
    RE::Inventory3DManager* a_manager,
    RE::TESBoundObject* a_item,
    RE::TESBoundObject* a_modelObject,
    RE::NiPointer<RE::NiAVObject>* a_model
) {
    auto* modelRoot = a_model ? a_model->get() : nullptr;
    if (!modelRoot) {
        a_original(a_manager, a_item, a_modelObject, a_model);
        return;
    }

    const auto modelPath = ResolveModelPath(a_modelObject, a_item);
    if (modelPath.empty()) {
        a_original(a_manager, a_item, a_modelObject, a_model);
        return;
    }

    const auto config = ConfigManager::GetSingleton()->GetConfig(modelPath);
    if (!config.has_value()) {
        a_original(a_manager, a_item, a_modelObject, a_model);
        return;
    }

    auto* marker = FindInventoryMarker(*modelRoot);
    if (!marker) {
        a_original(a_manager, a_item, a_modelObject, a_model);
        return;
    }

    MarkerOverrideScope markerOverride {*marker};
    markerOverride.Apply(*config);

    logger::debug(
        "Applied preview marker override | model={} | zoom={} | rotationX={} | rotationY={} | rotationZ={}",
        modelPath,
        config->zoom.has_value(),
        config->rotation.x.has_value(),
        config->rotation.y.has_value(),
        config->rotation.z.has_value()
    );
    a_original(a_manager, a_item, a_modelObject, a_model);
}

std::optional<std::string> GetSelectedInventoryModelPath() {
    auto path = NormalizeModelPath(GetModelPath(GetInventoryPreviewModelObject(GetSelectedInventoryObject())));
    if (path.empty()) {
        return std::nullopt;
    }

    return path;
}
}
