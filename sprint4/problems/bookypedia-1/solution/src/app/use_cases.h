#pragma once

#include <string>

#include "../domain/author.h"

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(domain::AuthorId autor_id, std::string title, int publication_year) = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
