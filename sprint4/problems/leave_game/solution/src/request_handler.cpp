#include <algorithm>

#include "request_handler.h"

using namespace std::literals;
namespace http_handler {

const std::unordered_map<std::string, std::string> FileRequestHandler::mime_types_ = {
        {".htm"s, "text/html"s}, {".html"s, "text/html"s},
        {".css"s, "text/css"s}, {".txt"s, "text/plain"s},
        {".js"s, "text/javascript"s}, {".json"s, "application/json"s},
        {".xml"s, "application/xml"s}, {".png"s, "image/png"s},
        {".jpg"s, "image/jpeg"s}, {".jpeg"s, "image/jpeg"s},
        {".gif"s, "image/gif"s}, {".bmp"s, "image/bmp"s},
        {".ico"s, "image/vnd.microsoft.icon"s}, {".tiff"s, "image/tiff"s},
        {".svg"s, "image/svg+xml"s}, {".mp3"s, "audio/mpeg"s}
};

std::string FileRequestHandler::GetMimeType(const std::string &extension) {
    auto it = mime_types_.find(boost::algorithm::to_lower_copy(extension));
    if (it != mime_types_.end()) {
        return it->second;
    }
    return "application/octet-stream"s;  // MIME тип по умолчанию
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
        full_path /= "index.html"s;
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

RequestHandler::RequestHandler(model::Game &game, app::Application &app, fs::path root, Strand api_strand,
                               int tick_period)
        : file_handler_(std::move(root)),
          api_handler_(game, app, tick_period),
          api_strand_(std::move(api_strand)) {
}

StringResponse RequestHandler::ReportServerError(unsigned int version, bool keep_alive) const {
    StringResponse res{http::status::internal_server_error, version};
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(keep_alive);
    res.body() = "Internal Server Error"s;
    res.prepare_payload();
    return res;
}

template<typename... Headers>
StringResponse ApiRequestHandler::GetErrorResponse(const HttpRequest &req, http::status status, const std::string &code,
                                                   const std::string &message, const Headers&... headers) const {
    json::object error_json{
            {"code"s, code},
            {"message"s, message}
    };
    StringResponse res{status, req.version()};
    res.set(http::field::content_type, "application/json");
//    res.set(http::field::cache_control, "no-cache");
    (res.set(headers.first, headers.second), ...);
    res.body() = json::serialize(error_json);
    res.prepare_payload();

    return res;
}

StringResponse ApiRequestHandler::GetApiResponse(const HttpRequest &req) const {
    const std::string target = std::string(req.target());
    if (target == "/api/v1/maps"s || target == "/api/v1/maps/"s) {
        return GetMaps(req);
    } else if (target.starts_with("/api/v1/maps/"s)) {
        return GetMapById(req);
    } else if (target.starts_with("/api/v1/game/records"s)) {
        return GetTableOfRecords(req);
    } else if (target == "/api/v1/game/join"s || target == "/api/v1/game/join/"s) {
        return HandleJoinGame(req);
    } else if (target == "/api/v1/game/players"s || target == "/api/v1/game/players/"s) {
        return GetPlayers(req);
    } else if (target == "/api/v1/game/state"s || target == "/api/v1/game/state/"s) {
        return GetGameState(req);
    } else if (target == "/api/v1/game/player/action"s || target == "/api/v1/game/player/action/"s) {
        return HandleMovePlayers(req);
    } else if (target == "/api/v1/game/tick"s || target == "/api/v1/game/tick/"s) {
        if (tick_period_) {
            return GetErrorResponse(req, http::status::bad_request, "badRequest"s, "Invalid endpoint"s);
        } else {
            return HandleTimeControl(req);
        }
    } else if (target.starts_with("/api/")) {
        return GetErrorResponse(req, http::status::bad_request, "badRequest"s, "Bad request"s);
    }
    return GetErrorResponse(req, http::status::bad_request, "badRequest"s, "Unknown API endpoint"s);
}

StringResponse ApiRequestHandler::GetMaps(const HttpRequest &req) const {

    // Проверяем метод GET или HEAD
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        return GetErrorResponse(req, http::status::method_not_allowed, "invalidMethod"s, "Invalid method"s,
                                std::make_pair(http::field::allow, "GET, HEAD"s),
                                std::make_pair(http::field::cache_control, "no-cache"s));
    }

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
    // Проверяем метод GET или HEAD
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        return GetErrorResponse(req, http::status::method_not_allowed, "invalidMethod"s, "Invalid method"s,
                                std::make_pair(http::field::allow, "GET, HEAD"s),
                                std::make_pair(http::field::cache_control, "no-cache"s));
    }

    const std::string target = std::string(req.target());
    const std::string map_id_str = target.substr(strlen("/api/v1/maps/"));
    const auto map_id = model::Map::Id{map_id_str};

    const json::object map_json = app_.GetMapsById(map_id);
    if (map_json.empty()) {
        return GetErrorResponse(req, http::status::not_found, "mapNotFound"s, "Map not found"s,
            std::make_pair(http::field::cache_control, "no-cache"s));
    }

    return GetJsonResponse(req, map_json);
}

StringResponse ApiRequestHandler::GetTableOfRecords(const HttpRequest &req) const {
    // Проверяем метод GET
    if (req.method() != http::verb::get) {
        return GetErrorResponse(req, http::status::method_not_allowed, "invalidMethod"s, "Invalid request"s,
                                std::make_pair(http::field::allow, "GET"s),
                                std::make_pair(http::field::cache_control, "no-cache"s));
    }
    const std::string target = std::string(req.target());
    std::string query_string;
    if (target != "/api/v1/game/records") {
        query_string = target.substr(strlen("/api/v1/game/records?"));
    }
    int start = 0;
    int max_items = 100;
    if (!query_string.empty()) {
        // Парсим URL-строку запроса
        auto params = ParseQueryString(query_string);
        try {
            for (const auto& [key, value] : params) {
                if (key == "start"s) {
                    start = std::stoi(value);
                    continue;
                }
                if (key == "maxItems"s) {
                    max_items = std::stoi(value);
                }
            }
        } catch (const std::exception&) {
            return GetErrorResponse(req, http::status::bad_request, "Bad request"s, "Invalid parameter maxItems"s);
        }
        // Проверяем на валидность параметр maxItems. Если maxItems > 100 возвращаем ошибку 400 Bad Request
        if (max_items > 100) {
            return GetErrorResponse(req, http::status::bad_request, "Bad request"s, "Invalid parameter maxItems"s);
        }
    }
    // Получаем таблицу рекордов в json
    const json::array table_of_records_json = app_.GetTableOfRecords(start, max_items);
    // Формируем JSON-ответ
    return GetJsonResponse(req, table_of_records_json);
}

ApiRequestHandler::ApiRequestHandler(model::Game &game, app::Application &app, int tick_period)
        : game_(game)
        , app_(app)
        , tick_period_(tick_period) {}

StringResponse ApiRequestHandler::HandleJoinGame(const HttpRequest &req) const {
    // Проверяем заголовок Content-Type
    if (req["Content-Type"] != "application/json") {
        return GetErrorResponse(req, http::status::bad_request, "invalidContentType"s, "Content-Type must be application/json"s,
                                std::make_pair(http::field::cache_control, "no-cache"s));
    }

    // Проверяем метод POST
    if (req.method() != http::verb::post) {
        return GetErrorResponse(req, http::status::method_not_allowed, "invalidMethod"s, "Method must be POST"s,
                                std::make_pair(http::field::allow, "POST"s),
                                std::make_pair(http::field::cache_control, "no-cache"s));
    }

    // Парсим JSON-тело запроса
    json::value body;
    try {
        body = json::parse(req.body());
    } catch (const std::exception&) {
        return GetErrorResponse(req, http::status::bad_request, "invalidJson"s, "Invalid JSON format"s,
                                std::make_pair(http::field::cache_control, "no-cache"s));
    }

    // Извлекаем поля user_name и map_id
    std::string user_name;
    std::string map_id_str;
    try {
        user_name = json::value_to<std::string>(body.at("userName"s));
        map_id_str = json::value_to<std::string>(body.at("mapId"s));
    } catch (const std::exception&) {
        return GetErrorResponse(req, http::status::bad_request, "invalidArgument"s, "User_name and map_id are required"s,
                                std::make_pair(http::field::cache_control, "no-cache"s));
    }

    // Проверяем поле user_name на пустое значение
    if (user_name.empty()) {
        return GetErrorResponse(req, http::status::bad_request, "invalidArgument"s, "User_name must not be empty"s,
                                std::make_pair(http::field::cache_control, "no-cache"s));
    }

    const model::Map::Id map_id(map_id_str);
    // Проверяем, существует ли карта с указанным map_id
    const model::Map* map = game_.FindMap(map_id);
    if (!map) {
        return GetErrorResponse(req, http::status::not_found, "mapNotFound"s, "The specified map ID does not exist"s,
                                std::make_pair(http::field::cache_control, "no-cache"s));
    }

    // Добавляем игрока и получаем токен
    app::JoinGameResult join_game_result = app_.JoinGame(map_id, user_name);

    // Формируем ответ
    boost::json::object json_body;
    json_body["authToken"s] = *join_game_result.token;
    json_body["playerId"s] = join_game_result.dog_id;

    return GetJsonResponse(req, json_body);
}

StringResponse ApiRequestHandler::GetPlayers(const HttpRequest &req) const {
    return ExecuteAuthorized(req, [req, this](const app::Token& token){

        // Проверяем метод GET или HEAD
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            return GetErrorResponse(req, http::status::method_not_allowed, "invalidMethod"s, "Invalid method"s,
                                    std::make_pair(http::field::allow, "GET, HEAD"s),
                                    std::make_pair(http::field::cache_control, "no-cache"s));
        }

        // Проверяем наличие игроков по предъявленному токену
        const auto players = app_.ListPlayers(token);
        if (!players) {
            return GetErrorResponse(req, http::status::unauthorized, "unknownToken"s, "Player token has not been found"s,
                                    std::make_pair(http::field::cache_control, "no-cache"s));
        }

        // Формируем JSON-ответ
        json::object players_json;
        for (const auto& p : *players) {
            json::object player_json;
            player_json["name"s] = p.second->GetName();
            players_json[std::to_string(p.second->GetId())] = player_json;
        }

        return GetJsonResponse(req, players_json);
    });
}

StringResponse ApiRequestHandler::GetGameState(const HttpRequest &req) const {
    // Проверка метода GET, HEAD
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        return GetErrorResponse(req, http::status::method_not_allowed, "invalidMethod"s, "Invalid method"s,
                                std::make_pair(http::field::allow, "GET, HEAD"s),
                                std::make_pair(http::field::cache_control, "no-cache"s));
    }
    return ExecuteAuthorized(req, [req, this](const app::Token& token) {

        const json::object json_body = app_.GameState(token);
        if (json_body.empty()) {
            return GetErrorResponse(req, http::status::unauthorized, "unknownToken"s, "Player token has not been found"s,
                                    std::make_pair(http::field::cache_control, "no-cache"s));
        }

        return GetJsonResponse(req, json_body);
    });
}

std::optional<app::Token> ApiRequestHandler::TryExtractToken(const HttpRequest &req) const {
    // Проверяем наличие и валидность заголовка Authorization
    if (req.find(http::field::authorization) == req.end() || !req[http::field::authorization].starts_with("Bearer "s)) {
        return std::nullopt;
    }

    // Извлечение токена из заголовка Authorization
    std::string_view auth_header = req[http::field::authorization];
    std::string token_str = std::string(auth_header.substr(7));
    app::Token token(token_str);

    if (!app::IsValidToken(token)) {
        return std::nullopt;
    }
    return token;
}

StringResponse ApiRequestHandler::HandleMovePlayers(const HttpRequest &req) const {
    return ExecuteAuthorized(req, [req, this](const app::Token& token) {

        // Проверка метода POST
        if (req.method() != http::verb::post) {
            return GetErrorResponse(req, http::status::method_not_allowed, "invalidMethod"s, "Invalid method"s,
                                    std::make_pair(http::field::allow, "POST"s),
                                    std::make_pair(http::field::cache_control, "no-cache"s));
        }
        // Проверка заголовка Content-Type
        if (req["Content-Type"] != "application/json") {
            return GetErrorResponse(req, http::status::bad_request, "invalidArgument"s, "Invalid content type"s,
                                    std::make_pair(http::field::cache_control, "no-cache"s));
        }
        // Парсим JSON
        json::value body;
        try {
            body = boost::json::parse(req.body());
        } catch (const std::exception&) {
            return GetErrorResponse(req, http::status::bad_request, "invalidArgument"s, "Invalid JSON"s,
                                    std::make_pair(http::field::cache_control, "no-cache"s));
        }
        // Пытаемся получить значение move
        std::string move;
        try {
            move = json::value_to<std::string>(body.at("move"));
        } catch (const std::exception&) {
            return GetErrorResponse(req, http::status::bad_request, "invalidArgument"s, "Failed to parse move request JSON"s,
                                    std::make_pair(http::field::cache_control, "no-cache"s));
        }

        // Обрабатываем move и получаем результат
        auto move_players_result = app_.MovePlayers(token, move);
        if (move_players_result == app::MovePlayersResult::UNKNOWN_TOKEN) {
            return GetErrorResponse(req, http::status::unauthorized, "unknownToken"s, "Player token has not been found"s,
                                    std::make_pair(http::field::cache_control, "no-cache"s));
        }
        if (move_players_result == app::MovePlayersResult::UNKNOWN_MOVE) {
            return GetErrorResponse(req, http::status::bad_request, "invalidArgument"s, "Invalid move value"s,
                                    std::make_pair(http::field::cache_control, "no-cache"s));
        }
        return GetJsonResponse(req, boost::json::object{});
    });
}

StringResponse ApiRequestHandler::HandleTimeControl(const HttpRequest &req) const {
    // Проверяем заголовок Content-Type
    if (req["Content-Type"] != "application/json") {
        return GetErrorResponse(req, http::status::bad_request, "invalidContentType"s, "Content-Type must be application/json"s,
                                std::make_pair(http::field::cache_control, "no-cache"s));
    }

    // Проверяем метод POST
    if (req.method() != http::verb::post) {
        return GetErrorResponse(req, http::status::method_not_allowed, "invalidMethod"s, "Method must be POST"s,
                                std::make_pair(http::field::allow, "POST"s),
                                std::make_pair(http::field::cache_control, "no-cache"s));
    }

    // Парсим JSON
    json::value body;
    try {
        body = boost::json::parse(req.body());
    } catch (const std::exception&) {
        return GetErrorResponse(req, http::status::bad_request, "invalidArgument"s, "Invalid JSON"s,
                                std::make_pair(http::field::cache_control, "no-cache"s));
    }
    // Пытаемся получить значение timeDelta
    int time_delta;
    try {
        if (!body.at("timeDelta"s).is_int64()) {
            return GetErrorResponse(req, http::status::bad_request, "invalidArgument"s,
                                    "'timeDelta' must be an integer"s,
                                    std::make_pair(http::field::cache_control, "no-cache"s));
        }
        time_delta = json::value_to<int>(body.at("timeDelta"s));
    } catch (const std::exception&) {
        return GetErrorResponse(req, http::status::bad_request, "invalidArgument"s, "Failed to parse tick request JSON"s,
                                std::make_pair(http::field::cache_control, "no-cache"s));
    }
    std::chrono::milliseconds time_delta_ms(time_delta);
    app_.Tick(time_delta_ms);

    return GetJsonResponse(req, boost::json::object{});
}

template<typename Fn>
StringResponse ApiRequestHandler::ExecuteAuthorized(const HttpRequest &req, Fn &&action) const {
    if (auto token = TryExtractToken(req)) {
        return action(*token);
    } else {
        return GetErrorResponse(req, http::status::unauthorized, "invalidToken"s, "Invalid authorization or token"s,
                                std::make_pair(http::field::cache_control, "no-cache"s));
    }
}

template<typename JsonBody>
StringResponse ApiRequestHandler::GetJsonResponse(const HttpRequest &req, const JsonBody &body) const {
    StringResponse res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::cache_control, "no-cache");
    res.keep_alive(req.keep_alive());
    res.body() = json::serialize(body);
    res.prepare_payload();

    return res;
}

std::map<std::string, std::string> ParseQueryString(const std::string &query) {
    std::map<std::string, std::string> result;
    size_t start = 0, end = 0;

    while ((end = query.find('&', start)) != std::string::npos) {
        auto token = query.substr(start, end - start);
        auto pos = token.find('=');
        if (pos != std::string::npos) {
            result[token.substr(0, pos)] = token.substr(pos + 1);
        }
        start = end + 1;
    }

    // Last parameter
    auto token = query.substr(start);
    auto pos = token.find('=');
    if (pos != std::string::npos) {
        result[token.substr(0, pos)] = token.substr(pos + 1);
    }

    return result;
}

Ticker::Ticker(Ticker::Strand strand, std::chrono::milliseconds period,
               std::function<void(std::chrono::milliseconds)> handler)
        : strand_{strand}
        , period_{period}
        , handler_{std::move(handler)} {
}

void Ticker::Start() {
    net::dispatch(strand_, [self = shared_from_this(), this] {
        last_tick_ = Clock::now();
        self->ScheduleTick();
    });
}

void Ticker::ScheduleTick() {
    assert(strand_.running_in_this_thread());
    timer_.expires_after(period_);
    timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
        self->OnTick(ec);
    });
}

void Ticker::OnTick(sys::error_code ec) {
    using namespace std::chrono;
    assert(strand_.running_in_this_thread());

    if (!ec) {
        auto this_tick = Clock::now();
        auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
        last_tick_ = this_tick;
        try {
            handler_(delta);
        } catch (...) {
        }
        ScheduleTick();
    }
}
}  // namespace http_handler
