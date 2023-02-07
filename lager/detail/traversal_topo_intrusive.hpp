#pragma once

#include <boost/intrusive/set.hpp>
#include <lager/detail/nodes.hpp>
#include <lager/detail/traversal.hpp>

namespace lager {
namespace detail {

class topo_intrusive_traversal : public traversal
{
    using unordered_map = boost::intrusive::unordered_multiset<
        reader_node_base,
        boost::intrusive::key_of_value<rank_is_key>,
        boost::intrusive::member_hook<reader_node_base,
                                      reader_node_base::hook_type,
                                      &reader_node_base::member_hook_>
        // boost::intrusive::constant_time_size<false>,
        // boost::intrusive::power_2_buckets<true>,
        // boost::intrusive::incremental<true>
        >;

public:
    topo_intrusive_traversal()                                = delete;
    topo_intrusive_traversal(const topo_intrusive_traversal&) = delete;
    topo_intrusive_traversal&
    operator=(const topo_intrusive_traversal&) = delete;

    topo_intrusive_traversal(const std::shared_ptr<reader_node_base>& root,
                             std::size_t N)
        : current_rank_(root->rank())
        , buckets_(N * 4)
    {
        assert(root);
        schedule_.insert(*root);
    }

    void visit() override
    {
        while (node_scheduled_) {
            node_scheduled_ = false;
            visit_rank_();
        }
        schedule_.clear();
    }

    void schedule(reader_node_base* n) override
    {
        // if node is already linked, it has already beek scheduled!
        if (!n->member_hook_.is_linked()) {
            schedule_.insert(*n);
            node_scheduled_ = true;
        }
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

    bool node_scheduled_ = true;
    long current_rank_   = 0;

    typename std::vector<unordered_map::bucket_type> buckets_;
    unordered_map schedule_{typename unordered_map::bucket_traits(
        buckets_.data(), buckets_.size())};
};

class topo_intrusive_traversal_rb : public traversal
{
    using unordered_map = boost::intrusive::multiset<
        reader_node_base,
        boost::intrusive::key_of_value<rank_is_key>,
        boost::intrusive::member_hook<reader_node_base,
                                      reader_node_base::hook_type_rb,
                                      &reader_node_base::member_hook_rb_>
        // boost::intrusive::constant_time_size<false>,
        // boost::intrusive::power_2_buckets<true>,
        // boost::intrusive::incremental<true>
        >;

public:
    topo_intrusive_traversal_rb()                                   = delete;
    topo_intrusive_traversal_rb(const topo_intrusive_traversal_rb&) = delete;
    topo_intrusive_traversal_rb&
    operator=(const topo_intrusive_traversal_rb&) = delete;

    topo_intrusive_traversal_rb(const std::shared_ptr<reader_node_base>& root,
                                std::size_t)
        : current_rank_(root->rank())
    {
        assert(root);
        schedule_.insert(*root);
    }

    void visit() override
    {
        while (node_scheduled_) {
            node_scheduled_ = false;
            visit_rank_();
        }
        schedule_.clear();
    }

    void schedule(reader_node_base* n) override
    {
        // if node is already linked, it has already beek scheduled!
        if (!n->member_hook_rb_.is_linked()) {
            schedule_.insert(*n);
            node_scheduled_ = true;
        }
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

    bool node_scheduled_ = true;
    long current_rank_   = 0;

    unordered_map schedule_{};
};

} // namespace detail
} // namespace lager
