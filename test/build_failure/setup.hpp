#pragma once

#include <lager/extra/struct.hpp>

#include <lager/store.hpp>
#include <lager/event_loop/manual.hpp>

#include <variant>
#include <string>

struct Foo
{
    int b{1};
    LAGER_STRUCT_NESTED(Foo, b);
};
struct Model
{
    int a{0};
    Foo foo{};
    LAGER_STRUCT_NESTED(Model, a);
};
struct ModelAction
{
    LAGER_STRUCT_NESTED(ModelAction);
};

using Action = std::variant<ModelAction>;
Model update(Model m, Action a) { return m; }

auto make_dummy_store() {
  return lager::make_store<ModelAction>(Model{}, lager::with_manual_event_loop{});
}
