#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

void UseCasesImpl::AddBook(AuthorId author_id, std::string title, int publication_year) {
    books_.Save({BookId::New(), author_id, title, publication_year});
}

std::vector<ui::detail::AuthorInfo> UseCasesImpl::GetAuthors() {
    return authors_.LoadAuthors();
}

std::vector<ui::detail::BookInfo> UseCasesImpl::GetAuthorBooks(const std::string &author_id) {
    return books_.LoadAuthorBooks(author_id);
}

std::vector<ui::detail::BookInfo> UseCasesImpl::GetBooks() {
    return books_.LoadBooks();
}
}  // namespace app
