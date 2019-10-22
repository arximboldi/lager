#include "todo.hpp"

#include <lager/util.hpp>

#include <fstream>

namespace todo {

model update(model m, action a)
{
    std::visit(lager::visitor{[&](add_todo_action&& a) {
                   m.todos = std::move(m.todos).push_front({false, a.text});
               }},
               std::move(a));
    return m;
}

void save(const std::string& fname, model todos)
{
    auto s = std::ofstream{fname};
    {
        auto a = cereal::JSONOutputArchive{s};
        a(todos);
    }
}

model load(const std::string& fname)
{
    auto s = std::ifstream{fname};
    auto r = model{};
    {
        auto a = cereal::JSONInputArchive{s};
        a(r);
    }
    return r;
}

} // namespace todo
