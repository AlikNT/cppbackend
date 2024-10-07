#include "logger.h"

namespace logger {

void MyFormatter(const logging::record_view &rec, logging::formatting_ostream &strm) {
    // Извлекаем атрибуты
    auto timestamp = logging::extract<boost::posix_time::ptime>("TimeStamp", rec);
    auto message = logging::extract<std::string>("Message", rec);
    auto add_data = logging::extract<json::value>("AdditionalData", rec);

    // Формируем JSON-объект для лог-записи
    boost::json::object log_entry;

    // Добавляем атрибуты в JSON-объект
    log_entry["timestamp"] = to_iso_extended_string(*timestamp);  // Время в формате ISO
    log_entry["message"] = message ? *message : "Unknown";        // Сообщение
    if (add_data) {
        log_entry["data"] = *add_data;  // Дополнительные данные (если есть)
    }

    // Выводим JSON в поток
    strm << boost::json::serialize(log_entry);
}

void InitLogging() {
    // Добавляем консольный логгер с кастомным форматтером
    logging::add_console_log(
            std::clog,
            keywords::format = &MyFormatter,
            keywords::auto_flush = true
    );

    // Добавляем стандартные атрибуты, такие как время записи
    logging::add_common_attributes();
}

void LogRequest(std::string_view ip, std::string_view uri, std::string_view method) {
    boost::json::value data = {
            {"ip",     ip},
            {"URI",    uri},
            {"method", method}
    };
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data)
                            << "request received";
}

void LogResponse(int response_time, int code, const std::string &content_type) {
    boost::json::value data = {
            {"response_time", response_time},
            {"code",          code},
            {"content_type",  content_type}
    };
    BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, data)
                            << "response sent";
}

void LogServerStart(int port, const std::string &address) {
    boost::json::value data = {
            {"port",    port},
            {"address", address}
    };
    BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, data)
                            << "server started";
}

void LogServerExit(int code, const std::string &exception) {
    boost::json::value data = {
            {"code", code},
            {"exception", exception}
    };
    BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, data)
                            << "server exited";
}

void LogServerError(int error_code, const std::string &error_message, const std::string &where) {
    json::value custom_data{
            {"code",  error_code},
            {"text",  error_message},
            {"where", where}
    };
    BOOST_LOG_TRIVIAL(error) << logging::add_value(additional_data, custom_data)
                             << "error";
}

} // namespace logger
