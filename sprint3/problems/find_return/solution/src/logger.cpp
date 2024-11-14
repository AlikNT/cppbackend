#include "logger.h"

namespace server_logging {
using namespace std::literals;

void MyFormatter(const logging::record_view &rec, logging::formatting_ostream &strm) {
    // Извлекаем атрибуты
    auto timestamp = logging::extract<boost::posix_time::ptime>("TimeStamp"s, rec);
    auto message = logging::extract<std::string>("Message"s, rec);
    auto add_data = logging::extract<json::value>("AdditionalData"s, rec);

    // Формируем JSON-объект для лог-записи
    boost::json::object log_entry;

    // Добавляем атрибуты в JSON-объект
    log_entry["timestamp"s] = to_iso_extended_string(*timestamp);  // Время в формате ISO
    log_entry["message"s] = message ? *message : "Unknown"s;        // Сообщение
    if (add_data) {
        log_entry["data"s] = *add_data;  // Дополнительные данные (если есть)
    }

    // Выводим JSON в поток
    strm << boost::json::serialize(log_entry);
}

void InitLogging() {
    // Добавляем консольный логгер с кастомным форматтером
    logging::add_console_log(
            std::cout,
            keywords::format = &MyFormatter,
            keywords::auto_flush = true
    );

    // Добавляем стандартные атрибуты, такие как время записи
    logging::add_common_attributes();
}

void LogServerStart(int port, const std::string &address) {
    boost::json::value data = {
            {"port"s,    port},
            {"address"s, address}
    };
    BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, data)
                            << "server started"s;
}

void LogServerExit(int code, const std::string &exception) {
    boost::json::value data = {
            {"code"s, code},
            {"exception"s, exception}
    };
    BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, data)
                            << "server exited"s;
}

void LogServerError(int error_code, const std::string &error_message, const std::string &where) {
    json::value custom_data{
            {"code"s,  error_code},
            {"text"s,  error_message},
            {"where"s, where}
    };
    BOOST_LOG_TRIVIAL(error) << logging::add_value(additional_data, custom_data)
                             << "error"s;
}

} // namespace server_logging
