#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../domain/author.h"
#include "../domain/book.h"
#include <tuple>

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} 
    {}

    void Save(const domain::Author& author) override;
    std::vector<domain::Author> GetAuthors() override;
    void Delete(std::string& name) override;
    void Edit(std::string& new_name, std::string& old_name) override;

private:
    pqxx::connection& connection_;
};

class BookRepositoryImpl : public domain::BookRepository
{
public:
    explicit BookRepositoryImpl(pqxx::connection& connection)
        : connection_{ connection }
    {}

    void Save(const domain::Book& book) override;
    std::vector<std::tuple<std::string, std::string, int, std::string>> ShowBooks() override;
    std::vector<domain::Book> GetAuthorBooks(const std::string& author_id) override;
    std::vector<std::tuple<std::string, std::string, int, std::string, std::set<std::string>>> ShowBook(std::string& book_name) override;
    void DeleteBook(std::string& book_id) override;
    void EditBook(std::string& title, int publication_year, std::set<std::string> tags, std::string& id) override;

private:
    pqxx::connection& connection_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return authors_;
    }

    BookRepositoryImpl& GetBooks() & {
        return books_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
    BookRepositoryImpl books_{ connection_ };
};

}  // namespace postgres