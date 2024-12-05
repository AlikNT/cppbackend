#include "actions.h"

#include <boost/algorithm/string/trim.hpp>
#include <cassert>
#include <iostream>
#include <sstream>

#include "../app/use_cases.h"
#include "../domain/author_fwd.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
namespace detail {

std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    out << book.title << " by " << book.author_name << ", " << book.publication_year;
    return out;
}

}  // namespace detail

template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

Actions::Actions(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} {
    menu_.AddAction("AddAuthor"s, "name"s, "Adds author"s, std::bind(&Actions::AddAuthor, this, ph::_1));
    menu_.AddAction("DeleteAuthor"s, "name"s, "Deletes author"s, std::bind(&Actions::DeleteAuthor, this, ph::_1));
    menu_.AddAction("EditAuthor"s, "name"s, "Edits author"s, std::bind(&Actions::EditAuthor, this, ph::_1));
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
                    std::bind(&Actions::AddBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&Actions::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&Actions::ShowBooks, this));
    menu_.AddAction("ShowBook"s, "title"s, "Show detail info of the book"s, std::bind(&Actions::ShowBook, this, ph::_1));
    menu_.AddAction("DeleteBook"s, "title"s, "Deletes book"s, std::bind(&Actions::DeleteBook, this, ph::_1));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s, std::bind(&Actions::ShowAuthorBooks, this));
}

bool Actions::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        if (name.empty()) {
            output_ << "Failed to add author"sv << std::endl;
            return true;
        }
        use_cases_.AddAuthor(std::move(name));
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool Actions::DeleteAuthor(std::istream &cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        if (name.empty()) {
            const std::optional<detail::AuthorInfo> author_info = SelectAuthorFromList();
            if (!author_info.has_value()) {
                output_ << "Failed to delete author"sv << std::endl;
                return true;
            }
            use_cases_.DeleteAuthor(author_info.value().id);
            return true;
        }
        const auto author_id = FindAuthorByName(name);
        if (!author_id.has_value()) {
            output_ << "Failed to delete author"sv << std::endl;
            return true;
        }
        use_cases_.DeleteAuthor(author_id.value());
    } catch (const std::exception&) {
        output_ << "Failed to delete author"sv << std::endl;
    }
    return true;
}

bool Actions::EditAuthor(std::istream &cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        if (name.empty()) {
            std::optional<detail::AuthorInfo> author_info = SelectAuthorFromList();
            if (!author_info.has_value()) {
                output_ << "Failed to edit author"sv << std::endl;
                return true;
            }
            use_cases_.EditAuthor(author_info.value().id, author_info.value().name);
            return true;
        }
        auto author_id = FindAuthorByName(name);
        if (!author_id.has_value()) {
            output_ << "Failed to edit author"sv << std::endl;
            return true;
        }
        use_cases_.EditAuthor(author_id.value(), name);
    } catch (std::exception&) {
        output_ << "Failed to edit author"sv << std::endl;
    }
    return true;
}

bool Actions::AddBook(std::istream& cmd_input) const {
    try {
        if (const auto params = GetBookParams(cmd_input)) {
            use_cases_.AddBook(*params);
        }
    } catch (const std::exception&) {
        output_ << "Failed to add book"sv << std::endl;
    }
    return true;
}

bool Actions::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
    return true;
}

bool Actions::ShowBooks() const {
    PrintVector(output_, GetBooks());
    return true;
}

bool Actions::ShowBook(std::istream &cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        if (title.empty()) {
            const std::optional<detail::BookInfo> book_info = SelectBookFromList();
            if (!book_info.has_value()) {
                return true;
            }
            PrintBookFull(book_info.value());
            return true;
        }
        std::vector<detail::BookInfo> books = FindBooksByTitle(title);
        if (books.empty()) {
            return true;
        }
        std::optional<detail::BookInfo> book_info;
        if (books.size() == 1) {
            book_info = books.front();
        } else {
            book_info = SelectBookFromList(books);
        }
        PrintBookFull(book_info.value());
    } catch (std::exception&) {
        throw std::runtime_error("Failed to show books");
    }
    return true;
}

bool Actions::DeleteBook(std::istream &cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        if (title.empty()) {
            const std::optional<detail::BookInfo> book_info = SelectBookFromList();
            if (!book_info.has_value()) {
                output_ << "Failed to delete book"sv << std::endl;
                return true;
            }
            use_cases_.DeleteBook(book_info.value().id);
            return true;
        }
        std::vector<detail::BookInfo> books = FindBooksByTitle(title);
        if (books.empty()) {
            output_ << "Failed to delete book"sv << std::endl;
            return true;
        }
        std::optional<detail::BookInfo> book_info;
        if (books.size() == 1) {
            book_info = books.front();
        } else {
            book_info = SelectBookFromList(books);
        }
        use_cases_.DeleteBook(book_info.value().id);
    } catch (std::exception&) {
        output_ << "Failed to delete book"sv << std::endl;
    }
    return true;
}

bool Actions::ShowAuthorBooks() const {
    try {
        if (auto author_info = SelectAuthor()) {
            if (author_info) {
                PrintVector(output_, GetAuthorBooks(author_info.value().id));
            }
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to Show Books");
    }
    return true;
}

std::optional<detail::AddBookParams> Actions::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;

    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);

    auto author_info = SelectAuthor();
    if (not author_info.has_value())
        return std::nullopt;
    else {
        params.author_id = author_info.value().id;
    }
    params.tags = SelectTags();
    return params;
}

std::optional<detail::AuthorInfo> Actions::SelectAuthorFromList() const {
    // output_ << "Select author:" << std::endl;
    auto authors = GetAuthors();
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid author num");
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= authors.size()) {
        throw std::runtime_error("Invalid author num");
    }
    return authors[author_idx];
}


std::optional<detail::AuthorInfo> Actions::SelectAuthorByName(const std::string &name) const {
    auto author_id = FindAuthorByName(name);
    if (author_id) {
        return detail::AuthorInfo{author_id.value(), name};
    }
    output_ << "No author found. Do you want to add Jack London (y/n)?" << std::endl;
    std::string str;
    input_ >> str;
    if (str != "y" && str != "Y") {
        return std::nullopt;
    }
    return detail::AuthorInfo{use_cases_.AddAuthor(name).value(), name};
}

std::optional<detail::AuthorInfo> Actions::SelectAuthor() const {
    output_ << "Enter author name or empty line to select from list:" << std::endl;
    std::string str;
    if (!std::getline(input_, str)) {
        return std::nullopt;
    }
    if (str.empty()) {
        return SelectAuthorFromList();
    }
    return SelectAuthorByName(str);
}

std::string Actions::NormalizeTag(const std::string &tag) {
    std::string result;
    std::ranges::unique_copy(tag, std::back_inserter(result),
                             [](const char a, const char b) {
                                 return std::isspace(a) && std::isspace(b);
                             });
    const size_t start = result.find_first_not_of(' ');
    const size_t end = result.find_last_not_of(' ');
    return (start == std::string::npos) ? "" : result.substr(start, end - start + 1);
}

std::set<std::string> Actions::SelectTags() const {
    output_ << "Enter tags (comma separated):" << std::endl;
    std::string str;
    std::getline(input_, str);
    std::set<std::string> unique_tags;

    std::istringstream stream(str);
    std::string tag;
    while (std::getline(stream, tag, ',')) {
        std::string normalized_tag = NormalizeTag(tag);
        if (!normalized_tag.empty()) {
            unique_tags.insert(normalized_tag);
        }
    }
    return unique_tags;
}

std::vector<detail::AuthorInfo> Actions::GetAuthors() const {
    return use_cases_.GetAuthors();
}

std::optional<std::string> Actions::FindAuthorByName(const std::string &name) const {
    return use_cases_.FindAuthorByName(name);
}

std::vector<detail::BookInfo> Actions::GetBooks() const {
    return use_cases_.GetBooks();
}

std::vector<detail::BookInfo> Actions::GetAuthorBooks(const std::string& author_id) const {
    return use_cases_.GetAuthorBooks(author_id);
}

std::vector<detail::BookInfo> Actions::FindBooksByTitle(const std::string &title) const {
    return use_cases_.FindBooksByTitle(title);
}


std::optional<detail::BookInfo> Actions::SelectBookFromList() const {
    return SelectBookFromList(GetBooks());
}

std::optional<detail::BookInfo> Actions::SelectBookFromList(const std::vector<detail::BookInfo> &books) const {
    PrintVector(output_, books);
    output_ << "Enter book # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }
    int book_idx;
    try {
        book_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid book num");
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= books.size()) {
        throw std::runtime_error("Invalid book num");
    }
    return books[book_idx];
}

void Actions::PrintBook(const detail::BookInfo &book_info) const {
    output_ << "Title: " << book_info.title << std::endl;
    output_ << "Author: " << book_info.author_name << std::endl;
    output_ << "Publication year: " << book_info.publication_year << std::endl;
}

void Actions::PrintBookTags(const std::string &book_id) const {
    const auto tags = use_cases_.GetTagsByBookId(book_id);
    if (tags.empty()) {
        return;
    }
    output_ << "Tags: ";
    for (auto it  = tags.cbegin(); it != tags.cend(); ++it) {
        output_ << *it;
        if (++it != tags.cend()) {
            output_ << ", ";
        }
    }
    output_ << std::endl;
}

void Actions::PrintBookFull(const detail::BookInfo &book_info) const {
    PrintBook(book_info);
    PrintBookTags(book_info.id);
}

}  // namespace ui
