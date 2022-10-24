// clang-format off
#include "setup.hpp"

#include <lager/lenses.hpp>
#include <lager/reader.hpp>

int main()
{
    auto s = make_dummy_store();
    auto l = lager::lenses::attr(&Model::foo);

    lager::reader<Foo> foo_reader = s.zoom(l); // happy path

    lager::reader<std::string> b = s.zoom(l); // offending line

    return 0;
}
