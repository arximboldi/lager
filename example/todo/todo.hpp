
#include <immer/flex_vector.hpp>

struct todo
{
    bool done = false;
    std::string text;
};

struct model
{
    std::string name;
    immer::flex_vector<todo> todos;
};
