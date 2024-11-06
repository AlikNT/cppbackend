#pragma once

#include <boost/json.hpp>

namespace json = boost::json;

namespace extra_data {

class ExtraDataStorage {
public:
   void AddLootTypesJson(json::array loot_types);
private:
    json::array loot_types_;
};

} // namespace extra_data