#pragma once
#include <string>
#include <optional>
#include <set>
#include <tuple>

#include "author.h"
#include "../util/tagged_uuid.h"

namespace domain {

    namespace detail {
        struct BookTag {};
    }  // namespace detail

    using BookId = util::TaggedUUID<detail::BookTag>;

    class Book {
    public:
        Book(BookId id, AuthorId a_id, std::string title, int year, std::optional<std::set<std::string>> tags)
            : b_id_(std::move(id)),
              a_id_(std::move(a_id)),
              title_(title),
              publication_year_(year),
              tags_(tags)
        {}

        const BookId& GetBookId() const noexcept {
            return b_id_;
        }

        const AuthorId& GetAuthorId() const noexcept {
            return a_id_;
        }

        const std::string& GetTitle() const noexcept {
            return title_;
        }

        const int GetPublicationYear() const noexcept {
            return publication_year_;
        }

        const std::optional<std::set<std::string>>& GetTags() const noexcept {
            return tags_;
        }

    private:
        BookId b_id_;
        AuthorId a_id_;
        std::string title_;
        int publication_year_;
        std::optional<std::set<std::string>> tags_;
    };

    class BookRepository {
    public:
        virtual void Save(const Book& book) = 0;
        virtual std::vector<std::tuple<std::string, std::string, int, std::string>> ShowBooks() = 0;
        virtual std::vector<domain::Book> GetAuthorBooks(const std::string& author_id) = 0;
        virtual std::vector<std::tuple<std::string, std::string, int, std::string, std::set<std::string>>> ShowBook(std::string& book_name) = 0;
        virtual void DeleteBook(std::string& book_id) = 0;
        virtual void EditBook(std::string& title, int publication_year, std::set<std::string> tags, std::string& id) = 0;

    protected:
        ~BookRepository() = default;
    };

}  // namespace domain
