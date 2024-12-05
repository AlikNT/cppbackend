#include "postgres.h"

#include <pqxx/zview.hxx>

#include "../domain/book.h"

#include
#include

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(std::string author_id, std::string name, const std::shared_ptr<pqxx::work> transaction_ptr) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    pqxx::result result = transaction_ptr->exec_params("SELECT id FROM authors WHERE name=$1"_zv, std::move(name));
    if (!result.empty()) {
        throw std::runtime_error("Failed to add author");
    }
    transaction_ptr->exec_params(
        R"(
            INSERT INTO authors (id, name) VALUES ($1, $2)
            ON CONFLICT (id) DO UPDATE SET name=$2;
        )"_zv,
            std::move(author_id), std::move(name));
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

std::optional<std::string> AuthorRepositoryImpl::FindAuthorByName(const std::string &name) const {
    pqxx::read_transaction r(connection_);
    pqxx::result result = r.exec_params("SELECT id FROM authors WHERE name = ;"_zv, name);
    if (result.empty()) {
        return std::nullopt;
    }
    return result[0]["name"].get<std::string>();
}

void AuthorRepositoryImpl::DeleteAuthor(const std::string& author_id, const std::shared_ptr<pqxx::work>& transaction_ptr) {
    transaction_ptr->exec_params(
        R"(
            DELETE FROM authors WHERE author_id = $1;
        )"_zv, author_id
    );
}

void AuthorRepositoryImpl::EditAuthor(const std::string &author_id, const std::string &name, const std::shared_ptr<pqxx::work> &transaction_ptr) {
    transaction_ptr->exec_params(
        R"(UPDATE authors SET name = $1 WHERE id = $2;)"_zv, name, author_id
    );
}

void BookRepositoryImpl::Save(std::string book_id, std::string author_id, std::string title, int publication_year, const std::shared_ptr<pqxx::work>
                              transaction_ptr) {
    transaction_ptr->exec_params(
        R"(
            INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
            ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
        )"_zv, std::move(book_id), std::move(author_id), std::move(title), publication_year
    );
}

void BookRepositoryImpl::DeleteBooksByAuthorId(const std::string &author_id, const std::shared_ptr<pqxx::work> transaction_ptr) {
    transaction_ptr->exec_params(
        R"(
            DELETE FROM books WHERE author_id = $1;
        )"_zv, author_id
    );
}

std::vector<ui::detail::BookInfo> BookRepositoryImpl::LoadAuthorBooks(const std::string &author_id) {
    std::vector<ui::detail::BookInfo> books;
    pqxx::read_transaction r(connection_);
    pqxx::result result = r.exec_params("SELECT title, publication_year FROM books WHERE author_id = $1 ORDER by publication_year, title;"_zv, author_id);
    for (const auto& row : result) {
        books.emplace_back(row[0].as<std::string>(), row[1].as<int>());
    }
    return books;
}

std::vector<ui::detail::BookInfo> BookRepositoryImpl::LoadBooks() {
    std::vector<ui::detail::BookInfo> books;
    pqxx::read_transaction r(connection_);
    auto query_text = "SELECT title, publication_year FROM books ORDER by title;"_zv;
    for (const auto& [title, publication_year] : r.query<std::string, int>(query_text)) {
        books.emplace_back(title, publication_year);
    }
    return books;
}

std::vector<std::string> BookRepositoryImpl::FindBooksIdByAuthorId(const std::string &author_id) const {
    std::vector<std::string> book_ids;
    pqxx::read_transaction r(connection_);
    pqxx::result result = r.exec_params("SELECT id FROM books WHERE author_id = $1;"_zv, author_id);
    for (const auto& row : result) {
        book_ids.emplace_back(row["id"].as<std::string>());
    }
    return book_ids;
}

void TagRepositoryImpl::Save(std::string book_id, const std::set<std::string>& book_tags, const std::shared_ptr<pqxx::work> transaction_ptr) {
    for (const auto& book_tag : book_tags) {
        transaction_ptr->exec_params(R"(INSERT INTO tags (book_id, tag) VALUES ($1, $2);)"_zv, std::move(book_id), book_tag);
    }
}

void TagRepositoryImpl::DeleteTagsByBookId(const std::string &book_id, const std::shared_ptr<pqxx::work> transaction_ptr) {
    transaction_ptr->exec_params(
        R"(
            DELETE FROM tags WHERE book_id = $1;
        )"_zv, book_id
    );
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
            publication_year integer
        );
    )"_zv);
    work.exec(R"(
        CREATE TABLE IF NOT EXIST book_tags (
            book_id UUID NOT NULL,
            tag varchar(30)
        );
    )"_zv);
    // коммитим изменения
    work.commit();
}

}  // namespace postgres