#include "postgres.h"
#include "../util/tagged_uuid.h"
#include <pqxx/zview.hxx>
#include <pqxx/pqxx>
#include "../app/use_cases.h"
#include <iostream>
#include <string>
#include <vector>
#include <tuple>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    pqxx::work work{connection_};
    work.exec("BEGIN;");
    work.exec("LOCK table authors");
    work.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2
)"_zv,
        author.GetId().ToString(), author.GetName());
    work.exec("END;");
    work.commit();
}

std::vector<domain::Author> postgres::AuthorRepositoryImpl::GetAuthors()
{
    std::vector<domain::Author> authors;
    pqxx::read_transaction r{ connection_ };

    for (const auto& [id, name] : r.query<std::string, std::string>("SELECT id, name FROM authors ORDER BY name;"_zv))
    {
        auto author_id_t = util::TaggedUUID<domain::detail::AuthorTag>::FromString(id);
        authors.emplace_back(author_id_t, name);
    }

    return authors;
}

void postgres::AuthorRepositoryImpl::Delete(std::string& name)
{
    //pqxx::read_transaction{ connection_ };
    pqxx::work work{ connection_ };
    std::vector<std::string> books_id;
    work.exec("BEGIN;");
    work.exec("LOCK TABLE authors, books, book_tags");
    std::string author_id = work.query_value<std::string>("SELECT id FROM authors WHERE name = " + work.quote(name));

    if (author_id.empty())
        throw std::runtime_error("");

    work.exec("DELETE FROM authors WHERE id = " + work.quote(author_id));
    for (auto [book_id] : work.query<std::string>("SELECT id FROM books WHERE author_id = " + work.quote(author_id)))
    {
        books_id.push_back(book_id);
    }

    work.exec("DELETE FROM books WHERE author_id = " + work.quote(author_id));

    for (const auto& book_id : books_id)
    {
        work.exec("DELETE FROM book_tags WHERE book_id = " + work.quote(book_id));
    }

    work.exec("END;");
    work.commit();
}

void postgres::AuthorRepositoryImpl::Edit(std::string& new_name, std::string& old_name)
{
    pqxx::work work{ connection_ };
    work.exec("BEGIN;");
    work.exec("LOCK TABLE authors");
    std::string author_id = work.query_value<std::string>("SELECT id FROM authors WHERE name = " + work.quote(old_name));
    if (author_id.empty())
        throw std::runtime_error("");

    work.exec("UPDATE authors SET name =" + work.quote(new_name) + " WHERE name = " + work.quote(old_name));
    work.exec("END;");
    work.commit();
}

void BookRepositoryImpl::Save(const domain::Book& book)
{
    pqxx::work work{ connection_ };
    
    if (!book.GetTags().has_value())
    {
        work.exec_params(
            R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
)"_zv,
book.GetBookId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetPublicationYear());
        work.commit();
        return;
    }
    
    work.exec("BEGIN;");
    work.exec("LOCK TABLE books, book_tags");

    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
)"_zv,
book.GetBookId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetPublicationYear());

    std::string query = "INSERT INTO book_tags(book_id, tag) VALUES ";
    auto book_id = book.GetBookId().ToString();
    auto tags = book.GetTags().value();
    for (auto tag = tags.begin(); tag != tags.end(); ++tag)
    {
        if (tag == std::prev(tags.end()))
        {
            query.append("('" + book_id + "', '" + *tag + "')");
            break;
        }
        query.append("('" + book_id + "', '" + *tag + "'), ");
    }
    work.exec(query);
    work.exec("END;");
    work.commit();
}

std::vector<std::tuple<std::string, std::string, int, std::string>> postgres::BookRepositoryImpl::ShowBooks()
{
    std::vector<std::tuple<std::string, std::string, int, std::string>> tv;

    pqxx::read_transaction read_trans(connection_);
    auto query_text = "SELECT books.title, authors.name, books.publication_year, books.id FROM authors, books WHERE authors.id=books.author_id ORDER BY books.title ASC, authors.name ASC, books.publication_year ASC;"_zv;
    
    for (auto [title, name, year, id] : read_trans.query<std::string, std::string, int, std::string>(query_text)) {
        tv.push_back({ title, name, year, id});
    }
    return tv;
}

std::vector<std::tuple<std::string, std::string, int, std::string, std::set<std::string>>> postgres::BookRepositoryImpl::ShowBook(std::string& book_name)
{
    std::vector<std::tuple<std::string, std::string, int, std::string, std::set<std::string>>> res;
    std::set<std::string> tags;
    pqxx::read_transaction r(connection_);

    auto query_text = "SELECT books.title, authors.name, books.publication_year, books.id FROM authors, books WHERE books.title = " + r.quote(book_name) + " AND authors.id=books.author_id;";

    for (auto [title, name, year, id] : r.query<std::string, std::string, int, std::string>(query_text))
    {
        auto second_query = "SELECT tag FROM book_tags WHERE book_id = " + r.quote(id) + ";";
        for (auto [tag] : r.query<std::string>(second_query))
        {
            tags.insert(tag);
        }
        res.push_back({ title, name, year, id, tags });
        tags.clear();
    }

    return res;
}

void postgres::BookRepositoryImpl::EditBook(std::string& title, int publication_year, std::set<std::string> tags, std::string& id)
{
    pqxx::work work{ connection_ };
    work.exec("BEGIN;");
    work.exec("LOCK TABLE books, book_tags");
    std::string book_title = work.query_value<std::string>("SELECT title FROM books WHERE id = " + work.quote(id));
    if (book_title.empty())
        throw std::runtime_error("");
    work.exec("UPDATE books SET title =" + work.quote(title) + " WHERE id = " + work.quote(id));
    work.exec("UPDATE books SET publication_year =" + std::to_string(publication_year) + " WHERE id = " + work.quote(id));
    work.exec("DELETE FROM book_tags WHERE book_id = " + work.quote(id));
    std::string query = "INSERT INTO book_tags(book_id, tag) VALUES ";

    for (auto tag = tags.begin(); tag != tags.end(); ++tag)
    {
        if (tag == std::prev(tags.end()))
        {
            query.append("('" + id + "', '" + *tag + "')");
            break;
        }
        query.append("('" + id + "', '" + *tag + "'), ");
    }

    work.exec(query);
    work.exec("END;");
    work.commit();
}

void postgres::BookRepositoryImpl::DeleteBook(std::string& book_id)
{
    pqxx::work work(connection_);

    work.exec("BEGIN;");
    work.exec("LOCK TABLE books, book_tags");

    std::string book_name = work.query_value<std::string>("SELECT title FROM books WHERE id = " + work.quote(book_id));
    if (book_name.empty())
        throw std::runtime_error("");

    work.exec("DELETE FROM books WHERE id = " + work.quote(book_id));
    work.exec("DELETE FROM book_tags WHERE book_id = " + work.quote(book_id));
    work.commit();
}

std::vector<domain::Book> postgres::BookRepositoryImpl::GetAuthorBooks(const std::string& author_id)
{
    std::vector<domain::Book> books;
    pqxx::read_transaction r{ connection_ };

    for (
        const auto& [id, author_id, title, publication_year] :
        r.query<std::string, std::string, std::string, int>("SELECT id, author_id, title, publication_year FROM books WHERE author_id = "s + r.quote(author_id) + " ORDER BY publication_year, title ASC;"s))
    {
        auto book_id_t = util::TaggedUUID<domain::detail::BookTag>::FromString(id);
        auto author_id_t = util::TaggedUUID<domain::detail::AuthorTag>::FromString(author_id);
        //books.emplace_back(book_id_t, author_id_t, title, publication_year); //todo
    }

    return books;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
    author_id UUID NOT NULL,
    title varchar(100) NOT NULL,
    publication_year integer NOT NULL
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
    book_id UUID,
    tag varchar(30) NOT NULL
);
)"_zv);

    // коммитим изменения
    work.commit();
}

}  // namespace postgres