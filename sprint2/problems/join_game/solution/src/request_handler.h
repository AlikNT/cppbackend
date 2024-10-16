#pragma once
#include "http_server.h"
#include "model.h"
#include "json_loader.h"
#include "logger.h"

#include <boost/json.hpp>
#include <utility>
#include <optional>
#include <variant>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace net = boost::asio;
//using tcp = net::ip::tcp;

namespace fs = std::filesystem;

using HttpRequest = http::request<http::string_body>;
using EmptyResponse = http::response<http::empty_body>;
using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;
using FileRequestResult = std::variant<EmptyResponse, StringResponse, FileResponse>;

class ApiRequestHandler {
public:
    explicit ApiRequestHandler(model::Game& game);

    [[nodiscard]] StringResponse GetErrorResponse(const HttpRequest& req, http::status status, const std::string& code,  const std::string& message) const;

    // Обработка запросов к API
    [[nodiscard]] StringResponse GetApiResponse(const HttpRequest& req) const;

private:
    model::Game& game_;

    template<typename JsonBody>
    [[nodiscard]] StringResponse GetJsonResponse(const HttpRequest& req, const JsonBody &body) const;

    // Обработка запроса на получение списка карт
    [[nodiscard]] StringResponse GetMaps (const HttpRequest& req) const;

    // Обработка запроса на получение карты по ID
    [[nodiscard]] StringResponse GetMapById (const HttpRequest& req) const;
};

class FileRequestHandler {
public:
    explicit FileRequestHandler(fs::path root);

    [[nodiscard]] FileRequestResult GetFileResponse(const HttpRequest& req);

private:
    fs::path root_;

    // MIME типы для статических файлов
    static const std::unordered_map<std::string, std::string> mime_types_;

    // Функция для декодирования URL
    static std::string UrlDecode(const std::string& url);

    // Возвращает true, если каталог p содержится внутри base_path.
    static bool IsSubPath(fs::path path, fs::path base);

    // Функция для определения MIME-типа по расширению файла
    static std::string GetMimeType(const std::string& extension);

    [[nodiscard]] StringResponse NotFoundResponse(unsigned int version) const;

    [[nodiscard]] StringResponse BadRequestResponse(unsigned int version) const;
};

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:
    using Strand = net::strand<net::io_context::executor_type>;

    explicit RequestHandler(model::Game& game, fs::path root, Strand api_strand);

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto version = req.version();
        auto keep_alive = req.keep_alive();

        try {
            if (req.target().starts_with("/api/")) {
                auto handle = [self = shared_from_this(), send,
                        req = std::forward<decltype(req)>(req), version, keep_alive] {
                    try {
                        // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                        assert(self->api_strand_.running_in_this_thread());
                        return send(self->HandleApiRequest(req));
                    } catch (...) {
                        send(self->ReportServerError(version, keep_alive));
                    }
                };
                return net::dispatch(api_strand_, handle);
            }
            // Возвращаем результат обработки запроса к файлу
            return std::visit(
                    [&send](auto&& result) {
                        send(std::forward<decltype(result)>(result));
                    },
                    HandleFileRequest(req));
        } catch (...) {
            send(ReportServerError(version, keep_alive));
        }
    }

    // Метод для получения информации для логирования
    server_logging::LogData GetLogInfo() const;

private:
    ApiRequestHandler api_handler_;
    FileRequestHandler file_handler_;

    Strand api_strand_;

    // Данные для логирования
    unsigned int response_status_code_{0};          // Код ответа
    std::string content_type_;             // Тип контента

    // Обработка запросов на статические файлы
    FileRequestResult HandleFileRequest(const HttpRequest& req);

    StringResponse HandleApiRequest(const HttpRequest& req);

    StringResponse ReportServerError(unsigned version, bool keep_alive) const;

};

}  // namespace http_handler
