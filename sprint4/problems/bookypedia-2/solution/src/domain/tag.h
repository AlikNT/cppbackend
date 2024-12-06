#pragma once

#include <string>

#include "author.h"
#include "../util/tagged_uuid.h"
#include "book.h"

namespace domain {
/*class BookTags {
public:
    BookTags(BookId book_id, std::set<std::string> tags)
    : book_id_(book_id)
    , tags_(std::move(tags)) {
    }
    [[nodiscard]] const BookId& GetBookId() const {
        return book_id_;
    }
    [[nodiscard]] const std::set<std::string>& GetTags() const {
        return tags_;
    }
private:
    BookId book_id_;
    std::set<std::string> tags_;
};*/

/*
class TagRepository {
public:
    virtual void Save(std::string book_id, const std::set<std::string>& book_tags, std::shared_ptr<pqxx::work> transaction_ptr) = 0;
    virtual void DeleteTagsByBookId(const std::string& book_id, std::shared_ptr<pqxx::work> transaction_ptr) = 0;
    virtual std::vector<std::string> LoadTagsByBookId(const std::string& book_id);
protected:
    ~TagRepository() = default;
};
*/

} // namespace domain