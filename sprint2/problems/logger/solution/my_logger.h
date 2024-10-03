#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>
#include <iostream>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    auto GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << "/var/log/sample_log_" << std::put_time(std::localtime(&t_c), "%Y_%m_%d") << ".log";
        return oss.str();
    }

    // Открывает файл для логирования с проверкой на смену даты
    void OpenLogFile() {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto new_log_file = GetFileTimeStamp();
        if (current_log_file_ != new_log_file) {
            log_file_.close();
            log_file_.open(new_log_file, std::ios::app);
            current_log_file_ = new_log_file;
        }
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args) {
        OpenLogFile(); // Проверяем, не нужно ли сменить файл
        std::lock_guard<std::mutex> lock(mutex_);

        // Запись временной метки и символа ':'
        log_file_ << GetTimeStamp() << ": ";

        // Запись всех аргументов
        (log_file_ << ... << args) << std::endl;
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            manual_ts_ = ts;
        }
        OpenLogFile(); // Обновляем лог-файл при смене времени
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    // Мьютекс для синхронизации доступа
    mutable std::mutex mutex_;

    // Текущий файл для записи логов
    std::ofstream log_file_;
    std::string current_log_file_;
};
