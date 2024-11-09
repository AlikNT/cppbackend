#include "extra_data.h"

const json::array & extra_data::ExtraDataStorage::GetLootTypes() const {
    return loot_types_;
}

void extra_data::ExtraDataStorage::AddLootTypesJson(json::array loot_types) {
    loot_types_ = std::move(loot_types);
}
