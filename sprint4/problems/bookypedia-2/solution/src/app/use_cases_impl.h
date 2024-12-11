#pragma once
// #include "../domain/author_fwd.h"
// #include "../domain/book_fwd.h"
#include "../postgres/postgres.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(postgres::UnitOfWork& unit_of_work)
        : unit_of_work_{unit_of_work} {
    }

    std::optional<std::string> AddAuthor(std::string name) override;
    void DeleteAuthor(const std::string &author_id) override;
    void EditAuthor(std::string &name, std::string &new_name) override;
    void AddBook(const ui::detail::AddBookParams &book_params) override;
    std::vector<ui::detail::AuthorInfo> GetAuthors() override;
    std::optional<std::string> FindAuthorByName(const std::string& name) override;
    std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string &author_id) override;
    std::vector<ui::detail::BookInfo> GetBooks() override;
    std::vector<ui::detail::BookInfo> FindBooksByTitle(const std::string &title) override;
    std::vector<std::string> GetTagsByBookId(const std::string& book_id) override;
    void DeleteBook(const std::string &book_id) override;
    void EditBook(const ui::detail::BookInfo &new_book_info, const std::set<std::string> & new_tags) override;
    void ChangeAuthorInBooks(const std::string & author_id, const std::string & new_author_id) override;

private:
    postgres::UnitOfWork& unit_of_work_;
};

}  // namespace app
