#pragma once

#include <lager/detail/nodes.hpp>
#include <lager/detail/traversal.hpp>
#include <utility>

namespace lager {
namespace detail {

class naive_mmap_topo_traversal : public traversal
{
public:
    naive_mmap_topo_traversal()                                 = delete;
    naive_mmap_topo_traversal(const naive_mmap_topo_traversal&) = delete;
    naive_mmap_topo_traversal&
    operator=(const naive_mmap_topo_traversal&) = delete;

    naive_mmap_topo_traversal(const std::shared_ptr<reader_node_base>& root,
                              std::size_t)
        : naive_mmap_topo_traversal(root.get())
    {}

    naive_mmap_topo_traversal(reader_node_base* root)
        : current_rank_(root->rank())
        , schedule_({{current_rank_, std::vector{root}}})
    {}

    void visit() override
    {
        while (node_scheduled_) {
            node_scheduled_ = false;
            visit_rank_();
        }
    }

    void schedule(reader_node_base* n) override
    {
        auto& v = schedule_[n->rank()];
        v.push_back(n);
        node_scheduled_ = true;
    }

private:
    void visit_rank_()
    {
        for (auto* node : schedule_[current_rank_]) {
            node->send_down(*this);
        }
        current_rank_++;
    }

    bool node_scheduled_                                               = true;
    long current_rank_                                                 = 0;
    std::unordered_map<long, std::vector<reader_node_base*>> schedule_ = {};
};
} // namespace detail
} // namespace lager
