#include "extra_data.h"

void extra_data::ExtraDataStorage::AddLootTypesJson(json::array loot_types) {
    loot_types_ = std::move(loot_types);
}
