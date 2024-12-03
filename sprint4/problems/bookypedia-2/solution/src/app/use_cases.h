#pragma once

#include <string>

#include "../domain/author.h"
#include "../ui/actions.h"

namespace app {

class UseCases {
public:
    virtual std::optional<std::string> AddAuthor(std::string name) = 0;
    virtual void AddBook(const ui::detail::AddBookParams &book_params) = 0;
    virtual std::vector<ui::detail::AuthorInfo> GetAuthors() = 0;
    virtual std::optional<std::string> FindAuthorByName(const std::string& name) = 0;
    virtual std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string &author_id) = 0;
    virtual std::vector<ui::detail::BookInfo> GetBooks() = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
