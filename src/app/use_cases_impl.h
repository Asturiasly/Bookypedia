#pragma once
#include <optional>
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "use_cases.h"
#include <set>
#include <tuple>

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
        : authors_{authors},
          books_{books}
    {}

    void AddAuthor(const std::string& name) override;
    void DeleteAuthor(std::string& name) override;
    void EditAuthor(std::string& new_name, std::string& old_name) override;
    void AddBook(int year, const std::string& title, domain::AuthorId id, std::optional<std::set<std::string>>) override;
    std::vector<domain::Author> GetAuthors() override;
    std::vector<std::tuple<std::string, std::string, int, std::string>> ShowBooks() override;
    std::vector<domain::Book> GetAuthorBooks(const std::string& author_id) override;
    std::vector<std::tuple<std::string, std::string, int, std::string, std::set<std::string>>> ShowBook(std::string& book_name) override;
    void DeleteBook(std::string& id) override;
    void EditBook(std::string& title, int publication_year, std::set<std::string> tags, std::string& id) override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};

}  // namespace app
