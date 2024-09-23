#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "bookypedia.h"

using namespace std::literals;
using pqxx::operator"" _zv;

namespace {

    //BOOKYPEDIA_DB_URL
    //postgres://postgres:Asturias1997@localhost:5432/postgres
constexpr const char DB_URL_ENV_NAME[]{"postgres://postgres:Asturias1997@localhost:5432/postgres"};

bookypedia::AppConfig GetConfigFromEnv() 
{
    bookypedia::AppConfig config;

    if (const auto* url = DB_URL_ENV_NAME) 
        config.db_url = url;
        //
    else 
        throw std::runtime_error(DB_URL_ENV_NAME + " environment variable not found"s);
    return config;
}

}  // namespace

int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[]) {
    try 
    {
        pqxx::connection conn(DB_URL_ENV_NAME);
        pqxx::work w(conn);
        w.exec("DROP TABLE authors, books, book_tags;"_zv);
        w.commit();

        bookypedia::Application app{GetConfigFromEnv()};
        app.Run();
    } 
    catch (const std::exception& e) 
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
