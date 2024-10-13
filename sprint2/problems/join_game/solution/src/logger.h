#pragma once
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/json.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>

namespace server_logging {

// Определяем ключевое слово для добавления произвольных данных в лог
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value)

namespace beast = boost::beast;
namespace http = beast::http;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace json = boost::json;
namespace net = boost::asio;
using tcp = net::ip::tcp;

struct LogData {
    int status_code;
    std::string content_type;
    int response_time_ms;
};

template<class RequestHandler>
class LoggingRequestHandler {
    static void LogRequest(std::string_view client_ip, std::string_view uri, std::string_view method) {
        boost::json::value data = {
                {"ip",     client_ip},
                {"URI",    uri},
                {"method", method}
        };
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data) << "request received";
    }
    static void LogResponse(int response_time_ms, int status_code, const std::string& content_type) {
        boost::json::value data = {
                {"response_time", response_time_ms},
                {"code",          status_code},
                {"content_type",  content_type}
        };
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, data)
                                << "response sent";
    }
public:
    explicit LoggingRequestHandler(std::shared_ptr<RequestHandler> decorated)
            : decorated_(decorated) {
    }
    template <typename Body, typename Allocator, typename Send>
    void operator()(const tcp::endpoint& endpoint, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Получаем URI и метод запроса
        std::string_view uri = req.target();
        std::string_view method = beast::http::to_string(req.method());
        LogRequest(endpoint.address().to_string(), uri, method);

        // Выполняем обработку запроса через основной обработчик
        (*decorated_)(std::move(req), std::forward<Send>(send));

        // Получаем информацию для логирования после обработки
        LogData log_data = (*decorated_).GetLogInfo();

        LogResponse(log_data.response_time_ms, log_data.status_code, log_data.content_type);
    }

private:
    std::shared_ptr<RequestHandler> decorated_;
};

// Форматтер, который выводит логи в JSON-формате
void MyFormatter(logging::record_view const &rec, logging::formatting_ostream &strm);

// Функция инициализации логирования
void InitLogging();

void LogServerStart(int port, const std::string &address);

void LogServerExit(int code, const std::string &exception);

void LogServerError(int error_code, const std::string &error_message, const std::string &where);

} // namespace server_logging