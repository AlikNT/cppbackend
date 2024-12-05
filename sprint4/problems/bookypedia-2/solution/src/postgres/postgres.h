#pragma once
#include <pqxx/pqxx>

#include "../domain/author.h"
#include "../domain/book.h"
#include "../domain/tag.h"

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
    [[nodiscard]] std::vector<ui::detail::BookInfo> FindBooksByTitle(const std::string& title) const;
    void DeleteBook(const std::string& book_id, const std::shared_ptr<pqxx::work>& transaction_ptr);

private:
    pqxx::connection& connection_;
};

class TagRepositoryImpl : public  domain::TagRepository {
public:
    explicit TagRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }
    void Save(std::string book_id, const std::set<std::string>& book_tags, std::shared_ptr<pqxx::work> transaction_ptr) override;
    void DeleteTagsByBookId(const std::string& book_id, std::shared_ptr<pqxx::work> transaction_ptr) override;
    std::vector<std::string> LoadTagsByBookId(const std::string& book_id) override;


private:
    pqxx::connection& connection_;
};

class UnitOfWork {
public:
    explicit UnitOfWork(pqxx::connection& connection);

    void Start();
    void Commit();
    void Rollback();
    [[nodiscard]] bool isActive() const;

    AuthorRepositoryImpl& GetAuthorsRepository() &;
    BookRepositoryImpl& GetBooksRepository() &;
    std::vector<ui::detail::AuthorInfo> GetAuthors();
    std::vector<ui::detail::BookInfo> GetBooks();
    void AddAuthor(std::string author_id, std::string name);
    void DeleteAuthor(const std::string& author_id);
    void EditAuthor(const std::string& author_id, const std::string &name);
    [[nodiscard]] std::optional<std::string> FindAuthorByName(const std::string& name) const;
    void AddBook(const ui::detail::AddBookParams& book_params);
    std::vector<ui::detail::BookInfo> FindBooksByTitle(const std::string& title);
    std::vector<std::string> GetTagsByBookId(const std::string& book_id);
    void DeleteBook(const std::string& book_id);
    std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string& author_id);

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

    AuthorRepositoryImpl& GetAuthors() &;
    BookRepositoryImpl& GetBooks() &;

private:
    pqxx::connection connection_;
    UnitOfWork unit_of_work_{connection_};
};


}  // namespace postgres