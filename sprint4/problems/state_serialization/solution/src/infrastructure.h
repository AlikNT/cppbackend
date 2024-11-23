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
    explicit SerializingListener(const app::Application& app, const milliseconds period, const std::string& save_file)
        : app_(app), save_period_(period), save_file_(save_file), temp_file_(save_file + ".tmp") {}

    void Save(milliseconds delta) {
        if (time_since_save_ < save_period_) {
            UpdateTime(delta);
            return;
        }
        try {
            // Открываем временный файл для записи
            std::ofstream ofs(temp_file_);
            if (!ofs.is_open()) {
                throw std::ios_base::failure("Failed to open temporary file for saving");
            }

            OutputArchive oa(ofs);
            serialization::GameSessionsRepr sessions_repr(app_.GetGame());
            oa << sessions_repr;
            serialization::PlayersRepr players_repr(app_.GetPlayers());
            oa << players_repr;
            serialization::PlayerTokensRepr tokens_repr(app_.GetPlayerTokens());
            oa << tokens_repr;
            ofs.close();

            // Заменяем основной файл временным
            std::filesystem::rename(temp_file_, save_file_);

            time_since_save_ = 0ms;
        } catch (const std::exception& ex) {
            std::cerr << "Error during saving: " << ex.what() << '\n';
            std::filesystem::remove(temp_file_);
        }
    }

private:
    milliseconds time_since_save_ = 0ms;
    const app::Application& app_;
    milliseconds save_period_;

    std::string save_file_;
    std::string temp_file_;

    void UpdateTime(const std::chrono::milliseconds delta) {
        time_since_save_ += delta;
    }
};
} // namespace infrastructure

