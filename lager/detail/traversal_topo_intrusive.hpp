#pragma once

#include <boost/intrusive/set.hpp>
#include <lager/detail/nodes.hpp>
#include <lager/detail/traversal.hpp>

namespace lager {
namespace detail {
class topo_intrusive_traversal : public traversal
{
    using unordered_map =
        boost::intrusive::set<reader_node_base,
                              boost::intrusive::key_of_value<rank_is_key>>;

public:
    topo_intrusive_traversal()                                = delete;
    topo_intrusive_traversal(const topo_intrusive_traversal&) = delete;
    topo_intrusive_traversal&
    operator=(const topo_intrusive_traversal&) = delete;
    topo_intrusive_traversal(const std::shared_ptr<reader_node_base>& root)
        : current_rank_(root->rank())
    {
        schedule_.insert(*root);
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
        schedule_.insert(*n);
        node_scheduled_ = true;
    }

private:
    void visit_rank_()
    {
        const auto range = schedule_.equal_range(current_rank_);

        for (auto node = range.first; node != range.second; ++node) {
            // WARN: modifying the map in which we're iterating within the loop,
            // which is bad
            node->send_down(*this);
        }
        current_rank_++;
    }

    bool node_scheduled_    = true;
    long current_rank_      = 0;
    unordered_map schedule_ = {};
};

} // namespace detail
} // namespace lager
