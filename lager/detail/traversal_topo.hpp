#pragma once

#include <lager/detail/nodes.hpp>
#include <lager/detail/traversal.hpp>

namespace lager {
namespace detail {
template <template <typename...> typename Multimap = std::unordered_multimap>
class topo_traversal : public traversal
{
public:
    topo_traversal()                                 = delete;
    topo_traversal(const topo_traversal&)            = delete;
    topo_traversal& operator=(const topo_traversal&) = delete;
    topo_traversal(const std::shared_ptr<reader_node_base>& root)
        : topo_traversal(root.get())
    {
    }
    topo_traversal(reader_node_base* root)
        : current_rank_(root->rank())
        , schedule_{{current_rank_, {root}}}
    {
    }

    void visit() override
    {
        while (node_scheduled_) {
            node_scheduled_ = false;
            visit_rank_();
        }
    }

    void schedule(reader_node_base* n) override
    {
        schedule_.emplace(n->rank(), n);
        node_scheduled_ = true;
    }

private:
    void visit_rank_()
    {
        const auto range      = schedule_.equal_range(current_rank_);
        const auto rank_count = std::distance(range.first, range.second);

        for (auto node = range.first;
             std::distance(range.first, node) < rank_count;
             ++node) {
            // WARN: modifying the map in which we're iterating within the loop,
            // which is bad
            node->second->send_down(*this);
        }
        current_rank_++;
    }

    bool node_scheduled_                       = true;
    long current_rank_                         = 0;
    Multimap<int, reader_node_base*> schedule_ = {};
};

} // namespace detail
} // namespace lager
