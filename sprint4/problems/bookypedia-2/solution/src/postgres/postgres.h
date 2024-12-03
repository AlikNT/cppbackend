#pragma once
#include <pqxx/pqxx>

#include "../domain/author.h"
#include "../domain/book.h"
#include "../domain/tag.h"

#include
#include
#include
#include
#include
#include
#include
#include
#include
#include
#include
#include

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Author& author, const std::shared_ptr<pqxx::work> transaction_ptr) override;

    std::vector<ui::detail::AuthorInfo> LoadAuthors() override;
    [[nodiscard]] std::optional<std::string> FindAuthorByName(const std::string& name) const;

private:
    pqxx::connection& connection_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(std::string book_id, std::string author_id, std::string title, int publication_year, std::shared_ptr<pqxx::work>
              transaction_ptr) override;

    std::vector<ui::detail::BookInfo> LoadAuthorBooks(const std::string &author_id) override;

    std::vector<ui::detail::BookInfo> LoadBooks() override;

private:
    pqxx::connection& connection_;
};

class TagRepositoryImpl : public  domain::TagRepository {
public:
    explicit TagRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }
    void Save(std::string book_id, const std::set<std::string>& book_tags, std::shared_ptr<pqxx::work> transaction) override;

private:
    pqxx::connection& connection_;
};

class UnitOfWork {
public:
    explicit UnitOfWork(pqxx::connection& connection)
        : connection_{connection} {
    }
    void Start() {
        if (transaction_ptr_) {
            throw std::runtime_error("A transaction is already active.");
        }
        transaction_ptr_ = std::make_unique<pqxx::work>(connection_);
    }
    void Commit() {
        if (!transaction_ptr_) {
            throw std::runtime_error("No active transaction to commit.");
        }
        transaction_ptr_->commit();
        transaction_ptr_.reset();
    }
    void Rollback() {
        if (!transaction_ptr_) {
            throw std::runtime_error("No active transaction to rollback.");
        }
        transaction_ptr_->abort();
        transaction_ptr_.reset();
    }
    [[nodiscard]] bool isActive() const {
        return transaction_ptr_ != nullptr;
    }
    AuthorRepositoryImpl& GetAuthors() & {
        return author_repository_;
    }
    BookRepositoryImpl& GetBooks() & {
        return book_repository_;
    }
    void SaveAuthor(std::string name) {
        Start();
        author_repository_.Save({domain::AuthorId::New(), std::move(name)}, transaction_ptr_);
        Commit();
    }
    [[nodiscard]] std::optional<std::string> FindAuthorByName(const std::string& name) const {
        return author_repository_.FindAuthorByName(name);
    }
    void AddBook(const ui::detail::AddBookParams& book_params) {
        Start();
        auto book_id = domain::BookId::New();
        book_repository_.Save(book_id.ToString(), book_params.author_id, book_params.title, book_params.publication_year, transaction_ptr_);
        tag_repository_.Save(book_id.ToString(), book_params.tags, transaction_ptr_);
    }

private:
    pqxx::connection& connection_;
    AuthorRepositoryImpl author_repository_{connection_};
    BookRepositoryImpl book_repository_{connection_};
    TagRepositoryImpl tag_repository_{connection_};
    std::shared_ptr<pqxx::work> transaction_ptr_ = nullptr;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return unit_of_work_.GetAuthors();
    }

    BookRepositoryImpl& GetBooks() & {
        return unit_of_work_.GetBooks();
    }

private:
    pqxx::connection connection_;
    UnitOfWork unit_of_work_{connection_};
};


}  // namespace postgres