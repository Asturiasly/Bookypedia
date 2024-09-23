#pragma once

#include <optional>
#include <string>
#include <set>
#include <tuple>
#include "../domain/author.h"
#include "../domain/book.h"

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(int year, const std::string& title, domain::AuthorId id, std::optional<std::set<std::string>> tags) = 0;
    virtual void DeleteAuthor(std::string& name) = 0;
    virtual void EditAuthor(std::string& new_name, std::string& old_name) = 0;
    virtual std::vector<domain::Author> GetAuthors() = 0;
    virtual std::vector<std::tuple<std::string, std::string, int, std::string>> ShowBooks() = 0;
    virtual std::vector<std::tuple<std::string, std::string, int, std::string, std::set<std::string>>> ShowBook(std::string& book_name) = 0;
    virtual std::vector<domain::Book> GetAuthorBooks(const std::string& author_id) = 0;
    virtual void DeleteBook(std::string& book_id) = 0;
    virtual void EditBook(std::string& title, int publication_year, std::set<std::string> tags, std::string& id) = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
