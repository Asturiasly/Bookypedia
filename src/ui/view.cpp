#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <cassert>
#include <iostream>

#include "../app/use_cases.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
namespace detail {

std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    out << book.title << ", " << book.publication_year;
    return out;
}

std::ostream& operator<<(std::ostream& out, const NewBooksInfo& book) {
    out << book.title << " by " << book.author << ", " << book.publication_year;
    return out;
}

}  // namespace detail

template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} 
{
    menu_.AddAction(  //
        "AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1)
        // либо
        // [this](auto& cmd_input) { return AddAuthor(cmd_input); }
    );
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
                    std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,
                    std::bind(&View::ShowAuthorBooks, this));
    menu_.AddAction("DeleteAuthor"s, "<author_name>"s, "Delete author"s, std::bind(&View::DeleteAuthor, this, ph::_1));
    menu_.AddAction("EditAuthor"s, "<author_name>"s, "Edit Author name"s, std::bind(&View::EditAuthor, this, ph::_1));
    menu_.AddAction("ShowBook"s, "<book_name>"s, "Shows book info"s, std::bind(&View::ShowBook, this, ph::_1));
    menu_.AddAction("DeleteBook"s, "<book_name>"s, "Delete book"s, std::bind(&View::DeleteBook, this, ph::_1));
    menu_.AddAction("EditBook"s, "<book_name>"s, "Edit book"s, std::bind(&View::EditBook, this, ph::_1));
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        if (name.empty())
            throw std::invalid_argument("");
        boost::algorithm::trim(name);
        use_cases_.AddAuthor(std::move(name));
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::DeleteAuthor(std::istream& cmd_input) const {
    std::string author_name;
    std::getline(cmd_input, author_name);

    try 
    {
        if (author_name == "")
        {
            auto id = SelectAuthor();

            if (id == std::nullopt)
                return true;

            auto authors = GetAuthors();

            auto author = std::find_if(authors.begin(), authors.end(), [&id](ui::detail::AuthorInfo inf)
                {
                    return id == inf.id;
                });

            if (author == authors.end())
                throw std::runtime_error("");

            use_cases_.DeleteAuthor(author->name);
        }
        else
        {
            boost::algorithm::trim(author_name);
            use_cases_.DeleteAuthor(author_name);
        }
    }
    catch (...)
    {
        output_ << "Failed to delete author"sv << std::endl;
    }
    return true;
}

bool View::EditAuthor(std::istream& cmd_input) const
{
    std::string author_name;
    std::getline(cmd_input, author_name);

    std::string new_name;
    try
    {
        if (author_name == "")
        {
            auto id = SelectAuthor();

            if (id == std::nullopt)
            {
                return true;
            }


            output_ << "Enter new name: " << std::endl;
            std::getline(input_, new_name);
            boost::algorithm::trim(new_name);

            auto authors = GetAuthors();
            auto author = std::find_if(authors.begin(), authors.end(), [&id](ui::detail::AuthorInfo inf)
                {
                    return id == inf.id;
                });

            if (author == authors.end())
                throw std::runtime_error("");

            use_cases_.EditAuthor(new_name, author->name);
        }
        else
        {
            output_ << "Enter new name: " << std::endl;
            std::getline(input_, new_name);
            boost::algorithm::trim(new_name);
            boost::algorithm::trim(author_name);

            auto authors = GetAuthors();
            auto author = std::find_if(authors.begin(), authors.end(), [&author_name](ui::detail::AuthorInfo inf)
                {
                    return author_name == inf.name;
                });

            if (author == authors.end())
                throw std::runtime_error("");

            use_cases_.EditAuthor(new_name, author_name);
        }
    }
    catch (...)
    {
        output_ << "Failed to edit author"sv << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        if (auto params = GetBookParams(cmd_input))
        {
            if (params == std::nullopt)
                return true;

            auto author_id_t = util::TaggedUUID<domain::detail::AuthorTag>::FromString(params->author_id);
            use_cases_.AddBook(params->publication_year, params->title, author_id_t, params->tags);
        }
    } catch (const std::exception&) {
        output_ << "Failed to add book"sv << std::endl;
    }
    return true;
}

bool View::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    PrintVector(output_, GetBooks());
    return true;
}

bool View::ShowBook(std::istream& cmd_input) const
{
    std::string book_name_str;
    std::getline(cmd_input, book_name_str);

    if (book_name_str == "")
    {
        auto id = SelectBook();
        auto books = GetBooks();
        auto book_name = std::find_if(books.begin(), books.end(), [&id](ui::detail::NewBooksInfo inf)
            {
                return id == inf.id;
            });
        auto same_name_books = GetBook(book_name.operator*().title);

        if (same_name_books.size() == 1)
        {
            auto res_book = same_name_books.back();
            output_ << "Title: " << res_book.title << std::endl;
            output_ << "Author: " << res_book.author << std::endl;
            output_ << "Publication year: " << res_book.publication_year << std::endl;

            if (res_book.tags.has_value() && !res_book.tags.value().empty())
            {
                output_ << "Tags: ";
                for (auto it = res_book.tags.value().begin(); it != res_book.tags.value().end(); ++it)
                {
                    if (it == std::prev(res_book.tags.value().end()))
                    {
                        output_ << *it << std::endl;
                        break;
                    }
                    output_ << *it << ", ";
                }
            }
        }
        else
        {
            auto res_book = std::find_if(same_name_books.begin(), same_name_books.end(), [&book_name](ui::detail::NewBooksInfo inf)
                {
                    return book_name.operator*().id == inf.id;
                });
            output_ << "Title: " << res_book.operator*().title << std::endl;
            output_ << "Author: " << res_book.operator*().author << std::endl;
            output_ << "Publication year: " << res_book.operator*().publication_year << std::endl;

            if (res_book.operator*().tags.has_value() && !res_book.operator*().tags.value().empty())
            {
                output_ << "Tags: ";
                for (auto it = res_book.operator*().tags.value().begin(); it != res_book.operator*().tags.value().end(); ++it)
                {
                    if (it == std::prev(res_book.operator*().tags.value().end()))
                    {
                        output_ << *it << std::endl;
                        break;
                    }
                    output_ << *it << ", ";
                }
            }
        }
    }
    else
    {
        boost::algorithm::trim(book_name_str);
        auto same_name_books = GetBook(book_name_str);
        if (same_name_books.empty())
            return true;

        ui::detail::NewBooksInfo book;
        if (same_name_books.size() > 1)
        {
            int i = 1;
            std::string index;
            for (const auto& book : same_name_books)
            {
                output_ << i++ << " " << book.title << " by " << book.author << ", " << book.publication_year << std::endl;
            }
            output_ << "Enter the book # or empty line to cancel :" << std::endl;
            std::getline(input_, index);

            if (index == "")
                return true;

            int indx = std::stoi(index);
            --indx;
            book = same_name_books[indx];
        }
        else
        {
            book = same_name_books.back();
        }

        output_ << "Title: " << book.title << std::endl;
        output_ << "Author: " << book.author << std::endl;
        output_ << "Publication year: " << book.publication_year << std::endl;

        if (book.tags.has_value() && !book.tags.value().empty())
        {
            output_ << "Tags: ";
            for (auto it = book.tags.value().begin(); it != book.tags.value().end(); ++it)
            {
                if (it == std::prev(book.tags.value().end()))
                {
                    output_ << *it << std::endl;
                    break;
                }
                output_ << *it << ", ";
            }
        }
    }
    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const
{
    try 
    {
        std::string book_name_str;
        std::getline(cmd_input, book_name_str);

        if (book_name_str == "")
        {
            auto id = SelectBook();

            if (id == std::nullopt)
            {
                return true;
            }

            auto books = GetBooks();
            auto book_name = std::find_if(books.begin(), books.end(), [&id](ui::detail::NewBooksInfo inf)
                {
                    return id == inf.id;
                });

            if (book_name == books.end())
                throw std::runtime_error("");

            auto same_name_books = GetBook(book_name.operator*().title);

            if (same_name_books.empty())
                throw std::runtime_error("");

            if (same_name_books.size() == 1)
            {
                use_cases_.DeleteBook(same_name_books.back().id);
            }
            else
            {
                auto res_book = std::find_if(same_name_books.begin(), same_name_books.end(), [&book_name](ui::detail::NewBooksInfo inf)
                    {
                        return book_name.operator*().id == inf.id;
                    });

                if (res_book == same_name_books.end())
                    throw std::runtime_error("");

                use_cases_.DeleteBook(res_book.operator*().id);
            }
        }
        else
        {
            boost::algorithm::trim(book_name_str);
            auto same_name_books = GetBook(book_name_str);
            if (same_name_books.empty())
                throw std::runtime_error("");
            ui::detail::NewBooksInfo book;
            if (same_name_books.size() > 1)
            {
                int i = 1;
                std::string index;
                for (const auto& book : same_name_books)
                {
                    output_ << i++ << " " << book.title << " by " << book.author << ", " << book.publication_year << std::endl;
                }
                output_ << "Enter the book # or empty line to cancel :" << std::endl;
                std::getline(input_, index);

                if (index == "")
                    return true;

                int indx = std::stoi(index);
                --indx;
                book = same_name_books[indx];
            }
            else
            {
                book = same_name_books.back();
            }
            use_cases_.DeleteBook(book.id);
        }
    }
    catch (...)
    {
        output_ << "Failed to delete book" << std::endl;
    }

    return true;
}

bool View::EditBook(std::istream& cmd_input) const
{
    try 
    {
        std::string book_name_str;
        std::getline(cmd_input, book_name_str);

        if (book_name_str == "")
        {
            auto id = SelectBook();

            if (id == std::nullopt)
                return true;

            auto books = GetBooks();
            auto book_name = std::find_if(books.begin(), books.end(), [&id](ui::detail::NewBooksInfo inf)
                {
                    return id == inf.id;
                });

            if (book_name == books.end())
                throw std::runtime_error("");

            auto same_name_books = GetBook(book_name.operator*().title);

            if (same_name_books.empty())
                throw std::runtime_error("");

            if (same_name_books.size() == 1)
            {
                auto book = same_name_books.back();
                std::string title;
                std::string publication_year;
                std::set<std::string> unique_sorted_tags;

                output_ << "Enter new title or empty line to use the current one ("s + book.title + "):"s << std::endl;
                std::getline(input_, title);
                if (title == "")
                    title = book.title;
                output_ << "Enter publication year or empty line to use the current one ("s + std::to_string(book.publication_year) + "):"s << std::endl;
                std::getline(input_, publication_year);
                if (publication_year == "")
                    publication_year = book.publication_year;
                output_ << "Enter tags (current tags: "s;
                for (auto it = book.tags.value().begin(); it != book.tags.value().end(); ++it)
                {
                    if (it == std::prev(book.tags.value().end()))
                    {
                        output_ << *it << "):" << std::endl;
                        break;
                    }
                    output_ << *it << ", ";
                }

                std::string tags_action;
                std::getline(input_, tags_action);
                if (tags_action == "")
                {
                    use_cases_.EditBook(title, std::stoi(publication_year), unique_sorted_tags, book.id);
                    return true;
                }

                std::vector<std::string> tags;
                boost::split(tags, tags_action, boost::is_any_of(","));
                for (auto& tag : tags)
                {
                    if (tag.empty())
                        continue;

                    boost::algorithm::trim(tag);
                    auto space_pos = tag.find(' ');
                    if (space_pos != std::string::npos)
                    {
                        for (size_t i = space_pos; i < tag.size(); ++i)
                        {
                            if (tag[i] == ' ' && tag[i + 1] == ' ')
                            {
                                tag.erase(i, 1);
                                --i;
                            }
                        }
                    }
                    unique_sorted_tags.insert(tag);
                }

                use_cases_.EditBook(title, std::stoi(publication_year), unique_sorted_tags, book.id);
            }
            else
            {
                auto res_book = std::find_if(same_name_books.begin(), same_name_books.end(), [&book_name](ui::detail::NewBooksInfo inf)
                    {
                        return book_name.operator*().id == inf.id;
                    });

                if (res_book == same_name_books.end())
                    throw std::runtime_error("");

                auto book = res_book;
                std::string title;
                std::string publication_year;
                std::set<std::string> unique_sorted_tags;

                output_ << "Enter new title or empty line to use the current one ("s + book.operator*().title + "):"s << std::endl;
                std::getline(input_, title);
                if (title == "")
                    title = book.operator*().title;
                output_ << "Enter publication year or empty line to use the current one ("s + std::to_string(book.operator*().publication_year) + "):"s << std::endl;
                std::getline(input_, publication_year);
                if (publication_year == "")
                    publication_year = book.operator*().publication_year;
                output_ << "Enter tags (current tags: "s;
                for (auto it = book.operator*().tags.value().begin(); it != book.operator*().tags.value().end(); ++it)
                {
                    if (it == std::prev(book.operator*().tags.value().end()))
                    {
                        output_ << *it << "):" << std::endl;
                        break;
                    }
                    output_ << *it << ", ";
                }

                std::string tags_action;
                std::getline(input_, tags_action);
                if (tags_action == "")
                {
                    use_cases_.EditBook(title, std::stoi(publication_year), unique_sorted_tags, book.operator*().id);
                    return true;
                }

                std::vector<std::string> tags;
                boost::split(tags, tags_action, boost::is_any_of(","));
                for (auto& tag : tags)
                {
                    if (tag.empty())
                        continue;

                    boost::algorithm::trim(tag);
                    auto space_pos = tag.find(' ');
                    if (space_pos != std::string::npos)
                    {
                        for (size_t i = space_pos; i < tag.size(); ++i)
                        {
                            if (tag[i] == ' ' && tag[i + 1] == ' ')
                            {
                                tag.erase(i, 1);
                                --i;
                            }
                        }
                    }
                    unique_sorted_tags.insert(tag);
                }

                use_cases_.EditBook(title, std::stoi(publication_year), unique_sorted_tags, book.operator*().id);
            }
        }
        else
        {
            boost::algorithm::trim(book_name_str);
            auto same_name_books = GetBook(book_name_str);
            if (same_name_books.empty())
                throw std::runtime_error("");

            ui::detail::NewBooksInfo book;
            if (same_name_books.size() > 1)
            {
                int i = 1;
                std::string index;
                for (const auto& book : same_name_books)
                {
                    output_ << i++ << " " << book.title << " by " << book.author << ", " << book.publication_year << std::endl;
                }
                output_ << "Enter the book # or empty line to cancel :" << std::endl;
                std::getline(input_, index);

                if (index == "")
                    return true;

                int indx = std::stoi(index);
                --indx;
                book = same_name_books[indx];
            }
            else
            {
                book = same_name_books.back();
            }

            std::string title;
            std::string publication_year;
            std::set<std::string> unique_sorted_tags;

            output_ << "Enter new title or empty line to use the current one ("s + book.title + "):"s << std::endl;
            std::getline(input_, title);
            if (title == "")
                title = book.title;
            output_ << "Enter publication year or empty line to use the current one ("s + std::to_string(book.publication_year) + "):"s << std::endl;
            std::getline(input_, publication_year);
            if (publication_year == "")
                publication_year = std::to_string(book.publication_year);
            output_ << "Enter tags (current tags: "s;
            for (auto it = book.tags.value().begin(); it != book.tags.value().end(); ++it)
            {
                if (it == std::prev(book.tags.value().end()))
                {
                    output_ << *it << "):" << std::endl;
                    break;
                }
                output_ << *it << ", ";
            }

            std::string tags_action;
            std::getline(input_, tags_action);
            if (tags_action == "")
            {
                use_cases_.EditBook(title, std::stoi(publication_year), unique_sorted_tags, book.id);
                return true;
            }

            std::vector<std::string> tags;
            boost::split(tags, tags_action, boost::is_any_of(","));
            for (auto& tag : tags)
            {
                if (tag.empty())
                    continue;

                boost::algorithm::trim(tag);
                auto space_pos = tag.find(' ');
                if (space_pos != std::string::npos)
                {
                    for (size_t i = space_pos; i < tag.size(); ++i)
                    {
                        if (tag[i] == ' ' && tag[i + 1] == ' ')
                        {
                            tag.erase(i, 1);
                            --i;
                        }
                    }
                }
                unique_sorted_tags.insert(tag);
            }

            use_cases_.EditBook(title, std::stoi(publication_year), unique_sorted_tags, book.id);
        }

    }
    catch (...)
    {
        output_ << "Book not found" << std::endl;
    }

    return true;
}

bool View::ShowAuthorBooks() const {
    // TODO: handle error
    try {
        if (auto author_id = SelectAuthor()) {
            PrintVector(output_, GetAuthorBooks(*author_id));
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to Show Books");
    }
    return true;
}

std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;

    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);

    output_ << "Enter author name or empty line to select from list: " << std::endl;
    std::string author_name;
    std::getline(input_, author_name);

    if (author_name == "")
    {
        auto author_id = SelectAuthor();
        if (author_id == std::nullopt)
            return std::nullopt;
        else 
            params.author_id = author_id.value();
    }
    else
    {
        boost::algorithm::trim(author_name);
        auto authors = GetAuthors();
        auto it = std::find_if(authors.begin(), authors.end(), [&author_name](ui::detail::AuthorInfo inf)
            {
                return inf.name == author_name;
            });
        bool is_author_found = it != authors.end();

        if (!is_author_found)
        {
            output_ << "No author found. Do you want to add " + author_name + " (y/n)?" << std::endl;
            std::string t;
            std::getline(input_, t);
            if (t.back() == 'y' || t.back() == 'Y')
            {
                use_cases_.AddAuthor(author_name);
                auto authors = GetAuthors();
                auto author = std::find_if(authors.begin(), authors.end(), [&author_name](ui::detail::AuthorInfo inf)
                    {
                        return inf.name == author_name;
                    });
                params.author_id = author->id;
            }
            else
                throw std::invalid_argument("");
        }
        else
            params.author_id = it->id;
    }

    std::vector<std::string> tags;
    std::set<std::string> unique_sorted_tags;
    std::string tags_str;
    output_ << "Enter tags (comma separated):" << std::endl;
    std::getline(input_, tags_str);

    if (tags_str.empty())
        return params;

    boost::split(tags, tags_str, boost::is_any_of(","));
    for (auto& tag : tags)
    {
        if (tag.empty())
            continue;

        boost::algorithm::trim(tag);
        auto space_pos = tag.find(' ');
        if (space_pos != std::string::npos)
        {
            for (size_t i = space_pos; i < tag.size(); ++i)
            {
                if (tag[i] == ' ' && tag[i + 1] == ' ')
                {
                    tag.erase(i, 1);
                    --i;
                }
            }
        }
        unique_sorted_tags.insert(tag);
    }

    if (!unique_sorted_tags.empty())
        params.tags = std::move(unique_sorted_tags);
    return params;
}

std::optional<std::string> View::SelectBook() const
{
    auto books = GetBooks();
    PrintVector(output_, books);
    output_ << "Enter the book # or empty line to cancel:" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int book_idx;
    try {
        book_idx = std::stoi(str);
    }
    catch (std::exception const&) {
        throw std::runtime_error("Invalid book num");
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= books.size()) {
        throw std::runtime_error("Invalid author num");
    }

    return books[book_idx].id;
}

std::optional<std::string> View::SelectAuthor() const {
    output_ << "Select author:" << std::endl;
    auto authors = GetAuthors();
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid author num");
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= authors.size()) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[author_idx].id;
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    std::vector<detail::AuthorInfo> dst_autors;

    for (const auto& author : use_cases_.GetAuthors())
    {
        dst_autors.emplace_back(author.GetId().ToString(), author.GetName());
    }
    return dst_autors;
}

std::vector<detail::NewBooksInfo> View::GetBooks() const {
    std::vector<detail::NewBooksInfo> books;
    
    for (const auto& book : use_cases_.ShowBooks())
    {
        books.emplace_back(std::get<0>(book), std::get<1>(book), std::get<2>(book), std::get<3>(book));
    }
    return books;
}

std::vector<detail::NewBooksInfo> View::GetBook(std::string& book_name) const {
    std::vector<detail::NewBooksInfo> books;

    for (const auto& book : use_cases_.ShowBook(book_name))
    {
        books.emplace_back(std::get<0>(book), std::get<1>(book), std::get<2>(book), std::get<3>(book), std::get<4>(book));
    }
    return books;
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    std::vector<detail::BookInfo> books;
    
    for (const auto& book : use_cases_.GetAuthorBooks(author_id))
    {
        books.emplace_back(book.GetTitle(), book.GetPublicationYear());
    }
    return books;
}

}  // namespace ui
