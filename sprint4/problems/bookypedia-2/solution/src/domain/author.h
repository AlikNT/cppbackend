#pragma once
// #include <string>
// #include <vector>
// #include <pqxx/transaction.hxx>
// #include <pqxx/pqxx>
//
#include "../util/tagged_uuid.h"
// #include "../ui/actions.h"

namespace domain {

namespace detail {
struct AuthorTag {};
}  // namespace detail

using AuthorId = util::TaggedUUID<detail::AuthorTag>;
/*
class Author {
public:
    Author(AuthorId id, std::string name)
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const AuthorId& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

private:
    AuthorId id_;
    std::string name_;
};

class AuthorRepository {
public:
    virtual void Save(::std::string author_id, ::std::string name, std::shared_ptr<pqxx::work> transaction_ptr) = 0;
    virtual std::vector<ui::detail::AuthorInfo> LoadAuthors() = 0;

protected:
    ~AuthorRepository() = default;
};
*/


}  // namespace domain
