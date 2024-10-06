#pragma once

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
#include <iostream>

namespace logger {

// Определяем ключевое слово для добавления произвольных данных в лог
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value)

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace json = boost::json;

// Форматтер, который выводит логи в JSON-формате
void MyFormatter(logging::record_view const &rec, logging::formatting_ostream &strm);

// Функция инициализации логирования
void InitLogging();

void LogRequest(std::string_view ip, std::string_view uri, std::string_view method);

void LogResponse(int response_time, int code, const std::string &content_type);

void LogServerStart(int port, const std::string &address);

void LogServerExit(int code);

void LogServerError(int error_code, const std::string &error_message, const std::string &where);

} // namespace logger