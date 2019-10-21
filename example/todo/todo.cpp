#include "todo.hpp"

namespace todo {

model update(model m, action a) { return m; }

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
