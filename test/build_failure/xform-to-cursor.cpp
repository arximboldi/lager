// clang-format off
#include <lager/cursor.hpp>

#include <string>

struct Foo
{};

int main()
{
    lager::cursor<std::string> str{};

    lager::cursor<int> num = str.xform(zug::map([](std::string) { return 1; }), zug::map([](int) { return ""; })); // happy path

    lager::cursor<Foo> num_fail = str.xform(zug::map([](std::string) { return 1; }), zug::map([](int) { return ""; })); // offending line

    return 0;
}
