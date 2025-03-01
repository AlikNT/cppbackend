#pragma once
#include "http_server.h"
#include "model.h"
#include "json_loader.h"

#include <boost/json.hpp>
#include <utility>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

namespace fs = std::filesystem;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game, std::string static_root)
        : game_{game},
        static_root_(std::move(static_root)) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        // Определение метода и пути
        if (req.method() != http::verb::get) {
            return SendBadRequest("Unsupported HTTP method", send);
        }
        // Проверяем, начинается ли запрос с "/api/"
        if (req.target().starts_with("/api/")) {
            HandleApiRequest(std::move(req), std::forward<Send>(send));
        } else {
            HandleStaticFileRequest(std::move(req), std::forward<Send>(send));
        }
    }

private:
    using HttpRequest = http::request<http::string_body>;
    using HttpResponse = http::response<http::string_body>;

    model::Game& game_;
    const std::string static_root_;

    // MIME типы для статических файлов
    std::unordered_map<std::string, std::string> mime_types_ = {
            {".htm", "text/html"}, {".html", "text/html"},
            {".css", "text/css"}, {".txt", "text/plain"},
            {".js", "text/javascript"}, {".json", "application/json"},
            {".xml", "application/xml"}, {".png", "image/png"},
            {".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"},
            {".gif", "image/gif"}, {".bmp", "image/bmp"},
            {".ico", "image/vnd.microsoft.icon"}, {".tiff", "image/tiff"},
            {".svg", "image/svg+xml"}, {".mp3", "audio/mpeg"}
    };

    // Функция для определения MIME-типа по расширению файла
    std::string GetMimeType(const std::string& extension);

    // Функция для декодирования URL
    std::string UrlDecode(const std::string& url);

    // Возвращает true, если каталог p содержится внутри base_path.
    bool IsSubPath(fs::path path, fs::path base);

    // Обработка запроса на получение списка карт
    template <typename Send>
    void HandleGetMaps(HttpRequest&& req, Send&& send);

    // Обработка запроса на получение карты по ID
    template <typename Send>
    void HandleGetMapById(HttpRequest&& req, Send&& send);

    // Отправка JSON-ответа
    template <typename Send, typename JsonBody>
    void SendJsonResponse(const HttpRequest& req, const JsonBody& body, Send&& send);

    // Обработка ошибок 404 (не найдено)
    template <typename Send>
    void SendNotFound(const std::string& message, const std::string& code, Send&& send);

    // Обработка ошибок 400 (плохой запрос)
    template <typename Send>
    void SendBadRequest(const std::string& message, Send&& send);

    // Отправка ответа с ошибкой
    template <typename Send>
    void SendErrorResponse(http::status status, const json::object& error_json, Send&& send);

    // Обработка запросов к API
    template <typename Body, typename Allocator, typename Send>
    void HandleApiRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        const std::string target = std::string(req.target());
        if (target == "/api/v1/maps" || target == "/api/v1/maps/") {
            HandleGetMaps(std::move(req), std::forward<Send>(send));
        } else if (target.starts_with("/api/v1/maps/")) {
            HandleGetMapById(std::move(req), std::forward<Send>(send));
        } else if (target.starts_with("/api/")) {
            return SendBadRequest("Bad request", send);
        } else {
            return SendBadRequest("Unknown API endpoint", send);
        }
    }

    // Обработка запросов на статические файлы
    template <typename Body, typename Allocator, typename Send>
    void HandleStaticFileRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Получаем полный путь к статическому каталогу
        fs::path static_root{static_root_};
        static_root = fs::weakly_canonical(static_root);

        // Декодируем URL получаем относительный путь
        auto test = std::string(req.target());
        fs::path rel_path{UrlDecode(std::string(req.target()))};
        rel_path = fs::weakly_canonical(rel_path);
        // Если путь абсолютный, делаем его относительным
        if (rel_path.is_absolute()) {
            rel_path = rel_path.relative_path();
        }

        // Формирование полного пути к файлу
        fs::path full_path = fs::weakly_canonical(static_root / rel_path);

        // Если путь является директорией, добавляем к нему index.html
        if (fs::is_directory(full_path)) {
            full_path /= "index.html";
        }

        // Проверка, что запрашиваемый путь находится внутри каталога статических файлов
        if (!fs::exists(full_path) || !IsSubPath(full_path, static_root)) {
            Send404(std::move(req), std::forward<Send>(send));
            return;
        }

        // Определяем MIME-тип по расширению файла
        std::string extension = full_path.extension().string();
        std::string mime_type = GetMimeType(extension);

        // Открываем файл
        beast::error_code ec;
        http::file_body::value_type file;
        file.open(full_path.string().c_str(), beast::file_mode::read, ec);
        if (ec) {
            Send404(std::move(req), std::forward<Send>(send));
            return;
        }

        // Формируем ответ с файлом
        http::response<http::file_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, mime_type);
        res.body() = std::move(file);
        res.prepare_payload();  // Устанавливаем заголовки Content-Length и Transfer-Encoding
        send(std::move(res));
    }

    // Функция для отправки 404 ошибки
    template <typename Body, typename Allocator, typename Send>
    void Send404(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::content_type, "text/plain");
        res.body() = "File not found";
        res.prepare_payload();
        send(std::move(res));
    }
};

template<typename Send>
void RequestHandler::SendErrorResponse(http::status status, const json::object &error_json, Send &&send) {
    HttpResponse res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.body() = json::serialize(error_json);
    res.prepare_payload();
    send(std::move(res));
}

template<typename Send>
void RequestHandler::SendBadRequest(const std::string &message, Send &&send) {
    json::object error_json{
            {"code", "badRequest"},
            {"message", message}
    };
    SendErrorResponse(http::status::bad_request, error_json, std::forward<Send>(send));
}

template<typename Send>
void RequestHandler::SendNotFound(const std::string &message, const std::string &code, Send &&send) {
    json::object error_json{
            {"code", code},
            {"message", message}
    };
    SendErrorResponse(http::status::not_found, error_json, std::forward<Send>(send));
}

template<typename Send, typename JsonBody>
void RequestHandler::SendJsonResponse(const RequestHandler::HttpRequest &req, const JsonBody &body, Send &&send) {
    HttpResponse res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = json::serialize(body);
    res.prepare_payload();
    send(std::move(res));
}

template<typename Send>
void RequestHandler::HandleGetMapById(RequestHandler::HttpRequest &&req, Send &&send) {
    const std::string target = std::string(req.target());
    const std::string map_id_str = target.substr(strlen("/api/v1/maps/"));
    auto map_id = model::Map::Id{map_id_str};

    const auto map = game_.FindMap(map_id);
    if (!map) {
        return SendNotFound("Map not found", "mapNotFound", send);
    }

    json::object map_json;
    map_json[OFFICE_ID] = *(map->GetId());
    map_json["name"] = map->GetName();

    // Добавление дорог
    json::array roads_json;
    for (const auto& road : map->GetRoads()) {
        json::object road_json;
        road_json[ROAD_BEGIN_X0] = road.GetStart().x;
        road_json[ROAD_BEGIN_Y0] = road.GetStart().y;

        if (road.IsHorizontal()) {
            road_json[ROAD_END_X1] = road.GetEnd().x;
        } else {
            road_json[ROAD_END_Y1] = road.GetEnd().y;
        }

        roads_json.push_back(road_json);
    }
    map_json["roads"] = roads_json;

    // Добавление зданий
    json::array buildings_json;
    for (const auto& building : map->GetBuildings()) {
        const auto& bounds = building.GetBounds();
        buildings_json.push_back({
                                         {X, bounds.position.x},
                                         {Y, bounds.position.y},
                                         {BUILDING_WIDTH, bounds.size.width},
                                         {BUILDING_HEIGHT, bounds.size.height}
                                 });
    }
    map_json["buildings"] = buildings_json;

    // Добавление офисов
    json::array offices_json;
    for (const auto& office : map->GetOffices()) {
        offices_json.push_back({
                                       {OFFICE_ID, *office.GetId()},
                                       {X, office.GetPosition().x},
                                       {Y, office.GetPosition().y},
                                       {OFFICE_OFFSET_X, office.GetOffset().dx},
                                       {OFFICE_OFFSET_Y, office.GetOffset().dy}
                               });
    }
    map_json["offices"] = offices_json;

    SendJsonResponse(req, map_json, std::forward<Send>(send));
}

template<typename Send>
void RequestHandler::HandleGetMaps(RequestHandler::HttpRequest &&req, Send &&send) {
    using namespace std::literals;
    json::array maps_json;

    for (const auto& map : game_.GetMaps()) {
        maps_json.push_back({
                                    {OFFICE_ID, *map.GetId()},
                                    {"name"s, map.GetName()}
                            });
    }

    SendJsonResponse(req, maps_json, std::forward<Send>(send));
}

}  // namespace http_handler
