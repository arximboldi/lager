// clang-format off
#include <lager/reader.hpp>
#include <lager/with.hpp>

#include "setup.hpp"

int main()
{
    lager::reader<std::string> a{};
    lager::reader<int> c{};

    lager::reader<std::tuple<std::string, int>> with_reader = lager::with(a, c); // happy path

    lager::reader<Model> b = lager::with(a, c); // offending line

    return 0;
}
