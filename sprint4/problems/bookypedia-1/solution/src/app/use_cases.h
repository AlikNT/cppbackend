#pragma once

#include <string>

#include "../domain/author.h"
#include "../ui/view.h"

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(domain::AuthorId author_id, std::string title, int publication_year) = 0;
    virtual std::vector<ui::detail::AuthorInfo> GetAuthors() = 0;
    virtual std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string &author_id) = 0;
    virtual std::vector<ui::detail::BookInfo> GetBooks() = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
