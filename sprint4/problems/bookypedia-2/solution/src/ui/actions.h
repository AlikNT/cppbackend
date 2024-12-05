#pragma once
#include <algorithm>
#include <iosfwd>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace menu {
class Menu;
}

namespace app {
class UseCases;
}

namespace ui {
namespace detail {

struct AddBookParams {
    std::string title;
    std::string author_id;
    int publication_year = 0;
    std::set<std::string> tags;
};

struct AuthorInfo {
    std::string id;
    std::string name;
};

struct BookInfo {
    std::string id;
    std::string title;
    int publication_year;
    std::string author_name;
};

}  // namespace detail

class Actions {
public:
    Actions(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

private:
    bool AddAuthor(std::istream& cmd_input) const;
    bool DeleteAuthor(std::istream& cmd_input) const;
    bool EditAuthor(std::istream& cmd_input) const;
    bool AddBook(std::istream& cmd_input) const;
    bool ShowAuthors() const;
    bool ShowBooks() const;
    bool ShowBook(std::istream &cmd_input) const;
    bool DeleteBook(std::istream &cmd_input) const;
    bool ShowAuthorBooks() const;

    std::optional<detail::AddBookParams> GetBookParams(std::istream& cmd_input) const;
    [[nodiscard]] std::optional<detail::AuthorInfo> SelectAuthorFromList() const;
    [[nodiscard]] std::optional<detail::AuthorInfo> SelectAuthorByName(const std::string &name) const;
    [[nodiscard]] std::optional<detail::AuthorInfo> SelectAuthor() const;
    static std::string NormalizeTag(const std::string &tag);
    [[nodiscard]] std::set<std::string> SelectTags() const;
    [[nodiscard]] std::vector<detail::AuthorInfo> GetAuthors() const;
    [[nodiscard]] std::optional<std::string> FindAuthorByName(const std::string &name) const;
    [[nodiscard]] std::vector<detail::BookInfo> GetBooks() const;
    [[nodiscard]] std::vector<detail::BookInfo> GetAuthorBooks(const std::string& author_id) const;
    [[nodiscard]] std::vector<detail::BookInfo> FindBooksByTitle(const std::string& title) const;
    [[nodiscard]] std::optional<detail::BookInfo> SelectBookFromList(const std::vector<detail::BookInfo>& books) const;
    [[nodiscard]] std::optional<detail::BookInfo> SelectBookFromList() const;
    void PrintBook(const detail::BookInfo& book_info) const;
    void PrintBookTags(const std::string& book_id) const;
    void PrintBookFull(const detail::BookInfo& book_info) const;

    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;
};


}  // namespace ui