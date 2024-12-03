#include <catch2/catch_test_macros.hpp>

#include "../src/app/use_cases_impl.h"
#include "../src/domain/author.h"
#include "../src/domain/book.h"

#include
#include
#include
#include
#include
#include 
#include 
#include 

namespace {

struct MockAuthorRepository : domain::AuthorRepository {
    std::vector<domain::Author> saved_authors;

    void Save(const domain::Author& author, const std::shared_ptr<pqxx::work> transaction_ptr) override {
        saved_authors.emplace_back(author);
    }
    std::vector<ui::detail::AuthorInfo> LoadAuthors() override {
        return {};
    }
};

struct MockBookRepository : domain::BookRepository {
    std::vector<domain::Book> saved_books;

    void Save(std::string book_id, std::string author_id, std::string title, int publication_year, std::shared_ptr<pqxx::work>
              transaction_ptr) override {
        saved_books.emplace_back(book_id);
    }
    std::vector<ui::detail::BookInfo> LoadAuthorBooks(const std::string &author_id) override {
        return {};
    }
    std::vector<ui::detail::BookInfo> LoadBooks() override {
        return {};
    }
};

struct Fixture {
    MockAuthorRepository authors;
    MockBookRepository books;
};

}  // namespace

SCENARIO_METHOD(Fixture, "Book Adding") {
    GIVEN("Use cases") {
        app::UseCasesImpl use_cases{authors, books};

        WHEN("Adding an author") {
            const auto author_name = "Joanne Rowling";
            use_cases.AddAuthor(author_name);
            const auto book_title = "Harry Potter";
            int publication_year = 2012;
            use_cases.AddBook(TODO);

            THEN("author with the specified name is saved to repository") {
                REQUIRE(authors.saved_authors.size() == 1);
                CHECK(authors.saved_authors.at(0).GetName() == author_name);
                CHECK(authors.saved_authors.at(0).GetId() != domain::AuthorId{});
                REQUIRE(books.saved_books.size() == 1);
                CHECK(books.saved_books.at(0).GetTitle() == book_title);
                CHECK(books.saved_books.at(0).GetId() != domain::BookId{});
                CHECK(books.saved_books.at(0).GetPublicationYear() == 2012);
                CHECK(books.saved_books.at(0).GetAuthorId() == authors.saved_authors.front().GetId());
            }
        }
    }
}