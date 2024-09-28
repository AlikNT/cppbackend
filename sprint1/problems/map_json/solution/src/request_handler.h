#pragma once
#include "http_server.h"
#include "model.h"

#include <boost/json.hpp>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        // Определение метода и пути
        if (req.method() != http::verb::get) {
            return send_bad_request("Unsupported HTTP method", send);
        }

        const std::string target = std::string(req.target());
        if (target == "/api/v1/maps") {
            handle_get_maps(std::move(req), std::forward<Send>(send));
        } else if (target.find("/api/v1/maps/") == 0) {
            handle_get_map_by_id(std::move(req), std::forward<Send>(send));
        } else if (target.find("/api/") == 0) {
            return send_bad_request("Bad request", send);
        } else {
            return send_bad_request("Unknown API endpoint", send);
        }
    }

private:
    using HttpRequest = http::request<http::string_body>;
    using HttpResponse = http::response<http::string_body>;

    model::Game& game_;

    // Обработка запроса на получение списка карт
    template <typename Send>
    void handle_get_maps(HttpRequest&& req, Send&& send) {
        json::array maps_json;

        for (const auto& map : game_.GetMaps()) {
            maps_json.push_back({
                                        {"id", *map.GetId()},
                                        {"name", map.GetName()}
                                });
        }

        send_json_response(req, maps_json, std::forward<Send>(send));
    }

    // Обработка запроса на получение карты по ID
    template <typename Send>
    void handle_get_map_by_id(HttpRequest&& req, Send&& send) {
        const std::string target = std::string(req.target());
        const std::string map_id_str = target.substr(strlen("/api/v1/maps/"));
        auto map_id = model::Map::Id{map_id_str};

        const auto* map = game_.FindMap(map_id);
        if (!map) {
            return send_not_found("Map not found", "mapNotFound", send);
        }

        json::object map_json;
        map_json["id"] = *map->GetId();
        map_json["name"] = map->GetName();

        // Добавление дорог
        json::array roads_json;
        for (const auto& road : map->GetRoads()) {
            json::object road_json;
            road_json["x0"] = road.GetStart().x;
            road_json["y0"] = road.GetStart().y;

            if (road.IsHorizontal()) {
                road_json["x1"] = road.GetEnd().x;
            } else {
                road_json["y1"] = road.GetEnd().y;
            }

            roads_json.push_back(road_json);
        }
        map_json["roads"] = roads_json;

        // Добавление зданий
        json::array buildings_json;
        for (const auto& building : map->GetBuildings()) {
            const auto& bounds = building.GetBounds();
            buildings_json.push_back({
                                             {"x", bounds.position.x},
                                             {"y", bounds.position.y},
                                             {"w", bounds.size.width},
                                             {"h", bounds.size.height}
                                     });
        }
        map_json["buildings"] = buildings_json;

        // Добавление офисов
        json::array offices_json;
        for (const auto& office : map->GetOffices()) {
            offices_json.push_back({
                                           {"id", *office.GetId()},
                                           {"x", office.GetPosition().x},
                                           {"y", office.GetPosition().y},
                                           {"offsetX", office.GetOffset().dx},
                                           {"offsetY", office.GetOffset().dy}
                                   });
        }
        map_json["offices"] = offices_json;

        send_json_response(req, map_json, std::forward<Send>(send));
    }

    // Отправка JSON-ответа
    template <typename Send, typename JsonBody>
    void send_json_response(const HttpRequest& req, const JsonBody& body, Send&& send) {
        HttpResponse res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = json::serialize(body);
        res.prepare_payload();
        send(std::move(res));
    }

    // Обработка ошибок 404 (не найдено)
    template <typename Send>
    void send_not_found(const std::string& message, const std::string& code, Send&& send) {
        json::object error_json{
                {"code", code},
                {"message", message}
        };
        send_error_response(http::status::not_found, error_json, std::forward<Send>(send));
    }

    // Обработка ошибок 400 (плохой запрос)
    template <typename Send>
    void send_bad_request(const std::string& message, Send&& send) {
        json::object error_json{
                {"code", "badRequest"},
                {"message", message}
        };
        send_error_response(http::status::bad_request, error_json, std::forward<Send>(send));
    }

    // Отправка ответа с ошибкой
    template <typename Send>
    void send_error_response(http::status status, const json::object& error_json, Send&& send) {
        HttpResponse res{status, 11};
        res.set(http::field::content_type, "application/json");
        res.body() = json::serialize(error_json);
        res.prepare_payload();
        send(std::move(res));
    }
};

}  // namespace http_handler
