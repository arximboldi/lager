#pragma once

#include <lager/detail/nodes.hpp>
#include <lager/detail/traversal.hpp>

namespace lager {
namespace detail {
class dfs_traversal : public traversal
{
public:
    dfs_traversal()                                = delete;
    dfs_traversal(const dfs_traversal&)            = delete;
    dfs_traversal& operator=(const dfs_traversal&) = delete;
    dfs_traversal(const std::shared_ptr<reader_node_base>& root)
        : root_{root.get()}
    {
    }

    void visit() override { root_->send_down(); }
    void schedule(reader_node_base*) override {}

private:
    reader_node_base* root_;
};

} // namespace detail
} // namespace lager
