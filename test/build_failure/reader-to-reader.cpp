// clang-format off
#include <lager/reader.hpp>

int main()
{
    lager::reader<int> a{};

    lager::reader<int> num = a; // happy path

    lager::reader<std::string> b = a; // offending line

    return 0;
}
