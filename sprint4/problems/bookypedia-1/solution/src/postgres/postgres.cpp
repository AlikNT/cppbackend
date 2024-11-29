#include "postgres.h"

#include <pqxx/zview.hxx>

#include "../domain/book.h"

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    pqxx::work work{connection_};
    work.exec_params(
        R"(
            INSERT INTO authors (id, name) VALUES ($1, $2)
            ON CONFLICT (id) DO UPDATE SET name=$2;
        )"_zv,
            author.GetId().ToString(), author.GetName());
            work.commit();
}

std::vector<ui::detail::AuthorInfo> AuthorRepositoryImpl::LoadAuthors() {
    std::vector<ui::detail::AuthorInfo> authors;
    pqxx::read_transaction r(connection_);
    auto query_text = "SELECT id, name FROM authors ORDER BY name;"_zv;
    for (const auto& [id, name] : r.query<std::string, std::string>(query_text)) {
        authors.emplace_back(id, name);
    }
    return authors;
}

void BookRepositoryImpl::Save(const domain::Book &book) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
            INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
            ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
        )"_zv,
            book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetPublicationYear());
    work.commit();
}

std::vector<ui::detail::BookInfo> BookRepositoryImpl::LoadAuthorBooks(const std::string &author_id) {
    std::vector<ui::detail::BookInfo> books;
    pqxx::read_transaction r(connection_);
    pqxx::result result = r.exec_params("SELECT title, publication_year FROM books WHERE id = $1 ORDER by publication_year, title;"_zv, author_id);
    for (const auto& row : result) {
        books.emplace_back(row[0].as<std::string>(), row[1].as<int>());
    }
    return books;
}

std::vector<ui::detail::BookInfo> BookRepositoryImpl::LoadBooks() {
    std::vector<ui::detail::BookInfo> books;
    pqxx::read_transaction r(connection_);
    auto query_text = "SELECT title, publication_year FROM books ORDER by title;"_zv;
    for (auto [title, publication_year] : r.query<std::string, int>(query_text)) {
        books.emplace_back(title, publication_year);
    }
    return books;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
        CREATE TABLE IF NOT EXISTS authors (
            id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
            name varchar(100) UNIQUE NOT NULL
        );
    )"_zv);
    // ... создать другие таблицы
    work.exec(R"(
        CREATE TABLE IF NOT EXISTS books (
            id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
            author_id UUID NOT NULL,
            title varchar(100) NOT NULL,
            publication_year interger
        );
    )"_zv);
    // коммитим изменения
    work.commit();
}

}  // namespace postgres