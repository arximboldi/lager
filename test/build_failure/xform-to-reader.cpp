// clang-format off
#include <lager/reader.hpp>

struct Foo{};

int main()
{
    lager::reader<int> num{};

    lager::reader<std::string> str = num.xform(zug::map([](int) { return std::string{""}; })); // happy path

    lager::reader<Foo> fail = num.xform(zug::map([](int) { return ""; })); // offending line

    return 0;
}
