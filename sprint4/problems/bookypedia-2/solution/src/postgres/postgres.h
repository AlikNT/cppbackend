#pragma once
#include <pqxx/pqxx>

#include "../ui/actions.h"

namespace postgres {

class AuthorRepositoryImpl {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const ::std::string& author_id, const ::std::string& name, const std::shared_ptr<pqxx::work>& transaction_ptr);
    std::vector<ui::detail::AuthorInfo> LoadAuthors();
    [[nodiscard]] std::optional<std::string> FindAuthorByName(const std::string& name, const std::shared_ptr<pqxx::work>& transaction_ptr) const;
    void DeleteAuthor(const std::string& author_id, const std::shared_ptr<pqxx::work>& transaction_ptr);
    void EditAuthor(const std::string& name, const std::string &new_name, const std::shared_ptr<pqxx::work>& transaction_ptr);

private:
    pqxx::connection& connection_;
};

class BookRepositoryImpl {
public:
    explicit BookRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const std::string& book_id, const std::string& author_id, const std::string& title, int publication_year, const std::shared_ptr<pqxx::work>&
              transaction_ptr);
    void DeleteBooksByAuthorId(const std::string &author_id, const std::shared_ptr<pqxx::work>& transaction_ptr);

    std::vector<ui::detail::BookInfo> LoadAuthorBooks(const std::string &author_id);
    std::vector<ui::detail::BookInfoWithAuthor> LoadBooks();
    [[nodiscard]] std::vector<std::string> FindBooksIdByAuthorId(const std::string &author_id) const;
    [[nodiscard]] std::vector<ui::detail::BookInfoWithAuthor> FindBooksByTitle(const std::string& title) const;
    void DeleteBook(const std::string& book_id, const std::shared_ptr<pqxx::work>& transaction_ptr);
    void EditBook(const ui::detail::BookInfoWithAuthor & book_info, const std::shared_ptr<pqxx::work>& transaction_ptr);
    void ChangeAuthor(const std::string & author_id, const std::string & new_author_id, const std::shared_ptr<pqxx::work>& transaction_ptr);

private:
    pqxx::connection& connection_;
};

class TagRepositoryImpl {
public:
    explicit TagRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }
    void Save(const std::string& book_id, const std::set<std::string>& book_tags, const std::shared_ptr<pqxx::work>& transaction_ptr);
    void DeleteTagsByBookId(const std::string& book_id, const std::shared_ptr<pqxx::work>& transaction_ptr);
    std::vector<std::string> LoadTagsByBookId(const std::string& book_id);

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
    std::vector<ui::detail::BookInfoWithAuthor> GetBooks();
    void AddAuthor(const std::string& author_id, const std::string& name);
    void DeleteAuthor(const std::string& author_id);
    void EditAuthor(const std::string& name, const std::string &new_name);
    std::optional<std::string> FindAuthorByName(const std::string& name);
    void AddBook(const ui::detail::AddBookParams& book_params);
    std::vector<ui::detail::BookInfoWithAuthor> FindBooksByTitle(const std::string& title);
    std::vector<std::string> GetTagsByBookId(const std::string& book_id);
    void DeleteBook(const std::string& book_id);

    std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string &author_id);
    void EditBook(const ui::detail::BookInfoWithAuthor &new_book_info, const std::set<std::string>& new_tags);
    void ChangeAuthorInBooks(const std::string & author_id, const std::string & new_author_id);

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
    UnitOfWork& GetUnitOfWork() &;

private:
    pqxx::connection connection_;
    UnitOfWork unit_of_work_{connection_};
};

}  // namespace postgres