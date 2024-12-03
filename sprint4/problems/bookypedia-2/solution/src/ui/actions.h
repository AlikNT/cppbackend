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
    std::string title;
    int publication_year;
};

}  // namespace detail

class Actions {
public:
    Actions(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

private:
    bool AddAuthor(std::istream& cmd_input) const;
    bool AddBook(std::istream& cmd_input) const;
    bool ShowAuthors() const;
    bool ShowBooks() const;
    bool ShowAuthorBooks() const;

    std::optional<detail::AddBookParams> GetBookParams(std::istream& cmd_input) const;
    std::optional<std::string> SelectAuthorFromList() const;
    std::optional<std::string> SelectAuthorByName(const std::string &name) const;
    std::optional<std::string> SelectAuthor() const;
    static std::string NormalizeTag(const std::string &tag);
    [[nodiscard]] std::set<std::string> SelectTags() const;
    std::vector<detail::AuthorInfo> GetAuthors() const;
    std::optional<std::string> FindAuthorByName(const std::string &name) const;
    std::vector<detail::BookInfo> GetBooks() const;
    std::vector<detail::BookInfo> GetAuthorBooks(const std::string& author_id) const;

    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;
};

}  // namespace ui