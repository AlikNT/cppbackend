#include <boost/algorithm/string.hpp>

#include "request_handler.h"

namespace http_handler {

const std::unordered_map<std::string, std::string> FileRequestHandler::mime_types_ = {
        {".htm", "text/html"}, {".html", "text/html"},
        {".css", "text/css"}, {".txt", "text/plain"},
        {".js", "text/javascript"}, {".json", "application/json"},
        {".xml", "application/xml"}, {".png", "image/png"},
        {".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"},
        {".gif", "image/gif"}, {".bmp", "image/bmp"},
        {".ico", "image/vnd.microsoft.icon"}, {".tiff", "image/tiff"},
        {".svg", "image/svg+xml"}, {".mp3", "audio/mpeg"}
};

std::string FileRequestHandler::GetMimeType(const std::string &extension) {
    auto it = mime_types_.find(boost::algorithm::to_lower_copy(extension));
    if (it != mime_types_.end()) {
        return it->second;
    }
    return "application/octet-stream";  // MIME тип по умолчанию
}

std::string FileRequestHandler::UrlDecode(const std::string &url) {
    std::string result;
    char hex_char[3];
    hex_char[2] = '\0';

    for (size_t i = 0; i < url.length(); ++i) {
        if (url[i] == '%' && i + 2 < url.length()) {
            hex_char[0] = url[i + 1];
            hex_char[1] = url[i + 2];
            result += static_cast<char>(std::strtol(hex_char, nullptr, 16));
            i += 2;
        } else if (url[i] == '+') {
            result += ' ';
        } else {
            result += url[i];
        }
    }

    return result;
}

bool FileRequestHandler::IsSubPath(fs::path path, fs::path base) {
    // Приводим оба пути к каноничному виду (без . и ..)
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    // Проверяем, что все компоненты base содержатся внутри path
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

StringResponse FileRequestHandler::NotFoundResponse(unsigned int version) const {
    StringResponse res{http::status::not_found, version};
    res.set(http::field::content_type, "text/plain");
    res.body() = "File not found";
    res.prepare_payload();

    return res;
}

StringResponse FileRequestHandler::BadRequestResponse(unsigned int version) const {
    StringResponse res{http::status::bad_request, version};
    res.set(http::field::content_type, "text/plain");
    res.body() = "Bad request";
    res.prepare_payload();

    return res;
}

FileRequestResult FileRequestHandler::GetFileResponse(const HttpRequest &req) {
    // Получаем полный путь к статическому каталогу
    fs::path static_root = fs::weakly_canonical(root_);

    // Декодируем URL получаем относительный путь
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
    if (!IsSubPath(full_path, static_root)) {
        return BadRequestResponse(req.version());
    }

    // Проверка, что запрашиваемый файл существует
    if (!fs::exists(full_path)) {
        return NotFoundResponse(req.version());
    }

    // Определяем MIME-тип по расширению файла
    std::string extension = full_path.extension().string();
    std::string mime_type = GetMimeType(extension);

    // Открываем файл
    beast::error_code ec;
    http::file_body::value_type file;
    file.open(full_path.string().c_str(), beast::file_mode::read, ec);
    if (ec) {
        return NotFoundResponse(req.version());
    }

    // Формируем ответ с файлом
    http::response<http::file_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, mime_type);
    res.body() = std::move(file);
    res.prepare_payload();

    return res;
}

FileRequestHandler::FileRequestHandler(fs::path root)
        : root_(std::move(root)) {}

FileRequestResult RequestHandler::HandleFileRequest(const HttpRequest &req) {
    auto result = file_handler_.GetFileResponse(req);
    unsigned int response_status_code;
    std::string content_type;
    std::visit([&response_status_code, &content_type](const auto& value) {
        response_status_code = value.result_int();
        content_type = value[http::field::content_type];
    }, result);
    response_status_code_ = response_status_code;
    content_type_ = std::move(content_type);
    return result;
}

StringResponse RequestHandler::HandleApiRequest(const HttpRequest &req) {
    StringResponse result = api_handler_.GetApiResponse(req);
    response_status_code_ = result.result_int();
    content_type_ = result[http::field::content_type];
    return result;
}

server_logging::LogData RequestHandler::GetLogInfo() const {
    return {response_status_code_, content_type_};
}

RequestHandler::RequestHandler(model::Game &game, fs::path root, RequestHandler::Strand api_strand)
        : file_handler_(std::move(root)),
          api_handler_(game),
          api_strand_(std::move(api_strand)) {
}

StringResponse RequestHandler::ReportServerError(unsigned int version, bool keep_alive) const {
    StringResponse res{http::status::internal_server_error, version};
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(keep_alive);
    res.body() = "Internal Server Error";
    res.prepare_payload();
    return res;
}

StringResponse ApiRequestHandler::GetErrorResponse(const HttpRequest &req, http::status status, const std::string &code,
                                                   const std::string &message) const {
    json::object error_json{
            {"code", code},
            {"message", message}
    };
    StringResponse res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.body() = json::serialize(error_json);
    res.prepare_payload();

    return res;
}

StringResponse ApiRequestHandler::GetApiResponse(const HttpRequest &req) const {
    const std::string target = std::string(req.target());
    if (target == "/api/v1/maps" || target == "/api/v1/maps/") {
        return GetMaps(req);
    } else if (target.starts_with("/api/v1/maps/")) {
        return GetMapById(req);
    } else if (target.starts_with("/api/")) {
        return GetErrorResponse(req, http::status::bad_request, "badRequest", "Bad request");
    }
    return GetErrorResponse(req, http::status::bad_request, "badRequest", "Unknown API endpoint");
}

StringResponse ApiRequestHandler::GetMaps(const HttpRequest &req) const {
    using namespace std::literals;
    json::array maps_json;

    for (const auto& map : game_.GetMaps()) {
        maps_json.push_back({
                                    {OFFICE_ID, *map.GetId()},
                                    {"name"s, map.GetName()}
                            });
    }

    return GetJsonResponse(req, maps_json);
}

StringResponse ApiRequestHandler::GetMapById(const HttpRequest &req) const {
    const std::string target = std::string(req.target());
    const std::string map_id_str = target.substr(strlen("/api/v1/maps/"));
    auto map_id = model::Map::Id{map_id_str};

    const auto map = game_.FindMap(map_id);
    if (!map) {
        return GetErrorResponse(req, http::status::not_found, "mapNotFound", "Map not found");
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

    return GetJsonResponse(req, map_json);
}

ApiRequestHandler::ApiRequestHandler(model::Game &game)
        : game_(game) {}

template<typename JsonBody>
StringResponse ApiRequestHandler::GetJsonResponse(const HttpRequest &req, const JsonBody &body) const {
    StringResponse res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = json::serialize(body);
    res.prepare_payload();

    return res;
}
}  // namespace http_handler
