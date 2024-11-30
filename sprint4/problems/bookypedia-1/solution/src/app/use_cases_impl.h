#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
        : authors_{authors}
        , books_{books} {
    }

    void AddAuthor(std::string name) override;
    void AddBook(domain::AuthorId author_id, std::string title, int publication_year) override;

    std::vector<ui::detail::AuthorInfo> GetAuthors() override;

    std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string &author_id) override;

    std::vector<ui::detail::BookInfo> GetBooks() override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};

}  // namespace app
