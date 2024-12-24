#include "infrastructure.h"

infrastructure::SerializingListener::SerializingListener(app::Application &app, const milliseconds period,
    const std::string &save_file): app_(app), save_period_(period), save_file_(save_file), temp_file_(save_file + ".tmp") {}

void infrastructure::SerializingListener::Save(milliseconds delta) {
    if (time_since_save_ < save_period_) {
        UpdateTime(delta);
        return;
    }
    Save();
}

void infrastructure::SerializingListener::Save() {
    try {
        // Открываем временный файл для записи
        std::ofstream ofs(temp_file_);
        if (!ofs.is_open()) {
            throw std::ios_base::failure("Failed to open temporary file for saving");
        }

        OutputArchive oa(ofs);
        oa << serialization::GameSessionsRepr(app_.GetGame());
        oa << serialization::TokenToDogRepr(app_.GetDogTokens());
        ofs.close();

        // Заменяем основной файл временным
        std::filesystem::rename(temp_file_, save_file_);

        time_since_save_ = 0ms;
    } catch (const std::exception& ex) {
        std::filesystem::remove(temp_file_);
        throw ex;
    }
}

void infrastructure::SerializingListener::Load() const {
    // Проверяем, существует ли файл
    if (!std::filesystem::exists(save_file_)) {
        return;
    }

    // Открываем файл для чтения
    std::ifstream ifs(save_file_);
    if (!ifs.is_open()) {
        throw std::ios_base::failure("Failed to open save file for loading");
    }

    InputArchive ia(ifs);
    serialization::GameSessionsRepr sessions_repr;
    ia >> sessions_repr;
    app_.SetSessions(sessions_repr.Restore(app_.GetGame()));
    serialization::TokenToDogRepr token_to_dog;
    ia >> token_to_dog;
    app_.SetTokenToDog(token_to_dog.RestoreTokenToDog());
    app_.SetTokenToSession(token_to_dog.RestoreTokenToSession(app_.GetGame()));
}

void infrastructure::SerializingListener::UpdateTime(const std::chrono::milliseconds delta) {
    time_since_save_ += delta;
}
