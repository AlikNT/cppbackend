#include <iostream>
#include <pqxx/pqxx>
#include <boost/json.hpp>

#define BOOST_BEAST_USE_STD_STRING_VIEW
#define BOOST_BEAST_USE_STD_STRING

using namespace std::literals;
// libpqxx использует zero-terminated символьные литералы вроде "abc"_zv;
using pqxx::operator"" _zv;

namespace json = boost::json;

// Создание таблицы, если её не существует
void create_table_if_not_exists(pqxx::connection &conn) {
    pqxx::work w(conn);
    w.exec(R"(
        CREATE TABLE IF NOT EXISTS books (
            id SERIAL PRIMARY KEY,
            title VARCHAR(100) NOT NULL,
            author VARCHAR(100) NOT NULL,
            year INTEGER NOT NULL,
            ISBN CHAR(13) UNIQUE
        )
    )"_zv);
    w.exec("DELETE FROM books;"_zv);
    w.commit();
}

// Добавление книги
bool add_book(pqxx::connection &conn, const json::value &payload) {
    try {
        pqxx::work w(conn);

        const auto title = static_cast<std::string>(payload.at("title").as_string());
        const auto author = static_cast<std::string>(payload.at("author").as_string());
        int year = static_cast<int>(payload.at("year").as_int64());
        const char *isbn = payload.at("ISBN").is_null() ? nullptr : payload.at("ISBN").as_string().c_str();

        w.exec_params(
            "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4)"_zv,
            title.c_str(), author.c_str(), year, isbn
        );

        w.commit();
        return true;

    } catch (const pqxx::sql_error &) {
        return false;
    }
}

// Вывод всех книг
json::array all_books(pqxx::connection &conn) {
    pqxx::read_transaction w(conn);
    auto rows = w.exec(R"(
        SELECT *
        FROM books
        ORDER BY year DESC, title, author, ISBN
    )"_zv);

    json::array books;
    for (const auto &row : rows) {
        json::object book;
        book["id"] = row["id"].as<int>();
        book["title"] = to_string(row["title"]);
        book["author"] = to_string(row["author"]);
        book["year"] = row["year"].as<int>();
        book["ISBN"] = row["ISBN"].is_null() ? nullptr : row["ISBN"].c_str();
        books.push_back(book);
    }

    return books;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: book_manager <connection_string>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string conn_str = argv[1];

    try {
        pqxx::connection conn(conn_str);
        create_table_if_not_exists(conn);

        std::string line;
        while (getline(std::cin, line)) {
            json::value request = json::parse(line);
            std::string action = static_cast<std::string>(request.at("action"s).as_string());
            const json::value &payload = request.at("payload");

            if (action == "add_book") {
                bool result = add_book(conn, payload);
                std::cout << json::serialize(json::object{{"result", result}}) << std::endl;
            } else if (action == "all_books") {
                json::array books = all_books(conn);
                std::cout << serialize(books) << std::endl;
            } else if (action == "exit") {
                break;
            } else {
                std::cerr << "Unknown action: " << action << std::endl;
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

