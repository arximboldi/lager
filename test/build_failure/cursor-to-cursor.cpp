// clang-format off
#include <lager/cursor.hpp>

#include <string>

int main()
{
    lager::cursor<int> a;

    lager::cursor int_cursor = a; // happy path

    lager::cursor<std::string> b = a; // offending line

    return 0;
}
