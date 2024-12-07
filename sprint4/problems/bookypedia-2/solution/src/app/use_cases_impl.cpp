#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

std::optional<std::string> UseCasesImpl::AddAuthor(std::string name) {
    auto author_id  = AuthorId::New();
    unit_of_work_.AddAuthor(author_id.ToString(), std::move(name));
    return author_id.ToString();
}

void UseCasesImpl::DeleteAuthor(const std::string &author_id) {
    unit_of_work_.DeleteAuthor(author_id);
}

void UseCasesImpl::EditAuthor(std::string &author_id, std::string &name) {
    unit_of_work_.EditAuthor(author_id, name);
}

void UseCasesImpl::AddBook(const ui::detail::AddBookParams &book_params) {
    unit_of_work_.AddBook(book_params);
}

std::vector<ui::detail::AuthorInfo> UseCasesImpl::GetAuthors() {
    return unit_of_work_.GetAuthors();
}

std::optional<std::string> UseCasesImpl::FindAuthorByName(const std::string &name) {
    return unit_of_work_.FindAuthorByName(name);
}

std::vector<ui::detail::BookInfo> UseCasesImpl::GetAuthorBooks(const std::string &author_id) {
    return unit_of_work_.GetAuthorBooks(author_id);
}

std::vector<ui::detail::BookInfo> UseCasesImpl::GetBooks() {
    return unit_of_work_.GetBooks();
}

std::vector<ui::detail::BookInfo> UseCasesImpl::FindBooksByTitle(const std::string &title) {
    return unit_of_work_.FindBooksByTitle(title);
}

std::vector<std::string> UseCasesImpl::GetTagsByBookId(const std::string &book_id) {
    return unit_of_work_.GetTagsByBookId(book_id);
}

void UseCasesImpl::DeleteBook(const std::string &book_id) {
    unit_of_work_.DeleteBook(book_id);
}
}  // namespace app
