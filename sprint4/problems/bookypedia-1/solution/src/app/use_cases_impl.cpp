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
}  // namespace app
