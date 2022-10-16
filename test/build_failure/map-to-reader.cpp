// clang-format off
#include <lager/reader.hpp>

#include "setup.hpp"

int main()
{
    auto s = make_dummy_store();

    lager::reader<int> int_reader = s.map([](auto) { return 1; }); // happy path

    lager::reader<std::string> a = s.map([](auto) { return 1; }); // offending line

    return 0;
}
