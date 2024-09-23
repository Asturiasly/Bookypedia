#include <vector>
#include <optional>
#include <tuple>
#include "use_cases_impl.h"
#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void app::UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

void app::UseCasesImpl::DeleteAuthor(std::string& name)
{
    authors_.Delete(name);
}

void app::UseCasesImpl::EditAuthor(std::string& new_name, std::string& old_name)
{
    authors_.Edit(new_name, old_name);
}

void app::UseCasesImpl::AddBook(int year, const std::string& title, domain::AuthorId id, std::optional<std::set<std::string>> tags)
{
    books_.Save({ BookId::New(), id, title, year, tags });
}

std::vector<domain::Author> app::UseCasesImpl::GetAuthors()
{
    return authors_.GetAuthors();
}

std::vector<std::tuple<std::string, std::string, int, std::string>> app::UseCasesImpl::ShowBooks()
{
    return books_.ShowBooks();
}

std::vector<domain::Book> app::UseCasesImpl::GetAuthorBooks(const std::string& author_id)
{
    return books_.GetAuthorBooks(author_id);
}

std::vector<std::tuple<std::string, std::string, int, std::string, std::set<std::string>>> app::UseCasesImpl::ShowBook(std::string& book_name)
{
    return books_.ShowBook(book_name);
}

void app::UseCasesImpl::DeleteBook(std::string& book_id)
{
    books_.DeleteBook(book_id);
}

void app::UseCasesImpl::EditBook(std::string& title, int publication_year, std::set<std::string> tags, std::string& id)
{
    books_.EditBook(title, publication_year, tags, id);
}

}  // namespace app
