#pragma once

#include <memory>
#include <unordered_map>

namespace lager {
namespace detail {
class reader_node_base;

class traversal
{
public:
    virtual void visit()                       = 0;
    virtual void schedule(reader_node_base* n) = 0;
};

} // namespace detail
} // namespace lager
