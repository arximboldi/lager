// clang-format off
#include <lager/cursor.hpp>
#include <lager/lenses.hpp>
#include <lager/state.hpp>

#include "setup.hpp"

int main()
{
    auto st = lager::make_state(Model{});
    auto l = lager::lenses::attr(&Model::foo);

    lager::cursor<Foo> foo_cursor = st.zoom(l); // happy path

    lager::cursor<std::string> a = st.zoom(l); // offending line

    return 0;
}
