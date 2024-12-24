#pragma once

#include <chrono>
#include <memory>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "app.h"
#include "model.h"
#include "model_serialization.h"

using namespace std::literals;
using milliseconds = std::chrono::milliseconds;

namespace infrastructure {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

class SerializingListener {
public:
    explicit SerializingListener(app::Application& app, milliseconds period, const std::string& save_file);

    void Save(milliseconds delta);
    void Save();
    void Load() const;

private:
    milliseconds time_since_save_ = 0ms;
    app::Application& app_;
    milliseconds save_period_;

    std::string save_file_;
    std::string temp_file_;

    void UpdateTime(std::chrono::milliseconds delta);
};
} // namespace infrastructure

