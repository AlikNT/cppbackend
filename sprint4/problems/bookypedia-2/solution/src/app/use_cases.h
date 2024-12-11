#pragma once

#include <string>

#include "../ui/actions.h"

namespace app {

class UseCases {
public:
    virtual std::optional<std::string> AddAuthor(std::string name) = 0;
    virtual void DeleteAuthor(const std::string &author_id) = 0;
    virtual void EditAuthor(std::string &name, std::string &new_name) = 0;
    virtual void AddBook(const ui::detail::AddBookParams &book_params) = 0;
    virtual std::vector<ui::detail::AuthorInfo> GetAuthors() = 0;
    virtual std::optional<std::string> FindAuthorByName(const std::string& name) = 0;
    virtual std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string &author_id) = 0;
    virtual std::vector<ui::detail::BookInfo> GetBooks() = 0;
    virtual std::vector<ui::detail::BookInfo> FindBooksByTitle(const std::string &title) = 0;
    virtual std::vector<std::string> GetTagsByBookId(const std::string& book_id) = 0;
    virtual void DeleteBook(const std::string &book_id) = 0;
    virtual void EditBook(const ui::detail::BookInfo &new_book_info, const std::set<std::string> & new_tags) = 0;
    virtual void ChangeAuthorInBooks(const std::string & author_id, const std::string & new_author_id) = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
