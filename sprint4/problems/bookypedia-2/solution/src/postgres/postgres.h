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

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(::std::string author_id, ::std::string name, std::shared_ptr<pqxx::work> transaction_ptr) override;
    std::vector<ui::detail::AuthorInfo> LoadAuthors() override;
    [[nodiscard]] std::optional<std::string> FindAuthorByName(const std::string& name) const;
    void DeleteAuthor(const std::string& author_id, const std::shared_ptr<pqxx::work>& transaction_ptr);
    void EditAuthor(const std::string& author_id, const std::string &name, const std::shared_ptr<pqxx::work>& transaction_ptr);

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
    void DeleteBooksByAuthorId(const std::string &author_id, std::shared_ptr<pqxx::work> transaction_ptr) override;
    std::vector<ui::detail::BookInfo> LoadAuthorBooks(const std::string &author_id) override;
    std::vector<ui::detail::BookInfo> LoadBooks() override;
    [[nodiscard]] std::vector<std::string> FindBooksIdByAuthorId(const std::string &author_id) const override;

private:
    pqxx::connection& connection_;
};

class TagRepositoryImpl : public  domain::TagRepository {
public:
    explicit TagRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }
    void Save(std::string book_id, const std::set<std::string>& book_tags, std::shared_ptr<pqxx::work> transaction) override;
    void DeleteTagsByBookId(const std::string& book_id, std::shared_ptr<pqxx::work> transaction_ptr) override;

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
    void AddAuthor(std::string author_id, std::string name) {
        Start();
        author_repository_.Save(std::move(author_id), std::move(name), transaction_ptr_);
        Commit();
    }
    void DeleteAuthor(const std::string& author_id) {
        Start();
        author_repository_.DeleteAuthor(author_id, transaction_ptr_);
        for (const auto& book_id: book_repository_.FindBooksIdByAuthorId(author_id) {
            tag_repository_.DeleteTagsByBookId(book_id, transaction_ptr_);
        }
        book_repository_.DeleteBooksByAuthorId(author_id, transaction_ptr_);
        Commit();
    }
    void EditAuthor(const std::string& author_id, const std::string &name) {
        Start();
        author_repository_.EditAuthor(author_id, name, transaction_ptr_);
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
        Commit();
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