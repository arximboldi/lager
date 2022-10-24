// clang-format off
#include <lager/cursor.hpp>
#include <lager/reader.hpp>

#include <string>

int main()
{
    lager::cursor<int> a{};

    lager::reader<int> int_reader = a; // happy path  

    lager::reader<std::string> b = a; // offending line

    return 0;
}
