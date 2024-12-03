#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

std::optional<std::string> UseCasesImpl::AddAuthor(std::string name) {
    auto author_id  = AuthorId::New();
    author_repository_.Save({AuthorId::New(), std::move(name)});
    return author_id;
}

void UseCasesImpl::AddBook(const ui::detail::AddBookParams &book_params) {
    unit_of_work_.AddBook(book_params);

std::vector<ui::detail::AuthorInfo> UseCasesImpl::GetAuthors() {
    return author_repository_.LoadAuthors();
}

std::optional<std::string> UseCasesImpl::FindAuthorByName(const std::string &name) {
    return unit_of_work_.FindAuthorByName(name);
}

std::vector<ui::detail::BookInfo> UseCasesImpl::GetAuthorBooks(const std::string &author_id) {
    return book_repository_.LoadAuthorBooks(author_id);
}

std::vector<ui::detail::BookInfo> UseCasesImpl::GetBooks() {
    return book_repository_.LoadBooks();
}
}  // namespace app
