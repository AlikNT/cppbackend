#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "../postgres/postgres.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(postgres::UnitOfWork& unit_of_work)
        : unit_of_work_{unit_of_work} {
    }

    std::optional<std::string> AddAuthor(std::string name) override;
    void DeleteAuthor(const std::string &name) override;
    void AddBook(const ui::detail::AddBookParams &book_params) override;
    std::vector<ui::detail::AuthorInfo> GetAuthors() override;
    std::optional<std::string> FindAuthorByName(const std::string& name) override;
    std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string &author_id) override;
    std::vector<ui::detail::BookInfo> GetBooks() override;

private:
    postgres::UnitOfWork& unit_of_work_;
};

}  // namespace app
