#include "todo.hpp"

#include <lager/util.hpp>

#include <fstream>

namespace todo {

item update_item(item m, item_action a)
{
    std::visit(lager::visitor{[&](toggle_item_action&& a) { m.done = !m.done; },
                              [&](remove_item_action&& a) {}},
               std::move(a));
    return m;
}

model update(model m, action a)
{
    lager::match(std::move(a))(
        [&](add_todo_action&& a) {
            if (!a.text.empty())
                m.todos = std::move(m.todos).push_front({false, a.text});
        },
        [&](std::pair<std::size_t, item_action>&& a) {
            if (a.first >= m.todos.size()) {
                std::cerr << "Invalid todo::item_action index!" << std::endl;
            } else {
                m.todos =
                    std::holds_alternative<remove_item_action>(a.second)
                        ? std::move(m.todos).erase(a.first)
                        : std::move(m.todos).update(a.first, [&](auto&& t) {
                              return update_item(t, a.second);
                          });
            }
        });
    return m;
}

model save(const std::string& fname, model todos)
{
    auto s = std::ofstream{fname};
    {
        auto a = cereal::JSONOutputArchive{s};
        a(todos);
    }
    return todos;
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
